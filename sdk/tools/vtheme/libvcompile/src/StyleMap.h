// StyleMap.h - declarations for the ported ReactOS XP-INI name->id lookups.
#pragma once
#include <windows.h>

// Defined in StyleMap.cpp (ported from ReactOS stylemap.c).
BOOL MSSTYLES_LookupPartState(LPCWSTR pszClass, LPCWSTR pszPart, LPCWSTR pszState,
                              int* iPartId, int* iStateId);
BOOL MSSTYLES_LookupProperty(LPCWSTR pszPropertyName, int* dwPrimitive, int* dwId);
BOOL MSSTYLES_LookupEnum(LPCWSTR pszValueName, int dwEnum, int* dwValue);
