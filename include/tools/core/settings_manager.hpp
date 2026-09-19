#pragma once

#include <string>
#include <functional>
#include <vector>

namespace tools::core {

class SettingsManager {
public:
    static SettingsManager& instance();

    int get_max_items() const { return max_items_; }
    void set_max_items(int count);

    bool is_autostart_enabled() const { return autostart_enabled_; }
    void set_autostart_enabled(bool enabled);

    void load_settings();
    void save_settings() const;

    using ChangeCallback = std::function<void()>;
    void on_changed(ChangeCallback cb);

private:
    SettingsManager();
    ~SettingsManager() = default;

    void update_autostart_file();
    std::string get_autostart_path() const;
    std::string get_ini_path() const;

    int max_items_{10};
    bool autostart_enabled_{false};
    std::vector<ChangeCallback> listeners_;
};

} // namespace tools::core
