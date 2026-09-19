#include "tools/core/shortcut_manager.hpp"
#include <cstdlib>
#include <unistd.h>
#include <array>
#include <memory>
#include <iostream>

namespace tools::core {

struct PipeCloser { void operator()(FILE *f) const { if (f) pclose(f); } };

static std::string exec_command(const std::string &cmd) {
    std::array<char, 256> buffer;
    std::string result;
    std::unique_ptr<FILE, PipeCloser> pipe(popen(cmd.c_str(), "r"));
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

bool ShortcutManager::is_gnome_shortcut_registered() {
    std::string out = exec_command("gsettings get org.gnome.settings-daemon.plugins.media-keys custom-keybindings 2>/dev/null");
    return out.find("tools-clipboard") != std::string::npos;
}

bool ShortcutManager::register_gnome_shortcut(const std::string &executable_path) {
    const std::string path = "/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/tools-clipboard/";
    const std::string schema = "org.gnome.settings-daemon.plugins.media-keys.custom-keybinding:" + path;

    // 1. Libera o atalho <Super>v da bandeja de mensagens do GNOME se estiver conflitante
    std::string tray = exec_command("gsettings get org.gnome.shell.keybindings toggle-message-tray 2>/dev/null");
    if (tray.find("<Super>v") != std::string::npos) {
        [[maybe_unused]] int r = std::system("gsettings set org.gnome.shell.keybindings toggle-message-tray \"['<Super>m']\"");
    }

    // 2. Se o wrapper universal ~/.local/bin/tools-clipboard existir, usa-o
    std::string cmd_path = executable_path;
    const char *home = std::getenv("HOME");
    if (home) {
        std::string wrapper = std::string(home) + "/.local/bin/tools-clipboard";
        if (access(wrapper.c_str(), X_OK) == 0) {
            cmd_path = wrapper;
        }
    }

    // Define nome, comando e combinação Super+v (Win+V)
    std::string cmd1 = "gsettings set " + schema + " name 'Clipboard History (Win+V)'";
    std::string cmd2 = "gsettings set " + schema + " command '" + cmd_path + " --toggle'";
    std::string cmd3 = "gsettings set " + schema + " binding '<Super>v'";

    [[maybe_unused]] int r1 = std::system(cmd1.c_str());
    [[maybe_unused]] int r2 = std::system(cmd2.c_str());
    [[maybe_unused]] int r3 = std::system(cmd3.c_str());

    // Atualiza a lista de atalhos ativos no GNOME
    std::string current_list = exec_command("gsettings get org.gnome.settings-daemon.plugins.media-keys custom-keybindings 2>/dev/null");

    if (current_list.find(path) == std::string::npos) {
        if (current_list.find("@as []") != std::string::npos || current_list.empty() || current_list == "[]\n") {
            std::string set_cmd = "gsettings set org.gnome.settings-daemon.plugins.media-keys custom-keybindings \"['" + path + "']\"";
            [[maybe_unused]] int r4 = std::system(set_cmd.c_str());
        } else {
            // Remove o último caractere ']' e adiciona o nosso caminho
            size_t end_bracket = current_list.rfind(']');
            if (end_bracket != std::string::npos) {
                std::string new_list = current_list.substr(0, end_bracket);
                if (new_list.find('\'') != std::string::npos) {
                    new_list += ", '" + path + "']";
                } else {
                    new_list += "'" + path + "']";
                }
                std::string set_cmd = "gsettings set org.gnome.settings-daemon.plugins.media-keys custom-keybindings \"" + new_list + "\"";
                [[maybe_unused]] int r5 = std::system(set_cmd.c_str());
            }
        }
    }

    return is_gnome_shortcut_registered();
}

} // namespace tools::core
