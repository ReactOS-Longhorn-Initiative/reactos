/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     WPP Trace Recorder
 */

/* INCLUDES ******************************************************************/

#include <wdm.h>
#include <debug.h>
#include "wpprecorder.h"

VOID 
WppAutoLogStart(
    _In_ PVOID WppCb,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PCUNICODE_STRING RegistryPath)
{
	UNIMPLEMENTED;
}

VOID 
WppAutoLogStop(
    _In_ PVOID WppCb,
    _In_ PDRIVER_OBJECT DriverObject)
{
	UNIMPLEMENTED;
}

NTSTATUS 
WppAutoLogTrace(
    _In_ PVOID AutoLogContext,
    _In_ UCHAR MessageLevel,
    _In_ ULONG MessageFlags,
    _In_ LPGUID MessageGuid,
    _In_ USHORT MessageNumber,
    _In_ UCHAR Unknown1,
    _In_ ULONG Unknown2)
{
	UNIMPLEMENTED_ONCE;
	return STATUS_SUCCESS;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
imp_WppRecorderConfigure(
    _In_ PVOID WppCb,
    _In_ PRECORDER_CONFIGURE_PARAMS ConfigureParams)
{
	UNIMPLEMENTED;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
imp_WppRecorderLogDumpLiveData(
    _In_ PVOID WppCb,
    _Out_ PVOID *OutBuffer,
    _Out_ PULONG OutBufferLength,
    _Out_ LPGUID Guid)
{
	UNIMPLEMENTED;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
imp_WppRecorderDumpLiveDriverData(
    _In_ PVOID WppCb,
    _Out_ PVOID *OutBuffer,
    _Out_ PULONG OutBufferLength,
    _Out_ LPGUID Guid)
{
	UNIMPLEMENTED;
}

WPP_RECORDER_COUNTER
imp_WppRecorderGetCounterHandle(_In_ PVOID WppCb)
{
	UNIMPLEMENTED;
	return NULL;
}

RECORDER_LOG
imp_WppRecorderLogGetDefault(_In_ PVOID WppCb)
{
	UNIMPLEMENTED;
	return NULL;
}

__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
imp_WppRecorderGetTriageInfo(
    _In_ PVOID WppCb,
    _Out_ PWPP_TRIAGE_INFO WppTriageInfo)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(PASSIVE_LEVEL)
BOOLEAN
imp_WppRecorderIsDefaultLogAvailable(_In_ PVOID WppCb)
{
	UNIMPLEMENTED;
	return FALSE;
}

__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
imp_WppRecorderLinkCounters(
    _In_ PVOID WppCb,
    _Out_ WPP_RECORDER_COUNTER CounterOwner)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
imp_WppRecorderLogCreate(
    _In_ PVOID WppCb,
    _In_ PRECORDER_LOG_CREATE_PARAMS CreateParams,
    _Out_ RECORDER_LOG *RecorderLog)
{
	UNIMPLEMENTED_ONCE;
	/* fake success for usbhub3 */
	return STATUS_SUCCESS;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
imp_WppRecorderLogDelete(
    _In_ PVOID WppCb,
    _In_ RECORDER_LOG RecorderLog)
{
	UNIMPLEMENTED;
}

__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
imp_WppRecorderLogSetIdentifier(
    _In_ PVOID WppCb,
    _In_ RECORDER_LOG RecorderLog,
    _In_ PSTR LogIdentifier)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    /* Page the driver */
    MmPageEntireDriver(DriverEntry);

    /* Return success */
    return STATUS_SUCCESS;
}

/* EOF */
