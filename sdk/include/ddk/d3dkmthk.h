/*
 * Copyright 2016 Henri Verbeet for CodeWeavers
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

#ifndef __WINE_D3DKMTHK_H
#define __WINE_D3DKMTHK_H

#include <d3dukmdt.h>

DEFINE_GUID(GUID_DEVINTERFACE_GRAPHICSPOWER, 0xea5c6870, 0xe93c, 0x4588, 0xbe, 0xf1, 0xfe, 0xc4, 0x2f, 0xc9, 0x42, 0x9a);

#define IOCTL_INTERNAL_GRAPHICSPOWER_REGISTER \
    CTL_CODE(FILE_DEVICE_VIDEO, 0xa01, METHOD_NEITHER, FILE_ANY_ACCESS)

#define DXGK_GRAPHICSPOWER_VERSION_1_0 0x1000
#define DXGK_GRAPHICSPOWER_VERSION_1_1 0x1001
#define DXGK_GRAPHICSPOWER_VERSION_1_2 0x1002
#define DXGK_GRAPHICSPOWER_VERSION DXGK_GRAPHICSPOWER_VERSION_1_2

typedef
UINT32
(NTAPI *PDXGK_SET_SHARED_POWER_COMPONENT_STATE)(
    PVOID DeviceHandle,
    PVOID PrivateHandle,
    ULONG ComponentIndex,
    BOOLEAN Active);

typedef
UINT32
(NTAPI *PDXGK_GRAPHICSPOWER_UNREGISTER)(
    PVOID DeviceHandle,
    PVOID PrivateHandle);

typedef struct _D3DKMT_CREATEDCFROMMEMORY
{
    void *pMemory;
    D3DDDIFORMAT Format;
    UINT Width;
    UINT Height;
    UINT Pitch;
    HDC hDeviceDc;
    PALETTEENTRY *pColorTable;
    HDC hDc;
    HANDLE hBitmap;
} D3DKMT_CREATEDCFROMMEMORY;

typedef struct _D3DKMT_DESTROYDCFROMMEMORY
{
    HDC hDc;
    HANDLE hBitmap;
} D3DKMT_DESTROYDCFROMMEMORY;

typedef
    _IRQL_requires_max_(PASSIVE_LEVEL)
VOID
(NTAPI *PDXGK_POWER_NOTIFICATION)(
    PVOID GraphicsDeviceHandle,
    DEVICE_POWER_STATE NewGrfxPowerState,
    BOOLEAN PreNotification,
    PVOID PrivateHandle);

typedef
    _IRQL_requires_max_(PASSIVE_LEVEL)
VOID
(NTAPI *PDXGK_REMOVAL_NOTIFICATION)(
    PVOID GraphicsDeviceHandle,
    PVOID PrivateHandle);

typedef
    _IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PDXGK_FSTATE_NOTIFICATION)(
    PVOID GraphicsDeviceHandle,
    ULONG ComponentIndex,
    UINT NewFState,
    BOOLEAN PreNotification,
    PVOID PrivateHandle);

typedef
    _IRQL_requires_(DISPATCH_LEVEL)
VOID
(NTAPI *PDXGK_INITIAL_COMPONENT_STATE)(
    PVOID GraphicsDeviceHandle,
    PVOID PrivateHandle,
    ULONG ComponentIndex,
    BOOLEAN IsBlockingType,
    UINT InitialFState,
    GUID ComponentGuid,
    UINT PowerComponentMappingFlag);

typedef struct _DXGK_GRAPHICSPOWER_REGISTER_INPUT_V_1_2 {
    ULONG Version;
    PVOID PrivateHandle;
    PDXGK_POWER_NOTIFICATION PowerNotificationCb;
    PDXGK_REMOVAL_NOTIFICATION RemovalNotificationCb;
    PDXGK_FSTATE_NOTIFICATION FStateNotificationCb;
    PDXGK_INITIAL_COMPONENT_STATE InitialComponentStateCb;
} DXGK_GRAPHICSPOWER_REGISTER_INPUT_V_1_2, *PDXGK_GRAPHICSPOWER_REGISTER_INPUT_V_1_2;

typedef DXGK_GRAPHICSPOWER_REGISTER_INPUT_V_1_2 DXGK_GRAPHICSPOWER_REGISTER_INPUT;
typedef DXGK_GRAPHICSPOWER_REGISTER_INPUT *PDXGK_GRAPHICSPOWER_REGISTER_INPUT;

typedef struct _DXGK_GRAPHICSPOWER_REGISTER_OUTPUT {
    PVOID                                  DeviceHandle;
    DEVICE_POWER_STATE                     InitialGrfxPowerState;
    PDXGK_SET_SHARED_POWER_COMPONENT_STATE SetSharedPowerComponentStateCb;
    PDXGK_GRAPHICSPOWER_UNREGISTER         UnregisterCb;
} DXGK_GRAPHICSPOWER_REGISTER_OUTPUT, *PDXGK_GRAPHICSPOWER_REGISTER_OUTPUT;

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

DWORD APIENTRY D3DKMTCreateDCFromMemory(_Inout_ D3DKMT_CREATEDCFROMMEMORY*);
DWORD APIENTRY D3DKMTDestroyDCFromMemory(_In_ CONST D3DKMT_DESTROYDCFROMMEMORY*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WINE_D3DKMTHK_H */
