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
    return NtGdiDdDDICreateAllocation(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCheckMonitorPowerState(_In_ CONST D3DKMT_CHECKMONITORPOWERSTATE* unnamedParam1)
{
    DPRINT1("D3DKMTCheckMonitorPowerState: Entry\n");
   return NtGdiDdDDICheckMonitorPowerState(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCheckOcclusion(_In_ CONST D3DKMT_CHECKOCCLUSION* unnamedParam1)
{
    DPRINT1("D3DKMTCheckOcclusion: Entry\n");
    return NtGdiDdDDICheckOcclusion(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCloseAdapter(_In_ CONST D3DKMT_CLOSEADAPTER* unnamedParam1)
{
    DPRINT1("D3DKMTCloseAdapter: Entry\n");
    return NtGdiDdDDICloseAdapter(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCreateContext(_Inout_ D3DKMT_CREATECONTEXT* unnamedParam1)
{
    DPRINT1("D3DKMTCreateContext: Entry\n");
    return NtGdiDdDDICreateContext(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCreateDevice(_Inout_ D3DKMT_CREATEDEVICE* unnamedParam1)
{
    DPRINT1("D3DKMTCreateDevice: Entry\n");
    return NtGdiDdDDICreateDevice(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCreateOverlay(_Inout_ D3DKMT_CREATEOVERLAY* unnamedParam1)
{
    DPRINT1("D3DKMTCreateOverlay: Entry\n");
    return NtGdiDdDDICreateOverlay(unnamedParam1);
}

BOOLEAN
WINAPI
D3DKMTCheckExclusiveOwnership(VOID)
{
    DPRINT1("D3DKMTCheckExclusiveOwnership: Entry\n");
    return NtGdiDdDDICheckExclusiveOwnership();
}

NTSTATUS
WINAPI
D3DKMTCreateSynchronizationObject(_Inout_ D3DKMT_CREATESYNCHRONIZATIONOBJECT* unnamedParam1)
{
    DPRINT1("D3DKMTCreateSynchronizationObject: Entry\n");
    return NtGdiDdDDICreateSynchronizationObject(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroyAllocation(_In_ CONST D3DKMT_DESTROYALLOCATION* unnamedParam1)
{
    DPRINT1("D3DKMTDestroyAllocation: Entry\n");
    return NtGdiDdDDIDestroyAllocation(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroyContext(_In_ CONST D3DKMT_DESTROYCONTEXT* unnamedParam1)
{
    DPRINT1("D3DKMTDestroyContext: Entry\n");
    return NtGdiDdDDIDestroyContext(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroyDevice(_In_ CONST D3DKMT_DESTROYDEVICE* unnamedParam1)
{
    DPRINT1("D3DKMTDestroyDevice: Entry\n");
    return NtGdiDdDDIDestroyDevice(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroyOverlay(_In_ CONST D3DKMT_DESTROYOVERLAY* unnamedParam1)
{
    DPRINT1("D3DKMTDestroyOverlay: Entry\n");
    return NtGdiDdDDIDestroyOverlay(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroySynchronizationObject(_In_ CONST D3DKMT_DESTROYSYNCHRONIZATIONOBJECT* unnamedParam1)
{
    DPRINT1("D3DKMTDestroySynchronizationObject: Entry\n");
    return NtGdiDdDDIDestroySynchronizationObject(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTEscape(_In_ CONST D3DKMT_ESCAPE* unnamedParam1)
{
    DPRINT1("D3DKMTEscape: Entry\n");
    return NtGdiDdDDIEscape(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTFlipOverlay(_In_ CONST D3DKMT_FLIPOVERLAY* unnamedParam1)
{
    DPRINT1("D3DKMTFlipOverlay: Entry\n");
    return NtGdiDdDDIFlipOverlay(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetContextSchedulingPriority(_Inout_ D3DKMT_GETCONTEXTSCHEDULINGPRIORITY* unnamedParam1)
{
    DPRINT1("D3DKMTGetContextSchedulingPriority: Entry\n");
    return NtGdiDdDDIGetContextSchedulingPriority(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetDeviceState(_Inout_ D3DKMT_GETDEVICESTATE* unnamedParam1)
{
    DPRINT1("D3DKMTGetDeviceState: Entry\n");
    return NtGdiDdDDIGetDeviceState(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetDisplayModeList(_Inout_ D3DKMT_GETDISPLAYMODELIST* unnamedParam1)
{
    DPRINT1("D3DKMTGetDisplayModeList: Entry\n");
    return NtGdiDdDDIGetDisplayModeList(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetMultisampleMethodList(_Inout_ D3DKMT_GETMULTISAMPLEMETHODLIST* unnamedParam1)
{
    DPRINT1("D3DKMTGetMultisampleMethodList: Entry\n");
    return NtGdiDdDDIGetMultisampleMethodList(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetPresentHistory(_Inout_ D3DKMT_GETPRESENTHISTORY* unnamedParam1)
{
    DPRINT1("D3DKMTGetPresentHistory: Entry\n");
    return NtGdiDdDDIGetPresentHistory(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetProcessSchedulingPriorityClass(
        _In_  HANDLE                                unnamedParam1,
        _Out_ D3DKMT_SCHEDULINGPRIORITYCLASS        *unnamedParam2)
{
    DPRINT1("D3DKMTGetProcessSchedulingPriorityClass: Entry\n");
    return NtGdiDdDDIGetProcessSchedulingPriorityClass(unnamedParam1, unnamedParam2);
}

NTSTATUS
WINAPI
D3DKMTGetRuntimeData(_In_ CONST D3DKMT_GETRUNTIMEDATA* unnamedParam1)
{
    DPRINT1("D3DKMTGetRuntimeData: Entry\n");
    return NtGdiDdDDIGetRuntimeData(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetScanLine(_In_ D3DKMT_GETSCANLINE* unnamedParam1)
{
    DPRINT1("D3DKMTGetScanLine: Entry\n");
    return NtGdiDdDDIGetScanLine(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetSharedPrimaryHandle(_Inout_ D3DKMT_GETSHAREDPRIMARYHANDLE* unnamedParam1)
{
    DPRINT1("D3DKMTGetSharedPrimaryHandle: Entry\n");
    return NtGdiDdDDIGetSharedPrimaryHandle(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTInvalidateActiveVidPn(_In_ CONST D3DKMT_INVALIDATEACTIVEVIDPN* unnamedParam1)
{
    DPRINT1("D3DKMTInvalidateActiveVidPn: Entry\n");
    return NtGdiDdDDIInvalidateActiveVidPn(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTLock(_Inout_ D3DKMT_LOCK* unnamedParam1)
{
    DPRINT1("D3DKMTLock: Entry\n");
    return NtGdiDdDDILock(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTOpenAdapterFromDeviceName(_Inout_ D3DKMT_OPENADAPTERFROMDEVICENAME* unnamedParam1)
{
    DPRINT1("D3DKMTOpenAdapterFromDeviceName: Entry\n");
    return NtGdiDdDDIOpenAdapterFromDeviceName(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTOpenAdapterFromGdiDisplayName(_Inout_ D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME* unnamedParam1)
{
    DPRINT1("D3DKMTOpenAdapterFromGdiDisplayName: Entry\n");
    return NtGdiDdDDIOpenAdapterFromGdiDisplayName(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTOpenAdapterFromHdc(_Inout_ D3DKMT_OPENADAPTERFROMHDC* unnamedParam1)
{
    DPRINT1("D3DKMTOpenAdapterFromHdc: Entry\n");
    return NtGdiDdDDIOpenAdapterFromHdc(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTOpenResource(_Inout_ D3DKMT_OPENRESOURCE* unnamedParam1)
{
    DPRINT1("D3DKMTOpenResource: Entry\n");
    return NtGdiDdDDIOpenResource(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTPollDisplayChildren(_In_ CONST D3DKMT_POLLDISPLAYCHILDREN* unnamedParam1)
{
    DPRINT1("D3DKMTPollDisplayChildren: Entry\n");
    return NtGdiDdDDIPollDisplayChildren(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTPresent(_In_ D3DKMT_PRESENT* unnamedParam1)
{
    DPRINT1("D3DKMTPresent: Entry\n");
    return NtGdiDdDDIPresent(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTQueryAdapterInfo(_Inout_ CONST D3DKMT_QUERYADAPTERINFO* unnamedParam1)
{
    DPRINT1("D3DKMTQueryAdapterInfo: Entry\n");
    return NtGdiDdDDIQueryAdapterInfo(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTQueryAllocationResidency(_In_ CONST D3DKMT_QUERYALLOCATIONRESIDENCY* unnamedParam1)
{
    DPRINT1("D3DKMTQueryAllocationResidency: Entry\n");
    return NtGdiDdDDIQueryAllocationResidency(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTQueryResourceInfo(_Inout_ D3DKMT_QUERYRESOURCEINFO* unnamedParam1)
{
    DPRINT1("D3DKMTQueryResourceInfo: Entry\n");
    return NtGdiDdDDIQueryResourceInfo(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTQueryStatistics(_Inout_ CONST D3DKMT_QUERYSTATISTICS* unnamedParam1)
{
    DPRINT1("D3DKMTQueryStatistics: Entry\n");
    return NtGdiDdDDIQueryStatistics(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTReleaseProcessVidPnSourceOwners(_In_ HANDLE unnamedParam1)
{
    DPRINT1("D3DKMTReleaseProcessVidPnSourceOwners: Entry\n");
    return NtGdiDdDDIReleaseProcessVidPnSourceOwners(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTRender(_In_ D3DKMT_RENDER* unnamedParam1)
{
    DPRINT1("D3DKMTRender: Entry\n");
    return NtGdiDdDDIRender(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetAllocationPriority(_In_ CONST D3DKMT_SETALLOCATIONPRIORITY* unnamedParam1)
{
    DPRINT1("D3DKMTSetAllocationPriority: Entry\n");
    return NtGdiDdDDISetAllocationPriority(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetContextSchedulingPriority(_In_ CONST D3DKMT_SETCONTEXTSCHEDULINGPRIORITY* unnamedParam1)
{
    DPRINT1("D3DKMTSetContextSchedulingPriority: Entry\n");
    return NtGdiDdDDISetContextSchedulingPriority(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetDisplayMode(_In_ CONST D3DKMT_SETDISPLAYMODE* unnamedParam1)
{
    DPRINT1("D3DKMTSetDisplayMode: Entry\n");
    return NtGdiDdDDISetDisplayMode(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetDisplayPrivateDriverFormat(_In_ CONST D3DKMT_SETDISPLAYPRIVATEDRIVERFORMAT* unnamedParam1)
{
    DPRINT1("D3DKMTSetDisplayPrivateDriverFormat: Entry\n");
    return NtGdiDdDDISetDisplayPrivateDriverFormat(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetGammaRamp(_In_ CONST D3DKMT_SETGAMMARAMP* unnamedParam1)
{
    DPRINT1("D3DKMTSetGammaRamp: Entry\n");
    return NtGdiDdDDISetGammaRamp(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetProcessSchedulingPriorityClass(
                _In_ HANDLE                                    unnamedParam1,
                _In_ D3DKMT_SCHEDULINGPRIORITYCLASS            unnamedParam2)
{
    DPRINT1("D3DKMTSetProcessSchedulingPriorityClass: Entry\n");
    return NtGdiDdDDISetProcessSchedulingPriorityClass(unnamedParam1,unnamedParam2);
}

NTSTATUS
WINAPI
D3DKMTSetQueuedLimit(_Inout_ CONST D3DKMT_SETQUEUEDLIMIT* unnamedParam1)
{
    DPRINT1("D3DKMTSetQueuedLimit: Entry\n");
    return NtGdiDdDDISetQueuedLimit(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetVidPnSourceOwner(_In_ CONST D3DKMT_SETVIDPNSOURCEOWNER* unnamedParam1)
{
    DPRINT1("D3DKMTSetVidPnSourceOwner: Entry\n");
    return NtGdiDdDDISetVidPnSourceOwner(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSharedPrimaryLockNotification(_In_ CONST D3DKMT_SHAREDPRIMARYLOCKNOTIFICATION* unnamedParam1)
{
    DPRINT1("D3DKMTSharedPrimaryLockNotification: Entry\n");
    return NtGdiDdDDISharedPrimaryLockNotification(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSharedPrimaryUnLockNotification(_In_ CONST D3DKMT_SHAREDPRIMARYUNLOCKNOTIFICATION* unnamedParam1)
{
    DPRINT1("D3DKMTSharedPrimaryUnLockNotification: Entry\n");
    return NtGdiDdDDISharedPrimaryUnLockNotification(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSignalSynchronizationObject(_In_ CONST D3DKMT_SIGNALSYNCHRONIZATIONOBJECT* unnamedParam1)
{
    DPRINT1("D3DKMTSignalSynchronizationObject: Entry\n");
    return NtGdiDdDDISignalSynchronizationObject(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTUnlock(_In_ CONST D3DKMT_UNLOCK* unnamedParam1)
{
    DPRINT1("D3DKMTUnlock: Entry\n");
    return NtGdiDdDDIUnlock(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTUpdateOverlay(_In_ CONST D3DKMT_UPDATEOVERLAY* unnamedParam1)
{
    DPRINT1("D3DKMTUpdateOverlay: Entry\n");
    return NtGdiDdDDIUpdateOverlay(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTWaitForIdle(_In_ CONST D3DKMT_WAITFORIDLE* unnamedParam1)
{
    DPRINT1("D3DKMTWaitForIdle: Entry\n");
    return NtGdiDdDDIWaitForIdle(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTWaitForSynchronizationObject(_In_ CONST D3DKMT_WAITFORSYNCHRONIZATIONOBJECT* unnamedParam1)
{
    DPRINT1("D3DKMTWaitForSynchronizationObject: Entry\n");
    return NtGdiDdDDIWaitForSynchronizationObject(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTWaitForVerticalBlankEvent(_In_ CONST D3DKMT_WAITFORVERTICALBLANKEVENT* unnamedParam1)
{
    DPRINT1("D3DKMTWaitForVerticalBlankEvent: Entry\n");
    return NtGdiDdDDIWaitForVerticalBlankEvent(unnamedParam1);
}
