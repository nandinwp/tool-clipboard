#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

VERSION="1.0.0"
ARCH="amd64"
PKG_NAME="tools-clipboard"
OUTPUT_DIR="$PROJECT_ROOT/dist/release"

echo "=========================================================="
echo "    Gerador de Pacotes de Release - Tools Clipboard       "
echo "    Versão: $VERSION ($ARCH)                              "
echo "=========================================================="
echo

# 1. Compilação Release
echo "==> [1/4] Compilando binário otimizado (Release)..."
cmake -S "$PROJECT_ROOT" --preset linux-release
cmake --build --preset linux-release -j"$(nproc)"

RELEASE_BIN="$PROJECT_ROOT/build/linux-release/Tools"
if [ ! -f "$RELEASE_BIN" ]; then
    echo "Erro: Binário não encontrado em $RELEASE_BIN"
    exit 1
fi

rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# ---------------------------------------------------------------------------
# 2. Construção do Pacote Debian / Ubuntu (.deb)
# ---------------------------------------------------------------------------
echo "==> [2/4] Criando pacote Debian / Ubuntu (.deb)..."
DEB_STAGE="$PROJECT_ROOT/dist/deb_staging"
rm -rf "$DEB_STAGE"
mkdir -p "$DEB_STAGE/DEBIAN"
mkdir -p "$DEB_STAGE/usr/bin"
mkdir -p "$DEB_STAGE/usr/lib/$PKG_NAME"
mkdir -p "$DEB_STAGE/usr/share/$PKG_NAME"
mkdir -p "$DEB_STAGE/usr/share/applications"
mkdir -p "$DEB_STAGE/usr/share/icons/hicolor/512x512/apps"
mkdir -p "$DEB_STAGE/usr/share/pixmaps"
mkdir -p "$DEB_STAGE/lib/udev/rules.d"

# Binário e Recursos
cp "$RELEASE_BIN" "$DEB_STAGE/usr/lib/$PKG_NAME/Tools"
chmod 755 "$DEB_STAGE/usr/lib/$PKG_NAME/Tools"

cp "$PROJECT_ROOT/resources/style.css" "$DEB_STAGE/usr/share/$PKG_NAME/"
cp "$PROJECT_ROOT/resources/compliance.png" "$DEB_STAGE/usr/share/$PKG_NAME/"
cp "$PROJECT_ROOT/resources/compliance.ico" "$DEB_STAGE/usr/share/$PKG_NAME/"

# Wrapper executável em /usr/bin/tools-clipboard
cat << 'EOF' > "$DEB_STAGE/usr/bin/tools-clipboard"
#!/bin/sh
exec /usr/lib/tools-clipboard/Tools "$@"
EOF
chmod 755 "$DEB_STAGE/usr/bin/tools-clipboard"
ln -sf tools-clipboard "$DEB_STAGE/usr/bin/Tools"

# Ícones
cp "$PROJECT_ROOT/resources/compliance.png" "$DEB_STAGE/usr/share/icons/hicolor/512x512/apps/tools-clipboard.png"
cp "$PROJECT_ROOT/resources/compliance.png" "$DEB_STAGE/usr/share/pixmaps/tools-clipboard.png"

# Regra udev para suporte a /dev/uinput (Auto-paste sem permissão de root)
cat << 'EOF' > "$DEB_STAGE/lib/udev/rules.d/99-tools-clipboard-uinput.rules"
# Permissão para simulação de teclado virtual pelo Tools Clipboard
KERNEL=="uinput", SUBSYSTEM=="misc", MODE="0660", GROUP="input", TAG+="uaccess"
EOF

# Arquivo .desktop
cat << 'EOF' > "$DEB_STAGE/usr/share/applications/tools-clipboard.desktop"
[Desktop Entry]
Type=Application
Name=Tools Clipboard
GenericName=Clipboard Manager
Comment=Gerenciador de Área de Transferência (Win + V)
Exec=tools-clipboard --toggle
Icon=tools-clipboard
Terminal=false
Categories=Utility;GTK;
StartupNotify=false
Keywords=clipboard;history;win+v;paste;copiar;colar;
EOF

# DEBIAN/control
cat << EOF > "$DEB_STAGE/DEBIAN/control"
Package: $PKG_NAME
Version: $VERSION
Architecture: $ARCH
Maintainer: Luís Andrade Cordeiro <contato@bnbb.com.br>
Installed-Size: $(du -ks "$DEB_STAGE" | cut -f1)
Depends: libgtk-4-1 (>= 4.0.0), libc6 (>= 2.34), libstdc++6 (>= 11)
Recommends: udev
Section: utils
Priority: optional
Homepage: https://bnbb.com.br
Description: Modern Windows Win+V style clipboard history manager for Linux
 Tools Clipboard runs in the background and opens with Win + V,
 storing copied history items, allowing pinning items across reboots,
 auto-pasting to active windows via virtual keyboard synthesis,
 and supporting multi-language settings (PT/EN/ES).
EOF

# DEBIAN/postinst
cat << 'EOF' > "$DEB_STAGE/DEBIAN/postinst"
#!/bin/sh
set -e

if which update-desktop-database >/dev/null 2>&1; then
    update-desktop-database -q /usr/share/applications || true
fi

if which gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor || true
fi

if which udevadm >/dev/null 2>&1; then
    udevadm control --reload-rules 2>/dev/null || true
    udevadm trigger --subsystem-match=misc 2>/dev/null || true
fi

modprobe uinput 2>/dev/null || true

exit 0
EOF
chmod 755 "$DEB_STAGE/DEBIAN/postinst"

# DEBIAN/postrm
cat << 'EOF' > "$DEB_STAGE/DEBIAN/postrm"
#!/bin/sh
set -e

if which update-desktop-database >/dev/null 2>&1; then
    update-desktop-database -q /usr/share/applications || true
fi

if which gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor || true
fi

exit 0
EOF
chmod 755 "$DEB_STAGE/DEBIAN/postrm"

DEB_FILE="$OUTPUT_DIR/${PKG_NAME}_${VERSION}_${ARCH}.deb"
dpkg-deb --build --root-owner-group "$DEB_STAGE" "$DEB_FILE"
rm -rf "$DEB_STAGE"
echo "   Criado: $DEB_FILE"

# ---------------------------------------------------------------------------
# 3. Construção do Pacote Universal Portátil (.tar.gz com instalador)
# ---------------------------------------------------------------------------
echo "==> [3/4] Criando pacote universal portátil (.tar.gz com script instalador)..."
TAR_STAGE="$PROJECT_ROOT/dist/tar_staging"
rm -rf "$TAR_STAGE"
mkdir -p "$TAR_STAGE/bin"
mkdir -p "$TAR_STAGE/resources"

cp "$RELEASE_BIN" "$TAR_STAGE/bin/Tools"
chmod 755 "$TAR_STAGE/bin/Tools"

cp "$PROJECT_ROOT/resources/style.css" "$TAR_STAGE/resources/"
cp "$PROJECT_ROOT/resources/compliance.png" "$TAR_STAGE/resources/"
cp "$PROJECT_ROOT/resources/compliance.ico" "$TAR_STAGE/resources/"

cat << 'EOF' > "$TAR_STAGE/tools-clipboard.desktop"
[Desktop Entry]
Type=Application
Name=Tools Clipboard
GenericName=Clipboard Manager
Comment=Gerenciador de Área de Transferência (Win + V)
Exec=tools-clipboard --toggle
Icon=tools-clipboard
Terminal=false
Categories=Utility;GTK;
StartupNotify=false
Keywords=clipboard;history;win+v;paste;
EOF

cat << 'EOF' > "$TAR_STAGE/99-tools-clipboard-uinput.rules"
KERNEL=="uinput", SUBSYSTEM=="misc", MODE="0660", GROUP="input", TAG+="uaccess"
EOF

# Script de instalação universal (sistema ou usuário)
cat << 'EOF' > "$TAR_STAGE/install.sh"
#!/usr/bin/env bash
set -e

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Define prefixo (sistema se root, usuário se usuário comum)
if [ "$EUID" -eq 0 ]; then
    PREFIX="/usr/local"
    IS_ROOT=true
else
    PREFIX="$HOME/.local"
    IS_ROOT=false
fi

for arg in "$@"; do
    case $arg in
        --user)
            PREFIX="$HOME/.local"
            IS_ROOT=false
            ;;
        --system)
            if [ "$EUID" -ne 0 ]; then
                echo "Aviso: Reexecutando como root via sudo..."
                exec sudo "$0" --system
            fi
            PREFIX="/usr/local"
            IS_ROOT=true
            ;;
        --prefix=*)
            PREFIX="${arg#*=}"
            ;;
    esac
done

echo "=========================================================="
echo "    Instalando Tools Clipboard (Win + V)                  "
echo "=========================================================="
echo "Prefixo de instalação: $PREFIX"
echo

mkdir -p "$PREFIX/bin"
mkdir -p "$PREFIX/share/tools-clipboard"
mkdir -p "$PREFIX/share/applications"
mkdir -p "$PREFIX/share/icons/hicolor/512x512/apps"

# Copia arquivos
cp "$HERE/bin/Tools" "$PREFIX/bin/tools-clipboard"
chmod +x "$PREFIX/bin/tools-clipboard"
ln -sf tools-clipboard "$PREFIX/bin/Tools"

cp -r "$HERE/resources/"* "$PREFIX/share/tools-clipboard/"
cp "$HERE/resources/compliance.png" "$PREFIX/share/icons/hicolor/512x512/apps/tools-clipboard.png"

# Ícone local adicional para garantir visualização imediata no GNOME
if [ "$IS_ROOT" = false ]; then
    mkdir -p "$HOME/.local/share/icons"
    cp "$HERE/resources/compliance.png" "$HOME/.local/share/icons/tools-clipboard.png"
fi

# Desktop Entry
cp "$HERE/tools-clipboard.desktop" "$PREFIX/share/applications/"
sed -i "s|^Exec=.*|Exec=$PREFIX/bin/tools-clipboard --toggle|" "$PREFIX/share/applications/tools-clipboard.desktop"

# Regra udev para suporte a simulação de colar (Auto-paste)
if [ "$IS_ROOT" = true ]; then
    if [ -d "/etc/udev/rules.d" ]; then
        cp "$HERE/99-tools-clipboard-uinput.rules" "/etc/udev/rules.d/"
        udevadm control --reload-rules 2>/dev/null || true
        udevadm trigger --subsystem-match=misc 2>/dev/null || true
        modprobe uinput 2>/dev/null || true
    fi
fi

# Atualiza caches do sistema se disponíveis
if which update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$PREFIX/share/applications" 2>/dev/null || true
fi
if which gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q -t -f "$PREFIX/share/icons/hicolor" 2>/dev/null || true
fi

# Configuração de Atalho no GNOME
if which gsettings >/dev/null 2>&1 && [ -n "$DISPLAY" -o -n "$WAYLAND_DISPLAY" ]; then
    echo "Configurando atalho Win + V (Super + V) no GNOME..."
    TRAY=$(gsettings get org.gnome.shell.keybindings toggle-message-tray 2>/dev/null || true)
    if [[ "$TRAY" == *"<Super>v"* ]]; then
        gsettings set org.gnome.shell.keybindings toggle-message-tray "['<Super>m']" 2>/dev/null || true
    fi

    PATH_SCHEMA="/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/tools-clipboard/"
    SCHEMA="org.gnome.settings-daemon.plugins.media-keys.custom-keybinding:$PATH_SCHEMA"

    gsettings set "$SCHEMA" name "Clipboard History (Win+V)" 2>/dev/null || true
    gsettings set "$SCHEMA" command "$PREFIX/bin/tools-clipboard --toggle" 2>/dev/null || true
    gsettings set "$SCHEMA" binding "<Super>v" 2>/dev/null || true

    CURRENT=$(gsettings get org.gnome.settings-daemon.plugins.media-keys custom-keybindings 2>/dev/null || true)
    if [[ "$CURRENT" != *"$PATH_SCHEMA"* ]]; then
        if [ "$CURRENT" = "@as []" ] || [ -z "$CURRENT" ] || [ "$CURRENT" = "[]" ]; then
            gsettings set org.gnome.settings-daemon.plugins.media-keys custom-keybindings "['$PATH_SCHEMA']" 2>/dev/null || true
        else
            NEW_LIST="${CURRENT%]*}"
            if [[ "$NEW_LIST" == *"'"* ]]; then
                NEW_LIST="$NEW_LIST, '$PATH_SCHEMA']"
            else
                NEW_LIST="$NEW_LIST'$PATH_SCHEMA']"
            fi
            gsettings set org.gnome.settings-daemon.plugins.media-keys custom-keybindings "$NEW_LIST" 2>/dev/null || true
        fi
    fi
fi

echo
echo "==> Instalação concluída com sucesso!"
echo "    Comando: $PREFIX/bin/tools-clipboard"
echo "    Atalho:  Win + V"
echo
echo "Para iniciar em segundo plano agora mesmo:"
echo "    $PREFIX/bin/tools-clipboard --daemon &"
echo
EOF
chmod +x "$TAR_STAGE/install.sh"

# Script de desinstalação
cat << 'EOF' > "$TAR_STAGE/uninstall.sh"
#!/usr/bin/env bash
set -e

if [ "$EUID" -eq 0 ]; then
    PREFIX="/usr/local"
else
    PREFIX="$HOME/.local"
fi

for arg in "$@"; do
    case $arg in
        --user) PREFIX="$HOME/.local" ;;
        --system)
            if [ "$EUID" -ne 0 ]; then
                exec sudo "$0" --system
            fi
            PREFIX="/usr/local"
            ;;
        --prefix=*) PREFIX="${arg#*=}" ;;
    esac
done

echo "Removendo Tools Clipboard de $PREFIX..."
rm -f "$PREFIX/bin/tools-clipboard"
rm -f "$PREFIX/bin/Tools"
rm -rf "$PREFIX/share/tools-clipboard"
rm -f "$PREFIX/share/applications/tools-clipboard.desktop"
rm -f "$PREFIX/share/icons/hicolor/512x512/apps/tools-clipboard.png"

if [ -f "/etc/udev/rules.d/99-tools-clipboard-uinput.rules" ] && [ "$EUID" -eq 0 ]; then
    rm -f "/etc/udev/rules.d/99-tools-clipboard-uinput.rules"
    udevadm control --reload-rules 2>/dev/null || true
fi

echo "Desinstalação concluída com sucesso!"
EOF
chmod +x "$TAR_STAGE/uninstall.sh"

# Arquivo README explicativo
cat << EOF > "$TAR_STAGE/README.txt"
==========================================================
 Tools Clipboard v$VERSION - Pacote de Instalação Linux
 Desenvolvido por: Luís Andrade Cordeiro (contato@bnbb.com.br)
 Site: https://bnbb.com.br
==========================================================

Opções de Instalação:

1) Para instalar para todos os usuários do sistema:
   sudo ./install.sh --system

2) Para instalar apenas para o seu usuário atual (sem precisar de senha root):
   ./install.sh --user

3) Para desinstalar futuramente:
   ./uninstall.sh
==========================================================
EOF

TAR_FILE="$OUTPUT_DIR/${PKG_NAME}-${VERSION}-linux-x64.tar.gz"
tar -czf "$TAR_FILE" -C "$PROJECT_ROOT/dist" "tar_staging" --transform "s|^tar_staging|tools-clipboard-${VERSION}|"
rm -rf "$TAR_STAGE"
echo "   Criado: $TAR_FILE"

# ---------------------------------------------------------------------------
# 4. Geração de Checksums (SHA256)
# ---------------------------------------------------------------------------
echo "==> [4/4] Gerando somas de verificação SHA256..."
cd "$OUTPUT_DIR"
sha256sum *.deb *.tar.gz > SHA256SUMS
echo "   Criado: $OUTPUT_DIR/SHA256SUMS"

echo
echo "=========================================================="
echo "    Pacotes de Release Gerados com Sucesso!               "
echo "=========================================================="
ls -lh "$OUTPUT_DIR"
echo "=========================================================="
