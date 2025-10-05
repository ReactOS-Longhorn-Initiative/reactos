/*
<<<<<<<< HEAD:sdk/include/dxsdk/exposeenums2managed.h
 * Copyright 2020 Jactry Zeng for CodeWeavers
========
 * The Wine project - Xinput Joystick Library
 * Copyright 2008 Andrew Fenn
>>>>>>>> d34fa7f7992 ([XAUDIO2_7][MEDIAFOUNDATION][DSOUND][AMSTREAM][QCAP][QEDIT][EVR][STRMIIDS][MFUUID][STRMBASE] Sync to Wine-10.0):dll/directx/wine/xaudio2_7/version.rc
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

<<<<<<<< HEAD:sdk/include/dxsdk/exposeenums2managed.h
#ifdef MANAGED_ENUMS

#define TAG(x) x
#define ENUM16 ENUM

#else /* MANAGED_ENUMS */

#define TAG(x) tag##x
#define ENUM16 typedef enum

#endif  /* MANAGED_ENUMS */
========
#define WINE_FILEDESCRIPTION_STR "Wine Common Audio API"
#define WINE_FILENAME_STR "xaudio2_7.dll"
#define WINE_FILEVERSION 9,12,589,0000
#define WINE_FILEVERSION_STR "9.12.589.0000"
#define WINE_PRODUCTVERSION 9,12,589,0000
#define WINE_PRODUCTVERSION_STR "9.12"

#include "wine/wine_common_ver.rc"
>>>>>>>> d34fa7f7992 ([XAUDIO2_7][MEDIAFOUNDATION][DSOUND][AMSTREAM][QCAP][QEDIT][EVR][STRMIIDS][MFUUID][STRMBASE] Sync to Wine-10.0):dll/directx/wine/xaudio2_7/version.rc
