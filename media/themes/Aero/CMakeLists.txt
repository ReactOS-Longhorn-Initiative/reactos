#
# Aero - a Vista-format (PACKTHEM_VERSION 4) visual style.
#
# Unlike the four styles beside it, this one is not linked from a .rc.  A v4
# style is not INI text in resources; it is a binary VSRECORD stream across the
# CMAP/VMAP/BCMAP/RMAP/VARIANT tables, and rc.exe has no way to produce one.
# It is compiled instead, by the vtheme host tool in sdk/tools/vtheme.
#
# The source form is exactly what `vtheme decompile` emits - a line-oriented
# theme.vtheme plus a directory of PNGs.  Both are text-and-images rather than
# an opaque binary, so the style is diffable and reviewable like any other
# source in the tree, which is the whole reason for building it here rather
# than checking in a .msstyles.
#
# uxtheme_new (DarkFiresReactOSModules/uxtheme_new) is what reads the result;
# see its DESIGN.md section 7.4.
#
# ---------------------------------------------------------------------------
# PROVENANCE - read this before shipping an image built with it.
#
# theme.vtheme and images/ here are a decompile of Microsoft's Vista SP1
# aero.msstyles.  The toolchain is clean-room; THE ARTWORK IS NOT.  Building
# your own image with it for local testing is exactly what it is for.  Putting
# that image anywhere public is redistributing Microsoft assets, and the fact
# that the compiler is ours does not change that.
#
# The pipeline is identical either way: author a .vtheme carrying a DWMWindow
# class, point it at original artwork, compile it with the same host tool.  So
# replacing the art is a file change, not a rework - which is the reason the
# source form and the art are kept separable.
# ---------------------------------------------------------------------------
#

set(_aero_src ${CMAKE_CURRENT_SOURCE_DIR}/theme.vtheme)
set(_aero_out ${CMAKE_CURRENT_BINARY_DIR}/aero.msstyles)

# Every PNG is an input: the compiler resolves @image:<id> against images/, so
# touching one has to rebuild the style.  Globbing at configure time means a
# newly ADDED image needs a re-run of cmake, which is the usual trade and the
# same one add_rc_deps makes for .rc dependencies elsewhere in the tree.
file(GLOB _aero_images ${CMAKE_CURRENT_SOURCE_DIR}/images/*)

add_custom_command(
    OUTPUT ${_aero_out}
    COMMAND native-vtheme compile ${_aero_src} -o ${_aero_out}
    DEPENDS native-vtheme ${_aero_src} ${_aero_images}
    COMMENT "Compiling Aero (v4 visual style)"
    VERBATIM)

add_custom_target(aero_msstyles ALL DEPENDS ${_aero_out})

add_cd_file(FILE ${_aero_out} DESTINATION reactos/Resources/Themes/Aero FOR all)

#
# Installing the style is necessary but not sufficient: ReactOS boots on
# Classic, so OpenThemeData returns NULL for every class until a style is
# actually SELECTED (uxtheme_new/DESIGN.md section 7.3).  Selection is a
# themeui/registry matter and is deliberately not done here.
#
