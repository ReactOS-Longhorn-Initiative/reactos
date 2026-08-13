#
# vtheme - the Vista (v4) visual-style compiler, as a ReactOS host tool.
#
# See README.md for provenance and for what was left out of this copy.
#
# The short version: this is VistaThemeTools with the RENDERER REMOVED.  The
# renderer is the only part that needs WIC, and a host tool that has to run on
# the build machine is better off not depending on it - the CLI already guards
# every renderer reference behind VTHEME_HAVE_RENDER, so dropping it is a
# supported configuration and not a fork.  What remains needs the PE resource
# APIs (FindResource / UpdateResource) and nothing else, which is a much
# smaller thing to replace if this build is ever hosted somewhere other than
# Windows.  DESIGN.md section 7.4 asks for that decision to be made before
# porting rather than after; this is it.
#

if(NOT WIN32)
    message(FATAL_ERROR
        "vtheme uses the Win32 PE resource APIs (FindResource/UpdateResource) "
        "and can only be built on a Windows host.  A cross-hosted build needs "
        "a resource writer that does not go through UpdateResource.")
endif()

# C++20: std::filesystem, designated initialisers, structured bindings.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(
    -DUNICODE -D_UNICODE
    -DNOMINMAX
    -DWIN32_LEAN_AND_MEAN
    -DVTHEME_HAVE_COMPILE)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/libvtheme/include
    ${CMAKE_CURRENT_SOURCE_DIR}/libvcompile/include)

list(APPEND SOURCE
    # libvtheme - the v4 parser
    libvtheme/src/PeModule.cpp
    libvtheme/src/Query.cpp
    libvtheme/src/ThemeFile.cpp
    libvtheme/src/ThemeLoader.cpp
    libvtheme/src/Tmt.cpp
    # libvcompile - .vtheme parser, VSRECORD packer, PE resource writer,
    #               and the XP-INI importer
    libvcompile/src/Parser.cpp
    libvcompile/src/Compiler.cpp
    libvcompile/src/StyleMap.cpp
    libvcompile/src/IniImport.cpp
    # the CLI
    cli/main.cpp
    cli/Util.cpp
    cli/Format.cpp
    cli/CompileCmd.cpp)

add_host_tool(vtheme ${SOURCE})

#
# themestub.dll - the empty carrier PE the compiler injects into.
#
# It has to land in TOOLS_FOLDER next to vtheme.exe, because that is where
# vcompile::writeModel looks for it (Compiler.cpp: `exeDir() / "themestub.dll"`).
# add_host_module puts it there.
#
# So the carrier is a HOST binary: on an x64 build machine the emitted
# .msstyles is a PE64, whatever ARCH the tree is being built for.  That is
# deliberate and harmless - a visual style is a resource-only file that both
# Vista's uxtheme and ours open with LOAD_LIBRARY_AS_DATAFILE, which maps the
# image without looking at its machine type.  Real Vista relies on the same
# thing: one aero.msstyles serves 32- and 64-bit processes alike.
#
# If that ever turns out not to hold on ReactOS, the fix is to build the
# carrier as a target module the way media/themes/Lautus builds lautus.msstyles
# and pass its path in - not to change the compiler.
#
add_host_module(themestub stub/stub.c)
add_dependencies(vtheme themestub)
