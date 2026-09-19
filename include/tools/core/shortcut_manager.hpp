#pragma once

#include <string>

namespace tools::core {

class ShortcutManager {
public:
    // Registra o atalho Super+V no ambiente GNOME para chamar 'Tools --toggle'
    static bool register_gnome_shortcut(const std::string &executable_path);

    // Verifica se o atalho já está registrado
    static bool is_gnome_shortcut_registered();
};

} // namespace tools::core
