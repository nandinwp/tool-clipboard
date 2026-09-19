#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Wrapper universal que detecta o binário correto automaticamente
WRAPPER="$HOME/.local/bin/tools-clipboard"
EXE="$WRAPPER"

if [ ! -x "$WRAPPER" ]; then
    if [ -x "$PROJECT_ROOT/dist/linux/bin/Tools" ]; then
        EXE="$PROJECT_ROOT/dist/linux/bin/Tools"
    elif [ -x "$PROJECT_ROOT/cmake-build-debug/Tools" ]; then
        EXE="$PROJECT_ROOT/cmake-build-debug/Tools"
    elif [ -x "$PROJECT_ROOT/build/linux-release/Tools" ]; then
        EXE="$PROJECT_ROOT/build/linux-release/Tools"
    fi
fi

echo "=========================================================="
echo " Configurando Atalho Win + V no GNOME (Super + V)         "
echo "=========================================================="
echo "Comando associado: $EXE --toggle"

# 1. Libera o <Super>v do atalho nativo de notificações do GNOME
TRAY=$(gsettings get org.gnome.shell.keybindings toggle-message-tray 2>/dev/null || true)
if [[ "$TRAY" == *"<Super>v"* ]]; then
    echo "Liberando <Super>v da bandeja de mensagens do GNOME..."
    gsettings set org.gnome.shell.keybindings toggle-message-tray "['<Super>m']"
fi

# 2. Configura o atalho customizado
PATH_SCHEMA="/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/tools-clipboard/"
SCHEMA="org.gnome.settings-daemon.plugins.media-keys.custom-keybinding:$PATH_SCHEMA"

gsettings set "$SCHEMA" name "Clipboard History (Win+V)"
gsettings set "$SCHEMA" command "$EXE --toggle"
gsettings set "$SCHEMA" binding "<Super>v"

CURRENT=$(gsettings get org.gnome.settings-daemon.plugins.media-keys custom-keybindings)
if [[ "$CURRENT" != *"$PATH_SCHEMA"* ]]; then
    if [ "$CURRENT" == "@as []" ] || [ -z "$CURRENT" ] || [ "$CURRENT" == "[]" ]; then
        gsettings set org.gnome.settings-daemon.plugins.media-keys custom-keybindings "['$PATH_SCHEMA']"
    else
        NEW_LIST="${CURRENT%]*}"
        if [[ "$NEW_LIST" == *"'"* ]]; then
            NEW_LIST="$NEW_LIST, '$PATH_SCHEMA']"
        else
            NEW_LIST="$NEW_LIST'$PATH_SCHEMA']"
        fi
        gsettings set org.gnome.settings-daemon.plugins.media-keys custom-keybindings "$NEW_LIST"
    fi
fi

echo "Atalho <Super>v configurado com sucesso!"
echo "Pressione Win + V a qualquer momento para abrir o histórico."
