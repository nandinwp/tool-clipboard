#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "=========================================================="
echo "   Iniciando Build Multiplataforma do Projeto Tools       "
echo "=========================================================="
echo "Diretório do projeto: $PROJECT_ROOT"
echo

# 1. Compilação Linux Release
echo "==> [1/3] Compilando Release para Linux..."
cmake -S "$PROJECT_ROOT" --preset linux-release
cmake --build --preset linux-release

# 2. Criação do Pacote de Distribuição
echo
echo "==> [2/3] Criando pacote de distribuição em dist/linux..."
DIST_DIR="$PROJECT_ROOT/dist/linux"
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR/bin"
mkdir -p "$DIST_DIR/resources"

cp "$PROJECT_ROOT/build/linux-release/Tools" "$DIST_DIR/bin/"
cp "$PROJECT_ROOT/resources/style.css" "$DIST_DIR/resources/"
cp "$PROJECT_ROOT/resources/compliance.png" "$DIST_DIR/resources/" 2>/dev/null || true
cp "$PROJECT_ROOT/resources/compliance.ico" "$DIST_DIR/resources/" 2>/dev/null || true

# Cria o script de inicialização portátil
cat << 'EOF' > "$DIST_DIR/run.sh"
#!/usr/bin/env bash
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$HERE"
exec "./bin/Tools" "$@"
EOF
chmod +x "$DIST_DIR/run.sh"

# 3. Empacotamento de Release (se solicitado ou por padrão)
if [ "$1" = "--package" ] || [ "$1" = "-p" ]; then
    echo
    echo "==> Gerando pacotes de instalação (.deb e .tar.gz)..."
    "$PROJECT_ROOT/scripts/package_release.sh"
fi

# 4. Resumo final
echo
echo "==> [3/3] Build finalizado com sucesso!"
echo "----------------------------------------------------------"
echo " Executável Linux: $DIST_DIR/bin/Tools"
echo " Script de execução portátil: $DIST_DIR/run.sh"
echo " Pacotes de instalação prontos em:"
echo "   $PROJECT_ROOT/dist/release/"
echo "   (Para gerar os pacotes a qualquer momento: ./scripts/package_release.sh)"
echo
echo " Para rodar agora:"
echo "   $DIST_DIR/run.sh"
echo "----------------------------------------------------------"
