#pragma once

#include <gio/gio.h>
#include <functional>
#include <string>

namespace tools::ui {

class TrayManager {
public:
    TrayManager();
    ~TrayManager();

    bool start();
    void stop();

    void set_on_toggle(std::function<void()> cb) { on_toggle_ = std::move(cb); }
    void set_on_clear_history(std::function<void()> cb) { on_clear_history_ = std::move(cb); }
    void set_on_open_settings(std::function<void()> cb) { on_open_settings_ = std::move(cb); }
    void set_on_quit(std::function<void()> cb) { on_quit_ = std::move(cb); }

    void refresh_menu();

private:
    void register_dbus_objects();
    void register_with_watcher();

    static void sni_method_call(GDBusConnection *conn, const gchar *sender, const gchar *path,
                                const gchar *iface, const gchar *method, GVariant *params,
                                GDBusMethodInvocation *inv, gpointer data);
    static GVariant* sni_get_prop(GDBusConnection *conn, const gchar *sender, const gchar *path,
                                  const gchar *iface, const gchar *prop, GError **err, gpointer data);

    static void menu_method_call(GDBusConnection *conn, const gchar *sender, const gchar *path,
                                 const gchar *iface, const gchar *method, GVariant *params,
                                 GDBusMethodInvocation *inv, gpointer data);
    static GVariant* menu_get_prop(GDBusConnection *conn, const gchar *sender, const gchar *path,
                                   const gchar *iface, const gchar *prop, GError **err, gpointer data);

    GVariant* create_icon_pixmap_variant() const;
    GVariant* build_item_props(int id, const std::string &label, bool is_separator) const;
    GVariant* build_item_variant(int id, const std::string &label, bool is_separator) const;

    GDBusConnection *bus_{nullptr};
    guint sni_reg_id_{0};
    guint menu_reg_id_{0};
    guint watcher_sub_id_{0};
    guint32 menu_revision_{1};

    std::function<void()> on_toggle_;
    std::function<void()> on_clear_history_;
    std::function<void()> on_open_settings_;
    std::function<void()> on_quit_;
};

} // namespace tools::ui
