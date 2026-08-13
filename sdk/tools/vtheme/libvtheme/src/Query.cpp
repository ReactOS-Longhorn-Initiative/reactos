#include "vtheme/Query.h"

namespace vtheme {

ThemeClass* findClassByName(ThemeFile& tf, const std::wstring& name) {
    for (auto& c : tf.classes)
        if (c.fullName == name)
            return &c;
    for (auto& c : tf.classes)
        if (c.className == name)
            return &c;
    return nullptr;
}

static const ThemeProperty* findIn(const std::vector<ThemeProperty>& props, int propertyId) {
    for (const auto& p : props)
        if (p.propertyId == propertyId)
            return &p;
    return nullptr;
}

const ThemeProperty* resolveProperty(ThemeFile& tf, const ThemeClass& cls, int partId,
                                     int stateId, int propertyId) {
    // Walk this class first.
    if (auto* part = const_cast<ThemeClass&>(cls).findPart(partId)) {
        if (stateId != 0) {
            if (auto* st = part->findState(stateId))
                if (auto* p = findIn(st->properties, propertyId))
                    return p;
        }
        if (auto* p = findIn(part->properties, propertyId))
            return p;
    }
    if (auto* p = findIn(cls.properties, propertyId))
        return p;

    // Then the base class chain.
    if (!cls.baseClassName.empty()) {
        if (auto* base = findClassByName(tf, cls.baseClassName)) {
            if (base != &cls)
                return resolveProperty(tf, *base, partId, stateId, propertyId);
        }
    }
    return nullptr;
}

} // namespace vtheme
