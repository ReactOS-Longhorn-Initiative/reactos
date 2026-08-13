// PeModule.h - read-only access to the named PE resources inside a .msstyles / .mui.
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace vtheme {

// RAII wrapper over LoadLibraryEx(..., LOAD_LIBRARY_AS_IMAGE_RESOURCE).
class PeModule {
public:
    PeModule() = default;
    explicit PeModule(void* handle) : handle_(handle) {}
    ~PeModule();
    PeModule(PeModule&& o) noexcept;
    PeModule& operator=(PeModule&& o) noexcept;
    PeModule(const PeModule&) = delete;
    PeModule& operator=(const PeModule&) = delete;

    // Open a module as a pure resource datafile. Returns nullopt on failure.
    static std::optional<PeModule> load(const std::wstring& path);

    bool valid() const { return handle_ != nullptr; }
    void* handle() const { return handle_; }

    // Raw resource bytes by string type/name (e.g. ("CMAP","CMAP")).
    std::optional<std::vector<uint8_t>> resource(const wchar_t* type, const wchar_t* name) const;
    // Raw resource bytes by string type + integer id (e.g. ("IMAGE", 1)).
    std::optional<std::vector<uint8_t>> resource(const wchar_t* type, uint32_t id) const;

    // The PACKTHEM_VERSION short (RT id 1). nullopt if absent.
    std::optional<int16_t> packthemVersion() const;

    // Enumerate integer-named resources of a string type.
    std::vector<uint32_t> enumIds(const wchar_t* type) const;

    // Load a string-table entry (used for MUI-backed string/font/rect values).
    std::optional<std::wstring> loadString(uint32_t id) const;

private:
    void* handle_ = nullptr;
};

} // namespace vtheme
