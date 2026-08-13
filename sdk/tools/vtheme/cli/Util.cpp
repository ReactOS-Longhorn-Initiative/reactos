#include "Util.h"

#include <cstdio>
#include <windows.h>

namespace cli {

std::string toUtf8(std::wstring_view w) {
    if (w.empty())
        return {};
    int n = ::WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

std::wstring fromUtf8(std::string_view s) {
    if (s.empty())
        return {};
    int n = ::MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}

void out(const std::string& s) { std::fwrite(s.data(), 1, s.size(), stdout); }
void outln(const std::string& s) {
    out(s);
    std::fputc('\n', stdout);
}

} // namespace cli
