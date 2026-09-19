#include "tools/ui/tray_manager.hpp"
#include "tools/core/i18n.hpp"
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <filesystem>
#include <iostream>
#include <vector>

namespace tools::ui {

using namespace tools::core;
namespace fs = std::filesystem;

static const char *SNI_XML = R"raw(
<node>
  <interface name="org.kde.StatusNotifierItem">
    <property name="Category" type="s" access="read"/>
    <property name="Id" type="s" access="read"/>
    <property name="Title" type="s" access="read"/>
    <property name="Status" type="s" access="read"/>
    <property name="IconName" type="s" access="read"/>
    <property name="IconThemePath" type="s" access="read"/>
    <property name="Menu" type="o" access="read"/>
    <property name="IconPixmap" type="a(iiay)" access="read"/>
    <method name="ContextMenu">
      <arg direction="in" name="x" type="i"/>
      <arg direction="in" name="y" type="i"/>
    </method>
    <method name="Activate">
      <arg direction="in" name="x" type="i"/>
      <arg direction="in" name="y" type="i"/>
    </method>
    <method name="SecondaryActivate">
      <arg direction="in" name="x" type="i"/>
      <arg direction="in" name="y" type="i"/>
    </method>
    <method name="Scroll">
      <arg direction="in" name="delta" type="i"/>
      <arg direction="in" name="orientation" type="s"/>
    </method>
    <signal name="NewIcon"/>
    <signal name="NewStatus">
      <arg name="status" type="s"/>
    </signal>
    <signal name="NewTitle"/>
  </interface>
</node>
)raw";

static const char *DBUSMENU_XML = R"raw(
<node>
  <interface name="com.canonical.dbusmenu">
    <property name="Version" type="u" access="read"/>
    <property name="TextDirection" type="s" access="read"/>
    <property name="Status" type="s" access="read"/>
    <property name="IconThemePath" type="as" access="read"/>
    <method name="GetLayout">
      <arg direction="in" name="parentId" type="i"/>
      <arg direction="in" name="recursionDepth" type="i"/>
      <arg direction="in" name="propertyNames" type="as"/>
      <arg direction="out" name="revision" type="u"/>
      <arg direction="out" name="layout" type="(ia{sv}av)"/>
    </method>
    <method name="GetGroupProperties">
      <arg direction="in" name="ids" type="ai"/>
      <arg direction="in" name="propertyNames" type="as"/>
      <arg direction="out" name="properties" type="a(ia{sv})"/>
    </method>
    <method name="GetProperty">
      <arg direction="in" name="id" type="i"/>
      <arg direction="in" name="name" type="s"/>
      <arg direction="out" name="value" type="v"/>
    </method>
    <method name="Event">
      <arg direction="in" name="id" type="i"/>
      <arg direction="in" name="eventId" type="s"/>
      <arg direction="in" name="data" type="v"/>
      <arg direction="in" name="timestamp" type="u"/>
    </method>
    <method name="AboutToShow">
      <arg direction="in" name="id" type="i"/>
      <arg direction="out" name="needUpdate" type="b"/>
    </method>
    <signal name="LayoutUpdated">
      <arg name="revision" type="u"/>
      <arg name="parent" type="i"/>
    </signal>
  </interface>
</node>
)raw";

static std::string locate_icon_file() {
    std::vector<std::string> candidates = {
        "resources/compliance.png",
        "compliance.png",
        "../resources/compliance.png"
    };

    const char *home = std::getenv("HOME");
    if (home) {
        candidates.push_back(std::string(home) + "/.config/Tools/app_icon.png");
        candidates.push_back(std::string(home) + "/.local/share/icons/tools-clipboard.png");
    }

    candidates.push_back("/usr/share/icons/hicolor/512x512/apps/tools-clipboard.png");
    candidates.push_back("/usr/share/tools-clipboard/compliance.png");
    candidates.push_back("/usr/lib/tools-clipboard/resources/compliance.png");

    try {
        if (fs::exists("/proc/self/exe")) {
            auto exe_dir = fs::canonical("/proc/self/exe").parent_path();
            candidates.push_back((exe_dir / "resources" / "compliance.png").string());
            candidates.push_back((exe_dir / "compliance.png").string());
            candidates.push_back((exe_dir / ".." / "resources" / "compliance.png").string());
            candidates.push_back((exe_dir / ".." / "share" / "tools-clipboard" / "compliance.png").string());
        }
    } catch (...) {}

    for (const auto &p : candidates) {
        if (fs::exists(p)) {
            return p;
        }
    }
    return "";
}

TrayManager::TrayManager() {
    I18n::instance().on_language_changed([this](Language) {
        refresh_menu();
    });
}

TrayManager::~TrayManager() {
    stop();
}

bool TrayManager::start() {
    GError *err = nullptr;
    bus_ = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &err);
    if (!bus_) {
        std::cerr << "[TrayManager] Falha ao conectar no D-Bus de sessao: "
                  << (err ? err->message : "desconhecido") << std::endl;
        if (err) g_error_free(err);
        return false;
    }

    register_dbus_objects();
    register_with_watcher();

    // Monitora caso o StatusNotifierWatcher apareça ou reinicie
    watcher_sub_id_ = g_bus_watch_name_on_connection(
        bus_,
        "org.kde.StatusNotifierWatcher",
        G_BUS_NAME_WATCHER_FLAGS_NONE,
        [](GDBusConnection*, const gchar*, const gchar*, gpointer user_data) {
            auto *self = static_cast<TrayManager*>(user_data);
            self->register_with_watcher();
        },
        nullptr,
        this,
        nullptr
    );

    return true;
}

void TrayManager::stop() {
    if (watcher_sub_id_ && bus_) {
        g_bus_unwatch_name(watcher_sub_id_);
        watcher_sub_id_ = 0;
    }
    if (sni_reg_id_ && bus_) {
        g_dbus_connection_unregister_object(bus_, sni_reg_id_);
        sni_reg_id_ = 0;
    }
    if (menu_reg_id_ && bus_) {
        g_dbus_connection_unregister_object(bus_, menu_reg_id_);
        menu_reg_id_ = 0;
    }
    if (bus_) {
        g_object_unref(bus_);
        bus_ = nullptr;
    }
}

void TrayManager::register_dbus_objects() {
    if (!bus_) return;

    GError *err = nullptr;

    GDBusNodeInfo *sni_node = g_dbus_node_info_new_for_xml(SNI_XML, nullptr);
    if (sni_node && sni_node->interfaces[0]) {
        static GDBusInterfaceVTable sni_vtable = {
            sni_method_call,
            sni_get_prop,
            nullptr,
            { 0 }
        };
        sni_reg_id_ = g_dbus_connection_register_object(
            bus_,
            "/StatusNotifierItem",
            sni_node->interfaces[0],
            &sni_vtable,
            this,
            nullptr,
            &err
        );
        if (!sni_reg_id_) {
            std::cerr << "[TrayManager] Erro ao registrar StatusNotifierItem: "
                      << (err ? err->message : "") << std::endl;
            if (err) { g_error_free(err); err = nullptr; }
        }
        g_dbus_node_info_unref(sni_node);
    }

    GDBusNodeInfo *menu_node = g_dbus_node_info_new_for_xml(DBUSMENU_XML, nullptr);
    if (menu_node && menu_node->interfaces[0]) {
        static GDBusInterfaceVTable menu_vtable = {
            menu_method_call,
            menu_get_prop,
            nullptr,
            { 0 }
        };
        menu_reg_id_ = g_dbus_connection_register_object(
            bus_,
            "/MenuBar",
            menu_node->interfaces[0],
            &menu_vtable,
            this,
            nullptr,
            &err
        );
        if (!menu_reg_id_) {
            std::cerr << "[TrayManager] Erro ao registrar MenuBar: "
                      << (err ? err->message : "") << std::endl;
            if (err) { g_error_free(err); err = nullptr; }
        }
        g_dbus_node_info_unref(menu_node);
    }
}

void TrayManager::register_with_watcher() {
    if (!bus_ || !sni_reg_id_) return;

    g_dbus_connection_call(
        bus_,
        "org.kde.StatusNotifierWatcher",
        "/StatusNotifierWatcher",
        "org.kde.StatusNotifierWatcher",
        "RegisterStatusNotifierItem",
        g_variant_new("(s)", "/StatusNotifierItem"),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        2000,
        nullptr,
        [](GObject *source, GAsyncResult *res, gpointer) {
            GError *error = nullptr;
            GVariant *result = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source), res, &error);
            if (result) {
                g_variant_unref(result);
            } else if (error) {
                // Silencioso se o watcher ainda nao estiver ativo
                g_error_free(error);
            }
        },
        nullptr
    );
}

void TrayManager::refresh_menu() {
    menu_revision_++;
    if (bus_ && menu_reg_id_) {
        g_dbus_connection_emit_signal(
            bus_,
            nullptr,
            "/MenuBar",
            "com.canonical.dbusmenu",
            "LayoutUpdated",
            g_variant_new("(ui)", menu_revision_, 0),
            nullptr
        );
    }
}

GVariant* TrayManager::create_icon_pixmap_variant() const {
    std::string icon_path = locate_icon_file();
    if (icon_path.empty()) {
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE("a(iiay)"));
        return g_variant_builder_end(&b);
    }

    GError *err = nullptr;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(icon_path.c_str(), 24, 24, TRUE, &err);
    if (!pixbuf) {
        if (err) g_error_free(err);
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE("a(iiay)"));
        return g_variant_builder_end(&b);
    }

    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    const guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

    std::vector<guint8> argb_bytes;
    argb_bytes.resize(width * height * 4);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const guchar *p = pixels + y * rowstride + x * n_channels;
            guint8 r = p[0];
            guint8 g = p[1];
            guint8 b = p[2];
            guint8 a = (n_channels == 4) ? p[3] : 255;

            size_t out_idx = (y * width + x) * 4;
            argb_bytes[out_idx + 0] = a;
            argb_bytes[out_idx + 1] = r;
            argb_bytes[out_idx + 2] = g;
            argb_bytes[out_idx + 3] = b;
        }
    }
    g_object_unref(pixbuf);

    GVariantBuilder b_array;
    g_variant_builder_init(&b_array, G_VARIANT_TYPE("a(iiay)"));

    GVariant *byte_arr = g_variant_new_fixed_array(
        G_VARIANT_TYPE_BYTE,
        argb_bytes.data(),
        argb_bytes.size(),
        sizeof(guint8)
    );

    GVariant *tuple = g_variant_new("(ii@ay)", width, height, byte_arr);
    g_variant_builder_add_value(&b_array, tuple);

    return g_variant_builder_end(&b_array);
}

GVariant* TrayManager::build_item_props(int id, const std::string &label, bool is_separator) const {
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));

    if (is_separator) {
        g_variant_builder_add(&b, "{sv}", "type", g_variant_new_string("separator"));
    } else {
        g_variant_builder_add(&b, "{sv}", "label", g_variant_new_string(label.c_str()));
        g_variant_builder_add(&b, "{sv}", "enabled", g_variant_new_boolean(TRUE));
        g_variant_builder_add(&b, "{sv}", "visible", g_variant_new_boolean(TRUE));
    }
    return g_variant_builder_end(&b);
}

GVariant* TrayManager::build_item_variant(int id, const std::string &label, bool is_separator) const {
    GVariant *props = build_item_props(id, label, is_separator);
    GVariantBuilder b_children;
    g_variant_builder_init(&b_children, G_VARIANT_TYPE("av"));
    GVariant *tuple = g_variant_new("(i@a{sv}av)", id, props, &b_children);
    return g_variant_new_variant(tuple);
}

void TrayManager::sni_method_call(
    GDBusConnection*, const gchar*, const gchar*, const gchar*,
    const gchar *method, GVariant*, GDBusMethodInvocation *inv, gpointer user_data) {
    auto *self = static_cast<TrayManager*>(user_data);

    if (g_strcmp0(method, "Activate") == 0 || g_strcmp0(method, "SecondaryActivate") == 0) {
        if (self->on_toggle_) {
            g_idle_add(+[](gpointer p) -> gboolean {
                auto *cb = static_cast<std::function<void()>*>(p);
                (*cb)();
                return G_SOURCE_REMOVE;
            }, &self->on_toggle_);
        }
    }

    g_dbus_method_invocation_return_value(inv, nullptr);
}

GVariant* TrayManager::sni_get_prop(
    GDBusConnection*, const gchar*, const gchar*, const gchar*,
    const gchar *prop, GError**, gpointer user_data) {
    auto *self = static_cast<TrayManager*>(user_data);

    if (g_strcmp0(prop, "Category") == 0) return g_variant_new_string("ApplicationStatus");
    if (g_strcmp0(prop, "Id") == 0) return g_variant_new_string("tools-clipboard");
    if (g_strcmp0(prop, "Title") == 0) return g_variant_new_string("Tools Clipboard");
    if (g_strcmp0(prop, "Status") == 0) return g_variant_new_string("Active");
    if (g_strcmp0(prop, "IconName") == 0) return g_variant_new_string("tools-clipboard");
    if (g_strcmp0(prop, "IconThemePath") == 0) {
        std::string icon_path = locate_icon_file();
        if (!icon_path.empty()) {
            return g_variant_new_string(fs::path(icon_path).parent_path().string().c_str());
        }
        return g_variant_new_string("");
    }
    if (g_strcmp0(prop, "Menu") == 0) return g_variant_new_object_path("/MenuBar");
    if (g_strcmp0(prop, "IconPixmap") == 0) return self->create_icon_pixmap_variant();

    return nullptr;
}

void TrayManager::menu_method_call(
    GDBusConnection*, const gchar*, const gchar*, const gchar*,
    const gchar *method, GVariant *params, GDBusMethodInvocation *inv, gpointer user_data) {
    auto *self = static_cast<TrayManager*>(user_data);
    auto &i18n = I18n::instance();

    if (g_strcmp0(method, "GetLayout") == 0) {
        GVariantBuilder b_root_props;
        g_variant_builder_init(&b_root_props, G_VARIANT_TYPE("a{sv}"));

        GVariantBuilder b_root_children;
        g_variant_builder_init(&b_root_children, G_VARIANT_TYPE("av"));

        // 1. Abrir Flyout Win + V
        g_variant_builder_add_value(&b_root_children,
            self->build_item_variant(1, i18n.get(StringId::TrayOpenClipboard), false));

        // 2. Limpar Histórico
        g_variant_builder_add_value(&b_root_children,
            self->build_item_variant(2, i18n.get(StringId::TrayClearHistory), false));

        // 3. Configurações
        g_variant_builder_add_value(&b_root_children,
            self->build_item_variant(3, i18n.get(StringId::SettingsTitle), false));

        // 4. Separador
        g_variant_builder_add_value(&b_root_children,
            self->build_item_variant(4, "", true));

        // 5. Sair
        g_variant_builder_add_value(&b_root_children,
            self->build_item_variant(5, i18n.get(StringId::TrayQuit), false));

        GVariant *root = g_variant_new("(ia{sv}av)", 0, &b_root_props, &b_root_children);
        GVariant *res = g_variant_new("(u@(ia{sv}av))", self->menu_revision_, root);
        g_dbus_method_invocation_return_value(inv, res);
        return;
    }

    if (g_strcmp0(method, "GetGroupProperties") == 0) {
        GVariantBuilder b_out;
        g_variant_builder_init(&b_out, G_VARIANT_TYPE("a(ia{sv})"));

        g_variant_builder_add(&b_out, "(i@a{sv})", 1, self->build_item_props(1, i18n.get(StringId::TrayOpenClipboard), false));
        g_variant_builder_add(&b_out, "(i@a{sv})", 2, self->build_item_props(2, i18n.get(StringId::TrayClearHistory), false));
        g_variant_builder_add(&b_out, "(i@a{sv})", 3, self->build_item_props(3, i18n.get(StringId::SettingsTitle), false));
        g_variant_builder_add(&b_out, "(i@a{sv})", 4, self->build_item_props(4, "", true));
        g_variant_builder_add(&b_out, "(i@a{sv})", 5, self->build_item_props(5, i18n.get(StringId::TrayQuit), false));

        g_dbus_method_invocation_return_value(inv, g_variant_new("(@a(ia{sv}))", g_variant_builder_end(&b_out)));
        return;
    }

    if (g_strcmp0(method, "Event") == 0) {
        gint id = 0;
        const gchar *event_id = nullptr;
        GVariant *data_v = nullptr;
        guint timestamp = 0;
        g_variant_get(params, "(isvu)", &id, &event_id, &data_v, &timestamp);

        if (g_strcmp0(event_id, "clicked") == 0) {
            switch (id) {
                case 1:
                    if (self->on_toggle_) {
                        g_idle_add(+[](gpointer p) -> gboolean {
                            auto *cb = static_cast<std::function<void()>*>(p);
                            (*cb)();
                            return G_SOURCE_REMOVE;
                        }, &self->on_toggle_);
                    }
                    break;
                case 2:
                    if (self->on_clear_history_) {
                        g_idle_add(+[](gpointer p) -> gboolean {
                            auto *cb = static_cast<std::function<void()>*>(p);
                            (*cb)();
                            return G_SOURCE_REMOVE;
                        }, &self->on_clear_history_);
                    }
                    break;
                case 3:
                    if (self->on_open_settings_) {
                        g_idle_add(+[](gpointer p) -> gboolean {
                            auto *cb = static_cast<std::function<void()>*>(p);
                            (*cb)();
                            return G_SOURCE_REMOVE;
                        }, &self->on_open_settings_);
                    }
                    break;
                case 5:
                    if (self->on_quit_) {
                        g_idle_add(+[](gpointer p) -> gboolean {
                            auto *cb = static_cast<std::function<void()>*>(p);
                            (*cb)();
                            return G_SOURCE_REMOVE;
                        }, &self->on_quit_);
                    }
                    break;
            }
        }

        if (data_v) g_variant_unref(data_v);
        g_dbus_method_invocation_return_value(inv, nullptr);
        return;
    }

    if (g_strcmp0(method, "AboutToShow") == 0) {
        g_dbus_method_invocation_return_value(inv, g_variant_new("(b)", FALSE));
        return;
    }

    g_dbus_method_invocation_return_value(inv, nullptr);
}

GVariant* TrayManager::menu_get_prop(
    GDBusConnection*, const gchar*, const gchar*, const gchar*,
    const gchar *prop, GError**, gpointer) {
    if (g_strcmp0(prop, "Version") == 0) return g_variant_new_uint32(3);
    if (g_strcmp0(prop, "TextDirection") == 0) return g_variant_new_string("ltr");
    if (g_strcmp0(prop, "Status") == 0) return g_variant_new_string("normal");
    if (g_strcmp0(prop, "IconThemePath") == 0) {
        std::string icon_path = locate_icon_file();
        if (!icon_path.empty()) {
            std::string dir = fs::path(icon_path).parent_path().string();
            const gchar *arr[] = { dir.c_str(), nullptr };
            return g_variant_new_strv(arr, -1);
        }
        const gchar *arr[] = { nullptr };
        return g_variant_new_strv(arr, 0);
    }
    return nullptr;
}

} // namespace tools::ui
