// ThemeLoader.h - parse a v4 .msstyles into a ThemeFile object model.
#pragma once
#include "ThemeFile.h"
#include <memory>
#include <stdexcept>
#include <string>

namespace vtheme {

class ThemeError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct LoadOptions {
    bool highContrast = false;
    // Explicit MUI path; if empty, derived as <dir>\en-US\<file>.mui.
    std::wstring muiPath;
    // If a color/size variant other than the default is wanted, set these to the
    // VARIANT resource name. Empty => use the VMAP default.
    std::wstring variantName;
};

// Parse the style at `path`. Throws ThemeError on failure.
std::unique_ptr<ThemeFile> loadTheme(const std::wstring& path, const LoadOptions& opts = {});

} // namespace vtheme
