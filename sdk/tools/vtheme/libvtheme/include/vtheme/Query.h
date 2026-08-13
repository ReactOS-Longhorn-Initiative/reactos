// Query.h - resolve effective properties with part/state/base-class inheritance.
#pragma once
#include "ThemeFile.h"

namespace vtheme {

// Find a class by raw full name ("Explorer::ListView"), or by plain class name
// ("ListView") if no exact full-name match.
ThemeClass* findClassByName(ThemeFile& tf, const std::wstring& name);

// Resolve a property for (class, part, state) following the native fallback chain:
//   state -> part(state 0) -> class(part 0) -> base class (recursively).
// Returns nullptr if undefined anywhere in the chain.
const ThemeProperty* resolveProperty(ThemeFile& tf, const ThemeClass& cls, int partId,
                                     int stateId, int propertyId);

} // namespace vtheme
