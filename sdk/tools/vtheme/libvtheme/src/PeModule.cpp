#include "vtheme/PeModule.h"

#include <cstring>
#include <windows.h>

namespace vtheme {

PeModule::~PeModule() {
    if (handle_)
        ::FreeLibrary(static_cast<HMODULE>(handle_));
}

PeModule::PeModule(PeModule&& o) noexcept : handle_(o.handle_) { o.handle_ = nullptr; }

PeModule& PeModule::operator=(PeModule&& o) noexcept {
    if (this != &o) {
        if (handle_)
            ::FreeLibrary(static_cast<HMODULE>(handle_));
        handle_ = o.handle_;
        o.handle_ = nullptr;
    }
    return *this;
}

std::optional<PeModule> PeModule::load(const std::wstring& path) {
    HMODULE h = ::LoadLibraryExW(path.c_str(), nullptr,
                                 LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    if (!h)
        h = ::LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE);
    if (!h)
        return std::nullopt;
    return PeModule(static_cast<void*>(h));
}

static std::optional<std::vector<uint8_t>> readResource(HMODULE h, const wchar_t* type,
                                                         const wchar_t* name) {
    HRSRC hRes = ::FindResourceExW(h, type, name, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
    if (!hRes)
        hRes = ::FindResourceW(h, name, type);
    if (!hRes)
        return std::nullopt;
    DWORD size = ::SizeofResource(h, hRes);
    HGLOBAL hData = ::LoadResource(h, hRes);
    if (!hData)
        return std::nullopt;
    const void* p = ::LockResource(hData);
    if (!p)
        return std::nullopt;
    const auto* bytes = static_cast<const uint8_t*>(p);
    return std::vector<uint8_t>(bytes, bytes + size);
}

std::optional<std::vector<uint8_t>> PeModule::resource(const wchar_t* type,
                                                       const wchar_t* name) const {
    if (!handle_)
        return std::nullopt;
    return readResource(static_cast<HMODULE>(handle_), type, name);
}

std::optional<std::vector<uint8_t>> PeModule::resource(const wchar_t* type, uint32_t id) const {
    if (!handle_)
        return std::nullopt;
    return readResource(static_cast<HMODULE>(handle_), type, MAKEINTRESOURCEW(id));
}

std::optional<int16_t> PeModule::packthemVersion() const {
    // RT id 1, name "PACKTHEM_VERSION".
    auto bytes = resource(L"PACKTHEM_VERSION", MAKEINTRESOURCEW(1));
    if (!bytes)
        bytes = resource(reinterpret_cast<const wchar_t*>(1), L"PACKTHEM_VERSION");
    if (!bytes || bytes->size() < sizeof(int16_t))
        return std::nullopt;
    int16_t v;
    std::memcpy(&v, bytes->data(), sizeof(v));
    return v;
}

static BOOL CALLBACK enumIdProc(HMODULE, LPCWSTR, LPWSTR name, LONG_PTR param) {
    auto* out = reinterpret_cast<std::vector<uint32_t>*>(param);
    if (IS_INTRESOURCE(name))
        out->push_back(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(name)));
    return TRUE;
}

std::vector<uint32_t> PeModule::enumIds(const wchar_t* type) const {
    std::vector<uint32_t> ids;
    if (handle_)
        ::EnumResourceNamesW(static_cast<HMODULE>(handle_), type, enumIdProc,
                             reinterpret_cast<LONG_PTR>(&ids));
    return ids;
}

std::optional<std::wstring> PeModule::loadString(uint32_t id) const {
    if (!handle_)
        return std::nullopt;
    // String resources live in tables of 16 strings; LoadStringW handles the bucketing.
    wchar_t* ptr = nullptr;
    int len = ::LoadStringW(static_cast<HMODULE>(handle_), id,
                            reinterpret_cast<LPWSTR>(&ptr), 0);
    if (len <= 0)
        return std::nullopt;
    return std::wstring(ptr, ptr + len);
}

} // namespace vtheme
