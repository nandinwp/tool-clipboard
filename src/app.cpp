#include "tools/app.hpp"
#include "tools/ui/style.hpp"
#include "tools/core/shortcut_manager.hpp"
#include <iostream>
#include <cstring>
#include <filesystem>

namespace tools {

namespace fs = std::filesystem;

Application::Application() {
    gtk_app_ = gtk_application_new(
        "com.tools.clipboard",
        static_cast<GApplicationFlags>(G_APPLICATION_HANDLES_COMMAND_LINE)
    );

    g_signal_connect(gtk_app_, "activate", G_CALLBACK(on_activate), this);
    g_signal_connect(gtk_app_, "command-line", G_CALLBACK(on_command_line), this);
}

Application::~Application() {
    if (is_held_ && gtk_app_) {
        g_application_release(G_APPLICATION(gtk_app_));
        is_held_ = false;
    }
    if (gtk_app_) {
        g_object_unref(gtk_app_);
        gtk_app_ = nullptr;
    }
}

int Application::run(int argc, char **argv) {
    return g_application_run(G_APPLICATION(gtk_app_), argc, argv);
}

void Application::setup_tray() {
    if (tray_started_) return;
    tray_started_ = true;

    tray_manager_ = std::make_unique<tools::ui::TrayManager>();
    tray_manager_->set_on_toggle([this]() {
        if (window_) {
            window_->toggle();
        }
    });
    tray_manager_->set_on_clear_history([this]() {
        clipboard_manager_.clear_history();
    });
    tray_manager_->set_on_open_settings([this]() {
        if (window_) {
            window_->open_settings();
        }
    });
    tray_manager_->set_on_quit([this]() {
        if (gtk_app_) {
            g_application_quit(G_APPLICATION(gtk_app_));
        }
    });

    tray_manager_->start();
}

void Application::on_activate(GtkApplication *gtk_app, gpointer user_data) {
    auto *self = static_cast<Application*>(user_data);

    // Mantém a aplicação viva em segundo plano no sistema
    if (!self->is_held_) {
        g_application_hold(G_APPLICATION(gtk_app));
        self->is_held_ = true;
    }

    tools::ui::apply_global_css();

    // Inicia o monitoramento da área de transferência
    if (!self->monitoring_started_) {
        GdkDisplay *display = gdk_display_get_default();
        self->clipboard_manager_.start_monitoring(display);
        self->monitoring_started_ = true;
    }

    if (!self->window_) {
        self->window_ = std::make_unique<tools::ui::ClipboardWindow>(
            gtk_app,
            self->clipboard_manager_
        );
    }

    // Inicializa o ícone da bandeja do sistema (System Tray)
    self->setup_tray();

    self->window_->show_and_refresh();
}

int Application::on_command_line(GApplication *app, GApplicationCommandLine *cmdline, gpointer user_data) {
    auto *self = static_cast<Application*>(user_data);

    int argc = 0;
    char **argv = g_application_command_line_get_arguments(cmdline, &argc);

    // Mantém o daemon vivo em segundo plano
    if (!self->is_held_) {
        g_application_hold(app);
        self->is_held_ = true;
    }

    tools::ui::apply_global_css();

    // Inicia monitoramento se ainda não iniciado
    if (!self->monitoring_started_) {
        GdkDisplay *display = gdk_display_get_default();
        self->clipboard_manager_.start_monitoring(display);
        self->monitoring_started_ = true;
    }

    if (!self->window_) {
        self->window_ = std::make_unique<tools::ui::ClipboardWindow>(
            GTK_APPLICATION(app),
            self->clipboard_manager_
        );

        // Registra o atalho Super+V no GNOME apontando para este executável
        if (argc > 0 && argv[0]) {
            try {
                std::string exe_path = fs::canonical(argv[0]).string();
                tools::core::ShortcutManager::register_gnome_shortcut(exe_path);
            } catch (...) {
                tools::core::ShortcutManager::register_gnome_shortcut(argv[0]);
            }
        }
    }

    // Inicializa o ícone da bandeja do sistema (System Tray)
    self->setup_tray();

    bool toggle_requested = false;
    bool daemon_only = false;
    bool clear_requested = false;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--toggle") == 0) {
            toggle_requested = true;
        } else if (std::strcmp(argv[i], "--daemon") == 0) {
            daemon_only = true;
        } else if (std::strcmp(argv[i], "--clear") == 0) {
            clear_requested = true;
            self->clipboard_manager_.clear_history();
        }
    }

    if (toggle_requested) {
        self->window_->toggle();
    } else if (!daemon_only && !clear_requested) {
        self->window_->show_and_refresh();
    }

    g_strfreev(argv);
    return 0;
}

} // namespace tools
