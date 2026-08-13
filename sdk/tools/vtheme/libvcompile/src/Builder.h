// Builder.h - internal model produced by the parser and consumed by the packer.
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "vcompile/Compiler.h"

namespace vcompile {

// A single property ready to pack into a VSRECORD.
struct Rec {
    int symbolVal = 0;
    int primitiveType = 0;
    int partId = 0;
    int stateId = 0;
    uint32_t resId = 0;            // non-zero => image/stream resource reference
    std::vector<uint8_t> inlineBytes;  // payload when resId == 0
};

struct CClass {
    std::wstring fullName;
    std::wstring baseName;
    std::vector<Rec> recs;  // each carries its own partId/stateId
};

struct Model {
    std::wstring variantName = L"Normal";
    std::wstring colorName = L"NormalColor";
    std::wstring sizeName = L"NormalSize";
    int version = 4;
    std::vector<Rec> rootRecs;
    std::vector<CClass> classes;
    std::map<uint32_t, std::vector<uint8_t>> images;  // id -> bytes, RT "IMAGE"
    // ReactOS: STREAM resources, kept apart from images because they are
    // emitted under a different resource type and are referenced by
    // DISKSTREAM/STREAM records rather than by FILENAME ones.  This is what
    // carries DWMWindow's frame atlas - see Parser.cpp's "@stream:" handling.
    std::map<uint32_t, std::vector<uint8_t>> streams;  // id -> bytes, RT "STREAM"
    int skipped = 0;
};

// Pack a model into a v4 .msstyles at outPath (defined in Compiler.cpp).
CompileStats writeModel(Model& m, const std::wstring& outPath);

// --- little-endian byte emit helpers ---------------------------------------
inline void putU32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(x & 0xFF);
    v.push_back((x >> 8) & 0xFF);
    v.push_back((x >> 16) & 0xFF);
    v.push_back((x >> 24) & 0xFF);
}
inline void putI32(std::vector<uint8_t>& v, int32_t x) { putU32(v, static_cast<uint32_t>(x)); }
inline void putU16(std::vector<uint8_t>& v, uint16_t x) {
    v.push_back(x & 0xFF);
    v.push_back((x >> 8) & 0xFF);
}
inline void putZStringW(std::vector<uint8_t>& v, const std::wstring& s) {
    for (wchar_t c : s)
        putU16(v, static_cast<uint16_t>(c));
    putU16(v, 0);
}
inline void alignTo(std::vector<uint8_t>& v, size_t a) {
    while (v.size() % a)
        v.push_back(0);
}

} // namespace vcompile
