#pragma once

typedef
NTSTATUS
(APIENTRY *DXGKCDD_INTERFACEREFERENCE)(PVOID Param);

typedef
NTSTATUS
(APIENTRY *DXGKCDD_CREATE)(PVOID Param1, PVOID Param2, PVOID Param3,
                           PVOID Param4, PVOID Param5);

typedef
NTSTATUS
(APIENTRY *DXGKCDD_DESTROY)(UINT32 AdapterId);


typedef
NTSTATUS
(APIENTRY *DXGKCDD_GETDISPLAYMODELIST)(PVOID DxgAdater, D3DKMT_GETDISPLAYMODELIST **GetDisplayModeList);

typedef struct _DXGKCDD_INTERFACE
{
    DWORD Version;
    DWORD Magic;
    DXGKCDD_INTERFACEREFERENCE DxgkCddInterfaceReference;
    DXGKCDD_INTERFACEREFERENCE DxgkCddInterfaceReferenceTwo;
    DXGKCDD_CREATE DxgkCddCreate;
    DXGKCDD_DESTROY DxgkCddDestroy;
    PVOID DxgkCddEnable;
    PVOID DxgkCddDisable;
    PVOID DxgkCddLock;
    PVOID DxgkCddUnlock;
    DXGKCDD_GETDISPLAYMODELIST DxgkCddGetDisplayModeList;
    PVOID DxgkCddPresent;
    PVOID DxgkCddSetGammaRamp;
    PVOID DxgkCddSetPalette;
    PVOID DxgkCddSetPointerPosition;
    PVOID DxgkCddSetPointerShape;
    PVOID DxgkCddSetOrigin;
    PVOID DxgkCddWaitForVerticalBlankEvent;
    PVOID DxgkCddTerminateThread;
    DWORD Unknown[75];
    /* element 19 */
} DXGKCDD_INTERFACE, *PDXGKCDD_INTERFACE;
