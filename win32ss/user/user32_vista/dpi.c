/*
 * PROJECT:     ReactOS Kernel - Vista+ APIs
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     DPI functions for user32 and user32_vista.
 * COPYRIGHT:   Copyright 2024 Carl Bialorucki <cbialo2@outlook.com>
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <windef.h>
#include <wingdi.h>
#include <winuser.h>

#define NDEBUG
#include <debug.h>

/*
 * @stub
 */
UINT
WINAPI
GetDpiForSystem(VOID)
{
    HDC hDC;
    UINT Dpi;
    hDC = GetDC(NULL);
    Dpi = GetDeviceCaps(hDC, LOGPIXELSY);
    ReleaseDC(NULL, hDC);
    return Dpi;
}

/*
 * @stub
 */
UINT
WINAPI
GetDpiForWindow(
    _In_ HWND hWnd)
{
    UNIMPLEMENTED_ONCE;
    UNREFERENCED_PARAMETER(hWnd);
    return GetDpiForSystem();
}

 
 
 
 
 

BOOL WINAPI CloseTouchInputHandle(
   PVOID  hTouchInput
)
{
  return 1;
}
BOOL WINAPI RegisterTouchWindow(
 HWND  hwnd,
 ULONG ulFlags
)
{
  return 1;
}
BOOL WINAPI IsThreadDesktopComposited()
{
   // UNIMPLEMENTED;
    return FALSE;
}
 
typedef struct DISPLAYCONFIG_DESKTOP_IMAGE_INFO {
  POINTL PathSourceSize;
  RECTL  DesktopImageRegion;
  RECTL  DesktopImageClip;
} DISPLAYCONFIG_DESKTOP_IMAGE_INFO;

typedef enum {
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_OTHER = -1,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_HD15 = 0,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_SVIDEO = 1,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_COMPOSITE_VIDEO = 2,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_COMPONENT_VIDEO = 3,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DVI = 4,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_HDMI = 5,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_LVDS = 6,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_D_JPN = 8,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_SDI = 9,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EXTERNAL = 10,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED = 11,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EXTERNAL = 12,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EMBEDDED = 13,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_SDTVDONGLE = 14,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_MIRACAST = 15,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INDIRECT_WIRED = 16,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INDIRECT_VIRTUAL = 17,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_USB_TUNNEL,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL = 0x80000000,
  DISPLAYCONFIG_OUTPUT_TECHNOLOGY_FORCE_UINT32 = 0xFFFFFFFF
} DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY;

typedef enum {
  DISPLAYCONFIG_ROTATION_IDENTITY = 1,
  DISPLAYCONFIG_ROTATION_ROTATE90 = 2,
  DISPLAYCONFIG_ROTATION_ROTATE180 = 3,
  DISPLAYCONFIG_ROTATION_ROTATE270 = 4,
  DISPLAYCONFIG_ROTATION_FORCE_UINT32 = 0xFFFFFFFF
} DISPLAYCONFIG_ROTATION;

typedef enum {
  DISPLAYCONFIG_SCALING_IDENTITY = 1,
  DISPLAYCONFIG_SCALING_CENTERED = 2,
  DISPLAYCONFIG_SCALING_STRETCHED = 3,
  DISPLAYCONFIG_SCALING_ASPECTRATIOCENTEREDMAX = 4,
  DISPLAYCONFIG_SCALING_CUSTOM = 5,
  DISPLAYCONFIG_SCALING_PREFERRED = 128,
  DISPLAYCONFIG_SCALING_FORCE_UINT32 = 0xFFFFFFFF
} DISPLAYCONFIG_SCALING;

typedef struct DISPLAYCONFIG_RATIONAL {
  UINT32 Numerator;
  UINT32 Denominator;
} DISPLAYCONFIG_RATIONAL;

typedef enum {
  DISPLAYCONFIG_SCANLINE_ORDERING_UNSPECIFIED = 0,
  DISPLAYCONFIG_SCANLINE_ORDERING_PROGRESSIVE = 1,
  DISPLAYCONFIG_SCANLINE_ORDERING_INTERLACED = 2,
  DISPLAYCONFIG_SCANLINE_ORDERING_INTERLACED_UPPERFIELDFIRST,
  DISPLAYCONFIG_SCANLINE_ORDERING_INTERLACED_LOWERFIELDFIRST = 3,
  DISPLAYCONFIG_SCANLINE_ORDERING_FORCE_UINT32 = 0xFFFFFFFF
} DISPLAYCONFIG_SCANLINE_ORDERING;

typedef struct DISPLAYCONFIG_PATH_TARGET_INFO {
  LUID                                  adapterId;
  UINT32                                id;
  union {
    UINT32 modeInfoIdx;
    struct {
      UINT32 desktopModeInfoIdx : 16;
      UINT32 targetModeInfoIdx : 16;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
  DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY outputTechnology;
  DISPLAYCONFIG_ROTATION                rotation;
  DISPLAYCONFIG_SCALING                 scaling;
  DISPLAYCONFIG_RATIONAL                refreshRate;
  DISPLAYCONFIG_SCANLINE_ORDERING       scanLineOrdering;
  BOOL                                  targetAvailable;
  UINT32                                statusFlags;
} DISPLAYCONFIG_PATH_TARGET_INFO;


typedef struct DISPLAYCONFIG_PATH_SOURCE_INFO {
  LUID   adapterId;
  UINT32 id;
  union {
    UINT32 modeInfoIdx;
    struct {
      UINT32 cloneGroupId : 16;
      UINT32 sourceModeInfoIdx : 16;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
  UINT32 statusFlags;
} DISPLAYCONFIG_PATH_SOURCE_INFO;

typedef struct DISPLAYCONFIG_PATH_INFO {
  DISPLAYCONFIG_PATH_SOURCE_INFO sourceInfo;
  DISPLAYCONFIG_PATH_TARGET_INFO targetInfo;
  UINT32                         flags;
} DISPLAYCONFIG_PATH_INFO;

typedef enum {
  DISPLAYCONFIG_PIXELFORMAT_8BPP = 1,
  DISPLAYCONFIG_PIXELFORMAT_16BPP = 2,
  DISPLAYCONFIG_PIXELFORMAT_24BPP = 3,
  DISPLAYCONFIG_PIXELFORMAT_32BPP = 4,
  DISPLAYCONFIG_PIXELFORMAT_NONGDI = 5,
  DISPLAYCONFIG_PIXELFORMAT_FORCE_UINT32 = 0xffffffff
} DISPLAYCONFIG_PIXELFORMAT;

typedef struct DISPLAYCONFIG_SOURCE_MODE
{
    UINT32                      width;
    UINT32                      height;
    DISPLAYCONFIG_PIXELFORMAT   pixelFormat;
    POINTL                      position;
} DISPLAYCONFIG_SOURCE_MODE;

typedef struct DISPLAYCONFIG_2DREGION {
  UINT32 cx;
  UINT32 cy;
} DISPLAYCONFIG_2DREGION;

typedef struct DISPLAYCONFIG_VIDEO_SIGNAL_INFO {
  UINT64                          pixelRate;
  DISPLAYCONFIG_RATIONAL          hSyncFreq;
  DISPLAYCONFIG_RATIONAL          vSyncFreq;
  DISPLAYCONFIG_2DREGION          activeSize;
  DISPLAYCONFIG_2DREGION          totalSize;
  union {
    struct {
      UINT32 videoStandard : 16;
      UINT32 vSyncFreqDivider : 6;
      UINT32 reserved : 10;
    } AdditionalSignalInfo;
    UINT32 videoStandard;
  } DUMMYUNIONNAME;
  DISPLAYCONFIG_SCANLINE_ORDERING scanLineOrdering;
} DISPLAYCONFIG_VIDEO_SIGNAL_INFO;

typedef struct DISPLAYCONFIG_TARGET_MODE
{
    DISPLAYCONFIG_VIDEO_SIGNAL_INFO   targetVideoSignalInfo;
} DISPLAYCONFIG_TARGET_MODE;

typedef enum {
  DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE = 1,
  DISPLAYCONFIG_MODE_INFO_TYPE_TARGET = 2,
  DISPLAYCONFIG_MODE_INFO_TYPE_DESKTOP_IMAGE = 3,
  DISPLAYCONFIG_MODE_INFO_TYPE_FORCE_UINT32 = 0xFFFFFFFF
} DISPLAYCONFIG_MODE_INFO_TYPE;

typedef struct DISPLAYCONFIG_MODE_INFO {
  DISPLAYCONFIG_MODE_INFO_TYPE infoType;
  UINT32                       id;
  LUID                         adapterId;
  union {
    DISPLAYCONFIG_TARGET_MODE        targetMode;
    DISPLAYCONFIG_SOURCE_MODE        sourceMode;
    DISPLAYCONFIG_DESKTOP_IMAGE_INFO desktopImageInfo;
  } DUMMYUNIONNAME;
} DISPLAYCONFIG_MODE_INFO;

typedef enum _DISPLAYCONFIG_TOPOLOGY_ID {
  DISPLAYCONFIG_TOPOLOGY_INTERNAL = 0x00000001,
  DISPLAYCONFIG_TOPOLOGY_CLONE = 0x00000002,
  DISPLAYCONFIG_TOPOLOGY_EXTEND = 0x00000004,
  DISPLAYCONFIG_TOPOLOGY_EXTERNAL = 0x00000008,
  DISPLAYCONFIG_TOPOLOGY_FORCE_UINT32 = 0xFFFFFFFF
} DISPLAYCONFIG_TOPOLOGY_ID;

LONG WINAPI GetDisplayConfigBufferSizes(
 UINT32 flags,
 UINT32 *numPathArrayElements,
 UINT32 *numModeInfoArrayElements
)
{
  *numPathArrayElements = 0;
  *numModeInfoArrayElements = 0;
  return 0;
}
LONG
WINAPI
QueryDisplayConfig(
 UINT32                    flags,
 UINT32                    *numPathArrayElements,
 DISPLAYCONFIG_PATH_INFO   *pathArray,
 UINT32                    *numModeInfoArrayElements,
 DISPLAYCONFIG_MODE_INFO   *modeInfoArray,
 DISPLAYCONFIG_TOPOLOGY_ID *currentTopologyId
)
{
  return ERROR_ACCESS_DENIED;
}


























/*
 * @unimplemented
 */
BOOL
WINAPI
LogicalToPhysicalPoint( HWND hWnd, LPPOINT lpPoint )
{
    UNIMPLEMENTED;
    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
ChangeWindowMessageFilter(
    UINT  message,
    DWORD dwFlag)
{
    UNIMPLEMENTED;
    return TRUE;
}

/*
 * @unimplemented
 */
BOOL WINAPI
ChangeWindowMessageFilterEx(HWND hwnd,
                            UINT message,
                            DWORD action,
                            PVOID pChangeFilterStruct)
{
    UNIMPLEMENTED;
    return TRUE;
}

BOOL
WINAPI
ShutdownBlockReasonCreate(HWND hWnd, LPCWSTR pwszReason)
{
    UNIMPLEMENTED;
    return 1;
}

BOOL
WINAPI
ShutdownBlockReasonQuery( HWND   hWnd,
  LPWSTR pwszBuff,
  DWORD  *pcchBuff)
{
    UNIMPLEMENTED;
    return 1;
}

BOOL
WINAPI
ShutdownBlockReasonDestroy(HWND hWnd)
{
    UNIMPLEMENTED;
    return 1;
}

BOOL
WINAPI
CalculatePopupWindowPosition(const POINT *anchorPoint,
                             const SIZE *windowSize,
                             UINT flags,
                             RECT *excludeRect,
                             RECT *popupWindowPosition)
{
    UNIMPLEMENTED;
    return TRUE;
}

BOOL
WINAPI
DwmGetDxSharedSurface(HWND hwnd,
                      HANDLE* phSurface,
                      LUID* pAdapterLuid,
                      ULONG* pFmtWindow,
                      ULONG* pPresentFlags,
                      ULONGLONG* pWin32kUpdateId)
{
    UNIMPLEMENTED;
    return TRUE;
}

HWND
WINAPI
GhostWindowFromHungWindow(HWND hwndGhost)
{
    UNIMPLEMENTED;
    return NULL;
}

HWND
WINAPI
HungWindowFromGhostWindow(HWND hwndGhost)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
WINAPI
IsProcessDPIAware(VOID)
{
    UNIMPLEMENTED;
    return TRUE;
}

BOOL
WINAPI
IsTouchWindow(HWND hwnd,
              PULONG pulFlags)
{
    UNIMPLEMENTED;
    return TRUE;
}

 

BOOL
WINAPI
IsTopLevelWindow(IN HWND hWnd)
{
    UNIMPLEMENTED;
    return TRUE;
}

BOOL
WINAPI
IsWindowRedirectedForPrint(IN HWND hWnd)
{
    UNIMPLEMENTED;
    return TRUE;
}

BOOL
WINAPI
GetWindowCompositionAttribute(HWND hwnd,
                              PVOID pAttrData) // WINCOMPATTRDATA
{
    UNIMPLEMENTED;
    return TRUE;
}

HICON
WINAPI
InternalGetWindowIcon(HWND hwnd, UINT iconType)
{
    UNIMPLEMENTED;
    return NULL;
}

 

PVOID // HPOWERNOTIFY
WINAPI
RegisterPowerSettingNotification(HANDLE hRecipient,
                                 LPCGUID PowerSettingGuid,
                                 DWORD Flags)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
WINAPI
SetWindowCompositionAttribute(HWND hwnd,
                              PVOID pAttrData) // WINCOMPATTRDATA
{
    UNIMPLEMENTED;
    return TRUE;
}

BOOL
WINAPI
UnregisterPowerSettingNotification(HANDLE Handle) // HPOWERNOTIFY
{
    UNIMPLEMENTED;
    return TRUE;
}

LONG 
WINAPI
DisplayConfigGetDeviceInfo(
  PVOID *requestPacket
)
{
    return 0;
}


BOOL SetProcessDPIAware()
{
  return 0;
}

BOOL
WINAPI
GetProcessDpiAwarenessInternal(HANDLE handle,DPI_AWARENESS* dpi )
{
    return FALSE;
}
BOOL
WINAPI
SetProcessDpiAwarenessInternal(DPI_AWARENESS dpi)
{
    return FALSE;
}
BOOL
WINAPI
GetDpiForMonitorInternal(HMONITOR monitor ,UINT one,UINT* two,UINT* three)
{
    return FALSE;
}
