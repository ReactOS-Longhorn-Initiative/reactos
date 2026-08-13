// Primitives.h - low-level on-disk structures of the v4 .msstyles format.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace vtheme {

// The 32-byte property record - the atom of the v4 format.
// Mirrors the original _VSRECORD (see CLAUDE.md 2.2).
#pragma pack(push, 1)
struct VSRecord {
    int32_t  symbolVal;   // TMT_* property id
    int32_t  type;        // TMT_* primitive type id
    int32_t  classId;     // index into CMAP class list (-1 = none)
    int32_t  partId;
    int32_t  stateId;
    uint32_t resId;       // !=0 -> value lives in IMAGE/STREAM/string resource; 0 -> inline
    int32_t  reserved;
    int32_t  byteLength;  // length of inline data when resId==0
};
#pragma pack(pop)
static_assert(sizeof(VSRecord) == 32, "VSRecord must be 32 bytes");

// COLORREF-style 0x00BBGGRR value.
struct Color {
    uint8_t r = 0, g = 0, b = 0;
    static Color fromColorRef(uint32_t v) {
        return Color{ static_cast<uint8_t>(v & 0xFF),
                      static_cast<uint8_t>((v >> 8) & 0xFF),
                      static_cast<uint8_t>((v >> 16) & 0xFF) };
    }
    uint32_t toColorRef() const {
        return static_cast<uint32_t>(r) | (static_cast<uint32_t>(g) << 8) |
               (static_cast<uint32_t>(b) << 16);
    }
};

struct Margins { int32_t left = 0, right = 0, top = 0, bottom = 0; };
struct RectVal { int32_t left = 0, top = 0, right = 0, bottom = 0; };
struct Position { int32_t x = 0, y = 0; };

struct FontSpec {
    std::wstring face;
    int32_t pointSize = 0;
    std::wstring options;  // e.g. "bold italic"
};

struct IntList { std::vector<int32_t> values; };

// Image / stream payload referenced via VSRecord::resId.
struct ResourceRef {
    uint32_t resId = 0;
    std::wstring resType;        // "IMAGE", "STREAM", ...
    std::vector<uint8_t> data;   // raw resource bytes (BMP/PNG/binary)
};

// Simplified-image authoring info (TMT_SIMPLIFIEDIMAGE).
struct ImageProperties { uint32_t borderColor = 0; uint32_t backgroundColor = 0; };

} // namespace vtheme
