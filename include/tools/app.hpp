#pragma once

#include "tools/ui/clipboard_window.hpp"
#include "tools/ui/tray_manager.hpp"
#include "tools/core/clipboard_manager.hpp"
#include <gtk/gtk.h>
#include <memory>

namespace tools {

class Application {
public:
    Application();
    ~Application();

    // Inicia a execução da aplicação GTK
    int run(int argc, char **argv);

    tools::core::ClipboardManager& get_clipboard_manager() { return clipboard_manager_; }
    tools::ui::ClipboardWindow* get_window() const { return window_.get(); }
    tools::ui::TrayManager* get_tray_manager() const { return tray_manager_.get(); }

private:
    static void on_activate(GtkApplication *gtk_app, gpointer user_data);
    static int on_command_line(GApplication *app, GApplicationCommandLine *cmdline, gpointer user_data);
    void setup_tray();

    GtkApplication *gtk_app_{nullptr};
    std::unique_ptr<tools::ui::ClipboardWindow> window_;
    std::unique_ptr<tools::ui::TrayManager> tray_manager_;
    tools::core::ClipboardManager clipboard_manager_;
    bool is_held_{false};
    bool monitoring_started_{false};
    bool tray_started_{false};
};

} // namespace tools
