
#pragma once

#include <apitest.h>
#include <stdio.h>
#include <pseh/pseh2.h>

extern void* load_function(const char *funcname);
int* __cdecl ntdll_errno(void);

#define STATUS_ACCESS_VIOLATION          ((DWORD   )0xC0000005L)    
