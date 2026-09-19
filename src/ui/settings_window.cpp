#include "tools/ui/settings_window.hpp"
#include "tools/core/i18n.hpp"
#include <filesystem>
#include <vector>
#include <iostream>

namespace tools::ui {

using namespace tools::core;

static std::string find_icon_path() {
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

static gboolean on_close_request(GtkWindow*, gpointer user_data) {
    auto *self = static_cast<SettingsWindow*>(user_data);
    self->hide();
    return TRUE;
}

SettingsWindow::SettingsWindow(GtkWindow *parent)
    : parent_(parent) {
    setup_ui();

    // Sincroniza textos quando o idioma for alterado
    I18n::instance().on_language_changed([this](Language) {
        update_texts();
    });

    // Sincroniza valores se forem alterados externamente
    SettingsManager::instance().on_changed([this]() {
        on_settings_changed();
    });
}

bool SettingsWindow::is_visible() const {
    return window_ && gtk_widget_get_visible(window_);
}

void SettingsWindow::show() {
    if (!window_) return;
    update_texts();
    on_settings_changed();
    gtk_widget_set_visible(window_, TRUE);
    gtk_window_present(GTK_WINDOW(window_));
}

void SettingsWindow::hide() {
    if (window_) {
        gtk_widget_set_visible(window_, FALSE);
    }
}

void SettingsWindow::on_settings_changed() {
    if (updating_ui_) return;
    updating_ui_ = true;

    auto &sm = SettingsManager::instance();
    if (max_items_spin_) {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(max_items_spin_), sm.get_max_items());
    }
    if (autostart_switch_) {
        gtk_switch_set_active(GTK_SWITCH(autostart_switch_), sm.is_autostart_enabled());
    }

    updating_ui_ = false;
}

void SettingsWindow::update_texts() {
    auto &i18n = I18n::instance();

    if (window_) {
        gtk_window_set_title(GTK_WINDOW(window_), i18n.get(StringId::SettingsTitle).c_str());
    }
    if (header_title_label_) {
        gtk_label_set_text(GTK_LABEL(header_title_label_), i18n.get(StringId::SettingsTitle).c_str());
    }
    if (pref_section_label_) {
        gtk_label_set_text(GTK_LABEL(pref_section_label_), i18n.get(StringId::PreferencesTitle).c_str());
    }
    if (max_items_label_) {
        gtk_label_set_text(GTK_LABEL(max_items_label_), i18n.get(StringId::MaxItemsLabel).c_str());
    }
    if (autostart_label_) {
        gtk_label_set_text(GTK_LABEL(autostart_label_), i18n.get(StringId::AutostartLabel).c_str());
    }
    if (about_section_label_) {
        gtk_label_set_text(GTK_LABEL(about_section_label_), i18n.get(StringId::AboutTitle).c_str());
    }
    if (dev_label_) {
        gtk_label_set_text(GTK_LABEL(dev_label_), i18n.get(StringId::DevelopedByLabel).c_str());
    }
    if (date_label_) {
        gtk_label_set_text(GTK_LABEL(date_label_), i18n.get(StringId::DevelopedDateLabel).c_str());
    }
    if (close_btn_) {
        gtk_button_set_label(GTK_BUTTON(close_btn_), i18n.get(StringId::CloseButton).c_str());
    }
}

void SettingsWindow::setup_ui() {
    auto &i18n = I18n::instance();
    auto &sm = SettingsManager::instance();

    window_ = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(window_), i18n.get(StringId::SettingsTitle).c_str());
    gtk_window_set_default_size(GTK_WINDOW(window_), 400, 480);
    gtk_window_set_resizable(GTK_WINDOW(window_), FALSE);

    if (parent_) {
        gtk_window_set_transient_for(GTK_WINDOW(window_), parent_);
        gtk_window_set_modal(GTK_WINDOW(window_), TRUE);
    }

    g_signal_connect(window_, "close-request", G_CALLBACK(on_close_request), this);

    // Escape fecha
    GtkEventController *key_controller = gtk_event_controller_key_new();
    g_signal_connect(key_controller, "key-pressed", G_CALLBACK(+[](GtkEventControllerKey*, guint keyval, guint, GdkModifierType, gpointer data) -> gboolean {
        if (keyval == GDK_KEY_Escape) {
            auto *self = static_cast<SettingsWindow*>(data);
            self->hide();
            return TRUE;
        }
        return FALSE;
    }), this);
    gtk_widget_add_controller(window_, key_controller);

    // Container raiz
    GtkWidget *root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_widget_add_css_class(root_box, "window-container");
    gtk_window_set_child(GTK_WINDOW(window_), root_box);

    // --- Header ---
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(root_box), header_box);

    GtkWidget *gear_icon = gtk_label_new("⚙");
    gtk_widget_add_css_class(gear_icon, "settings-gear-icon");
    gtk_box_append(GTK_BOX(header_box), gear_icon);

    header_title_label_ = gtk_label_new(i18n.get(StringId::SettingsTitle).c_str());
    gtk_widget_add_css_class(header_title_label_, "header-title");
    gtk_box_append(GTK_BOX(header_box), header_title_label_);

    // --- Seção 1: Preferências ---
    pref_section_label_ = gtk_label_new(i18n.get(StringId::PreferencesTitle).c_str());
    gtk_widget_add_css_class(pref_section_label_, "section-title");
    gtk_label_set_xalign(GTK_LABEL(pref_section_label_), 0.0f);
    gtk_box_append(GTK_BOX(root_box), pref_section_label_);

    GtkWidget *pref_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(pref_card, "settings-card");
    gtk_box_append(GTK_BOX(root_box), pref_card);

    // Linha 1: Quantidade de Itens
    GtkWidget *row_max = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(pref_card), row_max);

    max_items_label_ = gtk_label_new(i18n.get(StringId::MaxItemsLabel).c_str());
    gtk_widget_add_css_class(max_items_label_, "settings-item-label");
    gtk_label_set_xalign(GTK_LABEL(max_items_label_), 0.0f);
    gtk_widget_set_hexpand(max_items_label_, TRUE);
    gtk_box_append(GTK_BOX(row_max), max_items_label_);

    max_items_spin_ = gtk_spin_button_new_with_range(3, 100, 1);
    gtk_widget_add_css_class(max_items_spin_, "settings-spin");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(max_items_spin_), sm.get_max_items());
    g_signal_connect(max_items_spin_, "value-changed", G_CALLBACK(+[](GtkSpinButton *spin, gpointer data) {
        auto *self = static_cast<SettingsWindow*>(data);
        if (self->updating_ui_) return;
        int val = gtk_spin_button_get_value_as_int(spin);
        SettingsManager::instance().set_max_items(val);
    }), this);
    gtk_box_append(GTK_BOX(row_max), max_items_spin_);

    // Separador sutil
    GtkWidget *sep1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_add_css_class(sep1, "settings-separator");
    gtk_box_append(GTK_BOX(pref_card), sep1);

    // Linha 2: Iniciar Automaticamente
    GtkWidget *row_autostart = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(pref_card), row_autostart);

    autostart_label_ = gtk_label_new(i18n.get(StringId::AutostartLabel).c_str());
    gtk_widget_add_css_class(autostart_label_, "settings-item-label");
    gtk_label_set_xalign(GTK_LABEL(autostart_label_), 0.0f);
    gtk_widget_set_hexpand(autostart_label_, TRUE);
    gtk_box_append(GTK_BOX(row_autostart), autostart_label_);

    autostart_switch_ = gtk_switch_new();
    gtk_widget_add_css_class(autostart_switch_, "settings-switch");
    gtk_switch_set_active(GTK_SWITCH(autostart_switch_), sm.is_autostart_enabled());
    g_signal_connect(autostart_switch_, "state-set", G_CALLBACK(+[](GtkSwitch*, gboolean state, gpointer data) -> gboolean {
        auto *self = static_cast<SettingsWindow*>(data);
        if (self->updating_ui_) return FALSE;
        SettingsManager::instance().set_autostart_enabled(state);
        return FALSE; // Permite ao GTK atualizar a animação
    }), this);
    gtk_box_append(GTK_BOX(row_autostart), autostart_switch_);

    // --- Seção 2: Informações Sobre ---
    about_section_label_ = gtk_label_new(i18n.get(StringId::AboutTitle).c_str());
    gtk_widget_add_css_class(about_section_label_, "section-title");
    gtk_label_set_xalign(GTK_LABEL(about_section_label_), 0.0f);
    gtk_box_append(GTK_BOX(root_box), about_section_label_);

    GtkWidget *about_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_add_css_class(about_card, "settings-card");
    gtk_box_append(GTK_BOX(root_box), about_card);

    // Logo e Nome do Aplicativo
    GtkWidget *app_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_append(GTK_BOX(about_card), app_header);

    std::string icon_path = find_icon_path();
    if (!icon_path.empty()) {
        GtkWidget *icon_img = gtk_image_new_from_file(icon_path.c_str());
        gtk_image_set_pixel_size(GTK_IMAGE(icon_img), 48);
        gtk_widget_add_css_class(icon_img, "about-app-icon");
        gtk_box_append(GTK_BOX(app_header), icon_img);
    }

    GtkWidget *app_meta_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_valign(app_meta_box, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(app_header), app_meta_box);

    GtkWidget *app_name = gtk_label_new("Tools Clipboard");
    gtk_widget_add_css_class(app_name, "about-app-name");
    gtk_label_set_xalign(GTK_LABEL(app_name), 0.0f);
    gtk_box_append(GTK_BOX(app_meta_box), app_name);

    GtkWidget *app_version = gtk_label_new("Versão 1.0.0");
    gtk_widget_add_css_class(app_version, "about-app-version");
    gtk_label_set_xalign(GTK_LABEL(app_version), 0.0f);
    gtk_box_append(GTK_BOX(app_meta_box), app_version);

    GtkWidget *sep2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_add_css_class(sep2, "settings-separator");
    gtk_box_append(GTK_BOX(about_card), sep2);

    // Detalhes do Desenvolvedor e Data
    dev_label_ = gtk_label_new(i18n.get(StringId::DevelopedByLabel).c_str());
    gtk_widget_add_css_class(dev_label_, "about-info-text");
    gtk_label_set_xalign(GTK_LABEL(dev_label_), 0.0f);
    gtk_box_append(GTK_BOX(about_card), dev_label_);

    date_label_ = gtk_label_new(i18n.get(StringId::DevelopedDateLabel).c_str());
    gtk_widget_add_css_class(date_label_, "about-info-text");
    gtk_label_set_xalign(GTK_LABEL(date_label_), 0.0f);
    gtk_box_append(GTK_BOX(about_card), date_label_);

    // Linha com links oficiais: bnbb.com.br e contato@bnbb.com.br
    GtkWidget *links_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_top(links_box, 4);
    gtk_box_append(GTK_BOX(about_card), links_box);

    website_btn_ = gtk_link_button_new_with_label("https://bnbb.com.br", "bnbb.com.br");
    gtk_widget_add_css_class(website_btn_, "about-link-btn");
    gtk_box_append(GTK_BOX(links_box), website_btn_);

    contact_btn_ = gtk_link_button_new_with_label("mailto:contato@bnbb.com.br", "contato@bnbb.com.br");
    gtk_widget_add_css_class(contact_btn_, "about-link-btn");
    gtk_box_append(GTK_BOX(links_box), contact_btn_);

    // Spacer
    GtkWidget *bottom_spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(bottom_spacer, TRUE);
    gtk_box_append(GTK_BOX(root_box), bottom_spacer);

    // Botão Fechar
    close_btn_ = gtk_button_new_with_label(i18n.get(StringId::CloseButton).c_str());
    gtk_widget_add_css_class(close_btn_, "settings-close-btn");
    g_signal_connect_swapped(close_btn_, "clicked", G_CALLBACK(+[](SettingsWindow *self) {
        self->hide();
    }), this);
    gtk_box_append(GTK_BOX(root_box), close_btn_);
}

} // namespace tools::ui
