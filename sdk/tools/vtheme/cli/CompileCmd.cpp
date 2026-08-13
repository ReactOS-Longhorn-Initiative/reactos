#include "Util.h"

#include <string>
#include <vcompile/Compiler.h>
#include <vcompile/IniImporter.h>

using cli::outln;
using cli::toUtf8;

namespace cli_cmd {

int cmdCompile(const std::wstring& src, const std::wstring& out) {
    auto st = vcompile::compileTheme(src, out);
    outln("Compiled " + toUtf8(src) + " -> " + toUtf8(out));
    outln("  classes : " + std::to_string(st.classes));
    outln("  records : " + std::to_string(st.records));
    outln("  images  : " + std::to_string(st.images));
    if (st.skipped)
        outln("  skipped : " + std::to_string(st.skipped) + " (unsupported property types)");
    return 0;
}

int cmdImportIni(const std::wstring& ini, const std::wstring& bitmaps, const std::wstring& out) {
    auto st = vcompile::importXpTheme(ini, bitmaps, out);
    outln("Imported XP theme " + toUtf8(ini) + " -> " + toUtf8(out));
    outln("  sections     : " + std::to_string(st.sections));
    outln("  classes      : " + std::to_string(st.compile.classes));
    outln("  records      : " + std::to_string(st.compile.records));
    outln("  images       : " + std::to_string(st.compile.images));
    if (st.unmappedSections)
        outln("  unmapped sections : " + std::to_string(st.unmappedSections));
    if (st.unmappedProps)
        outln("  unmapped props    : " + std::to_string(st.unmappedProps));
    if (st.missingImages)
        outln("  missing bitmaps   : " + std::to_string(st.missingImages));
    return 0;
}

} // namespace cli_cmd
