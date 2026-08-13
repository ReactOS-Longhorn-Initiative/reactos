// IniImporter.h - convert a classic XP/2003 visual-style INI (the textual
// .msstyles definition, e.g. ReactOS Lautus) into a Vista-format v4 .msstyles.
#pragma once
#include "Compiler.h"
#include <string>

namespace vcompile {

struct IniImportStats {
    CompileStats compile;
    int sections = 0;
    int unmappedSections = 0;
    int unmappedProps = 0;
    int missingImages = 0;
};

// Parse `iniPath` (an XP visual-style INI), resolve class/part/state/property
// names to numeric ids via the schema map, embed referenced bitmaps from
// `bitmapsDir`, and write a v4 .msstyles to `outPath`.
IniImportStats importXpTheme(const std::wstring& iniPath, const std::wstring& bitmapsDir,
                             const std::wstring& outPath);

} // namespace vcompile
