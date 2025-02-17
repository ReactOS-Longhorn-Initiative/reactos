/*
 * PROJECT:     ReactOS Display Driver Model
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     
 * COPYRIGHT:   Copyright 2025 Justin Miller <justin.miller@reactos.org>
 */

 #include <win32k.h>
 #include <reactos/rddm/rxgkinterface.h>
 #include <debug.h>

/* 
 * Why?
 * There's a few reasons this exists, a bit of history is required:
 * Vista (NT6.0) had the ability to run both XDDM and WDDM drivers, when XDDM was active
 * Every D3DKMT api returns `STATUS_PROCEDURE_NOT_FOUND` as the real logic to the syscalls live inside dxgkrnl
 * leaving win32k to do zero real work. 
 * Win7 extended this 
 * Win8 (NT6.2) introduced basicrenderer.sys and basicdisplay.sys meaning a basic set of KMT APIs
 * always existed as they'd be filed out by the RendererOnlyDisplayDriver (basicrenderer.sys)
 * WINE 3+ started using these KMT apis for tiny little pieces of logic, and by WINE 8.x they required all the 
 * KMT apis filled out by basic display
 * The issue is Win8 completely kills off XDDM so we have a very quirky hack
 * 
 * Win32k will have logic (this file) to act out what basicrenderer.sys does in Windows via
 * calling into XDDM or just internal tracking.
 */
NTSTATUS
NTAPI
XDDMWrapSetVidPnSourceOwner(_In_ const D3DKMT_SETVIDPNSOURCEOWNER* unnamedParam1)
{
    DPRINT1("XDDMWrapSetVidPnSourceOwner Entry\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
XDDMWrapCloseAdapter(_In_ const D3DKMT_CLOSEADAPTER *desc )
{
    DPRINT1("XDDMWrapCloseAdapter Entry\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
XDDMWrapCreateDevice(_Inout_ D3DKMT_CREATEDEVICE* unnamedParam1)
{
    DPRINT1("XDDMWrapCreateDevice Entry\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
XDDMWrapDestroyDevice(_In_ const D3DKMT_DESTROYDEVICE* unnamedParam1)
{
    DPRINT1("XDDMWrapDestroyDevice Entry\n");
    return STATUS_SUCCESS;
}