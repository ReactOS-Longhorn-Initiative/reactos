#include "k32_vista.h"

#define NDEBUG
#include <debug.h>

/*
 * the good news about this function is that i know very well how to implement it
 * the problem is i need an actual case where it's ACTIVELY being used to do so
 * if this debug break is triggered please @The_DarkFire_ so i can implement this
 * */
BOOL CopyContext(
    PCONTEXT Destination,
    DWORD    ContextFlags,
    PCONTEXT Source
)
{
    DPRINT1("Hey! You found a place where Kernel32!CopyContext is used\n Please @ The_DarkFire!!!!!!!\n");
    __debugbreak();
    return FALSE;
}
