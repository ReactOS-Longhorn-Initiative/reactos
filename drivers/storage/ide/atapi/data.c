/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Global pageable data
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

const WCHAR* const AtapTargetToDeviceTypeKey[] =
{
    L"MasterDeviceType", L"SlaveDeviceType",
#if defined(_M_IX86)
    L"MasterDeviceType2", L"SlaveDeviceType2"
#endif
};

static
const ATAPORT_DEVICE_NAME AtapGenericDeviceNames[] =
{
    /* Device Type Generic Type     Peripheral ID             */
    {"Disk",       "GenDisk",       "DiskPeripheral"           }, // DIRECT_ACCESS_DEVICE
    {"Sequential", "GenSequential", "TapePeripheral"           }, // SEQUENTIAL_ACCESS_DEVICE
    {"Printer",    "GenPrinter",    "PrinterPeripheral"        }, // PRINTER_DEVICE
    /* There is no 'ProcessorPeripheral' */
    {"Processor",  "GenProcessor",  "OtherPeripheral"          }, // PROCESSOR_DEVICE
    {"Worm",       "GenWorm",       "WormPeripheral"           }, // WRITE_ONCE_READ_MULTIPLE_DEVICE
    {"CdRom",      "GenCdRom",      "CdRomPeripheral"          }, // READ_ONLY_DIRECT_ACCESS_DEVICE
    {"Scanner",    "GenScanner",    "ScannerPeripheral"        }, // SCANNER_DEVICE
    {"Optical",    "GenOptical",    "OpticalDiskPeripheral"    }, // OPTICAL_DEVICE
    {"Changer",    "GenChanger",    "MediumChangerPeripheral"  }, // MEDIUM_CHANGER
    {"Net",        "GenNet",        "CommunicationsPeripheral" }, // COMMUNICATION_DEVICE,
    {"Other",      "GenOther",      "OtherPeripheral"          }, // 10
};

/* FUNCTIONS ******************************************************************/

CODE_SEG("PAGE")
PCSTR
AtaTypeCodeToName(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ DEVICE_TYPE_NAME Type)
{
    UCHAR DeviceType = DeviceExtension->InquiryData.DeviceType;

    PAGED_CODE();

    switch (Type)
    {
        case GetDeviceType:
        {
            if (DeviceType < RTL_NUMBER_OF(AtapGenericDeviceNames))
                return AtapGenericDeviceNames[DeviceType].DeviceType;
            else
                return "Other";
        }
        case GetGenericType:
        {
            if (DeviceExtension->Flags & DEVICE_IS_SUPER_FLOPPY)
                return "GenSFloppy";
            else if (DeviceType < RTL_NUMBER_OF(AtapGenericDeviceNames))
                return AtapGenericDeviceNames[DeviceType].GenericType;
            else
                return "IdeOther";
        }
        case GetPeripheralId:
        {
            if (DeviceType < RTL_NUMBER_OF(AtapGenericDeviceNames))
                return AtapGenericDeviceNames[DeviceType].PeripheralId;
            else
                return "OtherPeripheral";
        }

        DEFAULT_UNREACHABLE;
    }
}
