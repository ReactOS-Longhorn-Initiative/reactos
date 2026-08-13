// Value.h - the decoded value of a theme property.
#pragma once
#include "Primitives.h"
#include <variant>

namespace vtheme {

// Primitive kind (TPID) - which loader produced the value. Mirrors the
// THEMEPRIMITIVEID table in UxThemeEx VSUnpack.cpp.
enum class Tpid {
    Invalid = -1,
    BitmapImage = 0, BitmapImage1, BitmapImage2, BitmapImage3, BitmapImage4,
    BitmapImage5, BitmapImage6, BitmapImage7, StockBitmapImage, GlyphImage,
    AtlasInputImage, AtlasImage, Enum, String, Int, Bool, Color, Margins,
    Filename, Size, Position, Rect, Font, IntList, DiskStream, Stream,
    Animation, TimingFunction, SimplifiedImage, HighContrastColorType,
    BitmapImageType, ComposedImageType, Float, FloatList,
};

using ValueData = std::variant<
    std::monostate,
    int32_t,             // Int / Size / Enum
    bool,                // Bool
    Color,               // Color
    float,               // Float
    std::wstring,        // String / Filename (text)
    Margins,             // Margins
    RectVal,             // Rect
    Position,            // Position
    FontSpec,            // Font
    IntList,             // IntList / FloatList(as ints)
    ResourceRef,         // image / stream payloads
    ImageProperties,     // simplified-image authoring data
    std::vector<uint8_t> // raw fallback
>;

struct Value {
    Tpid kind = Tpid::Invalid;
    ValueData data;

    bool isNone() const { return std::holds_alternative<std::monostate>(data); }

    template <class T> const T* get() const { return std::get_if<T>(&data); }
    template <class T> T* get() { return std::get_if<T>(&data); }
};

} // namespace vtheme
