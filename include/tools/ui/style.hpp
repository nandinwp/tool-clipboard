#pragma once

#include <gtk/gtk.h>
#include <string>

namespace tools::ui {

// Aplica estilo CSS global ao display do GTK
void apply_global_css(const std::string &custom_css = "");

} // namespace tools::ui
