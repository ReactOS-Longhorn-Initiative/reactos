/*
 * PROJECT:     ReactOS Display Driver Model
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Core Header
 * COPYRIGHT:   Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#include <ntddk.h>
#include <windef.h>
#include <ntstatus.h>
#include <stdio.h>
#include <ntddvdeo.h>
#include <dispmprt.h>
#include <reactos/rddm/rddm_private.h>
typedef struct _RXGK_PRIVATE_EXTENSION
{
    // Driver Data
    PDRIVER_OBJECT MiniportDriverObject;
    ULONG InternalDeviceNumber;
    PDRIVER_OBJECT DriverObject;
    PDEVICE_OBJECT MiniportFdo;
    PDEVICE_OBJECT MiniportPdo;
    INTERFACE_TYPE AdapterInterfaceType;
    PVOID MiniportContext;
    UNICODE_STRING RegistryPath;
    UNICODE_STRING NewRegistryPath;
    PDEVICE_OBJECT NextDeviceObject;
    ULONG SystemIoBusNumber; // ACPI or PCI Currently
    ULONG SystemIoSlotNumber; // ACPI or PCI Currently
    KDPC DpcObject;
    // Driver PFNs
    ULONG                                    Version;
    PDXGKDDI_ADD_DEVICE                      DxgkDdiAddDevice;
    PDXGKDDI_START_DEVICE                    DxgkDdiStartDevice;
    PDXGKDDI_STOP_DEVICE                     DxgkDdiStopDevice;
    PDXGKDDI_REMOVE_DEVICE                   DxgkDdiRemoveDevice;
    PDXGKDDI_DISPATCH_IO_REQUEST             DxgkDdiDispatchIoRequest;
    PDXGKDDI_INTERRUPT_ROUTINE               DxgkDdiInterruptRoutine;
    PDXGKDDI_DPC_ROUTINE                     DxgkDdiDpcRoutine;
} RXGK_PRIVATE_EXTENSION, *PRXGK_PRIVATE_EXTENSION;

#include "include/rxgkport.h"
