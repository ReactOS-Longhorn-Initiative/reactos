#include "vtheme/ThemeLoader.h"
#include "vtheme/PeModule.h"
#include "vtheme/Tmt.h"

#include <array>
#include <cstring>
#include <filesystem>

namespace vtheme {
namespace {

// ---- primitive-type table (THEMEPRIMITIVEID / PROPERTYMAP) -----------------
struct PropMap {
    Tpid kind;
    int symbolVal;  // 0 => match on type alone
    int typeId;     // VSRecord::type to match
    int byteLen;    // fixed payload size, -1 = variable
};

constexpr PropMap kPropMap[] = {
    {Tpid::BitmapImage,           0xBB9, 206, 16},
    {Tpid::BitmapImage1,          0xBBA, 206, 16},
    {Tpid::BitmapImage2,          0xBBB, 206, 16},
    {Tpid::BitmapImage3,          0xBBC, 206, 16},
    {Tpid::BitmapImage4,          0xBBD, 206, 16},
    {Tpid::BitmapImage5,          0xBBE, 206, 16},
    {Tpid::BitmapImage6,          0xBC1, 206, 16},
    {Tpid::BitmapImage7,          0xBC2, 206, 16},
    {Tpid::StockBitmapImage,      0xBBF, 206, 16},
    {Tpid::GlyphImage,            0xBC0, 206, 16},
    {Tpid::AtlasImage,            0x1F40, 213, -1},
    {Tpid::AtlasInputImage,       0x1F41, 201, 16},
    {Tpid::Enum,                  0, 200, 4},
    {Tpid::String,                0, 201, -1},
    {Tpid::Int,                   0, 202, 4},
    {Tpid::Bool,                  0, 203, 4},
    {Tpid::Color,                 0, 204, 4},
    {Tpid::Margins,               0, 205, 16},
    {Tpid::Filename,              0, 206, -1},
    {Tpid::Size,                  0, 207, 4},
    {Tpid::Position,              0, 208, 8},
    {Tpid::Rect,                  0, 209, 16},
    {Tpid::Font,                  0, 210, 0x5C},
    {Tpid::IntList,               0, 211, -1},
    {Tpid::DiskStream,            0, 213, -1},
    {Tpid::Stream,                0, 214, -1},
    {Tpid::Animation,             0x4E20, 0xF1, -1},
    {Tpid::TimingFunction,        0x4E84, 0xF2, -1},
    {Tpid::SimplifiedImage,       0, 0xF0, -1},
    {Tpid::HighContrastColorType, 0, 0xF1, 4},
    {Tpid::BitmapImageType,       0, 0xF2, 16},
    {Tpid::ComposedImageType,     0, 0xF3, 16},
    {Tpid::Float,                 0, 216, 4},
    {Tpid::FloatList,             0, 217, -1},
};

Tpid resolveTpid(int symbolVal, int type) {
    Tpid id = Tpid::Invalid;
    for (const auto& m : kPropMap) {
        if (m.typeId == type) {
            id = m.kind;
            if (m.symbolVal != 0 && m.symbolVal == symbolVal)
                break;
        }
    }
    return id;
}

bool isImageFileProp(int sym) {
    switch (sym) {
    case TMT_IMAGEFILE: case TMT_IMAGEFILE1: case TMT_IMAGEFILE2: case TMT_IMAGEFILE3:
    case TMT_IMAGEFILE4: case TMT_IMAGEFILE5: case TMT_IMAGEFILE6: case TMT_IMAGEFILE7:
    case TMT_GLYPHIMAGEFILE:
    case TMT_COMPOSEDIMAGEFILE: case TMT_COMPOSEDIMAGEFILE1: case TMT_COMPOSEDIMAGEFILE2:
    case TMT_COMPOSEDIMAGEFILE3: case TMT_COMPOSEDIMAGEFILE4: case TMT_COMPOSEDIMAGEFILE5:
    case TMT_COMPOSEDIMAGEFILE6: case TMT_COMPOSEDIMAGEFILE7:
        return true;
    default:
        return false;
    }
}

// ---- little-endian byte cursor --------------------------------------------
class Cursor {
public:
    Cursor(const uint8_t* p, size_t n) : p_(p), n_(n) {}
    size_t pos() const { return i_; }
    size_t size() const { return n_; }
    bool eof() const { return i_ >= n_; }
    void seek(size_t i) { i_ = i; }

    void alignTo(size_t a) {
        size_t r = i_ % a;
        if (r)
            i_ += a - r;
    }
    uint32_t u32() {
        uint32_t v = 0;
        if (i_ + 4 <= n_)
            std::memcpy(&v, p_ + i_, 4);
        i_ += 4;
        return v;
    }
    int32_t i32() { return static_cast<int32_t>(u32()); }
    uint16_t u16() {
        uint16_t v = 0;
        if (i_ + 2 <= n_)
            std::memcpy(&v, p_ + i_, 2);
        i_ += 2;
        return v;
    }
    std::wstring zstringW() {
        std::wstring s;
        for (;;) {
            uint16_t c = u16();
            if (c == 0 || i_ > n_)
                break;
            s.push_back(static_cast<wchar_t>(c));
        }
        return s;
    }
    std::wstring pascalZStringW() {
        uint32_t count = u32();  // chars including terminator
        std::wstring s;
        for (uint32_t k = 0; k + 1 < count; ++k)
            s.push_back(static_cast<wchar_t>(u16()));
        if (count > 0)
            u16();  // terminator
        return s;
    }
    const uint8_t* at(size_t off) const { return p_ + off; }

private:
    const uint8_t* p_;
    size_t n_;
    size_t i_ = 0;
};

std::wstring deriveMuiPath(const std::wstring& path) {
    namespace fs = std::filesystem;
    fs::path p(path);
    fs::path dir = p.has_parent_path() ? p.parent_path() : fs::path(L".");
    return (dir / L"en-US" / (p.filename().wstring() + L".mui")).wstring();
}

FontSpec parseFontString(const std::wstring& s) {
    // "Segoe UI, 9, " or "Segoe UI, 9, bold"
    FontSpec f;
    size_t a = s.find(L", ");
    if (a == std::wstring::npos) {
        f.face = s;
        return f;
    }
    f.face = s.substr(0, a);
    size_t b = s.find(L", ", a + 2);
    std::wstring pt = (b == std::wstring::npos) ? s.substr(a + 2) : s.substr(a + 2, b - (a + 2));
    try { f.pointSize = std::stoi(pt); } catch (...) {}
    if (b != std::wstring::npos)
        f.options = s.substr(b + 2);
    return f;
}

FontSpec parseLogFont(const uint8_t* p, size_t len) {
    FontSpec f;
    if (len < 0x5C)
        return f;
    int32_t lfHeight;
    std::memcpy(&lfHeight, p, 4);
    int32_t lfWeight;
    std::memcpy(&lfWeight, p + 16, 4);
    uint8_t lfItalic = p[20];
    // lfFaceName: 32 wchars at offset 28
    const auto* face = reinterpret_cast<const wchar_t*>(p + 28);
    size_t maxc = (len - 28) / 2;
    for (size_t k = 0; k < 32 && k < maxc && face[k]; ++k)
        f.face.push_back(face[k]);
    // pointSize = -MulDiv(lfHeight, 72, 96) (inverse of authoring formula at 96dpi)
    if (lfHeight < 0)
        f.pointSize = (-lfHeight * 72 + 48) / 96;
    if (lfWeight >= 700)
        f.options = L"bold";
    if (lfItalic) {
        if (!f.options.empty())
            f.options += L" ";
        f.options += L"italic";
    }
    return f;
}

bool parseRectString(const std::wstring& s, RectVal& r) {
    return std::swscanf(s.c_str(), L"%d, %d, %d, %d", &r.left, &r.top, &r.right, &r.bottom) == 4;
}

// ---- loader ----------------------------------------------------------------
class Loader {
public:
    Loader(const PeModule& style, const PeModule* mui, bool hc)
        : style_(style), mui_(mui), hc_(hc) {}

    void loadInto(ThemeFile& tf, const std::wstring& variantName) {
        readClassMap(tf);
        readVariantMap(tf);
        auto bcmap = readBaseClassMap();

        readProperties(tf, L"RMAP", L"RMAP", tf.rootProperties, true);

        std::wstring vName = variantName.empty() ? tf.variant.name : variantName;
        readClassProperties(tf, vName);

        applyBaseClasses(tf, bcmap);
        tf.sort();
    }

private:
    void readClassMap(ThemeFile& tf) {
        auto bytes = style_.resource(L"CMAP", L"CMAP");
        if (!bytes)
            throw ThemeError("CMAP resource not found (not a v4 .msstyles?)");
        Cursor c(bytes->data(), bytes->size());
        while (!c.eof()) {
            tf.classNames.push_back(c.zstringW());
            c.alignTo(8);
        }
    }

    void readVariantMap(ThemeFile& tf) {
        auto bytes = style_.resource(L"VMAP", L"VMAP");
        if (!bytes)
            throw ThemeError("VMAP resource not found");
        Cursor c(bytes->data(), bytes->size());
        c.alignTo(4);
        tf.variant.name = c.pascalZStringW();
        c.alignTo(4);
        tf.variant.size = c.pascalZStringW();
        c.alignTo(4);
        tf.variant.color = c.pascalZStringW();
    }

    std::vector<std::pair<int, int>> readBaseClassMap() {
        std::vector<std::pair<int, int>> out;
        auto bytes = style_.resource(L"BCMAP", L"BCMAP");
        if (!bytes)
            return out;
        Cursor c(bytes->data(), bytes->size());
        int count = c.i32();
        for (int i = 0; i < count; ++i) {
            int base = c.i32();
            if (base != -1)
                out.emplace_back(i, base);
        }
        return out;
    }

    void applyBaseClasses(ThemeFile& tf, const std::vector<std::pair<int, int>>& bcmap) {
        if (tf.classNames.size() <= 4)
            return;
        // BCMAP indices are relative to the class list after the first 4 entries.
        const size_t off = 4;
        for (auto [cls, base] : bcmap) {
            size_t ci = off + cls, bi = off + base;
            if (ci >= tf.classNames.size() || bi >= tf.classNames.size())
                continue;
            const std::wstring& cname = tf.classNames[ci];
            const std::wstring& bname = tf.classNames[bi];
            std::wstring app, plain = cname;
            auto colon = cname.find(L"::");
            if (colon != std::wstring::npos) {
                app = cname.substr(0, colon);
                plain = cname.substr(colon + 2);
            }
            if (auto* c = tf.findClass(app, plain))
                c->baseClassName = bname;
        }
    }

    void readClassProperties(ThemeFile& tf, const std::wstring& variantName) {
        std::vector<ThemeProperty> dummy;
        readProperties(tf, L"VARIANT", variantName.c_str(), dummy, false);
    }

    // Walk a VSRecord stream from resource (type,name).
    void readProperties(ThemeFile& tf, const wchar_t* type, const wchar_t* name,
                        std::vector<ThemeProperty>& rootSink, bool isRoot) {
        auto bytes = style_.resource(type, name);
        if (!bytes)
            return;
        Cursor c(bytes->data(), bytes->size());
        int recordIndex = 0;
        while (c.pos() + sizeof(VSRecord) <= c.size()) {
            VSRecord rec;
            std::memcpy(&rec, c.at(c.pos()), sizeof(rec));
            c.seek(c.pos() + sizeof(rec));
            size_t valueOffset = c.pos();

            Value val = decodeValue(rec, c.at(0), c.size(), valueOffset);

            ThemeProperty prop;
            prop.recordIndex = recordIndex++;
            prop.partId = rec.partId;
            prop.stateId = rec.stateId;
            prop.propertyId = rec.symbolVal;
            prop.primitiveType = rec.type;
            prop.value = std::move(val);

            if (isRoot) {
                rootSink.push_back(std::move(prop));
            } else {
                placeProperty(tf, rec.classId, std::move(prop));
            }

            if (rec.resId == 0)
                c.seek(c.pos() + rec.byteLength);
            c.alignTo(8);
        }
    }

    void placeProperty(ThemeFile& tf, int classId, ThemeProperty&& prop) {
        if (classId < 0 || static_cast<size_t>(classId) >= tf.classNames.size())
            return;
        ThemeClass& cls = tf.ensureClass(tf.classNames[classId]);
        if (prop.partId == 0) {
            cls.properties.push_back(std::move(prop));
        } else {
            ThemePart& part = cls.ensurePart(prop.partId);
            if (prop.stateId == 0)
                part.properties.push_back(std::move(prop));
            else
                part.ensureState(prop.stateId).properties.push_back(std::move(prop));
        }
    }

    Value decodeValue(const VSRecord& rec, const uint8_t* base, size_t total, size_t off) {
        Value v;
        Tpid id = resolveTpid(rec.symbolVal, rec.type);
        v.kind = id;
        if (id == Tpid::Invalid)
            return v;

        if (rec.resId == 0)
            decodeInline(rec, id, base, total, off, v);
        else
            decodeResource(rec, id, v);
        return v;
    }

    void decodeInline(const VSRecord& rec, Tpid id, const uint8_t* base, size_t total,
                      size_t off, Value& v) {
        auto avail = [&](size_t need) { return off + need <= total; };
        const uint8_t* p = base + off;
        switch (id) {
        case Tpid::String:
        case Tpid::Filename: {
            Cursor sc(base, total);
            sc.seek(off);
            v.data = sc.zstringW();
            break;
        }
        case Tpid::Enum:
        case Tpid::Int:
        case Tpid::Size:
        case Tpid::HighContrastColorType:
            if (avail(4)) {
                int32_t x;
                std::memcpy(&x, p, 4);
                v.data = x;
            }
            break;
        case Tpid::Bool:
            if (avail(4)) {
                int32_t x;
                std::memcpy(&x, p, 4);
                v.data = (x == 1);
            }
            break;
        case Tpid::Color:
            if (avail(4)) {
                uint32_t x;
                std::memcpy(&x, p, 4);
                v.data = Color::fromColorRef(x);
            }
            break;
        case Tpid::Float:
            if (avail(4)) {
                float x;
                std::memcpy(&x, p, 4);
                v.data = x;
            }
            break;
        case Tpid::Margins:
            if (avail(16)) {
                Margins m;
                std::memcpy(&m, p, 16);
                v.data = m;
            }
            break;
        case Tpid::Rect:
            if (avail(16)) {
                RectVal r;
                std::memcpy(&r, p, 16);
                v.data = r;
            }
            break;
        case Tpid::Position:
            if (avail(8)) {
                Position pt;
                std::memcpy(&pt, p, 8);
                v.data = pt;
            }
            break;
        case Tpid::Font:
            v.data = parseLogFont(p, rec.byteLength);
            break;
        case Tpid::IntList:
        case Tpid::FloatList: {
            IntList list;
            int n = rec.byteLength / 4;
            list.values.resize(n);
            for (int k = 0; k < n && avail((k + 1) * 4); ++k)
                std::memcpy(&list.values[k], p + k * 4, 4);
            v.data = std::move(list);
            break;
        }
        case Tpid::SimplifiedImage: {
            if (rec.symbolVal != TMT_HCSIMPLIFIEDIMAGE && avail(8)) {
                ImageProperties ip;
                std::memcpy(&ip.borderColor, p, 4);
                std::memcpy(&ip.backgroundColor, p + 4, 4);
                v.data = ip;
            } else {
                v.data = std::vector<uint8_t>(p, p + std::min<size_t>(rec.byteLength, total - off));
            }
            break;
        }
        default:
            // unknown / composed-image-type / etc - keep raw
            if (rec.byteLength > 0 && avail(rec.byteLength))
                v.data = std::vector<uint8_t>(p, p + rec.byteLength);
            break;
        }
    }

    void decodeResource(const VSRecord& rec, Tpid id, Value& v) {
        switch (id) {
        case Tpid::BitmapImage: case Tpid::BitmapImage1: case Tpid::BitmapImage2:
        case Tpid::BitmapImage3: case Tpid::BitmapImage4: case Tpid::BitmapImage5:
        case Tpid::BitmapImage6: case Tpid::BitmapImage7: case Tpid::StockBitmapImage:
        case Tpid::GlyphImage: case Tpid::ComposedImageType: case Tpid::BitmapImageType:
            loadResImage(rec.resId, v);
            break;
        case Tpid::Filename:
            if (isImageFileProp(rec.symbolVal))
                loadResImage(rec.resId, v);
            else
                loadMuiString(rec.resId, v);
            break;
        case Tpid::String:
            loadMuiString(rec.resId, v);
            break;
        case Tpid::Bool:
            if (mui_) {
                if (auto s = mui_->loadString(rec.resId)) {
                    v.data = (*s == L"1" || _wcsicmp(s->c_str(), L"true") == 0);
                    return;
                }
            }
            break;
        case Tpid::Rect:
            if (mui_) {
                if (auto s = mui_->loadString(rec.resId)) {
                    RectVal r;
                    if (parseRectString(*s, r))
                        v.data = r;
                    return;
                }
            }
            break;
        case Tpid::Font:
            if (mui_) {
                if (auto s = mui_->loadString(rec.resId))
                    v.data = parseFontString(*s);
            }
            break;
        case Tpid::DiskStream:
        case Tpid::Stream: {
            if (auto bytes = style_.resource(L"STREAM", rec.resId)) {
                v.data = ResourceRef{rec.resId, L"STREAM", std::move(*bytes)};
            } else {
                v.data = ResourceRef{rec.resId, L"STREAM", {}};
            }
            break;
        }
        default:
            v.data = ResourceRef{rec.resId, L"", {}};
            break;
        }
    }

    void loadResImage(uint32_t resId, Value& v) {
        ResourceRef ref{resId, L"IMAGE", {}};
        if (auto bytes = style_.resource(L"IMAGE", resId))
            ref.data = std::move(*bytes);
        v.data = std::move(ref);
    }

    void loadMuiString(uint32_t resId, Value& v) {
        if (mui_) {
            if (auto s = mui_->loadString(resId)) {
                v.data = *s;
                return;
            }
        }
        v.data = std::wstring(L"<<MUI missing>>");
    }

    const PeModule& style_;
    const PeModule* mui_;
    bool hc_;
};

} // namespace

std::unique_ptr<ThemeFile> loadTheme(const std::wstring& path, const LoadOptions& opts) {
    auto style = PeModule::load(path);
    if (!style)
        throw ThemeError("failed to open style file as a PE module");

    auto ver = style->packthemVersion();
    int version = ver.value_or(0);
    if (version != 4) {
        // Still attempt to parse - but the binary VARIANT tables only exist for v4.
        if (version == 3)
            throw ThemeError("this is a v3 (XP/2003) .msstyles; only v4 (Vista+) is supported");
    }

    std::wstring muiPath = opts.muiPath.empty() ? deriveMuiPath(path) : opts.muiPath;
    std::optional<PeModule> mui;
    if (std::filesystem::exists(muiPath))
        mui = PeModule::load(muiPath);

    auto tf = std::make_unique<ThemeFile>();
    tf->path = path;
    tf->muiPath = mui ? muiPath : L"";
    tf->version = version;

    Loader loader(*style, mui ? &*mui : nullptr, opts.highContrast);
    loader.loadInto(*tf, opts.variantName);
    return tf;
}

} // namespace vtheme
