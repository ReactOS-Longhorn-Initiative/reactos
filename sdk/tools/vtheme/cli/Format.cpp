#include "Format.h"
#include "Util.h"

#include <cstdio>
#include <vtheme/Tmt.h>

using namespace vtheme;

namespace cli {

static std::string colorHex(const Color& c) {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "#%02X%02X%02X", c.r, c.g, c.b);
    return buf;
}

static std::string quote(const std::wstring& w) {
    std::string s = toUtf8(w);
    std::string out = "\"";
    for (char ch : s) {
        if (ch == '"' || ch == '\\')
            out.push_back('\\');
        out.push_back(ch);
    }
    out.push_back('"');
    return out;
}

std::string formatValue(const ThemeProperty& prop) {
    const Value& v = prop.value;
    switch (v.kind) {
    case Tpid::Enum:
        if (auto p = v.get<int32_t>())
            return enumValueName(prop.propertyId, *p);
        return "0";
    case Tpid::Int:
    case Tpid::Size:
    case Tpid::HighContrastColorType:
        if (auto p = v.get<int32_t>())
            return std::to_string(*p);
        return "0";
    case Tpid::Bool:
        if (auto p = v.get<bool>())
            return *p ? "true" : "false";
        return "false";
    case Tpid::Color:
        if (auto p = v.get<Color>())
            return colorHex(*p);
        return "#000000";
    case Tpid::Float:
        if (auto p = v.get<float>()) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%g", *p);
            return buf;
        }
        return "0";
    case Tpid::String:
    case Tpid::Filename:
        if (auto p = v.get<std::wstring>())
            return quote(*p);
        return "\"\"";
    case Tpid::Margins:
        if (auto p = v.get<Margins>()) {
            char b[64];
            std::snprintf(b, sizeof(b), "%d, %d, %d, %d", p->left, p->right, p->top, p->bottom);
            return b;
        }
        return "0, 0, 0, 0";
    case Tpid::Rect:
        if (auto p = v.get<RectVal>()) {
            char b[64];
            std::snprintf(b, sizeof(b), "%d, %d, %d, %d", p->left, p->top, p->right, p->bottom);
            return b;
        }
        return "0, 0, 0, 0";
    case Tpid::Position:
        if (auto p = v.get<Position>()) {
            char b[32];
            std::snprintf(b, sizeof(b), "%d, %d", p->x, p->y);
            return b;
        }
        return "0, 0";
    case Tpid::Font:
        if (auto p = v.get<FontSpec>()) {
            std::string s = toUtf8(p->face) + ", " + std::to_string(p->pointSize);
            if (!p->options.empty())
                s += ", " + toUtf8(p->options);
            return "\"" + s + "\"";
        }
        return "\"\"";
    case Tpid::IntList:
    case Tpid::FloatList:
        if (auto p = v.get<IntList>()) {
            std::string s = "[";
            for (size_t i = 0; i < p->values.size(); ++i) {
                if (i)
                    s += " ";
                s += std::to_string(p->values[i]);
            }
            s += "]";
            return s;
        }
        return "[]";
    case Tpid::SimplifiedImage:
        if (auto p = v.get<ImageProperties>()) {
            char b[64];
            std::snprintf(b, sizeof(b), "border=#%06X bg=#%06X", p->borderColor & 0xFFFFFF,
                          p->backgroundColor & 0xFFFFFF);
            return b;
        }
        return "{}";
    default:
        if (auto p = v.get<ResourceRef>()) {
            std::string tag = p->resType == L"STREAM" ? "@stream:" : "@image:";
            return tag + std::to_string(p->resId);
        }
        if (auto p = v.get<std::vector<uint8_t>>()) {
            std::string s = "<raw " + std::to_string(p->size()) + " bytes>";
            return s;
        }
        return "<?>";
    }
}

std::string primitiveTag(const ThemeProperty& prop) {
    return tmtName(prop.primitiveType);
}

} // namespace cli
