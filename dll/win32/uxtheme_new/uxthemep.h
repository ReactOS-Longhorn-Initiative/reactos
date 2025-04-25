#pragma once.

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winnls.h>
#include <windowsx.h>
#include <undocuser.h>
#include <undocgdi.h>
#include <uxtheme.h>
#include <uxundoc.h>
#include <vfwmsgs.h>
#include <tmschema.h>

#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <ndk/rtltypes.h>


#ifdef _MSC_VER >= 1200
#define UXUAPI EXTERN_C DECLSPEC_IMPORT
#else
#define UXUAPI EXTERN_C
#endif

