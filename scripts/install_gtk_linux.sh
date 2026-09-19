#!/usr/bin/env bash
set -e

echo "======================================================"
echo " Instalando dependências de desenvolvimento do GTK 4 "
echo "======================================================"
echo

sudo apt update
sudo apt install -y libgtk-4-dev build-essential pkg-config

echo
echo "======================================================"
echo " GTK 4 instalado com sucesso!"
echo " Versao instalada: $(pkg-config --modversion gtk4)"
echo " Agora voce pode voltar ao CLion e clicar em Run/Build."
echo "======================================================"
