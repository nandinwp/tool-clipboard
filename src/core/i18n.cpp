#include "tools/core/i18n.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>

namespace tools::core {

namespace fs = std::filesystem;

static fs::path get_config_dir() {
    const char *home = std::getenv("HOME");
    if (home) {
        return fs::path(home) / ".config" / "Tools";
    }
    return fs::current_path();
}

static fs::path get_language_file() {
    return get_config_dir() / "language.cfg";
}

I18n& I18n::instance() {
    static I18n s_instance;
    return s_instance;
}

I18n::I18n() {
    load_settings();
}

Language I18n::detect_system_language() const {
    const char *lang_env = std::getenv("LANG");
    if (!lang_env) {
        lang_env = std::getenv("LC_ALL");
    }

    if (lang_env) {
        std::string s(lang_env);
        if (s.rfind("pt", 0) == 0) {
            return Language::PT_BR;
        } else if (s.rfind("es", 0) == 0) {
            return Language::ES_ES;
        } else if (s.rfind("en", 0) == 0) {
            return Language::EN_US;
        }
    }

    return Language::PT_BR;
}

void I18n::load_settings() {
    fs::path cfg = get_language_file();
    if (fs::exists(cfg)) {
        std::ifstream file(cfg);
        std::string lang_code;
        if (file >> lang_code) {
            if (lang_code == "pt_BR" || lang_code == "pt") {
                current_lang_ = Language::PT_BR;
                return;
            } else if (lang_code == "en_US" || lang_code == "en") {
                current_lang_ = Language::EN_US;
                return;
            } else if (lang_code == "es_ES" || lang_code == "es") {
                current_lang_ = Language::ES_ES;
                return;
            }
        }
    }

    current_lang_ = detect_system_language();
}

void I18n::save_settings() {
    try {
        fs::path dir = get_config_dir();
        fs::create_directories(dir);
        fs::path cfg = get_language_file();
        std::ofstream file(cfg, std::ios::trunc);
        if (file.is_open()) {
            switch (current_lang_) {
                case Language::PT_BR: file << "pt_BR\n"; break;
                case Language::EN_US: file << "en_US\n"; break;
                case Language::ES_ES: file << "es_ES\n"; break;
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "[I18n] Erro ao salvar configuracao de idioma: " << e.what() << std::endl;
    }
}

Language I18n::get_language() const {
    return current_lang_;
}

void I18n::set_language(Language lang) {
    if (current_lang_ == lang) return;

    current_lang_ = lang;
    save_settings();

    for (const auto &cb : listeners_) {
        if (cb) cb(current_lang_);
    }
}

void I18n::on_language_changed(std::function<void(Language)> callback) {
    listeners_.push_back(std::move(callback));
}

std::string I18n::get_language_name(Language lang) const {
    switch (lang) {
        case Language::PT_BR: return "Português";
        case Language::EN_US: return "English";
        case Language::ES_ES: return "Español";
    }
    return "Português";
}

std::string I18n::get(StringId id) const {
    switch (current_lang_) {
        case Language::PT_BR:
            switch (id) {
                case StringId::WindowTitle: return "Área de Transferência";
                case StringId::ClearAll: return "Limpar tudo";
                case StringId::EmptyTitle: return "Histórico vazio";
                case StringId::EmptyHint: return "Copie qualquer texto (Ctrl + C) para que ele apareça aqui.\nAté 10 itens serão guardados.";
                case StringId::DeleteTooltip: return "Excluir";
                case StringId::LanguageLabel: return "Idioma";
                case StringId::CopiedNotification: return "Copiado para a área de transferência";
                case StringId::PinTooltip: return "Fixar";
                case StringId::UnpinTooltip: return "Desafixar";
                case StringId::SettingsTitle: return "Configurações";
                case StringId::AboutTitle: return "Sobre";
                case StringId::PreferencesTitle: return "Preferências";
                case StringId::MaxItemsLabel: return "Quantidade de itens salvos";
                case StringId::AutostartLabel: return "Iniciar automaticamente com o sistema";
                case StringId::DevelopedByLabel: return "Desenvolvido por Luís Andrade Cordeiro";
                case StringId::DevelopedDateLabel: return "Data de desenvolvimento: 19 de setembro de 2026";
                case StringId::WebsiteLabel: return "Site oficial";
                case StringId::ContactLabel: return "Contato";
                case StringId::CloseButton: return "Fechar";
            }
            break;

        case Language::EN_US:
            switch (id) {
                case StringId::WindowTitle: return "Clipboard History";
                case StringId::ClearAll: return "Clear all";
                case StringId::EmptyTitle: return "History is empty";
                case StringId::EmptyHint: return "Copy any text (Ctrl + C) and it will appear here.\nUp to 10 items will be saved.";
                case StringId::DeleteTooltip: return "Delete";
                case StringId::LanguageLabel: return "Language";
                case StringId::CopiedNotification: return "Copied to clipboard";
                case StringId::PinTooltip: return "Pin";
                case StringId::UnpinTooltip: return "Unpin";
                case StringId::SettingsTitle: return "Settings";
                case StringId::AboutTitle: return "About";
                case StringId::PreferencesTitle: return "Preferences";
                case StringId::MaxItemsLabel: return "Saved items limit";
                case StringId::AutostartLabel: return "Start automatically with system";
                case StringId::DevelopedByLabel: return "Developed by Luís Andrade Cordeiro";
                case StringId::DevelopedDateLabel: return "Development date: September 19, 2026";
                case StringId::WebsiteLabel: return "Official Website";
                case StringId::ContactLabel: return "Contact";
                case StringId::CloseButton: return "Close";
            }
            break;

        case Language::ES_ES:
            switch (id) {
                case StringId::WindowTitle: return "Historial del Portapapeles";
                case StringId::ClearAll: return "Borrar todo";
                case StringId::EmptyTitle: return "Historial vacío";
                case StringId::EmptyHint: return "Copie cualquier texto (Ctrl + C) para que aparezca aquí.\nSe guardarán hasta 10 elementos.";
                case StringId::DeleteTooltip: return "Eliminar";
                case StringId::LanguageLabel: return "Idioma";
                case StringId::CopiedNotification: return "Copiado al portapapeles";
                case StringId::PinTooltip: return "Fijar";
                case StringId::UnpinTooltip: return "Desfijar";
                case StringId::SettingsTitle: return "Configuración";
                case StringId::AboutTitle: return "Acerca de";
                case StringId::PreferencesTitle: return "Preferencias";
                case StringId::MaxItemsLabel: return "Cantidad de elementos guardados";
                case StringId::AutostartLabel: return "Iniciar automáticamente con el sistema";
                case StringId::DevelopedByLabel: return "Desarrollado por Luís Andrade Cordeiro";
                case StringId::DevelopedDateLabel: return "Fecha de desarrollo: 19 de septiembre de 2026";
                case StringId::WebsiteLabel: return "Sitio oficial";
                case StringId::ContactLabel: return "Contacto";
                case StringId::CloseButton: return "Cerrar";
            }
            break;
    }
    return "";
}

} // namespace tools::core
