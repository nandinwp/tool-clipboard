#include "tools/ui/clipboard_window.hpp"
#include "tools/ui/settings_window.hpp"
#include "tools/core/i18n.hpp"
#include "tools/core/keyboard_synthesizer.hpp"
#include <iostream>
#include <filesystem>
#include <vector>

namespace tools::ui {

using namespace tools::core;

ClipboardWindow::~ClipboardWindow() = default;

static std::string find_app_icon_path() {
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
    candidates.push_back("/usr/share/icons/hicolor/scalable/apps/tools-clipboard.png");
    candidates.push_back("/usr/share/tools-clipboard/compliance.png");
    candidates.push_back("/usr/lib/tools-clipboard/resources/compliance.png");

    try {
        if (std::filesystem::exists("/proc/self/exe")) {
            auto exe_dir = std::filesystem::canonical("/proc/self/exe").parent_path();
            candidates.push_back((exe_dir / "resources" / "compliance.png").string());
            candidates.push_back((exe_dir / "compliance.png").string());
            candidates.push_back((exe_dir / ".." / "resources" / "compliance.png").string());
            candidates.push_back((exe_dir / ".." / "share" / "tools-clipboard" / "compliance.png").string());
        }
    } catch (...) {}

    for (const auto &p : candidates) {
        if (std::filesystem::exists(p)) {
            return p;
        }
    }
    return "";
}

static gboolean on_window_close_request(GtkWindow *window, gpointer user_data) {
    auto *self = static_cast<ClipboardWindow*>(user_data);
    self->hide();
    return TRUE; // Evita que a janela seja destruída
}

ClipboardWindow::ClipboardWindow(GtkApplication *app, tools::core::ClipboardManager &manager)
    : app_(app), manager_(manager) {
    setup_ui();

    // Atualiza automaticamente a lista quando um novo texto for copiado no sistema
    manager_.set_update_callback([this]() {
        if (window_ && gtk_widget_get_visible(window_)) {
            rebuild_item_list();
        }
    });
}

bool ClipboardWindow::is_visible() const {
    return window_ && gtk_widget_get_visible(window_);
}

void ClipboardWindow::show_and_refresh() {
    if (!window_) return;

    manager_.check_current_clipboard();
    update_texts();
    rebuild_item_list();
    gtk_widget_set_visible(window_, TRUE);
    gtk_window_present(GTK_WINDOW(window_));
}

void ClipboardWindow::hide() {
    if (window_) {
        gtk_widget_set_visible(window_, FALSE);
    }
}

void ClipboardWindow::toggle() {
    if (is_visible()) {
        hide();
    } else {
        show_and_refresh();
    }
}

void ClipboardWindow::open_settings() {
    if (!settings_window_) {
        settings_window_ = std::make_unique<SettingsWindow>(GTK_WINDOW(window_));
    }
    settings_window_->show();
}

void ClipboardWindow::update_texts() {
    auto &i18n = I18n::instance();
    if (window_) {
        gtk_window_set_title(GTK_WINDOW(window_), i18n.get(StringId::WindowTitle).c_str());
    }
    if (title_label_) {
        gtk_label_set_text(GTK_LABEL(title_label_), i18n.get(StringId::WindowTitle).c_str());
    }
    if (clear_btn_) {
        gtk_button_set_label(GTK_BUTTON(clear_btn_), i18n.get(StringId::ClearAll).c_str());
    }
    if (settings_btn_) {
        gtk_widget_set_tooltip_text(settings_btn_, i18n.get(StringId::SettingsTitle).c_str());
    }
}

void ClipboardWindow::setup_ui() {
    auto &i18n = I18n::instance();

    window_ = gtk_application_window_new(app_);
    gtk_window_set_title(GTK_WINDOW(window_), i18n.get(StringId::WindowTitle).c_str());
    gtk_window_set_default_size(GTK_WINDOW(window_), 420, 530);
    gtk_window_set_resizable(GTK_WINDOW(window_), TRUE);

    // Oculta em vez de fechar
    g_signal_connect(window_, "close-request", G_CALLBACK(on_window_close_request), this);

    // Captura a tecla Escape para fechar rapidamente
    GtkEventController *key_controller = gtk_event_controller_key_new();
    g_signal_connect(key_controller, "key-pressed", G_CALLBACK(+[](GtkEventControllerKey*, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
        if (keyval == GDK_KEY_Escape) {
            auto *win = static_cast<ClipboardWindow*>(data);
            win->hide();
            return TRUE;
        }
        return FALSE;
    }), this);
    gtk_widget_add_controller(window_, key_controller);

    // Ao ganhar foco, sincroniza a área de transferência (otimização Wayland)
    g_signal_connect(window_, "notify::is-active", G_CALLBACK(+[](GObject *obj, GParamSpec*, gpointer data) {
        if (gtk_window_is_active(GTK_WINDOW(obj))) {
            auto *win = static_cast<ClipboardWindow*>(data);
            win->manager_.check_current_clipboard();
        }
    }), this);

    // Container raiz
    GtkWidget *root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(root_box, "window-container");
    gtk_window_set_child(GTK_WINDOW(window_), root_box);

    // 1. Cabeçalho estilo Win + V
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_bottom(header_box, 6);
    gtk_box_append(GTK_BOX(root_box), header_box);

    // Ícone da aplicação no cabeçalho
    std::string icon_path = find_app_icon_path();
    if (!icon_path.empty()) {
        GtkWidget *app_icon_img = gtk_image_new_from_file(icon_path.c_str());
        gtk_image_set_pixel_size(GTK_IMAGE(app_icon_img), 20);
        gtk_widget_add_css_class(app_icon_img, "header-app-icon");
        gtk_box_append(GTK_BOX(header_box), app_icon_img);

        GdkDisplay *display = gdk_display_get_default();
        if (display) {
            GtkIconTheme *theme = gtk_icon_theme_get_for_display(display);
            try {
                auto dir = std::filesystem::path(icon_path).parent_path().string();
                gtk_icon_theme_add_search_path(theme, dir.c_str());
            } catch (...) {}
        }
    }
    gtk_window_set_icon_name(GTK_WINDOW(window_), "tools-clipboard");

    title_label_ = gtk_label_new(i18n.get(StringId::WindowTitle).c_str());
    gtk_widget_add_css_class(title_label_, "header-title");
    gtk_box_append(GTK_BOX(header_box), title_label_);

    GtkWidget *badge = gtk_label_new("Win + V");
    gtk_widget_add_css_class(badge, "shortcut-badge");
    gtk_box_append(GTK_BOX(header_box), badge);

    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(header_box), spacer);

    // Seletor de idioma (PT | EN | ES)
    const char * const lang_labels[] = {
        "PT",
        "EN",
        "ES",
        nullptr
    };
    lang_dropdown_ = gtk_drop_down_new_from_strings(lang_labels);
    gtk_widget_add_css_class(lang_dropdown_, "lang-dropdown");
    gtk_drop_down_set_selected(
        GTK_DROP_DOWN(lang_dropdown_),
        static_cast<guint>(i18n.get_language())
    );
    g_signal_connect(lang_dropdown_, "notify::selected", G_CALLBACK(+[](GObject *obj, GParamSpec *, gpointer data) {
        auto *self = static_cast<ClipboardWindow*>(data);
        guint sel = gtk_drop_down_get_selected(GTK_DROP_DOWN(obj));
        I18n::instance().set_language(static_cast<Language>(sel));
        self->update_texts();
        self->rebuild_item_list();
    }), this);
    gtk_box_append(GTK_BOX(header_box), lang_dropdown_);

    // Botão Limpar Tudo
    clear_btn_ = gtk_button_new_with_label(i18n.get(StringId::ClearAll).c_str());
    gtk_widget_add_css_class(clear_btn_, "clear-button");
    g_signal_connect_swapped(clear_btn_, "clicked", G_CALLBACK(+[](ClipboardWindow *self) {
        self->manager_.clear_history();
        self->rebuild_item_list();
    }), this);
    gtk_box_append(GTK_BOX(header_box), clear_btn_);

    // Botão de Configurações (⚙)
    settings_btn_ = gtk_button_new_with_label("⚙");
    gtk_widget_add_css_class(settings_btn_, "header-settings-btn");
    gtk_widget_set_tooltip_text(settings_btn_, i18n.get(StringId::SettingsTitle).c_str());
    g_signal_connect_swapped(settings_btn_, "clicked", G_CALLBACK(+[](ClipboardWindow *self) {
        if (!self->settings_window_) {
            self->settings_window_ = std::make_unique<SettingsWindow>(GTK_WINDOW(self->window_));
        }
        self->settings_window_->show();
    }), this);
    gtk_box_append(GTK_BOX(header_box), settings_btn_);

    // 2. Área de rolagem para os 10 itens
    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_box_append(GTK_BOX(root_box), scrolled);

    list_container_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(list_container_, 4);
    gtk_widget_set_margin_bottom(list_container_, 4);
    gtk_widget_set_margin_start(list_container_, 2);
    gtk_widget_set_margin_end(list_container_, 2);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), list_container_);

    // Constrói a lista inicial
    rebuild_item_list();
}

void ClipboardWindow::rebuild_item_list() {
    if (!list_container_) return;

    auto &i18n = I18n::instance();

    // Remove filhos anteriores
    GtkWidget *child = gtk_widget_get_first_child(list_container_);
    while (child != nullptr) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(list_container_), child);
        child = next;
    }

    const auto &items = manager_.get_items();

    if (items.empty()) {
        GtkWidget *empty_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_widget_set_halign(empty_box, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(empty_box, GTK_ALIGN_CENTER);
        gtk_widget_set_vexpand(empty_box, TRUE);

        GtkWidget *empty_label = gtk_label_new(i18n.get(StringId::EmptyTitle).c_str());
        gtk_widget_add_css_class(empty_label, "header-title");
        gtk_box_append(GTK_BOX(empty_box), empty_label);

        GtkWidget *hint_label = gtk_label_new(i18n.get(StringId::EmptyHint).c_str());
        gtk_widget_add_css_class(hint_label, "empty-state-label");
        gtk_label_set_justify(GTK_LABEL(hint_label), GTK_JUSTIFY_CENTER);
        gtk_box_append(GTK_BOX(empty_box), hint_label);

        gtk_box_append(GTK_BOX(list_container_), empty_box);
        return;
    }

    for (size_t i = 0; i < items.size(); ++i) {
        gtk_box_append(GTK_BOX(list_container_), create_item_card(items[i], i));
    }
}

GtkWidget* ClipboardWindow::create_item_card(const tools::core::ClipboardItem &item, size_t index) {
    auto &i18n = I18n::instance();

    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_add_css_class(card, "clipboard-card");

    // Linha superior: [#1] 14:05 e botão de deletar
    GtkWidget *top_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(card), top_row);

    std::string badge_str = "#" + std::to_string(index + 1);
    GtkWidget *idx_badge = gtk_label_new(badge_str.c_str());
    gtk_widget_add_css_class(idx_badge, "item-index-badge");
    gtk_box_append(GTK_BOX(top_row), idx_badge);

    GtkWidget *time_label = gtk_label_new(item.timestamp.c_str());
    gtk_widget_add_css_class(time_label, "item-time");
    gtk_box_append(GTK_BOX(top_row), time_label);

    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(top_row), spacer);

    if (item.pinned) {
        gtk_widget_add_css_class(card, "pinned");
    }

    // Botão de fixar (Pin)
    GtkWidget *pin_btn = gtk_button_new_with_label(item.pinned ? "📌" : "📍");
    gtk_widget_add_css_class(pin_btn, "item-pin-btn");
    if (item.pinned) {
        gtk_widget_add_css_class(pin_btn, "pinned");
        gtk_widget_set_tooltip_text(pin_btn, i18n.get(StringId::UnpinTooltip).c_str());
    } else {
        gtk_widget_set_tooltip_text(pin_btn, i18n.get(StringId::PinTooltip).c_str());
    }

    struct PinData {
        ClipboardWindow *win;
        size_t idx;
    };
    auto *pin_data = new PinData{this, index};
    g_signal_connect_data(
        pin_btn,
        "clicked",
        G_CALLBACK(+[](GtkButton*, gpointer p) {
            auto *d = static_cast<PinData*>(p);
            d->win->manager_.toggle_pin(d->idx);
            d->win->rebuild_item_list();
        }),
        pin_data,
        [](gpointer p, GClosure*) { delete static_cast<PinData*>(p); },
        G_CONNECT_DEFAULT
    );
    gtk_box_append(GTK_BOX(top_row), pin_btn);

    // Botão de deletar item específico
    GtkWidget *del_btn = gtk_button_new_with_label("✕");
    gtk_widget_add_css_class(del_btn, "item-delete-btn");
    gtk_widget_set_tooltip_text(del_btn, i18n.get(StringId::DeleteTooltip).c_str());

    struct DelData {
        ClipboardWindow *win;
        size_t idx;
    };
    auto *del_data = new DelData{this, index};
    g_signal_connect_data(
        del_btn,
        "clicked",
        G_CALLBACK(+[](GtkButton*, gpointer p) {
            auto *d = static_cast<DelData*>(p);
            d->win->manager_.remove_item(d->idx);
            d->win->rebuild_item_list();
        }),
        del_data,
        [](gpointer p, GClosure*) { delete static_cast<DelData*>(p); },
        G_CONNECT_DEFAULT
    );
    gtk_box_append(GTK_BOX(top_row), del_btn);

    // Conteúdo pré-visualizado (truncado para visualização agradável)
    std::string preview_text = item.text;
    if (preview_text.length() > 250) {
        preview_text = preview_text.substr(0, 250) + "...";
    }

    GtkWidget *content_label = gtk_label_new(preview_text.c_str());
    gtk_widget_add_css_class(content_label, "item-content-preview");
    gtk_label_set_wrap(GTK_LABEL(content_label), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(content_label), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_lines(GTK_LABEL(content_label), 4);
    gtk_label_set_ellipsize(GTK_LABEL(content_label), PANGO_ELLIPSIZE_END);
    gtk_label_set_xalign(GTK_LABEL(content_label), 0.0f);
    gtk_box_append(GTK_BOX(card), content_label);

    // Gesto de clique no card: Copia para o clipboard, fecha a janela e cola automaticamente
    GtkGesture *click_gesture = gtk_gesture_click_new();
    struct ClickData {
        ClipboardWindow *win;
        size_t idx;
        GtkWidget *card;
        GtkWidget *pin_btn;
        GtkWidget *del_btn;
    };
    auto *click_data = new ClickData{this, index, card, pin_btn, del_btn};
    g_signal_connect_data(
        click_gesture,
        "released",
        G_CALLBACK(+[](GtkGestureClick*, int, double x, double y, gpointer p) {
            auto *d = static_cast<ClickData*>(p);

            // Verifica se o clique ocorreu sobre o botão de fixar ou excluir (ou seus filhos)
            GtkWidget *hit = gtk_widget_pick(d->card, x, y, GTK_PICK_DEFAULT);
            if (hit) {
                if (hit == d->pin_btn || hit == d->del_btn ||
                    gtk_widget_is_ancestor(hit, d->pin_btn) ||
                    gtk_widget_is_ancestor(hit, d->del_btn)) {
                    return; // Ignora se clicou em Pin ou Delete
                }
            }

            // Copia para o clipboard ativo
            d->win->manager_.copy_to_clipboard(d->idx);

            // Oculta a janela flyout
            d->win->hide();

            // Dispara simulação de Ctrl+V na janela anterior com pequeno delay para o compositor devolver o foco
            g_timeout_add(150, [](gpointer) -> gboolean {
                tools::core::KeyboardSynthesizer::instance().simulate_paste();
                return G_SOURCE_REMOVE;
            }, nullptr);
        }),
        click_data,
        [](gpointer p, GClosure*) { delete static_cast<ClickData*>(p); },
        G_CONNECT_DEFAULT
    );
    gtk_widget_add_controller(card, GTK_EVENT_CONTROLLER(click_gesture));

    return card;
}

} // namespace tools::ui
