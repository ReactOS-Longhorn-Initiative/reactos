#include "Builder.h"
#include "StyleMap.h"
#include "vcompile/IniImporter.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>

#include <vssym32.h>
#include <windows.h>

namespace fs = std::filesystem;

namespace vcompile {
namespace {

std::wstring widen(const std::string& s) {
    int n = ::MultiByteToWideChar(CP_ACP, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    ::MultiByteToWideChar(CP_ACP, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos)
        return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
    return s;
}

std::vector<int> nums(const std::string& s) {
    std::vector<int> out;
    std::string cur;
    auto flush = [&] {
        if (!cur.empty()) {
            try { out.push_back(std::stoi(cur)); } catch (...) {}
            cur.clear();
        }
    };
    for (char c : s) {
        if ((c >= '0' && c <= '9') || c == '-')
            cur.push_back(c);
        else
            flush();
    }
    flush();
    return out;
}

void buildLogFont(std::vector<uint8_t>& out, const std::string& spec) {
    std::string face = spec;
    int pt = 8;
    bool bold = false, italic = false;
    auto a = spec.find(',');
    if (a != std::string::npos) {
        face = trim(spec.substr(0, a));
        auto rest = spec.substr(a + 1);
        auto b = rest.find(',');
        std::string ptStr = (b == std::string::npos) ? rest : rest.substr(0, b);
        try { pt = std::stoi(trim(ptStr)); } catch (...) {}
        std::string opt = upper(b == std::string::npos ? "" : rest.substr(b + 1));
        bold = opt.find("BOLD") != std::string::npos;
        italic = opt.find("ITALIC") != std::string::npos;
    }
    std::vector<uint8_t> f(92, 0);
    int32_t lfHeight = -((pt * 96 + 36) / 72);
    std::memcpy(&f[0], &lfHeight, 4);
    int32_t lfWeight = bold ? 700 : 400;
    std::memcpy(&f[16], &lfWeight, 4);
    f[20] = italic ? 1 : 0;
    f[23] = 1;  // DEFAULT_CHARSET
    std::wstring wface = widen(face);
    for (size_t i = 0; i < wface.size() && i < 31; ++i) {
        uint16_t c = (uint16_t)wface[i];
        f[28 + i * 2] = c & 0xFF;
        f[28 + i * 2 + 1] = (c >> 8) & 0xFF;
    }
    out.insert(out.end(), f.begin(), f.end());
}

// Parse "[App::Class.Part(State)]" forms.
struct Section {
    std::wstring outClass;   // full name for CMAP (app::class or class)
    std::wstring baseClass;  // class for schema lookup (after ::)
    std::string part;        // may be empty
    std::string state;       // may be empty
};

Section parseSection(const std::string& raw) {
    Section s;
    std::string body = raw;
    // strip [ ]
    if (!body.empty() && body.front() == '[')
        body = body.substr(1);
    if (!body.empty() && body.back() == ']')
        body.pop_back();

    // state in parens
    auto lp = body.find('(');
    if (lp != std::string::npos) {
        auto rp = body.find(')', lp);
        s.state = body.substr(lp + 1, (rp == std::string::npos ? body.size() : rp) - lp - 1);
        body = body.substr(0, lp);
    }
    // app::class split
    std::string classAndPart = body;
    auto cc = body.find("::");
    std::string appPrefix;
    if (cc != std::string::npos) {
        appPrefix = body.substr(0, cc);
        classAndPart = body.substr(cc + 2);
    }
    // class.part split (first dot in the class-and-part region)
    std::string cls = classAndPart, part;
    auto dot = classAndPart.find('.');
    if (dot != std::string::npos) {
        cls = classAndPart.substr(0, dot);
        part = classAndPart.substr(dot + 1);
    }
    s.baseClass = widen(cls);
    s.outClass = appPrefix.empty() ? widen(cls) : widen(appPrefix + "::" + cls);
    s.part = part;
    return s;
}

class IniImporter {
public:
    IniImporter(Model& m, fs::path bitmapsDir) : m_(m), bitmaps_(std::move(bitmapsDir)) {}

    IniImportStats run(std::istream& in) {
        std::string line;
        while (std::getline(in, line)) {
            std::string t = trim(line);
            if (t.empty() || t[0] == ';')
                continue;
            if (t.front() == '[') {
                beginSection(t);
                continue;
            }
            if (cur_)
                parseProperty(t);
        }
        st_.compile = writeModel(m_, outPath_);
        return st_;
    }

    void setOutPath(std::wstring p) { outPath_ = std::move(p); }

private:
    void beginSection(const std::string& raw) {
        Section s = parseSection(raw);
        ++st_.sections;
        cur_ = nullptr;
        curPart_ = curState_ = 0;

        // [Globals] / [SysMetrics] become classes "globals" / "sysmetrics".
        std::wstring lc = s.outClass;
        std::transform(lc.begin(), lc.end(), lc.begin(), ::towlower);
        if (lc == L"globals" || lc == L"sysmetrics") {
            cur_ = &ensureClass(lc);
            return;
        }

        int pid = 0, sid = 0;
        const wchar_t* partW = nullptr;
        std::wstring partWs, stateWs;
        if (!s.part.empty()) {
            partWs = widen(s.part);
            partW = partWs.c_str();
        }
        const wchar_t* stateW = nullptr;
        if (!s.state.empty()) {
            stateWs = widen(s.state);
            stateW = stateWs.c_str();
        }
        if (!MSSTYLES_LookupPartState(s.baseClass.c_str(), partW, stateW, &pid, &sid)) {
            ++st_.unmappedSections;
            return;
        }
        cur_ = &ensureClass(s.outClass);
        curPart_ = pid;
        curState_ = sid;
    }

    void parseProperty(const std::string& t) {
        auto eq = t.find('=');
        if (eq == std::string::npos)
            return;
        std::string name = trim(t.substr(0, eq));
        std::string val = trim(t.substr(eq + 1));
        std::wstring nameW = widen(name);

        int prim = 0, propId = 0;
        if (!MSSTYLES_LookupProperty(nameW.c_str(), &prim, &propId)) {
            ++st_.unmappedProps;
            return;
        }
        Rec rec;
        rec.symbolVal = propId;
        rec.primitiveType = prim;
        rec.partId = curPart_;
        rec.stateId = curState_;
        if (!buildValue(rec, prim, propId, val)) {
            ++st_.unmappedProps;
            return;
        }
        cur_->recs.push_back(std::move(rec));
    }

    bool buildValue(Rec& rec, int prim, int propId, const std::string& val) {
        switch (prim) {
        case TMT_ENUM: {
            int e = 0;
            std::wstring vW = widen(trim(val));
            if (!MSSTYLES_LookupEnum(vW.c_str(), propId, &e))
                return false;
            putI32(rec.inlineBytes, e);
            return true;
        }
        case TMT_INT:
        case TMT_SIZE: {
            auto v = nums(val);
            putI32(rec.inlineBytes, v.empty() ? 0 : v[0]);
            return true;
        }
        case TMT_BOOL: {
            std::string u = upper(trim(val));
            putI32(rec.inlineBytes, (u == "TRUE" || u == "1") ? 1 : 0);
            return true;
        }
        case TMT_COLOR: {
            auto v = nums(val);
            if (v.size() < 3)
                return false;
            uint32_t cr = (v[0] & 0xFF) | ((v[1] & 0xFF) << 8) | ((v[2] & 0xFF) << 16);
            putU32(rec.inlineBytes, cr);
            return true;
        }
        case TMT_MARGINS:
        case TMT_RECT: {
            auto v = nums(val);
            if (v.size() < 4)
                return false;
            for (int i = 0; i < 4; ++i)
                putI32(rec.inlineBytes, v[i]);
            return true;
        }
        case TMT_POSITION: {
            auto v = nums(val);
            if (v.size() < 2)
                return false;
            putI32(rec.inlineBytes, v[0]);
            putI32(rec.inlineBytes, v[1]);
            return true;
        }
        case TMT_FONT:
            buildLogFont(rec.inlineBytes, trim(val));
            return true;
        case TMT_INTLIST: {
            auto v = nums(val);
            for (int x : v)
                putI32(rec.inlineBytes, x);
            return true;
        }
        case TMT_FILENAME: {
            uint32_t id = embedBitmap(val);
            if (!id)
                return false;
            rec.resId = id;
            return true;
        }
        case TMT_STRING: {
            std::wstring w = widen(trim(val));
            for (wchar_t c : w)
                putU16(rec.inlineBytes, (uint16_t)c);
            putU16(rec.inlineBytes, 0);
            return true;
        }
        default:
            return false;
        }
    }

    // "Normal\button.bmp" -> bitmaps/NORMAL_BUTTON.bmp
    uint32_t embedBitmap(const std::string& ref) {
        std::string key = upper(trim(ref));
        std::replace(key.begin(), key.end(), '\\', '_');
        std::replace(key.begin(), key.end(), '/', '_');
        auto it = idByFile_.find(key);
        if (it != idByFile_.end())
            return it->second;

        // strip extension to match against directory entries
        fs::path target = bitmaps_ / widen(key);
        std::vector<uint8_t> bytes;
        if (fs::exists(target))
            bytes = readFile(target);
        if (bytes.empty()) {
            ++st_.missingImages;
            return 0;
        }
        uint32_t id = nextId_++;
        m_.images[id] = std::move(bytes);
        idByFile_[key] = id;
        return id;
    }

    static std::vector<uint8_t> readFile(const fs::path& p) {
        std::ifstream f(p, std::ios::binary);
        if (!f)
            return {};
        return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),
                                    std::istreambuf_iterator<char>());
    }

    CClass& ensureClass(const std::wstring& fullName) {
        for (auto& c : m_.classes)
            if (c.fullName == fullName)
                return c;
        // XP styles don't declare a base class, so leave it empty.
        m_.classes.push_back(CClass{fullName, L"", {}});
        return m_.classes.back();
    }

    Model& m_;
    fs::path bitmaps_;
    std::wstring outPath_;
    CClass* cur_ = nullptr;
    int curPart_ = 0, curState_ = 0;
    uint32_t nextId_ = 1;
    std::map<std::string, uint32_t> idByFile_;
    IniImportStats st_;
};

} // namespace

IniImportStats importXpTheme(const std::wstring& iniPath, const std::wstring& bitmapsDir,
                             const std::wstring& outPath) {
    std::ifstream in(fs::path(iniPath), std::ios::binary);
    if (!in)
        throw CompileError("cannot open INI file");

    Model m;
    m.version = 4;
    m.variantName = L"Normal";
    m.colorName = L"NormalColor";
    m.sizeName = L"NormalSize";

    IniImporter imp(m, fs::path(bitmapsDir));
    imp.setOutPath(outPath);
    return imp.run(in);
}

} // namespace vcompile
