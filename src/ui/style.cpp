#include "tools/ui/style.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <vector>

namespace tools::ui {

namespace fs = std::filesystem;

constexpr const char *DEFAULT_FALLBACK_CSS = R"(
window { background-color: #181825; color: #cdd6f4; border-radius: 12px; }
.window-container { background-color: #1e1e2e; border-radius: 12px; border: 1px solid #313244; padding: 12px; }
.header-title { font-size: 16px; font-weight: 700; color: #cdd6f4; }
.header-app-icon { border-radius: 4px; }
.shortcut-badge { background-color: #313244; color: #89b4fa; font-weight: bold; font-size: 11px; padding: 3px 8px; border-radius: 6px; border: 1px solid #45475a; }
.clear-button { background-color: transparent; color: #f38ba8; border-radius: 6px; padding: 4px 8px; border: 1px solid #45475a; font-size: 12px; }
.clear-button:hover { background-color: #45475a; }
.lang-dropdown { background-color: #252538; color: #cdd6f4; border-radius: 6px; border: 1px solid #45475a; font-size: 11px; padding: 0px 4px; }
.lang-dropdown:hover { background-color: #313244; }
.header-settings-btn { background-color: transparent; color: #cdd6f4; border-radius: 6px; padding: 4px 8px; border: 1px solid #45475a; font-size: 13px; }
.header-settings-btn:hover { background-color: #313244; color: #89b4fa; border-color: #89b4fa; }
.settings-gear-icon { font-size: 16px; color: #89b4fa; }
.section-title { font-size: 13px; font-weight: 700; color: #89b4fa; text-transform: uppercase; letter-spacing: 0.5px; margin-top: 4px; }
.settings-card { background-color: #252538; border-radius: 8px; border: 1px solid #313244; padding: 12px 14px; }
.settings-item-label { font-size: 13px; color: #cdd6f4; }
.settings-spin { background-color: #1e1e2e; color: #cdd6f4; border-radius: 6px; border: 1px solid #45475a; }
.settings-separator { background-color: #313244; margin-top: 4px; margin-bottom: 4px; }
.about-app-icon { border-radius: 8px; }
.about-app-name { font-size: 15px; font-weight: 700; color: #cdd6f4; }
.about-app-version { font-size: 12px; color: #a6adc8; }
.about-info-text { font-size: 13px; color: #bac2de; }
.about-link-btn { color: #89b4fa; font-size: 13px; padding: 4px 8px; border-radius: 6px; background-color: #1e1e2e; border: 1px solid #313244; }
.about-link-btn:hover { background-color: #313244; color: #b4befe; border-color: #89b4fa; }
.settings-close-btn { background-color: #313244; color: #cdd6f4; border-radius: 6px; padding: 8px 16px; border: 1px solid #45475a; font-size: 13px; font-weight: 600; }
.settings-close-btn:hover { background-color: #89b4fa; color: #181825; }
.clipboard-card { background-color: #252538; border-radius: 8px; border: 1px solid #313244; padding: 10px 12px; margin-bottom: 6px; }
.clipboard-card:hover { background-color: #2f2f47; border-color: #89b4fa; }
.clipboard-card:focus { background-color: #363652; border-color: #b4befe; }
.item-index-badge { background-color: #313244; color: #89b4fa; font-weight: bold; font-size: 11px; padding: 2px 6px; border-radius: 4px; }
.item-time { font-size: 11px; color: #6c7086; }
.item-content-preview { font-size: 13px; color: #cdd6f4; font-family: monospace, sans-serif; margin-top: 4px; }
.item-delete-btn { background: transparent; color: #a6adc8; border: 1px solid transparent; padding: 3px 8px; border-radius: 6px; font-size: 13px; font-weight: bold; opacity: 0.7; }
.item-delete-btn:hover { color: #ffffff; background: #e78284; border-color: #ea999c; opacity: 1.0; }
.item-pin-btn { background: transparent; border: 1px solid transparent; padding: 3px 8px; border-radius: 6px; font-size: 12px; opacity: 0.6; }
.item-pin-btn:hover { opacity: 1.0; background: #313244; border-color: #89b4fa; }
.item-pin-btn.pinned { opacity: 1.0; background: #313244; border-color: #89b4fa; }
.clipboard-card.pinned { border-left: 3px solid #89b4fa; background-color: #26263b; }
.empty-state-label { color: #6c7086; font-style: italic; padding: 24px; }
)";

static std::string locate_css_file() {
    std::vector<std::string> candidates = {
        "resources/style.css",
        "style.css",
        "../resources/style.css"
    };

    try {
        if (fs::exists("/proc/self/exe")) {
            auto exe_dir = fs::canonical("/proc/self/exe").parent_path();
            candidates.push_back((exe_dir / "resources" / "style.css").string());
            candidates.push_back((exe_dir / "style.css").string());
            candidates.push_back((exe_dir / ".." / "resources" / "style.css").string());
            candidates.push_back((exe_dir / ".." / "share" / "tools-clipboard" / "style.css").string());
        }
    } catch (...) {}

    candidates.push_back("/usr/share/tools-clipboard/style.css");
    candidates.push_back("/usr/lib/tools-clipboard/resources/style.css");
    candidates.push_back("/usr/local/share/tools-clipboard/style.css");

    const char *home = std::getenv("HOME");
    if (home) {
        candidates.push_back(std::string(home) + "/.local/share/tools-clipboard/style.css");
        candidates.push_back(std::string(home) + "/.config/Tools/style.css");
    }

    for (const auto &p : candidates) {
        if (fs::exists(p)) {
            return p;
        }
    }
    return "";
}

void apply_global_css(const std::string &custom_css) {
    GtkCssProvider *provider = gtk_css_provider_new();

    std::string css_data;
    if (!custom_css.empty()) {
        css_data = custom_css;
    } else {
        std::string path = locate_css_file();
        if (!path.empty()) {
            std::ifstream f(path);
            if (f.is_open()) {
                std::stringstream buffer;
                buffer << f.rdbuf();
                css_data = buffer.str();
            }
        }
        if (css_data.empty()) {
            css_data = DEFAULT_FALLBACK_CSS;
        }
    }

    gtk_css_provider_load_from_string(provider, css_data.c_str());

    GdkDisplay *display = gdk_display_get_default();
    if (display) {
        gtk_style_context_add_provider_for_display(
            display,
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }
    g_object_unref(provider);
}

} // namespace tools::ui
