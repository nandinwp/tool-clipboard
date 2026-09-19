#pragma once

#include <gtk/gtk.h>
#include "tools/core/settings_manager.hpp"

namespace tools::ui {

class SettingsWindow {
public:
    explicit SettingsWindow(GtkWindow *parent = nullptr);
    ~SettingsWindow() = default;

    void show();
    void hide();
    bool is_visible() const;

    GtkWidget* get_widget() const { return window_; }

private:
    void setup_ui();
    void update_texts();
    void on_settings_changed();

    GtkWindow *parent_{nullptr};
    GtkWidget *window_{nullptr};
    GtkWidget *header_title_label_{nullptr};
    GtkWidget *pref_section_label_{nullptr};
    GtkWidget *max_items_label_{nullptr};
    GtkWidget *max_items_spin_{nullptr};
    GtkWidget *autostart_label_{nullptr};
    GtkWidget *autostart_switch_{nullptr};
    GtkWidget *about_section_label_{nullptr};
    GtkWidget *dev_label_{nullptr};
    GtkWidget *date_label_{nullptr};
    GtkWidget *website_btn_{nullptr};
    GtkWidget *contact_btn_{nullptr};
    GtkWidget *close_btn_{nullptr};

    bool updating_ui_{false};
};

} // namespace tools::ui
