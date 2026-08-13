// Util.h - small string/console helpers shared by the CLI subcommands.
#pragma once
#include <string>
#include <string_view>

namespace cli {

std::string toUtf8(std::wstring_view w);
std::wstring fromUtf8(std::string_view s);

// Print a UTF-8 string (already encoded) to stdout.
void out(const std::string& s);
void outln(const std::string& s);

} // namespace cli
