#include "Builder.h"
#include "vcompile/Compiler.h"
#include <vtheme/Tmt.h>

#include <algorithm>
#include <filesystem>

#include <windows.h>

namespace fs = std::filesystem;

namespace vcompile {

// Defined in Parser.cpp
void parseSource(Model& m, const std::wstring& srcPath);

namespace {

void putPascalZ(std::vector<uint8_t>& v, const std::wstring& s) {
    putU32(v, static_cast<uint32_t>(s.size() + 1));
    for (wchar_t c : s)
        putU16(v, static_cast<uint16_t>(c));
    putU16(v, 0);
}

// ReactOS: cbData for a RESOURCE-BACKED record.
//
// This used to be 0, on the reasonable-sounding grounds that a record whose
// value lives in a resource has no inline payload to measure.  Vista disagrees,
// and it is not cosmetic - LoadVSRecordData (uxtheme.dll.c:22844) checks
//
//     cbType = property_map[GetThemePrimitiveID(sym, type)].cbType;
//     if (cbType > 0 && cbType != pRecord->cbData) return E_INVALIDARG;
//
// before it will dispatch to the loader.  With cbData == 0 every image
// reference in the style is rejected by Vista's own uxtheme, and by
// uxtheme_new, which implements the same check.  A style compiled that way
// loads (the class/part/state tree is fine) but silently has no artwork, which
// is a much worse failure than not loading at all.
//
// The value is the size of the payload the resource loader PRODUCES, not of the
// resource.  Measured over real Vista aero.msstyles, every resource-backed
// record in it is one of:
//
//     type 203 BOOL      cbData 4       (parsed from a .mui string)
//     type 206 FILENAME  cbData 16      the 16-byte image descriptor
//     type 209 RECT      cbData 16
//     type 210 FONT      cbData 92      sizeof(LOGFONTW)
//     type 213 DISKSTREAM cbData 8      the {offset, size} locator
//
// which is exactly property_map's cbType, with 8 for the two stream types whose
// row says "variable".
uint32_t resBackedByteLength(int symbolVal, int primitiveType) {
    switch (primitiveType) {
    case vtheme::TMT_ENUM:       // 200
    case vtheme::TMT_INT:        // 202
    case vtheme::TMT_BOOL:       // 203
    case vtheme::TMT_COLOR:      // 204
    case vtheme::TMT_SIZE:       // 207
        return 4;
    case vtheme::TMT_POSITION:   // 208
        return 8;
    case vtheme::TMT_MARGINS:    // 205
    case vtheme::TMT_FILENAME:   // 206 - an image descriptor, not a path
    case vtheme::TMT_RECT:       // 209
        return 16;
    case vtheme::TMT_FONT:       // 210
        return 92;
    case vtheme::TMT_DISKSTREAM: // 213 - ULARGE_INTEGER {offset, size}
    case vtheme::TMT_STREAM:     // 214
        return 8;
    default:
        (void)symbolVal;
        return 0;
    }
}

void emitRecord(std::vector<uint8_t>& v, const Rec& r, int classId) {
    uint32_t byteLength = r.resId ? resBackedByteLength(r.symbolVal, r.primitiveType)
                                  : static_cast<uint32_t>(r.inlineBytes.size());
    putI32(v, r.symbolVal);
    putI32(v, r.primitiveType);
    putI32(v, classId);
    putI32(v, r.partId);
    putI32(v, r.stateId);
    putU32(v, r.resId);
    putI32(v, 0);  // reserved
    putU32(v, byteLength);
    if (r.resId == 0)
        v.insert(v.end(), r.inlineBytes.begin(), r.inlineBytes.end());
    alignTo(v, 8);
}

std::wstring exeDir() {
    wchar_t buf[MAX_PATH];
    DWORD n = ::GetModuleFileNameW(nullptr, buf, MAX_PATH);
    fs::path p(std::wstring(buf, buf + n));
    return p.parent_path().wstring();
}

struct ResItem {
    std::wstring type;      // string type (always a string here)
    bool nameIsInt = false;
    std::wstring nameStr;
    uint16_t nameInt = 0;
    std::vector<uint8_t> data;
    const wchar_t* name() const {
        return nameIsInt ? MAKEINTRESOURCEW(nameInt) : nameStr.c_str();
    }
};

// Commit resources in batches. EndUpdateResource builds the whole .rsrc in
// memory and fails (ERROR_INVALID_DATA) when a single session stages too many
// resources, so we flush in chunks, re-opening with bDeleteExistingResources=FALSE.
void writeResources(const std::wstring& outPath, const std::vector<ResItem>& items) {
    const size_t kBatch = 48;
    bool first = true;
    for (size_t i = 0; i < items.size(); i += kBatch) {
        HANDLE h = ::BeginUpdateResourceW(outPath.c_str(), first ? TRUE : FALSE);
        if (!h)
            throw CompileError("BeginUpdateResource failed, err=" +
                               std::to_string(::GetLastError()));
        first = false;
        for (size_t j = i; j < i + kBatch && j < items.size(); ++j) {
            const ResItem& it = items[j];
            if (!::UpdateResourceW(h, it.type.c_str(), it.name(),
                                   MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                   const_cast<uint8_t*>(it.data.data()),
                                   static_cast<DWORD>(it.data.size()))) {
                DWORD e = ::GetLastError();
                ::EndUpdateResourceW(h, TRUE);
                throw CompileError("UpdateResource failed, err=" + std::to_string(e));
            }
        }
        if (!::EndUpdateResourceW(h, FALSE))
            throw CompileError("EndUpdateResource failed, err=" +
                               std::to_string(::GetLastError()));
    }
}

} // namespace

CompileStats writeModel(Model& m, const std::wstring& outPath) {
    // --- order classes: globals, sysmetrics first, then the rest -----------
    std::vector<CClass*> ordered;
    auto pull = [&](const std::wstring& nm) {
        for (auto& c : m.classes)
            if (c.fullName == nm) {
                ordered.push_back(&c);
                return;
            }
    };
    pull(L"globals");
    pull(L"sysmetrics");
    for (auto& c : m.classes) {
        if (c.fullName == L"globals" || c.fullName == L"sysmetrics")
            continue;
        ordered.push_back(&c);
    }

    // classId for each class = 4 + position in `ordered`
    std::map<std::wstring, int> ordinalByName;
    for (size_t i = 0; i < ordered.size(); ++i)
        ordinalByName[ordered[i]->fullName] = static_cast<int>(i);

    // --- CMAP -------------------------------------------------------------
    std::vector<uint8_t> cmap;
    auto emitName = [&](const std::wstring& s) {
        putZStringW(cmap, s);
        alignTo(cmap, 8);
    };
    emitName(L"documentation");
    emitName(L"sizevariant." + m.sizeName);
    emitName(L"sizevariant.Default");
    emitName(L"colorvariant." + m.colorName);
    for (auto* c : ordered)
        emitName(c->fullName);

    // --- VMAP -------------------------------------------------------------
    std::vector<uint8_t> vmap;
    alignTo(vmap, 4);
    putPascalZ(vmap, m.variantName);
    alignTo(vmap, 4);
    putPascalZ(vmap, m.sizeName);
    alignTo(vmap, 4);
    putPascalZ(vmap, m.colorName);

    // --- BCMAP ------------------------------------------------------------
    std::vector<uint8_t> bcmap;
    putI32(bcmap, static_cast<int>(ordered.size()));
    for (auto* c : ordered) {
        int base = -1;
        if (!c->baseName.empty()) {
            auto it = ordinalByName.find(c->baseName);
            if (it != ordinalByName.end())
                base = it->second;
        }
        putI32(bcmap, base);
    }

    // --- VARIANT (packed records) -----------------------------------------
    std::vector<uint8_t> variant;
    int recCount = 0;
    for (auto* c : ordered) {
        int classId = 4 + ordinalByName[c->fullName];
        for (const auto& r : c->recs) {
            emitRecord(variant, r, classId);
            ++recCount;
        }
    }

    // --- RMAP (root / documentation records) ------------------------------
    std::vector<uint8_t> rmap;
    for (const auto& r : m.rootRecs) {
        emitRecord(rmap, r, 0);
        ++recCount;
    }

    // --- write the carrier PE and inject resources ------------------------
    fs::path stub = fs::path(exeDir()) / L"themestub.dll";
    if (!fs::exists(stub))
        throw CompileError("themestub.dll not found next to the executable");

    std::error_code ec;
    fs::create_directories(fs::path(outPath).parent_path(), ec);
    fs::copy_file(stub, fs::path(outPath), fs::copy_options::overwrite_existing, ec);
    if (ec)
        throw CompileError("failed to create output file: " + ec.message());

    std::vector<uint8_t> ver;
    putU16(ver, static_cast<uint16_t>(m.version));

    std::vector<ResItem> items;
    auto addStr = [&](const wchar_t* type, const wchar_t* name, std::vector<uint8_t> data) {
        items.push_back(ResItem{type, false, name, 0, std::move(data)});
    };
    auto addInt = [&](const wchar_t* type, uint16_t id, std::vector<uint8_t> data) {
        items.push_back(ResItem{type, true, L"", id, std::move(data)});
    };

    addInt(L"PACKTHEM_VERSION", 1, ver);
    addStr(L"CMAP", L"CMAP", cmap);
    addStr(L"VMAP", L"VMAP", vmap);
    addStr(L"BCMAP", L"BCMAP", bcmap);
    addStr(L"VARIANT", m.variantName.c_str(), variant);
    if (!rmap.empty())
        addStr(L"RMAP", L"RMAP", rmap);
    for (const auto& [id, bytes] : m.images)
        addInt(L"IMAGE", static_cast<uint16_t>(id), bytes);
    /* ReactOS: STREAM payloads - see the DISKSTREAM case in Parser.cpp. */
    for (const auto& [id, bytes] : m.streams)
        addInt(L"STREAM", static_cast<uint16_t>(id), bytes);

    writeResources(outPath, items);

    CompileStats st;
    st.classes = static_cast<int>(ordered.size());
    st.records = recCount;
    st.images = static_cast<int>(m.images.size());
    st.skipped = m.skipped;
    return st;
}

CompileStats compileTheme(const std::wstring& srcPath, const std::wstring& outPath) {
    Model m;
    parseSource(m, srcPath);
    return writeModel(m, outPath);
}

} // namespace vcompile
