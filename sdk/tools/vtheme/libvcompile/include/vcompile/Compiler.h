// Compiler.h - build a v4 .msstyles from a .vtheme source description.
#pragma once
#include <stdexcept>
#include <string>

namespace vcompile {

class CompileError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct CompileStats {
    int classes = 0;
    int records = 0;
    int images = 0;
    int skipped = 0;  // properties that could not be reconstructed
};

// Compile `srcPath` (a .vtheme file) into a Vista-format .msstyles at `outPath`.
// Images referenced as @image:<id> are loaded from <srcDir>/images/<id>.*,
// and @file:<path> embeds an arbitrary PNG/BMP. Throws CompileError on failure.
CompileStats compileTheme(const std::wstring& srcPath, const std::wstring& outPath);

} // namespace vcompile
