/*
<<<<<<<< HEAD:modules/rostests/winetests/mscoree/loadpaths.dll.cs
 * Copyright 2021 Rémi Bernon for CodeWeavers
========
 * Copyright (c) 2018 Ethan Lee for CodeWeavers
>>>>>>>> d34fa7f7992 ([XAUDIO2_7][MEDIAFOUNDATION][DSOUND][AMSTREAM][QCAP][QEDIT][EVR][STRMIIDS][MFUUID][STRMBASE] Sync to Wine-10.0):dll/directx/wine/xaudio2_7/xaudio_allocator.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

<<<<<<<< HEAD:modules/rostests/winetests/mscoree/loadpaths.dll.cs
using System.Reflection;

#if NEUTRAL
[assembly: AssemblyCulture("")]
#else
[assembly: AssemblyCulture("en")]
#endif

namespace LoadPaths
{
    public class Test2
    {
        public int Foo() { return 0; }
    }
========
#include <stdarg.h>

#define COBJMACROS

#include "ole2.h"

void* XAudio_Internal_Malloc(size_t size)
{
    return CoTaskMemAlloc(size);
}

void XAudio_Internal_Free(void* ptr)
{
    CoTaskMemFree(ptr);
}

void* XAudio_Internal_Realloc(void* ptr, size_t size)
{
    return CoTaskMemRealloc(ptr, size);
>>>>>>>> d34fa7f7992 ([XAUDIO2_7][MEDIAFOUNDATION][DSOUND][AMSTREAM][QCAP][QEDIT][EVR][STRMIIDS][MFUUID][STRMBASE] Sync to Wine-10.0):dll/directx/wine/xaudio2_7/xaudio_allocator.c
}
