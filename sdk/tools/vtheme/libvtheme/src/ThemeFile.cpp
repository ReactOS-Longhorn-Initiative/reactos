#include "vtheme/ThemeFile.h"

#include <algorithm>

namespace vtheme {

ThemeState* ThemePart::findState(int stateId) {
    for (auto& s : states)
        if (s.id == stateId)
            return &s;
    return nullptr;
}

ThemeState& ThemePart::ensureState(int stateId) {
    if (auto* s = findState(stateId))
        return *s;
    states.push_back(ThemeState{stateId, {}, {}});
    return states.back();
}

ThemePart* ThemeClass::findPart(int partId) {
    for (auto& p : parts)
        if (p.id == partId)
            return &p;
    return nullptr;
}

ThemePart& ThemeClass::ensurePart(int partId) {
    if (auto* p = findPart(partId))
        return *p;
    parts.push_back(ThemePart{partId, {}, {}, {}});
    return parts.back();
}

ThemeClass* ThemeFile::findClass(const std::wstring& appName, const std::wstring& className) {
    for (auto& c : classes)
        if (c.appName == appName && c.className == className)
            return &c;
    return nullptr;
}

ThemeClass& ThemeFile::ensureClass(const std::wstring& fullName) {
    std::wstring app, cls = fullName;
    auto colon = fullName.find(L"::");
    if (colon != std::wstring::npos) {
        app = fullName.substr(0, colon);
        cls = fullName.substr(colon + 2);
    }
    if (auto* c = findClass(app, cls))
        return *c;
    classes.push_back(ThemeClass{fullName, app, cls, {}, {}, {}});
    return classes.back();
}

void ThemeFile::sort() {
    std::sort(classes.begin(), classes.end(), [](const ThemeClass& a, const ThemeClass& b) {
        return a.fullName < b.fullName;
    });
    for (auto& c : classes) {
        std::sort(c.parts.begin(), c.parts.end(),
                  [](const ThemePart& a, const ThemePart& b) { return a.id < b.id; });
        for (auto& p : c.parts)
            std::sort(p.states.begin(), p.states.end(),
                      [](const ThemeState& a, const ThemeState& b) { return a.id < b.id; });
    }
}

} // namespace vtheme
