#include "tools/core/clipboard_manager.hpp"
#include "tools/core/settings_manager.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <iostream>

namespace tools::core {

namespace fs = std::filesystem;

static std::string get_current_time_str() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%H:%M");
    return ss.str();
}

static std::string get_ini_storage_path() {
    const char *xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) {
        return std::string(xdg) + "/Tools/clipboard_history.ini";
    }
    const char *home = std::getenv("HOME");
    if (home && *home) {
        return std::string(home) + "/.config/Tools/clipboard_history.ini";
    }
    return "./clipboard_history.ini";
}

static std::string get_legacy_storage_path() {
    const char *xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) {
        return std::string(xdg) + "/Tools/clipboard_history.txt";
    }
    const char *home = std::getenv("HOME");
    if (home && *home) {
        return std::string(home) + "/.config/Tools/clipboard_history.txt";
    }
    return "./clipboard_history.txt";
}

static std::string escape_ini_string(const std::string &input) {
    std::string out;
    out.reserve(input.size() + 16);
    for (char c : input) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

static std::string unescape_ini_string(const std::string &input) {
    std::string out;
    out.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\' && i + 1 < input.size()) {
            char next = input[++i];
            switch (next) {
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case '\\': out += '\\'; break;
                default: out += next; break;
            }
        } else {
            out += input[i];
        }
    }
    return out;
}

static std::string trim(const std::string &s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

ClipboardManager::ClipboardManager() {
    config_file_path_ = get_ini_storage_path();
    max_items_ = SettingsManager::instance().get_max_items();
    load_history();

    SettingsManager::instance().on_changed([this]() {
        set_max_items(SettingsManager::instance().get_max_items());
    });
}

ClipboardManager::~ClipboardManager() {
    // save_history() já é chamado imediatamente a cada modificação (add, remove, clear, pin)
}

void ClipboardManager::start_monitoring(GdkDisplay *display) {
    if (!display) return;

    clipboard_ = gdk_display_get_clipboard(display);
    if (clipboard_) {
        g_signal_connect(clipboard_, "changed", G_CALLBACK(on_clipboard_changed), this);
    }
}

void ClipboardManager::check_current_clipboard() {
    if (!clipboard_) {
        GdkDisplay *disp = gdk_display_get_default();
        if (disp) {
            clipboard_ = gdk_display_get_clipboard(disp);
        }
    }
    if (clipboard_) {
        on_clipboard_changed(clipboard_, this);
    }
}

void ClipboardManager::on_clipboard_changed(GdkClipboard *clipboard, gpointer user_data) {
    auto *self = static_cast<ClipboardManager*>(user_data);

    gdk_clipboard_read_text_async(
        clipboard,
        nullptr,
        [](GObject *source, GAsyncResult *res, gpointer data) {
            auto *manager = static_cast<ClipboardManager*>(data);
            GError *error = nullptr;
            char *text = gdk_clipboard_read_text_finish(GDK_CLIPBOARD(source), res, &error);

            if (text && !error) {
                std::string str(text);
                g_free(text);

                // Executa a adição na thread principal
                g_idle_add_full(
                    G_PRIORITY_DEFAULT,
                    [](gpointer p) -> gboolean {
                        auto *pair = static_cast<std::pair<ClipboardManager*, std::string>*>(p);
                        pair->first->add_item(pair->second);
                        delete pair;
                        return G_SOURCE_REMOVE;
                    },
                    new std::pair<ClipboardManager*, std::string>(manager, str),
                    nullptr
                );
            }
            if (error) {
                g_error_free(error);
            }
        },
        self
    );
}

void ClipboardManager::toggle_pin(size_t index) {
    if (index < items_.size()) {
        items_[index].pinned = !items_[index].pinned;
        save_history();
        if (update_callback_) {
            update_callback_();
        }
    }
}

bool ClipboardManager::is_pinned(size_t index) const {
    if (index < items_.size()) {
        return items_[index].pinned;
    }
    return false;
}

bool ClipboardManager::add_item(const std::string &text) {
    // Ignora texto vazio ou apenas espaços
    if (text.empty() || text.find_first_not_of(" \t\r\n") == std::string::npos) {
        return false;
    }

    // Se o item já for igual ao primeiro da lista, não duplica
    if (!items_.empty() && items_.front().text == text) {
        return false;
    }

    bool was_pinned = false;
    // Se o texto já existia em outra posição, preserva seu status fixado e remove da posição antiga
    auto it = std::find_if(items_.begin(), items_.end(), [&](const ClipboardItem &item) {
        return item.text == text;
    });
    if (it != items_.end()) {
        was_pinned = it->pinned;
        items_.erase(it);
    }

    // Insere no início da lista (mais recente)
    items_.insert(items_.begin(), ClipboardItem{text, get_current_time_str(), was_pinned});

    // Mantém no máximo a quantidade configurada de itens
    // Prioriza remover itens NÃO fixados mais antigos
    if (items_.size() > static_cast<size_t>(max_items_)) {
        auto remove_it = items_.end();
        for (auto rit = items_.rbegin(); rit != items_.rend(); ++rit) {
            if (!rit->pinned) {
                remove_it = (rit + 1).base();
                break;
            }
        }

        if (remove_it != items_.end()) {
            items_.erase(remove_it);
        } else {
            // Se todos forem fixados, descarta o último
            items_.pop_back();
        }
    }

    save_history();

    if (update_callback_) {
        update_callback_();
    }

    return true;
}

void ClipboardManager::set_max_items(int max_count) {
    if (max_count < 3) max_count = 3;
    if (max_count > 100) max_count = 100;
    max_items_ = max_count;

    while (items_.size() > static_cast<size_t>(max_items_)) {
        auto remove_it = items_.end();
        for (auto rit = items_.rbegin(); rit != items_.rend(); ++rit) {
            if (!rit->pinned) {
                remove_it = (rit + 1).base();
                break;
            }
        }
        if (remove_it != items_.end()) {
            items_.erase(remove_it);
        } else {
            items_.pop_back();
        }
    }
    save_history();
    if (update_callback_) {
        update_callback_();
    }
}

bool ClipboardManager::copy_to_clipboard(size_t index) {
    if (!clipboard_) {
        GdkDisplay *disp = gdk_display_get_default();
        if (disp) {
            clipboard_ = gdk_display_get_clipboard(disp);
        }
    }
    if (index >= items_.size() || !clipboard_) {
        return false;
    }

    std::string text = items_[index].text;

    // Atualiza a área de transferência do sistema
    gdk_clipboard_set_text(clipboard_, text.c_str());

    // Move o item selecionado para o topo da lista preservando se estava fixado
    if (index > 0) {
        ClipboardItem item = items_[index];
        items_.erase(items_.begin() + index);
        items_.insert(items_.begin(), item);
        save_history();

        if (update_callback_) {
            update_callback_();
        }
    }

    return true;
}

void ClipboardManager::remove_item(size_t index) {
    if (index < items_.size()) {
        items_.erase(items_.begin() + index);
        save_history();
        if (update_callback_) {
            update_callback_();
        }
    }
}

void ClipboardManager::clear_history() {
    // Preserva itens fixados (Pin), removendo apenas os não-fixados
    items_.erase(
        std::remove_if(items_.begin(), items_.end(), [](const ClipboardItem &item) {
            return !item.pinned;
        }),
        items_.end()
    );
    save_history();
    if (update_callback_) {
        update_callback_();
    }
}

void ClipboardManager::load_history() {
    items_.clear();

    if (fs::exists(config_file_path_)) {
        std::ifstream file(config_file_path_);
        if (!file.is_open()) return;

        std::string line;
        ClipboardItem current_item;
        bool in_item_section = false;

        auto flush_item = [&]() {
            if (in_item_section && !current_item.text.empty() && items_.size() < static_cast<size_t>(max_items_)) {
                if (current_item.timestamp.empty()) {
                    current_item.timestamp = get_current_time_str();
                }
                items_.push_back(current_item);
            }
            current_item = ClipboardItem{};
            in_item_section = false;
        };

        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == ';' || line[0] == '#') continue;

            if (line.front() == '[' && line.back() == ']') {
                flush_item();
                std::string section = line.substr(1, line.size() - 2);
                if (section.rfind("item_", 0) == 0) {
                    in_item_section = true;
                }
                continue;
            }

            if (in_item_section) {
                auto eq = line.find('=');
                if (eq != std::string::npos) {
                    std::string key = trim(line.substr(0, eq));
                    std::string val = trim(line.substr(eq + 1));
                    if (key == "text") {
                        current_item.text = unescape_ini_string(val);
                    } else if (key == "timestamp") {
                        current_item.timestamp = val;
                    } else if (key == "pinned") {
                        current_item.pinned = (val == "true" || val == "1" || val == "yes");
                    }
                }
            }
        }
        flush_item();
        return;
    }

    // Fallback: tenta carregar do formato anterior (.txt) se existir
    std::string legacy_path = get_legacy_storage_path();
    if (fs::exists(legacy_path)) {
        std::ifstream file(legacy_path);
        if (file.is_open()) {
            std::string line;
            std::string current_text;
            std::string current_time = get_current_time_str();

            while (std::getline(file, line)) {
                if (line == "---CLIPBOARD_ITEM_DELIMITER---") {
                    if (!current_text.empty() && items_.size() < static_cast<size_t>(max_items_)) {
                        items_.push_back({current_text, current_time, false});
                        current_text.clear();
                    }
                } else {
                    if (!current_text.empty()) {
                        current_text += "\n";
                    }
                    current_text += line;
                }
            }
            if (!current_text.empty() && items_.size() < static_cast<size_t>(max_items_)) {
                items_.push_back({current_text, current_time, false});
            }
            // Migra para .ini imediatamente
            save_history();
        }
    }
}

void ClipboardManager::save_history() const {
    std::error_code ec;
    fs::create_directories(fs::path(config_file_path_).parent_path(), ec);

    std::ofstream file(config_file_path_, std::ios::trunc);
    if (!file.is_open()) return;

    auto &settings = SettingsManager::instance();

    file << "; Tools - Clipboard History (.ini)\n";
    file << "[settings]\n";
    file << "version=1\n";
    file << "max_items=" << max_items_ << "\n";
    file << "autostart=" << (settings.is_autostart_enabled() ? "true" : "false") << "\n";
    file << "count=" << items_.size() << "\n\n";

    for (size_t i = 0; i < items_.size() && i < static_cast<size_t>(max_items_); ++i) {
        file << "[item_" << i << "]\n";
        file << "pinned=" << (items_[i].pinned ? "true" : "false") << "\n";
        file << "timestamp=" << items_[i].timestamp << "\n";
        file << "text=" << escape_ini_string(items_[i].text) << "\n\n";
    }
}

} // namespace tools::core
