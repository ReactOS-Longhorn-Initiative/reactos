#include "Builder.h"
#include "vcompile/Compiler.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <vtheme/Tmt.h>
#include <windows.h>

namespace fs = std::filesystem;

namespace vcompile {

namespace {

std::wstring widen(const std::string& s) {
    if (s.empty())
        return {};
    int n = ::MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos)
        return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::string unquote(const std::string& s) {
    std::string t = trim(s);
    if (t.size() >= 2 && t.front() == '"' && t.back() == '"') {
        std::string out;
        for (size_t i = 1; i + 1 < t.size(); ++i) {
            if (t[i] == '\\' && i + 2 < t.size())
                ++i;
            out.push_back(t[i]);
        }
        return out;
    }
    return t;
}

std::vector<int> parseInts(const std::string& s) {
    std::vector<int> out;
    std::string cur;
    for (char c : s) {
        if (c == ',' || c == ' ' || c == '[' || c == ']' || c == '\t') {
            if (!cur.empty()) {
                try { out.push_back(std::stoi(cur)); } catch (...) {}
                cur.clear();
            }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty())
        try { out.push_back(std::stoi(cur)); } catch (...) {}
    return out;
}

bool parseColor(const std::string& s, uint32_t& colorref) {
    std::string t = trim(s);
    if (t.empty() || t[0] != '#')
        return false;
    unsigned r, g, b;
    if (std::sscanf(t.c_str(), "#%02x%02x%02x", &r, &g, &b) != 3)
        return false;
    colorref = (r & 0xFF) | ((g & 0xFF) << 8) | ((b & 0xFF) << 16);  // COLORREF 0x00BBGGRR
    return true;
}

void buildLogFont(std::vector<uint8_t>& out, const std::string& spec) {
    // "Face, pt, options"
    std::string face = spec;
    int pt = 9;
    bool bold = false, italic = false;
    auto a = spec.find(", ");
    if (a != std::string::npos) {
        face = spec.substr(0, a);
        auto b = spec.find(", ", a + 2);
        std::string ptStr = (b == std::string::npos) ? spec.substr(a + 2) : spec.substr(a + 2, b - (a + 2));
        try { pt = std::stoi(trim(ptStr)); } catch (...) {}
        if (b != std::string::npos) {
            std::string opt = spec.substr(b + 2);
            bold = opt.find("bold") != std::string::npos;
            italic = opt.find("italic") != std::string::npos;
        }
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

} // namespace

class Parser {
public:
    Parser(Model& m, fs::path srcDir) : m_(m), srcDir_(std::move(srcDir)) {}

    void parse(std::istream& in) {
        std::string line;
        while (std::getline(in, line)) {
            std::string t = trim(line);
            if (t.empty() || t[0] == '#')
                continue;
            if (t == "}") {
                if (!ctx_.empty()) {
                    std::string top = ctx_.back();
                    ctx_.pop_back();
                    if (top == "STATE") {
                        state_ = false;
                        stateId_ = 0;
                    } else if (top == "PART") {
                        part_ = false;
                        partId_ = 0;
                        state_ = false;
                        stateId_ = 0;
                    } else if (top == "CLASS") {
                        curClass_ = nullptr;
                        part_ = state_ = false;
                    }
                }
                continue;
            }
            if (t.rfind("THEME", 0) == 0 && t.back() == '{') {
                ctx_.push_back("THEME");
                continue;
            }
            if (t.rfind("ROOT", 0) == 0 && t.back() == '{') {
                ctx_.push_back("ROOT");
                curClass_ = nullptr;
                continue;
            }
            if (t.rfind("CLASS ", 0) == 0 && t.back() == '{') {
                beginClass(t);
                ctx_.push_back("CLASS");
                continue;
            }
            if (t.rfind("PART ", 0) == 0 && t.back() == '{') {
                beginPart(t);
                ctx_.push_back("PART");
                continue;
            }
            if (t.rfind("STATE ", 0) == 0 && t.back() == '{') {
                beginState(t);
                ctx_.push_back("STATE");
                continue;
            }
            // key/value lines
            if (!ctx_.empty() && ctx_.back() == "THEME") {
                parseThemeKV(t);
            } else {
                parseProperty(t);
            }
        }
    }

private:
    void beginClass(const std::string& t) {
        // CLASS "Name" [: "Base"] {
        std::string body = t.substr(6);
        body = body.substr(0, body.find_last_of('{'));
        std::string name, base;
        size_t q1 = body.find('"');
        size_t q2 = body.find('"', q1 + 1);
        if (q1 != std::string::npos && q2 != std::string::npos)
            name = body.substr(q1 + 1, q2 - q1 - 1);
        size_t colon = body.find(':', q2);
        if (colon != std::string::npos) {
            size_t b1 = body.find('"', colon);
            size_t b2 = body.find('"', b1 + 1);
            if (b1 != std::string::npos && b2 != std::string::npos)
                base = body.substr(b1 + 1, b2 - b1 - 1);
        }
        m_.classes.push_back(CClass{widen(name), widen(base), {}});
        curClass_ = &m_.classes.back();
        part_ = state_ = false;
        partId_ = stateId_ = 0;
    }

    void beginPart(const std::string& t) {
        partId_ = atoiAfter(t, "PART ");
        part_ = true;
        state_ = false;
        stateId_ = 0;
    }
    void beginState(const std::string& t) {
        stateId_ = atoiAfter(t, "STATE ");
        state_ = true;
    }

    static int atoiAfter(const std::string& t, const char* kw) {
        std::string rest = trim(t.substr(std::strlen(kw)));
        return std::atoi(rest.c_str());
    }

    void parseThemeKV(const std::string& t) {
        auto eq = t.find('=');
        if (eq == std::string::npos)
            return;
        std::string key = trim(t.substr(0, eq));
        std::string val = trim(t.substr(eq + 1));
        if (key == "version")
            m_.version = std::atoi(val.c_str());
        else if (key == "variant")
            m_.variantName = widen(val);
        else if (key == "color")
            m_.colorName = widen(val);
        else if (key == "size")
            m_.sizeName = widen(val);
    }

    void parseProperty(const std::string& t) {
        // NAME (TYPE) = value
        auto lp = t.find('(');
        auto rp = t.find(')', lp);
        auto eq = t.find('=', rp);
        if (lp == std::string::npos || rp == std::string::npos || eq == std::string::npos)
            return;
        std::string name = trim(t.substr(0, lp));
        std::string type = trim(t.substr(lp + 1, rp - lp - 1));
        std::string val = trim(t.substr(eq + 1));

        int sym = vtheme::tmtFromName(name);
        int prim = vtheme::tmtFromName(type);
        if (sym < 0 || prim < 0) {
            ++m_.skipped;
            return;
        }

        Rec rec;
        rec.symbolVal = sym;
        rec.primitiveType = prim;
        rec.partId = part_ ? partId_ : 0;
        rec.stateId = state_ ? stateId_ : 0;

        if (!buildValue(rec, prim, sym, val)) {
            ++m_.skipped;
            return;
        }
        sink().push_back(std::move(rec));
    }

    std::vector<Rec>& sink() {
        if (!ctx_.empty() && ctx_.back() == "ROOT")
            return m_.rootRecs;
        if (curClass_)
            return curClass_->recs;
        return m_.rootRecs;
    }

    bool buildValue(Rec& rec, int prim, int sym, const std::string& val) {
        using namespace vtheme;
        switch (prim) {
        case TMT_ENUM: {
            int e = 0;
            if (!enumValueFromName(sym, trim(val), e))
                return false;
            putI32(rec.inlineBytes, e);
            return true;
        }
        case TMT_INT:
        case TMT_SIZE:
        case TMT_HCCOLOR:
            putI32(rec.inlineBytes, std::atoi(val.c_str()));
            return true;
        case TMT_BOOL: {
            bool b = (trim(val) == "true" || trim(val) == "1");
            putI32(rec.inlineBytes, b ? 1 : 0);
            return true;
        }
        case TMT_COLOR: {
            uint32_t c;
            if (!parseColor(val, c))
                return false;
            putU32(rec.inlineBytes, c);
            return true;
        }
        case TMT_FLOAT: {
            float f = std::strtof(val.c_str(), nullptr);
            uint32_t bits;
            std::memcpy(&bits, &f, 4);
            putU32(rec.inlineBytes, bits);
            return true;
        }
        case TMT_STRING: {
            putZStringW(rec.inlineBytes, widen(unquote(val)));
            return true;
        }
        case TMT_FILENAME: {
            std::string v = trim(val);
            if (v.rfind("@image:", 0) == 0) {
                uint32_t id = (uint32_t)std::atoi(v.c_str() + 7);
                if (loadImageById(id))
                    rec.resId = id;
                else
                    return false;
                return true;
            }
            if (v.rfind("@file:", 0) == 0) {
                std::string path = unquote(v.substr(6));
                uint32_t id = embedFile(widen(path));
                if (!id)
                    return false;
                rec.resId = id;
                return true;
            }
            putZStringW(rec.inlineBytes, widen(unquote(v)));
            return true;
        }
        case TMT_MARGINS:
        case TMT_RECT: {
            auto v = parseInts(val);
            if (v.size() < 4)
                return false;
            for (int i = 0; i < 4; ++i)
                putI32(rec.inlineBytes, v[i]);
            return true;
        }
        case TMT_POSITION: {
            auto v = parseInts(val);
            if (v.size() < 2)
                return false;
            putI32(rec.inlineBytes, v[0]);
            putI32(rec.inlineBytes, v[1]);
            return true;
        }
        case TMT_FONT: {
            buildLogFont(rec.inlineBytes, unquote(val));
            return true;
        }
        case TMT_INTLIST:
        case TMT_FLOATLIST: {
            auto v = parseInts(val);
            for (int x : v)
                putI32(rec.inlineBytes, x);
            return true;
        }
        // ReactOS: DISKSTREAM / STREAM.
        //
        // These used to fall through to the default and be counted as
        // `skipped`, so a recompiled Aero came out with exactly one record
        // missing.  That record is DWMWindow's `DISKSTREAM = @stream:850` -
        // the packed frame atlas, and the only source of artwork for the
        // entire DWMWindow class, whose parts carry an ATLASRECT each and no
        // IMAGEFILE of their own.  A style compiled without it has 58 DWM
        // parts describing sub-rectangles of an image that is not there.
        //
        // The decompiler already writes the payload out (main.cpp names it
        // images/<id>.bin); nothing was reading it back.
        case TMT_DISKSTREAM:
        case TMT_STREAM: {
            std::string v = trim(val);
            if (v.rfind("@stream:", 0) != 0)
                return false;
            uint32_t id = (uint32_t)std::atoi(v.c_str() + 8);
            if (!loadStreamById(id))
                return false;
            rec.resId = id;
            return true;
        }
        default:
            return false;  // unsupported primitive - skip
        }
    }

    bool loadStreamById(uint32_t id) {
        if (m_.streams.count(id))
            return true;
        fs::path p = srcDir_ / "images" / (std::to_wstring(id) + L".bin");
        if (!fs::exists(p))
            return false;
        auto bytes = readFile(p);
        if (bytes.empty())
            return false;
        m_.streams[id] = std::move(bytes);
        return true;
    }

    bool loadImageById(uint32_t id) {
        if (m_.images.count(id))
            return true;
        fs::path dir = srcDir_ / "images";
        if (!fs::exists(dir))
            return false;
        for (auto& e : fs::directory_iterator(dir)) {
            if (e.path().stem() == std::to_wstring(id)) {
                auto bytes = readFile(e.path());
                if (bytes.empty())
                    return false;
                m_.images[id] = std::move(bytes);
                return true;
            }
        }
        return false;
    }

    uint32_t embedFile(const std::wstring& path) {
        auto bytes = readFile(fs::path(path));
        if (bytes.empty())
            return 0;
        uint32_t id = nextId_++;
        m_.images[id] = std::move(bytes);
        return id;
    }

    static std::vector<uint8_t> readFile(const fs::path& p) {
        std::ifstream f(p, std::ios::binary);
        if (!f)
            return {};
        return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),
                                    std::istreambuf_iterator<char>());
    }

    Model& m_;
    fs::path srcDir_;
    std::vector<std::string> ctx_;
    CClass* curClass_ = nullptr;
    bool part_ = false, state_ = false;
    int partId_ = 0, stateId_ = 0;
    uint32_t nextId_ = 50000;
};

void parseSource(Model& m, const std::wstring& srcPath) {
    std::ifstream in(fs::path(srcPath), std::ios::binary);
    if (!in)
        throw CompileError("cannot open source file");
    fs::path dir = fs::path(srcPath).has_parent_path() ? fs::path(srcPath).parent_path() : ".";
    Parser p(m, dir);
    p.parse(in);
}

} // namespace vcompile
