// Format.h - render a decoded theme Value as a .vtheme text token.
#pragma once
#include <string>
#include <vtheme/ThemeFile.h>

namespace cli {

// UTF-8 token for the value of `prop` (e.g. "STRETCH", "#3C7FB1", "4, 4, 3, 3").
// Image/stream values render as "@image:<id>" / "@stream:<id>".
std::string formatValue(const vtheme::ThemeProperty& prop);

// A short type tag for headers, e.g. "COLOR", "IMAGEFILE".
std::string primitiveTag(const vtheme::ThemeProperty& prop);

} // namespace cli
