# Tools - Gerenciador de Área de Transferência (Win + V para Linux)

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![GTK4](https://img.shields.io/badge/GTK-4-green.svg)](https://www.gtk.org/)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows-orange.svg)]()

> 🟢 **Software Livre e de Código Aberto (Open Source) — Livre Uso:**  
> Este projeto é software livre disponibilizado sob a **GNU General Public License v3.0 (GPLv3)**, sendo de **total e livre uso**, estudo, modificação, distribuição e compartilhamento por qualquer usuário ou empresa.

---

Um gerenciador de área de transferência moderno e leve desenvolvido em **C++20** com **GTK 4**, inspirado no atalho **Win + V** do Windows. Roda em segundo plano no Linux, oferecendo histórico persistente, fixação de itens favoritos, colagem automática via teclado virtual, internacionalização e interface gráfica moderna com tema escuro.

---

## ⚡ Recursos

* **Atalho Global Win + V:** Integrado ao GNOME via `<Super>v` para abertura instantânea do popup na tela.
* **Histórico Configurável:** Salva dinamicamente entre 3 e 100 itens da área de transferência (`Ctrl + C` e seleções), sem duplicatas consecutivas.
* **Fixar Itens (Pin 📌):** Permite fixar itens importantes para que nunca sejam apagados automaticamente, mantendo-os salvos no arquivo `.ini` mesmo após reiniciar o computador.
* **Colagem Automática (Auto-Paste):** Ao clicar em qualquer item salvo, ele é copiado para a área de transferência ativa e colado instantaneamente na janela em foco (navegador, terminal, editor de texto, etc.) simulando `Ctrl + V`.
* **Menu de Configurações (`⚙`):**
  * Ajuste da quantidade de itens salvos (3 a 100 itens).
  * Chave seletora para **iniciar automaticamente com o sistema**.
  * Aba **Sobre** com informações do desenvolvedor e links oficiais.
* **Multi-idioma (i18n):** Suporte nativo a **Português (pt-BR)**, **Inglês (en-US)** e **Espanhol (es-ES)** com troca imediata pela interface.
* **Navegação Rápida:** Pressione <kbd>Esc</kbd> para fechar a qualquer instante.
* **Persistência em `.ini`:** Histórico e preferências salvas em `~/.config/Tools/clipboard_history.ini`.

---

## 📦 Pacotes de Instalação (Release)

Os pacotes prontos para instalação estão disponíveis no diretório `dist/release/`:

```bash
# Para gerar os pacotes a qualquer momento:
./scripts/package_release.sh
```

### Opção 1: Instalação via Pacote Debian/Ubuntu (.deb)
Recomendado para Ubuntu, Debian, Pop!_OS, Linux Mint e distribuições baseadas em `.deb`:
```bash
sudo apt install ./dist/release/tools-clipboard_1.0.0_amd64.deb
# Ou:
sudo dpkg -i ./dist/release/tools-clipboard_1.0.0_amd64.deb
```

O pacote `.deb` instala automaticamente:
* O executável em `/usr/bin/tools-clipboard`
* O lançador no menu de aplicativos (`tools-clipboard.desktop`)
* O ícone do aplicativo em alta resolução em `/usr/share/icons`
* As regras de `/dev/uinput` para que a colagem automática funcione perfeitamente

### Opção 2: Instalador Universal Portátil (.tar.gz)
Compatível com qualquer distribuição Linux:
```bash
tar -xzf dist/release/tools-clipboard-1.0.0-linux-x64.tar.gz
cd tools-clipboard-1.0.0

# Instalação para o seu usuário (sem precisar de senha sudo/root):
./install.sh --user

# Ou instalação no sistema inteiro:
sudo ./install.sh --system
```

Para desinstalar:
```bash
./uninstall.sh
```

---

## 🚀 Como Executar

### Pelo Terminal
```bash
# Iniciar em segundo plano (daemon)
tools-clipboard --daemon &

# Ou alternar a visibilidade da janela (Win + V)
tools-clipboard --toggle
```

### Configurar Atalho Win + V no GNOME
Se não tiver instalado via pacote, você pode configurar o atalho diretamente com:
```bash
./scripts/setup_shortcut.sh
```

---

## 🛠️ Como Compilar do Código-Fonte

### Pré-requisitos (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build libgtk-4-dev pkg-config
```

### Compilar
```bash
# Compilação Release
cmake --preset linux-release
cmake --build --preset linux-release

# Ou compilar e gerar os pacotes de instalação:
./scripts/build.sh --package
```

---

## 📄 Licença

Este projeto é software livre e de **livre uso**, licenciado sob a **[GNU General Public License v3.0 (GPLv3)](LICENSE)**. Qualquer pessoa tem a liberdade de executar, estudar, modificar e redistribuir o código sem custos.

---

## ℹ️ Sobre o Projeto
* **Desenvolvido por:** Luís Andrade Cordeiro
* **Data de Desenvolvimento:** 19 de setembro de 2026
* **Site Oficial:** [bnbb.com.br](https://bnbb.com.br)
* **Contato:** [contato@bnbb.com.br](mailto:contato@bnbb.com.br)
