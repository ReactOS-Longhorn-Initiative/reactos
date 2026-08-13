// vtheme - decompile / inspect / render / compile Vista+ visual styles.
#include "Format.h"
#include "Util.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>
#include <windows.h>

#include <vtheme/ThemeLoader.h>
#include <vtheme/Tmt.h>

#ifdef VTHEME_HAVE_RENDER
#include <vrender/Renderer.h>
#endif
#ifdef VTHEME_HAVE_COMPILE
#include <vcompile/Compiler.h>
#endif

namespace fs = std::filesystem;
using namespace vtheme;
using cli::fromUtf8;
using cli::outln;
using cli::toUtf8;

namespace {

int usage() {
    outln("VistaThemeTools - Vista+ (.msstyles v4) toolkit\n");
    outln("Usage:");
    outln("  vtheme info <style.msstyles>");
    outln("  vtheme list <style.msstyles>");
    outln("  vtheme decompile <style.msstyles> [-o <out-dir>] [--variant <NAME>]");
#ifdef VTHEME_HAVE_RENDER
    outln("  vtheme render <style.msstyles> --class <NAME> --part <id> --state <id> "
          "[--size WxH] -o <out.png>");
#endif
#ifdef VTHEME_HAVE_COMPILE
    outln("  vtheme compile <src.vtheme> -o <out.msstyles>");
    outln("  vtheme import-ini <xp.ini> --bitmaps <dir> -o <out.msstyles>");
#endif
    return 2;
}

const char* extForImage(const std::vector<uint8_t>& b) {
    if (b.size() >= 8 && b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G')
        return "png";
    if (b.size() >= 2 && b[0] == 'B' && b[1] == 'M')
        return "bmp";
    return "bin";
}

// Walk every property in a class so we can extract referenced resources.
template <class F>
void forEachProperty(const ThemeFile& tf, F&& fn) {
    for (const auto& p : tf.rootProperties)
        fn(p);
    for (const auto& c : tf.classes) {
        for (const auto& p : c.properties)
            fn(p);
        for (const auto& part : c.parts) {
            for (const auto& p : part.properties)
                fn(p);
            for (const auto& st : part.states)
                for (const auto& p : st.properties)
                    fn(p);
        }
    }
}

int countStates(const ThemeFile& tf) {
    int n = 0;
    for (const auto& c : tf.classes)
        for (const auto& part : c.parts)
            n += (int)part.states.size();
    return n;
}
int countParts(const ThemeFile& tf) {
    int n = 0;
    for (const auto& c : tf.classes)
        n += (int)c.parts.size();
    return n;
}

void writeProps(std::ofstream& f, const std::vector<ThemeProperty>& props, const char* indent) {
    for (const auto& p : props) {
        f << indent << tmtName(p.propertyId) << " (" << cli::primitiveTag(p)
          << ") = " << cli::formatValue(p) << "\n";
    }
}

int cmdInfo(const std::wstring& path) {
    auto tf = loadTheme(path);
    outln("File      : " + toUtf8(tf->path));
    outln("MUI       : " + (tf->muiPath.empty() ? std::string("(none)") : toUtf8(tf->muiPath)));
    outln("Version   : " + std::to_string(tf->version) + (tf->version == 4 ? "  (Vista+ v4)" : ""));
    outln("Variant   : " + toUtf8(tf->variant.name));
    outln("  Color   : " + toUtf8(tf->variant.color));
    outln("  Size    : " + toUtf8(tf->variant.size));
    outln("Classes   : " + std::to_string(tf->classes.size()));
    outln("Parts     : " + std::to_string(countParts(*tf)));
    outln("States    : " + std::to_string(countStates(*tf)));
    std::set<uint32_t> images;
    forEachProperty(*tf, [&](const ThemeProperty& p) {
        if (auto r = p.value.get<ResourceRef>())
            if (r->resType == L"IMAGE")
                images.insert(r->resId);
    });
    outln("Images    : " + std::to_string(images.size()));
    return 0;
}

int cmdList(const std::wstring& path) {
    auto tf = loadTheme(path);
    for (const auto& c : tf->classes) {
        std::string line = toUtf8(c.fullName);
        if (!c.baseClassName.empty())
            line += " : " + toUtf8(c.baseClassName);
        line += "  [parts=" + std::to_string(c.parts.size()) +
                ", classProps=" + std::to_string(c.properties.size()) + "]";
        outln(line);
    }
    return 0;
}

int cmdClassMap(const std::wstring& path) {
    auto tf = loadTheme(path);
    for (size_t i = 0; i < tf->classNames.size(); ++i)
        outln("[" + std::to_string(i) + "] \"" + toUtf8(tf->classNames[i]) + "\"");
    return 0;
}

int cmdDecompile(const std::wstring& path, const std::wstring& outDir,
                 const std::wstring& variant) {
    LoadOptions opts;
    opts.variantName = variant;
    auto tf = loadTheme(path, opts);

    fs::path root = outDir.empty() ? fs::path(path).stem() : fs::path(outDir);
    fs::create_directories(root);
    fs::path imgDir = root / "images";

    // Extract all referenced IMAGE/STREAM resources (deduplicated).
    std::set<uint32_t> done;
    size_t nImg = 0;
    forEachProperty(*tf, [&](const ThemeProperty& p) {
        auto r = p.value.get<ResourceRef>();
        if (!r || r->data.empty() || done.count(r->resId))
            return;
        done.insert(r->resId);
        fs::create_directories(imgDir);
        const char* ext = r->resType == L"STREAM" ? "bin" : extForImage(r->data);
        fs::path fp = imgDir / (std::to_string(r->resId) + "." + ext);
        std::ofstream of(fp, std::ios::binary);
        of.write(reinterpret_cast<const char*>(r->data.data()), r->data.size());
        ++nImg;
    });

    fs::path srcPath = root / "theme.vtheme";
    std::ofstream f(srcPath, std::ios::binary);
    f << "# Decompiled by VistaThemeTools\n";
    f << "# Source: " << toUtf8(fs::path(path).filename().wstring()) << "\n\n";
    f << "THEME {\n";
    f << "    version = " << tf->version << "\n";
    f << "    variant = " << toUtf8(tf->variant.name) << "\n";
    f << "    color   = " << toUtf8(tf->variant.color) << "\n";
    f << "    size    = " << toUtf8(tf->variant.size) << "\n";
    f << "}\n\n";

    if (!tf->rootProperties.empty()) {
        f << "ROOT {\n";
        writeProps(f, tf->rootProperties, "    ");
        f << "}\n\n";
    }

    for (const auto& c : tf->classes) {
        f << "CLASS " << "\"" << toUtf8(c.fullName) << "\"";
        if (!c.baseClassName.empty())
            f << " : \"" << toUtf8(c.baseClassName) << "\"";
        f << " {\n";
        writeProps(f, c.properties, "    ");
        for (const auto& part : c.parts) {
            f << "    PART " << part.id;
            if (!part.name.empty())
                f << " \"" << toUtf8(part.name) << "\"";
            f << " {\n";
            writeProps(f, part.properties, "        ");
            for (const auto& st : part.states) {
                f << "        STATE " << st.id;
                if (!st.name.empty())
                    f << " \"" << toUtf8(st.name) << "\"";
                f << " {\n";
                writeProps(f, st.properties, "            ");
                f << "        }\n";
            }
            f << "    }\n";
        }
        f << "}\n\n";
    }

    outln("Decompiled to: " + toUtf8(root.wstring()));
    outln("  theme.vtheme  (" + std::to_string(tf->classes.size()) + " classes)");
    outln("  images/       (" + std::to_string(nImg) + " resources)");
    return 0;
}

} // namespace

namespace cli_cmd {
#ifdef VTHEME_HAVE_RENDER
int cmdRender(int argc, wchar_t** argv);
#endif
#ifdef VTHEME_HAVE_COMPILE
int cmdCompile(const std::wstring& src, const std::wstring& out);
int cmdImportIni(const std::wstring& ini, const std::wstring& bitmaps, const std::wstring& out);
#endif
} // namespace cli_cmd

int wmain(int argc, wchar_t** argv) {
    ::SetConsoleOutputCP(65001);
    if (argc < 2)
        return usage();

    std::wstring cmd = argv[1];
    try {
        if (cmd == L"info" && argc >= 3)
            return cmdInfo(argv[2]);
        if (cmd == L"list" && argc >= 3)
            return cmdList(argv[2]);
        if (cmd == L"classmap" && argc >= 3)
            return cmdClassMap(argv[2]);
        if (cmd == L"decompile" && argc >= 3) {
            std::wstring out, variant;
            for (int i = 3; i < argc; ++i) {
                std::wstring a = argv[i];
                if (a == L"-o" && i + 1 < argc)
                    out = argv[++i];
                else if (a == L"--variant" && i + 1 < argc)
                    variant = argv[++i];
            }
            return cmdDecompile(argv[2], out, variant);
        }
#ifdef VTHEME_HAVE_RENDER
        if (cmd == L"render")
            return cli_cmd::cmdRender(argc, argv);
#endif
#ifdef VTHEME_HAVE_COMPILE
        if (cmd == L"compile" && argc >= 3) {
            std::wstring out;
            for (int i = 3; i < argc; ++i) {
                std::wstring a = argv[i];
                if (a == L"-o" && i + 1 < argc)
                    out = argv[++i];
            }
            if (out.empty()) {
                outln("error: compile requires -o <out.msstyles>");
                return 2;
            }
            return cli_cmd::cmdCompile(argv[2], out);
        }
        if (cmd == L"import-ini" && argc >= 3) {
            std::wstring out, bitmaps;
            for (int i = 3; i < argc; ++i) {
                std::wstring a = argv[i];
                if (a == L"-o" && i + 1 < argc)
                    out = argv[++i];
                else if (a == L"--bitmaps" && i + 1 < argc)
                    bitmaps = argv[++i];
            }
            if (out.empty() || bitmaps.empty()) {
                outln("error: import-ini requires --bitmaps <dir> and -o <out.msstyles>");
                return 2;
            }
            return cli_cmd::cmdImportIni(argv[2], bitmaps, out);
        }
#endif
    } catch (const std::exception& e) {
        outln(std::string("error: ") + e.what());
        return 1;
    }
    return usage();
}
