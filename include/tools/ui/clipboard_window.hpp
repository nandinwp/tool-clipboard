#pragma once

#include "tools/core/clipboard_manager.hpp"
#include <gtk/gtk.h>
#include <memory>

namespace tools::ui {

class SettingsWindow;

class ClipboardWindow {
public:
    ClipboardWindow(GtkApplication *app, tools::core::ClipboardManager &manager);
    ~ClipboardWindow();

    // Exibe a janela flutuante e recarrega os itens
    void show_and_refresh();

    // Oculta a janela
    void hide();

    // Alterna visibilidade (Toggle estilo Win + V)
    void toggle();

    // Retorna se a janela está atualmente visível
    bool is_visible() const;

    GtkWidget* get_widget() const { return window_; }

private:
    void setup_ui();
    void update_texts();
    void rebuild_item_list();
    GtkWidget* create_item_card(const tools::core::ClipboardItem &item, size_t index);

    GtkWidget *window_{nullptr};
    GtkWidget *title_label_{nullptr};
    GtkWidget *clear_btn_{nullptr};
    GtkWidget *settings_btn_{nullptr};
    GtkWidget *lang_dropdown_{nullptr};
    GtkWidget *list_container_{nullptr};
    GtkApplication *app_{nullptr};
    tools::core::ClipboardManager &manager_;
    std::unique_ptr<SettingsWindow> settings_window_;
};

} // namespace tools::ui
