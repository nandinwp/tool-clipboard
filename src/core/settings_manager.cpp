#include "tools/core/settings_manager.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace tools::core {

namespace fs = std::filesystem;

SettingsManager& SettingsManager::instance() {
    static SettingsManager s_instance;
    return s_instance;
}

std::string SettingsManager::get_autostart_path() const {
    const char *home = std::getenv("HOME");
    if (home) {
        return (fs::path(home) / ".config" / "autostart" / "tools-clipboard.desktop").string();
    }
    return "";
}

std::string SettingsManager::get_ini_path() const {
    const char *home = std::getenv("HOME");
    if (home) {
        return (fs::path(home) / ".config" / "Tools" / "clipboard_history.ini").string();
    }
    return "./clipboard_history.ini";
}

SettingsManager::SettingsManager() {
    load_settings();
}

void SettingsManager::load_settings() {
    std::string ini_file = get_ini_path();
    if (fs::exists(ini_file)) {
        std::ifstream file(ini_file);
        std::string line;
        bool in_settings = false;

        while (std::getline(file, line)) {
            // Trim
            auto start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) continue;
            auto end = line.find_last_not_of(" \t\r\n");
            line = line.substr(start, end - start + 1);

            if (line == "[settings]") {
                in_settings = true;
                continue;
            }
            if (line.front() == '[' && line.back() == ']') {
                in_settings = false;
                continue;
            }

            if (in_settings) {
                auto eq = line.find('=');
                if (eq != std::string::npos) {
                    std::string key = line.substr(0, eq);
                    std::string val = line.substr(eq + 1);
                    if (key == "max_items") {
                        try {
                            int count = std::stoi(val);
                            if (count >= 3 && count <= 100) {
                                max_items_ = count;
                            }
                        } catch (...) {}
                    } else if (key == "autostart") {
                        autostart_enabled_ = (val == "true" || val == "1");
                    }
                }
            }
        }
    }

    // Verifica também se o arquivo de autostart já existe fisicamente no sistema
    std::string autostart_file = get_autostart_path();
    if (!autostart_file.empty() && fs::exists(autostart_file)) {
        autostart_enabled_ = true;
    }
}

void SettingsManager::set_max_items(int count) {
    if (count < 3) count = 3;
    if (count > 100) count = 100;
    if (max_items_ == count) return;

    max_items_ = count;
    save_settings();

    for (const auto &cb : listeners_) {
        if (cb) cb();
    }
}

void SettingsManager::set_autostart_enabled(bool enabled) {
    autostart_enabled_ = enabled;
    update_autostart_file();
    save_settings();

    for (const auto &cb : listeners_) {
        if (cb) cb();
    }
}

void SettingsManager::update_autostart_file() {
    std::string path = get_autostart_path();
    if (path.empty()) return;

    try {
        if (autostart_enabled_) {
            fs::create_directories(fs::path(path).parent_path());

            // Descobre o executável wrapper
            std::string exe = "/home/nandinwp/.local/bin/tools-clipboard";
            const char *home = std::getenv("HOME");
            if (home) {
                std::string wrapper = std::string(home) + "/.local/bin/tools-clipboard";
                if (fs::exists(wrapper)) {
                    exe = wrapper;
                }
            }

            std::ofstream f(path, std::ios::trunc);
            if (f.is_open()) {
                f << "[Desktop Entry]\n";
                f << "Type=Application\n";
                f << "Name=Tools Clipboard\n";
                f << "Comment=Gerenciador de Área de Transferência (Win + V)\n";
                f << "Exec=" << exe << " --daemon\n";
                f << "Icon=tools-clipboard\n";
                f << "Terminal=false\n";
                f << "StartupNotify=false\n";
                f << "X-GNOME-Autostart-enabled=true\n";
            }
        } else {
            if (fs::exists(path)) {
                fs::remove(path);
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "[SettingsManager] Erro ao atualizar autostart: " << e.what() << std::endl;
    }
}

void SettingsManager::save_settings() const {
    std::string path = get_ini_path();
    if (path.empty()) return;

    try {
        fs::create_directories(fs::path(path).parent_path());

        std::vector<std::string> lines;
        bool file_exists = fs::exists(path);
        bool in_settings = false;
        bool has_settings_section = false;
        bool has_max_items = false;
        bool has_autostart = false;

        if (file_exists) {
            std::ifstream file(path);
            std::string line;
            while (std::getline(file, line)) {
                auto start = line.find_first_not_of(" \t\r\n");
                std::string trimmed = (start == std::string::npos) ? "" : line.substr(start);

                if (trimmed == "[settings]") {
                    in_settings = true;
                    has_settings_section = true;
                    lines.push_back(line);
                    continue;
                }
                if (!trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']') {
                    if (in_settings) {
                        if (!has_max_items) {
                            lines.push_back("max_items=" + std::to_string(max_items_));
                            has_max_items = true;
                        }
                        if (!has_autostart) {
                            lines.push_back(std::string("autostart=") + (autostart_enabled_ ? "true" : "false"));
                            has_autostart = true;
                        }
                    }
                    in_settings = false;
                    lines.push_back(line);
                    continue;
                }

                if (in_settings) {
                    auto eq = trimmed.find('=');
                    if (eq != std::string::npos) {
                        std::string k = trimmed.substr(0, eq);
                        if (k == "max_items") {
                            lines.push_back("max_items=" + std::to_string(max_items_));
                            has_max_items = true;
                            continue;
                        } else if (k == "autostart") {
                            lines.push_back(std::string("autostart=") + (autostart_enabled_ ? "true" : "false"));
                            has_autostart = true;
                            continue;
                        }
                    }
                }

                lines.push_back(line);
            }

            if (in_settings) {
                if (!has_max_items) {
                    lines.push_back("max_items=" + std::to_string(max_items_));
                }
                if (!has_autostart) {
                    lines.push_back(std::string("autostart=") + (autostart_enabled_ ? "true" : "false"));
                }
            }
        }

        if (!has_settings_section) {
            std::vector<std::string> new_lines;
            new_lines.push_back("; Tools - Clipboard History (.ini)");
            new_lines.push_back("[settings]");
            new_lines.push_back("version=1");
            new_lines.push_back("max_items=" + std::to_string(max_items_));
            new_lines.push_back(std::string("autostart=") + (autostart_enabled_ ? "true" : "false"));
            new_lines.push_back("");
            for (const auto &l : lines) {
                new_lines.push_back(l);
            }
            lines = std::move(new_lines);
        }

        std::ofstream out(path, std::ios::trunc);
        for (const auto &l : lines) {
            out << l << "\n";
        }
    } catch (const std::exception &e) {
        std::cerr << "[SettingsManager] Erro ao salvar configuracoes: " << e.what() << std::endl;
    }
}

void SettingsManager::on_changed(ChangeCallback cb) {
    listeners_.push_back(std::move(cb));
}

} // namespace tools::core
