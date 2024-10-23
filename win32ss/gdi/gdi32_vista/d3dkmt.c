/*
 * PROJECT:     ReactOS Display Driver Model
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     D3DKMT dxgkrnl syscalls
 * COPYRIGHT:   Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

#include <gdi32_vista.h>
#include <d3dkmddi.h>
#include <debug.h>
NTSTATUS
WINAPI
D3DKMTCreateAllocation(_Inout_ D3DKMT_CREATEALLOCATION* unnamedParam1)
{
    DPRINT1("D3DKMTCreateAllocation: Entry\n");
__debugbreak();
    return NtGdiDdDDICreateAllocation(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCheckMonitorPowerState(_In_ CONST D3DKMT_CHECKMONITORPOWERSTATE* unnamedParam1)
{
    DPRINT1("D3DKMTCheckMonitorPowerState: Entry\n");
__debugbreak();
   return NtGdiDdDDICheckMonitorPowerState(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCheckOcclusion(_In_ CONST D3DKMT_CHECKOCCLUSION* unnamedParam1)
{
    DPRINT1("D3DKMTCheckOcclusion: Entry\n");
__debugbreak();
    return NtGdiDdDDICheckOcclusion(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCloseAdapter(_In_ CONST D3DKMT_CLOSEADAPTER* unnamedParam1)
{
    DPRINT1("D3DKMTCloseAdapter: Entry\n");
__debugbreak();
    return NtGdiDdDDICloseAdapter(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCreateContext(_Inout_ D3DKMT_CREATECONTEXT* unnamedParam1)
{
    DPRINT1("D3DKMTCreateContext: Entry\n");
__debugbreak();
    return NtGdiDdDDICreateContext(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCreateDevice(_Inout_ D3DKMT_CREATEDEVICE* unnamedParam1)
{
    DPRINT1("D3DKMTCreateDevice: Entry\n");
__debugbreak();
    return NtGdiDdDDICreateDevice(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCreateOverlay(_Inout_ D3DKMT_CREATEOVERLAY* unnamedParam1)
{
    DPRINT1("D3DKMTCreateOverlay: Entry\n");
__debugbreak();
    return NtGdiDdDDICreateOverlay(unnamedParam1);
}

BOOLEAN
WINAPI
D3DKMTCheckExclusiveOwnership(VOID)
{
    DPRINT1("D3DKMTCheckExclusiveOwnership: Entry\n");
__debugbreak();
    return NtGdiDdDDICheckExclusiveOwnership();
}

NTSTATUS
WINAPI
D3DKMTCreateSynchronizationObject(_Inout_ D3DKMT_CREATESYNCHRONIZATIONOBJECT* unnamedParam1)
{
    DPRINT1("D3DKMTCreateSynchronizationObject: Entry\n");
__debugbreak();
    return NtGdiDdDDICreateSynchronizationObject(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroyAllocation(_In_ CONST D3DKMT_DESTROYALLOCATION* unnamedParam1)
{
    DPRINT1("D3DKMTDestroyAllocation: Entry\n");
__debugbreak();
    return NtGdiDdDDIDestroyAllocation(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroyContext(_In_ CONST D3DKMT_DESTROYCONTEXT* unnamedParam1)
{
    DPRINT1("D3DKMTDestroyContext: Entry\n");
__debugbreak();
    return NtGdiDdDDIDestroyContext(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroyDevice(_In_ CONST D3DKMT_DESTROYDEVICE* unnamedParam1)
{
    DPRINT1("D3DKMTDestroyDevice: Entry\n");
__debugbreak();
    return NtGdiDdDDIDestroyDevice(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroyOverlay(_In_ CONST D3DKMT_DESTROYOVERLAY* unnamedParam1)
{
    DPRINT1("D3DKMTDestroyOverlay: Entry\n");
__debugbreak();
    return NtGdiDdDDIDestroyOverlay(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroySynchronizationObject(_In_ CONST D3DKMT_DESTROYSYNCHRONIZATIONOBJECT* unnamedParam1)
{
    DPRINT1("D3DKMTDestroySynchronizationObject: Entry\n");
__debugbreak();
    return NtGdiDdDDIDestroySynchronizationObject(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTEscape(_In_ CONST D3DKMT_ESCAPE* unnamedParam1)
{
    DPRINT1("D3DKMTEscape: Entry\n");
__debugbreak();
    return NtGdiDdDDIEscape(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTFlipOverlay(_In_ CONST D3DKMT_FLIPOVERLAY* unnamedParam1)
{
    DPRINT1("D3DKMTFlipOverlay: Entry\n");
__debugbreak();
    return NtGdiDdDDIFlipOverlay(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetContextSchedulingPriority(_Inout_ D3DKMT_GETCONTEXTSCHEDULINGPRIORITY* unnamedParam1)
{
    DPRINT1("D3DKMTGetContextSchedulingPriority: Entry\n");
__debugbreak();
    return NtGdiDdDDIGetContextSchedulingPriority(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetDeviceState(_Inout_ D3DKMT_GETDEVICESTATE* unnamedParam1)
{
    DPRINT1("D3DKMTGetDeviceState: Entry\n");
__debugbreak();
    return NtGdiDdDDIGetDeviceState(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetDisplayModeList(_Inout_ D3DKMT_GETDISPLAYMODELIST* unnamedParam1)
{
    DPRINT1("D3DKMTGetDisplayModeList: Entry\n");
__debugbreak();
    return NtGdiDdDDIGetDisplayModeList(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetMultisampleMethodList(_Inout_ D3DKMT_GETMULTISAMPLEMETHODLIST* unnamedParam1)
{
    DPRINT1("D3DKMTGetMultisampleMethodList: Entry\n");
__debugbreak();
    return NtGdiDdDDIGetMultisampleMethodList(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetPresentHistory(_Inout_ D3DKMT_GETPRESENTHISTORY* unnamedParam1)
{
    DPRINT1("D3DKMTGetPresentHistory: Entry\n");
__debugbreak();
    return NtGdiDdDDIGetPresentHistory(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetProcessSchedulingPriorityClass(
        _In_  HANDLE                                unnamedParam1,
        _Out_ D3DKMT_SCHEDULINGPRIORITYCLASS        *unnamedParam2)
{
    DPRINT1("D3DKMTGetProcessSchedulingPriorityClass: Entry\n");
__debugbreak();
    return NtGdiDdDDIGetProcessSchedulingPriorityClass(unnamedParam1, unnamedParam2);
}

NTSTATUS
WINAPI
D3DKMTGetRuntimeData(_In_ CONST D3DKMT_GETRUNTIMEDATA* unnamedParam1)
{
    DPRINT1("D3DKMTGetRuntimeData: Entry\n");
__debugbreak();
    return NtGdiDdDDIGetRuntimeData(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetScanLine(_In_ D3DKMT_GETSCANLINE* unnamedParam1)
{
    DPRINT1("D3DKMTGetScanLine: Entry\n");
__debugbreak();
    return NtGdiDdDDIGetScanLine(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetSharedPrimaryHandle(_Inout_ D3DKMT_GETSHAREDPRIMARYHANDLE* unnamedParam1)
{
    DPRINT1("D3DKMTGetSharedPrimaryHandle: Entry\n");
__debugbreak();
    return NtGdiDdDDIGetSharedPrimaryHandle(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTInvalidateActiveVidPn(_In_ CONST D3DKMT_INVALIDATEACTIVEVIDPN* unnamedParam1)
{
    DPRINT1("D3DKMTInvalidateActiveVidPn: Entry\n");
__debugbreak();
    return NtGdiDdDDIInvalidateActiveVidPn(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTLock(_Inout_ D3DKMT_LOCK* unnamedParam1)
{
    DPRINT1("D3DKMTLock: Entry\n");
__debugbreak();
    return NtGdiDdDDILock(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTOpenAdapterFromDeviceName(_Inout_ D3DKMT_OPENADAPTERFROMDEVICENAME* unnamedParam1)
{
    DPRINT1("D3DKMTOpenAdapterFromDeviceName: Entry\n");
__debugbreak();
    return NtGdiDdDDIOpenAdapterFromDeviceName(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTOpenAdapterFromHdc(_Inout_ D3DKMT_OPENADAPTERFROMHDC* unnamedParam1)
{
    DPRINT1("D3DKMTOpenAdapterFromHdc: Entry\n");
__debugbreak();
    return NtGdiDdDDIOpenAdapterFromHdc(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTOpenResource(_Inout_ D3DKMT_OPENRESOURCE* unnamedParam1)
{
    DPRINT1("D3DKMTOpenResource: Entry\n");
__debugbreak();
    return NtGdiDdDDIOpenResource(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTPollDisplayChildren(_In_ CONST D3DKMT_POLLDISPLAYCHILDREN* unnamedParam1)
{
    DPRINT1("D3DKMTPollDisplayChildren: Entry\n");
__debugbreak();
    return NtGdiDdDDIPollDisplayChildren(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTPresent(_In_ D3DKMT_PRESENT* unnamedParam1)
{
    DPRINT1("D3DKMTPresent: Entry\n");
__debugbreak();
    return NtGdiDdDDIPresent(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTQueryAdapterInfo(_Inout_ CONST D3DKMT_QUERYADAPTERINFO* unnamedParam1)
{
    DPRINT1("D3DKMTQueryAdapterInfo: Entry\n");
__debugbreak();
    return NtGdiDdDDIQueryAdapterInfo(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTQueryAllocationResidency(_In_ CONST D3DKMT_QUERYALLOCATIONRESIDENCY* unnamedParam1)
{
    DPRINT1("D3DKMTQueryAllocationResidency: Entry\n");
__debugbreak();
    return NtGdiDdDDIQueryAllocationResidency(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTQueryResourceInfo(_Inout_ D3DKMT_QUERYRESOURCEINFO* unnamedParam1)
{
    DPRINT1("D3DKMTQueryResourceInfo: Entry\n");
__debugbreak();
    return NtGdiDdDDIQueryResourceInfo(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTQueryStatistics(_Inout_ CONST D3DKMT_QUERYSTATISTICS* unnamedParam1)
{
    DPRINT1("D3DKMTQueryStatistics: Entry\n");
__debugbreak();
    return NtGdiDdDDIQueryStatistics(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTReleaseProcessVidPnSourceOwners(_In_ HANDLE unnamedParam1)
{
    DPRINT1("D3DKMTReleaseProcessVidPnSourceOwners: Entry\n");
__debugbreak();
    return NtGdiDdDDIReleaseProcessVidPnSourceOwners(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTRender(_In_ D3DKMT_RENDER* unnamedParam1)
{
    DPRINT1("D3DKMTRender: Entry\n");
__debugbreak();
    return NtGdiDdDDIRender(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetAllocationPriority(_In_ CONST D3DKMT_SETALLOCATIONPRIORITY* unnamedParam1)
{
    DPRINT1("D3DKMTSetAllocationPriority: Entry\n");
__debugbreak();
    return NtGdiDdDDISetAllocationPriority(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetContextSchedulingPriority(_In_ CONST D3DKMT_SETCONTEXTSCHEDULINGPRIORITY* unnamedParam1)
{
    DPRINT1("D3DKMTSetContextSchedulingPriority: Entry\n");
__debugbreak();
    return NtGdiDdDDISetContextSchedulingPriority(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetDisplayMode(_In_ CONST D3DKMT_SETDISPLAYMODE* unnamedParam1)
{
    DPRINT1("D3DKMTSetDisplayMode: Entry\n");
__debugbreak();
    return NtGdiDdDDISetDisplayMode(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetDisplayPrivateDriverFormat(_In_ CONST D3DKMT_SETDISPLAYPRIVATEDRIVERFORMAT* unnamedParam1)
{
    DPRINT1("D3DKMTSetDisplayPrivateDriverFormat: Entry\n");
__debugbreak();
    return NtGdiDdDDISetDisplayPrivateDriverFormat(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetGammaRamp(_In_ CONST D3DKMT_SETGAMMARAMP* unnamedParam1)
{
    DPRINT1("D3DKMTSetGammaRamp: Entry\n");
__debugbreak();
    return NtGdiDdDDISetGammaRamp(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetProcessSchedulingPriorityClass(
                _In_ HANDLE                                    unnamedParam1,
                _In_ D3DKMT_SCHEDULINGPRIORITYCLASS            unnamedParam2)
{
    DPRINT1("D3DKMTSetProcessSchedulingPriorityClass: Entry\n");
__debugbreak();
    return NtGdiDdDDISetProcessSchedulingPriorityClass(unnamedParam1,unnamedParam2);
}

NTSTATUS
WINAPI
D3DKMTSetQueuedLimit(_Inout_ CONST D3DKMT_SETQUEUEDLIMIT* unnamedParam1)
{
    DPRINT1("D3DKMTSetQueuedLimit: Entry\n");
__debugbreak();
    return NtGdiDdDDISetQueuedLimit(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetVidPnSourceOwner(_In_ CONST D3DKMT_SETVIDPNSOURCEOWNER* unnamedParam1)
{
    DPRINT1("D3DKMTSetVidPnSourceOwner: Entry\n");
__debugbreak();
    return NtGdiDdDDISetVidPnSourceOwner(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSharedPrimaryLockNotification(_In_ CONST D3DKMT_SHAREDPRIMARYLOCKNOTIFICATION* unnamedParam1)
{
    DPRINT1("D3DKMTSharedPrimaryLockNotification: Entry\n");
__debugbreak();
    return NtGdiDdDDISharedPrimaryLockNotification(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSharedPrimaryUnLockNotification(_In_ CONST D3DKMT_SHAREDPRIMARYUNLOCKNOTIFICATION* unnamedParam1)
{
    DPRINT1("D3DKMTSharedPrimaryUnLockNotification: Entry\n");
__debugbreak();
    return NtGdiDdDDISharedPrimaryUnLockNotification(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSignalSynchronizationObject(_In_ CONST D3DKMT_SIGNALSYNCHRONIZATIONOBJECT* unnamedParam1)
{
    DPRINT1("D3DKMTSignalSynchronizationObject: Entry\n");
__debugbreak();
    return NtGdiDdDDISignalSynchronizationObject(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTUnlock(_In_ CONST D3DKMT_UNLOCK* unnamedParam1)
{
    DPRINT1("D3DKMTUnlock: Entry\n");
__debugbreak();
    return NtGdiDdDDIUnlock(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTUpdateOverlay(_In_ CONST D3DKMT_UPDATEOVERLAY* unnamedParam1)
{
    DPRINT1("D3DKMTUpdateOverlay: Entry\n");
__debugbreak();
    return NtGdiDdDDIUpdateOverlay(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTWaitForIdle(_In_ CONST D3DKMT_WAITFORIDLE* unnamedParam1)
{
    DPRINT1("D3DKMTWaitForIdle: Entry\n");
__debugbreak();
    return NtGdiDdDDIWaitForIdle(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTWaitForSynchronizationObject(_In_ CONST D3DKMT_WAITFORSYNCHRONIZATIONOBJECT* unnamedParam1)
{
    DPRINT1("D3DKMTWaitForSynchronizationObject: Entry\n");
__debugbreak();
    return NtGdiDdDDIWaitForSynchronizationObject(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTWaitForVerticalBlankEvent(_In_ CONST D3DKMT_WAITFORVERTICALBLANKEVENT* unnamedParam1)
{
    DPRINT1("D3DKMTWaitForVerticalBlankEvent: Entry\n");
__debugbreak();
    return NtGdiDdDDIWaitForVerticalBlankEvent(unnamedParam1);
}
