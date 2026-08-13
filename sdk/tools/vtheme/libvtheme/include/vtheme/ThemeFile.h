// ThemeFile.h - the parsed object model of a v4 visual style.
#pragma once
#include "Value.h"
#include <memory>
#include <string>
#include <vector>

namespace vtheme {

struct ThemeProperty {
    int recordIndex = 0;
    int partId = 0;
    int stateId = 0;
    int propertyId = 0;     // TMT symbol id
    int primitiveType = 0;  // TMT primitive type id
    Value value;
};

struct ThemeState {
    int id = 0;
    std::wstring name;
    std::vector<ThemeProperty> properties;
};

struct ThemePart {
    int id = 0;
    std::wstring name;
    std::vector<ThemeProperty> properties;  // state 0
    std::vector<ThemeState> states;

    ThemeState* findState(int stateId);
    ThemeState& ensureState(int stateId);
};

struct ThemeClass {
    std::wstring fullName;    // raw CMAP name, e.g. "Explorer::ListView"
    std::wstring appName;     // "Explorer" or empty
    std::wstring className;   // "ListView"
    std::wstring baseClassName;
    std::vector<ThemeProperty> properties;  // part 0 / state 0
    std::vector<ThemePart> parts;

    ThemePart* findPart(int partId);
    ThemePart& ensurePart(int partId);
};

struct VariantInfo {
    std::wstring name;   // VARIANT resource name, e.g. "NORMAL"
    std::wstring size;   // e.g. "NormalSize"
    std::wstring color;  // e.g. "NormalColor"
};

class ThemeFile {
public:
    std::wstring path;
    std::wstring muiPath;
    int version = 0;
    VariantInfo variant;
    std::vector<std::wstring> classNames;        // CMAP entries
    std::vector<ThemeProperty> rootProperties;   // RMAP records
    std::vector<ThemeClass> classes;

    ThemeClass* findClass(const std::wstring& appName, const std::wstring& className);
    ThemeClass& ensureClass(const std::wstring& fullName);
    void sort();
};

} // namespace vtheme
