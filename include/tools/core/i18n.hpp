#pragma once

#include <string>
#include <vector>
#include <functional>

namespace tools::core {

enum class Language {
    PT_BR = 0,
    EN_US = 1,
    ES_ES = 2
};

enum class StringId {
    WindowTitle,
    ClearAll,
    EmptyTitle,
    EmptyHint,
    DeleteTooltip,
    LanguageLabel,
    CopiedNotification,
    PinTooltip,
    UnpinTooltip,
    SettingsTitle,
    AboutTitle,
    PreferencesTitle,
    MaxItemsLabel,
    AutostartLabel,
    DevelopedByLabel,
    DevelopedDateLabel,
    WebsiteLabel,
    ContactLabel,
    CloseButton
};

class I18n {
public:
    static I18n& instance();

    Language get_language() const;
    void set_language(Language lang);

    std::string get(StringId id) const;
    std::string get_language_name(Language lang) const;

    void on_language_changed(std::function<void(Language)> callback);

private:
    I18n();
    void load_settings();
    void save_settings();
    Language detect_system_language() const;

    Language current_lang_{Language::PT_BR};
    std::vector<std::function<void(Language)>> listeners_;
};

} // namespace tools::core
