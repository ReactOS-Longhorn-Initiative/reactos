/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/iomgr/iorsrce.c
 * PURPOSE:         Hardware resource managment
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Alex Ionescu (alex@relsoft.net)
 *                  Pierre Schweitzer (pierre.schweitzer@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include "../pnpio.h"

//#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern PCM_RESOURCE_LIST IopInitHalResources;

static CONFIGURATION_INFORMATION
_SystemConfigurationInformation = { 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, 0 };

/* API Parameters to Pass in IopQueryBusDescription */
typedef struct IO_QUERY {
    PINTERFACE_TYPE  BusType;
    PULONG  BusNumber;
    PCONFIGURATION_TYPE  ControllerType;
    PULONG  ControllerNumber;
    PCONFIGURATION_TYPE  PeripheralType;
    PULONG  PeripheralNumber;
    PIO_QUERY_DEVICE_ROUTINE  CalloutRoutine;
    PVOID  Context;
} IO_QUERY, *PIO_QUERY;

PWSTR ArcTypes[42] = {
    L"System",
    L"CentralProcessor",
    L"FloatingPointProcessor",
    L"PrimaryICache",
    L"PrimaryDCache",
    L"SecondaryICache",
    L"SecondaryDCache",
    L"SecondaryCache",
    L"EisaAdapter",
    L"TcAdapter",
    L"ScsiAdapter",
    L"DtiAdapter",
    L"MultifunctionAdapter",
    L"DiskController",
    L"TapeController",
    L"CdRomController",
    L"WormController",
    L"SerialController",
    L"NetworkController",
    L"DisplayController",
    L"ParallelController",
    L"PointerController",
    L"KeyboardController",
    L"AudioController",
    L"OtherController",
    L"DiskPeripheral",
    L"FloppyDiskPeripheral",
    L"TapePeripheral",
    L"ModemPeripheral",
    L"MonitorPeripheral",
    L"PrinterPeripheral",
    L"PointerPeripheral",
    L"KeyboardPeripheral",
    L"TerminalPeripheral",
    L"OtherPeripheral",
    L"LinePeripheral",
    L"NetworkPeripheral",
    L"SystemMemory",
    L"DockingInformation",
    L"RealModeIrqRoutingTable",
    L"RealModePCIEnumeration",
    L"Undefined"
};

/* PRIVATE FUNCTIONS **********************************************************/

/*
 * IopQueryDeviceDescription
 *
 * FUNCTION:
 *     Reads and returns Hardware information from the appropriate hardware
 *     registry key. Helper sub of IopQueryBusDescription.
 *
 * ARGUMENTS:
 *     Query          - What the parent function wants.
 *     RootKey        - Which key to look in
 *     RootKeyHandle  - Handle to the key
 *     Bus            - Bus Number.
 *     BusInformation - The Configuration Information Sent
 *
 * RETURNS:
 *      Status
 */

NTSTATUS NTAPI
IopQueryDeviceDescription(
   PIO_QUERY Query,
   UNICODE_STRING RootKey,
   HANDLE RootKeyHandle,
   ULONG Bus,
   PKEY_VALUE_FULL_INFORMATION *BusInformation)
{
   NTSTATUS Status = STATUS_SUCCESS;

   /* Controller Stuff */
   UNICODE_STRING ControllerString;
   UNICODE_STRING ControllerRootRegName = RootKey;
   UNICODE_STRING ControllerRegName;
   HANDLE ControllerKeyHandle;
   PKEY_FULL_INFORMATION ControllerFullInformation = NULL;
   PKEY_VALUE_FULL_INFORMATION ControllerInformation[3] = {NULL, NULL, NULL};
   ULONG ControllerNumber;
   ULONG ControllerLoop;
   ULONG MaximumControllerNumber;

   /* Peripheral Stuff */
   UNICODE_STRING PeripheralString;
   HANDLE PeripheralKeyHandle;
   PKEY_FULL_INFORMATION PeripheralFullInformation;
   PKEY_VALUE_FULL_INFORMATION PeripheralInformation[3] = {NULL, NULL, NULL};
   ULONG PeripheralNumber;
   ULONG PeripheralLoop;
   ULONG MaximumPeripheralNumber;

   /* Global Registry Stuff */
   OBJECT_ATTRIBUTES ObjectAttributes;
   ULONG LenFullInformation;
   ULONG LenKeyFullInformation;
   UNICODE_STRING TempString;
   WCHAR TempBuffer[14];
   PWSTR Strings[3] = {
      L"Identifier",
      L"Configuration Data",
      L"Component Information"
   };

   /* Temporary String */
   TempString.MaximumLength = sizeof(TempBuffer);
   TempString.Length = 0;
   TempString.Buffer = TempBuffer;

   /* Add Controller Name to String */
   RtlAppendUnicodeToString(&ControllerRootRegName, L"\\");
   RtlAppendUnicodeToString(&ControllerRootRegName, ArcTypes[*Query->ControllerType]);

   /* Set the Controller Number if specified */
   if (Query->ControllerNumber && *(Query->ControllerNumber))
   {
      ControllerNumber = *Query->ControllerNumber;
      MaximumControllerNumber = ControllerNumber + 1;
   } else {
      /* Find out how many Controller Numbers there are */
      InitializeObjectAttributes(
         &ObjectAttributes,
         &ControllerRootRegName,
         OBJ_CASE_INSENSITIVE,
         NULL,
         NULL);

      Status = ZwOpenKey(&ControllerKeyHandle, KEY_READ, &ObjectAttributes);
      if (NT_SUCCESS(Status))
      {
         /* How much buffer space */
         ZwQueryKey(ControllerKeyHandle, KeyFullInformation, NULL, 0, &LenFullInformation);

         /* Allocate it */
         ControllerFullInformation = ExAllocatePoolWithTag(PagedPool, LenFullInformation, TAG_IO_RESOURCE);

         /* Get the Information */
         Status = ZwQueryKey(ControllerKeyHandle, KeyFullInformation, ControllerFullInformation, LenFullInformation, &LenFullInformation);
         ZwClose(ControllerKeyHandle);
         ControllerKeyHandle = NULL;
      }

      /* No controller was found, go back to function. */
      if (!NT_SUCCESS(Status))
      {
         if (ControllerFullInformation != NULL)
            ExFreePoolWithTag(ControllerFullInformation, TAG_IO_RESOURCE);
         return Status;
      }

      /* Find out Controller Numbers */
      ControllerNumber = 0;
      MaximumControllerNumber = ControllerFullInformation->SubKeys;

      /* Free Memory */
      ExFreePoolWithTag(ControllerFullInformation, TAG_IO_RESOURCE);
      ControllerFullInformation = NULL;
   }

   /* Save String */
   ControllerRegName = ControllerRootRegName;

   /* Loop through controllers */
   for (; ControllerNumber < MaximumControllerNumber; ControllerNumber++)
   {
      /* Load String */
      ControllerRootRegName = ControllerRegName;

      /* Controller Number to Registry String */
      Status = RtlIntegerToUnicodeString(ControllerNumber, 10, &TempString);

      /* Create String */
      Status |= RtlAppendUnicodeToString(&ControllerRootRegName, L"\\");
      Status |= RtlAppendUnicodeStringToString(&ControllerRootRegName, &TempString);

      /* Something messed up */
      if (!NT_SUCCESS(Status)) break;

      /* Open the Registry Key */
      InitializeObjectAttributes(
         &ObjectAttributes,
         &ControllerRootRegName,
         OBJ_CASE_INSENSITIVE,
         NULL,
         NULL);

      Status = ZwOpenKey(&ControllerKeyHandle, KEY_READ, &ObjectAttributes);

      /* Read the Configuration Data... */
      if (NT_SUCCESS(Status))
      {
         for (ControllerLoop = 0; ControllerLoop < 3; ControllerLoop++)
         {
            /* Identifier String First */
            RtlInitUnicodeString(&ControllerString, Strings[ControllerLoop]);

            /* How much buffer space */
            Status = ZwQueryValueKey(ControllerKeyHandle, &ControllerString, KeyValueFullInformation, NULL, 0, &LenKeyFullInformation);

            if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL && Status != STATUS_BUFFER_OVERFLOW)
               continue;

            /* Allocate it */
            ControllerInformation[ControllerLoop] = ExAllocatePoolWithTag(PagedPool, LenKeyFullInformation, TAG_IO_RESOURCE);

            /* Get the Information */
            Status = ZwQueryValueKey(ControllerKeyHandle, &ControllerString, KeyValueFullInformation, ControllerInformation[ControllerLoop], LenKeyFullInformation, &LenKeyFullInformation);
         }

         /* Clean Up */
         ZwClose(ControllerKeyHandle);
         ControllerKeyHandle = NULL;
      }

      /* Something messed up */
      if (!NT_SUCCESS(Status))
         goto EndLoop;

      /* We now have Bus *AND* Controller Information.. is it enough? */
      if (!Query->PeripheralType || !(*Query->PeripheralType))
      {
         Status = Query->CalloutRoutine(
            Query->Context,
            &ControllerRootRegName,
            *Query->BusType,
            Bus,
            BusInformation,
            *Query->ControllerType,
            ControllerNumber,
            ControllerInformation,
            0,
            0,
            NULL);
         goto EndLoop;
      }

      /* Not enough...caller also wants peripheral name */
      Status = RtlAppendUnicodeToString(&ControllerRootRegName, L"\\");
      Status |= RtlAppendUnicodeToString(&ControllerRootRegName, ArcTypes[*Query->PeripheralType]);

      /* Something messed up */
      if (!NT_SUCCESS(Status)) goto EndLoop;

      /* Set the Peripheral Number if specified */
      if (Query->PeripheralNumber && *Query->PeripheralNumber)
      {
         PeripheralNumber = *Query->PeripheralNumber;
         MaximumPeripheralNumber = PeripheralNumber + 1;
      } else {
         /* Find out how many Peripheral Numbers there are */
         InitializeObjectAttributes(
            &ObjectAttributes,
            &ControllerRootRegName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

         Status = ZwOpenKey(&PeripheralKeyHandle, KEY_READ, &ObjectAttributes);

         if (NT_SUCCESS(Status))
         {
            /* How much buffer space */
            ZwQueryKey(PeripheralKeyHandle, KeyFullInformation, NULL, 0, &LenFullInformation);

            /* Allocate it */
            PeripheralFullInformation = ExAllocatePoolWithTag(PagedPool, LenFullInformation, TAG_IO_RESOURCE);

            /* Get the Information */
            Status = ZwQueryKey(PeripheralKeyHandle, KeyFullInformation, PeripheralFullInformation, LenFullInformation, &LenFullInformation);
            ZwClose(PeripheralKeyHandle);
            PeripheralKeyHandle = NULL;
         }

         /* No controller was found, go back to function but clean up first */
         if (!NT_SUCCESS(Status))
         {
            Status = STATUS_SUCCESS;
            goto EndLoop;
         }

         /* Find out Peripheral Number */
         PeripheralNumber = 0;
         MaximumPeripheralNumber = PeripheralFullInformation->SubKeys;

         /* Free Memory */
         ExFreePoolWithTag(PeripheralFullInformation, TAG_IO_RESOURCE);
         PeripheralFullInformation = NULL;
      }

      /* Save Name */
      ControllerRegName = ControllerRootRegName;

      /* Loop through Peripherals */
      for (; PeripheralNumber < MaximumPeripheralNumber; PeripheralNumber++)
      {
         /* Restore Name */
         ControllerRootRegName = ControllerRegName;

         /* Peripheral Number to Registry String */
         Status = RtlIntegerToUnicodeString(PeripheralNumber, 10, &TempString);

         /* Create String */
         Status |= RtlAppendUnicodeToString(&ControllerRootRegName, L"\\");
         Status |= RtlAppendUnicodeStringToString(&ControllerRootRegName, &TempString);

         /* Something messed up */
         if (!NT_SUCCESS(Status)) break;

         /* Open the Registry Key */
         InitializeObjectAttributes(
            &ObjectAttributes,
            &ControllerRootRegName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

         Status = ZwOpenKey(&PeripheralKeyHandle, KEY_READ, &ObjectAttributes);

         if (NT_SUCCESS(Status))
         {
            for (PeripheralLoop = 0; PeripheralLoop < 3; PeripheralLoop++)
            {
               /* Identifier String First */
               RtlInitUnicodeString(&PeripheralString, Strings[PeripheralLoop]);

               /* How much buffer space */
               Status = ZwQueryValueKey(PeripheralKeyHandle, &PeripheralString, KeyValueFullInformation, NULL, 0, &LenKeyFullInformation);

               if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL && Status != STATUS_BUFFER_OVERFLOW)
               {
                 PeripheralInformation[PeripheralLoop] = NULL;
                 continue;
               }

               /* Allocate it */
               PeripheralInformation[PeripheralLoop] = ExAllocatePoolWithTag(PagedPool, LenKeyFullInformation, TAG_IO_RESOURCE);

               /* Get the Information */
               Status = ZwQueryValueKey(PeripheralKeyHandle, &PeripheralString, KeyValueFullInformation, PeripheralInformation[PeripheralLoop], LenKeyFullInformation, &LenKeyFullInformation);
            }

            /* Clean Up */
            ZwClose(PeripheralKeyHandle);
            PeripheralKeyHandle = NULL;

            /* We now have everything the caller could possibly want */
            if (NT_SUCCESS(Status))
            {
               Status = Query->CalloutRoutine(
                  Query->Context,
                  &ControllerRootRegName,
                  *Query->BusType,
                  Bus,
                  BusInformation,
                  *Query->ControllerType,
                  ControllerNumber,
                  ControllerInformation,
                  *Query->PeripheralType,
                  PeripheralNumber,
                  PeripheralInformation);
            }

            /* Free the allocated memory */
            for (PeripheralLoop = 0; PeripheralLoop < 3; PeripheralLoop++)
            {
               if (PeripheralInformation[PeripheralLoop])
               {
                  ExFreePoolWithTag(PeripheralInformation[PeripheralLoop], TAG_IO_RESOURCE);
                  PeripheralInformation[PeripheralLoop] = NULL;
               }
            }

            /* Something Messed up */
            if (!NT_SUCCESS(Status)) break;
         }
      }

EndLoop:
      /* Free the allocated memory */
      for (ControllerLoop = 0; ControllerLoop < 3; ControllerLoop++)
      {
         if (ControllerInformation[ControllerLoop])
         {
            ExFreePoolWithTag(ControllerInformation[ControllerLoop], TAG_IO_RESOURCE);
            ControllerInformation[ControllerLoop] = NULL;
         }
      }

      /* Something Messed up */
      if (!NT_SUCCESS(Status)) break;
   }

   return Status;
}

/*
 * IopQueryBusDescription
 *
 * FUNCTION:
 *      Reads and returns Hardware information from the appropriate hardware
 *      registry key. Helper sub of IoQueryDeviceDescription. Has two modes
 *      of operation, either looking for Root Bus Types or for sub-Bus
 *      information.
 *
 * ARGUMENTS:
 *      Query         - What the parent function wants.
 *      RootKey	      - Which key to look in
 *      RootKeyHandle - Handle to the key
 *      Bus           - Bus Number.
 *      KeyIsRoot     - Whether we are looking for Root Bus Types or
 *                      information under them.
 *
 * RETURNS:
 *      Status
 */

NTSTATUS NTAPI
IopQueryBusDescription(
   PIO_QUERY Query,
   UNICODE_STRING RootKey,
   HANDLE RootKeyHandle,
   PULONG Bus,
   BOOLEAN KeyIsRoot)
{
   NTSTATUS Status;
   ULONG BusLoop;
   UNICODE_STRING SubRootRegName;
   UNICODE_STRING BusString;
   UNICODE_STRING SubBusString;
   ULONG LenBasicInformation = 0;
   ULONG LenFullInformation;
   ULONG LenKeyFullInformation;
   ULONG LenKey;
   HANDLE SubRootKeyHandle;
   PKEY_FULL_INFORMATION FullInformation;
   PKEY_BASIC_INFORMATION BasicInformation = NULL;
   OBJECT_ATTRIBUTES ObjectAttributes;
   PKEY_VALUE_FULL_INFORMATION BusInformation[3] = {NULL, NULL, NULL};

   /* How much buffer space */
   Status = ZwQueryKey(RootKeyHandle, KeyFullInformation, NULL, 0, &LenFullInformation);

   if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL && Status != STATUS_BUFFER_OVERFLOW)
      return Status;

   /* Allocate it */
   FullInformation = ExAllocatePoolWithTag(PagedPool, LenFullInformation, TAG_IO_RESOURCE);

   if (!FullInformation)
     return STATUS_NO_MEMORY;

   /* Get the Information */
   Status = ZwQueryKey(RootKeyHandle, KeyFullInformation, FullInformation, LenFullInformation, &LenFullInformation);

   /* Everything was fine */
   if (NT_SUCCESS(Status))
   {
      /* Buffer needed for all the keys under this one */
      LenBasicInformation = FullInformation->MaxNameLen + sizeof(KEY_BASIC_INFORMATION);

      /* Allocate it */
      BasicInformation = ExAllocatePoolWithTag(PagedPool, LenBasicInformation, TAG_IO_RESOURCE);
   }

   /* Deallocate the old Buffer */
   ExFreePoolWithTag(FullInformation, TAG_IO_RESOURCE);

   /* Try to find a Bus */
   for (BusLoop = 0; NT_SUCCESS(Status); BusLoop++)
   {
      /* Bus parameter was passed and number was matched */
      if ((Query->BusNumber) && (*(Query->BusNumber)) == *Bus) break;

      /* Enumerate the Key */
      Status = ZwEnumerateKey(
         RootKeyHandle,
         BusLoop,
         KeyBasicInformation,
         BasicInformation,
         LenBasicInformation,
         &LenKey);

      /* Everything enumerated */
      if (!NT_SUCCESS(Status)) break;

      /* What Bus are we going to go down? (only check if this is a Root Key) */
      if (KeyIsRoot)
      {
         if (wcsncmp(BasicInformation->Name, L"MultifunctionAdapter", BasicInformation->NameLength / 2) &&
             wcsncmp(BasicInformation->Name, L"EisaAdapter", BasicInformation->NameLength / 2) &&
             wcsncmp(BasicInformation->Name, L"TcAdapter", BasicInformation->NameLength / 2))
         {
            /* Nothing found, check next */
            continue;
         }
      }

      /* Enumerate the Bus. */
      BusString.Buffer = BasicInformation->Name;
      BusString.Length = (USHORT)BasicInformation->NameLength;
      BusString.MaximumLength = (USHORT)BasicInformation->NameLength;

      /* Open a handle to the Root Registry Key */
      InitializeObjectAttributes(
         &ObjectAttributes,
         &BusString,
         OBJ_CASE_INSENSITIVE,
         RootKeyHandle,
         NULL);

      Status = ZwOpenKey(&SubRootKeyHandle, KEY_READ, &ObjectAttributes);

      /* Go on if we can't */
      if (!NT_SUCCESS(Status)) continue;

      /* Key opened. Create the path */
      SubRootRegName = RootKey;
      RtlAppendUnicodeToString(&SubRootRegName, L"\\");
      RtlAppendUnicodeStringToString(&SubRootRegName, &BusString);

      if (!KeyIsRoot)
      {
         /* Parsing a SubBus-key */
         int SubBusLoop;
         PWSTR Strings[3] = {
            L"Identifier",
            L"Configuration Data",
            L"Component Information"};

         for (SubBusLoop = 0; SubBusLoop < 3; SubBusLoop++)
         {
            /* Identifier String First */
            RtlInitUnicodeString(&SubBusString, Strings[SubBusLoop]);

            /* How much buffer space */
            ZwQueryValueKey(SubRootKeyHandle, &SubBusString, KeyValueFullInformation, NULL, 0, &LenKeyFullInformation);

            /* Allocate it */
            BusInformation[SubBusLoop] = ExAllocatePoolWithTag(PagedPool, LenKeyFullInformation, TAG_IO_RESOURCE);

            /* Get the Information */
            Status = ZwQueryValueKey(SubRootKeyHandle, &SubBusString, KeyValueFullInformation, BusInformation[SubBusLoop], LenKeyFullInformation, &LenKeyFullInformation);
         }

         if (NT_SUCCESS(Status))
         {
            /* Do we have something */
            if (BusInformation[1] != NULL &&
                BusInformation[1]->DataLength != 0 &&
                /* Does it match what we want? */
                (((PCM_FULL_RESOURCE_DESCRIPTOR)((ULONG_PTR)BusInformation[1] + BusInformation[1]->DataOffset))->InterfaceType == *(Query->BusType)))
            {
               /* Found a bus */
               (*Bus)++;

               /* Is it the bus we wanted */
               if (Query->BusNumber == NULL || *(Query->BusNumber) == *Bus)
               {
                  /* If we don't want Controller Information, we're done... call the callback */
                  if (Query->ControllerType == NULL)
                  {
                     Status = Query->CalloutRoutine(
                        Query->Context,
                        &SubRootRegName,
                        *(Query->BusType),
                        *Bus,
                        BusInformation,
                        0,
                        0,
                        NULL,
                        0,
                        0,
                        NULL);
                  } else {
                     /* We want Controller Info...get it */
                     Status = IopQueryDeviceDescription(Query, SubRootRegName, RootKeyHandle, *Bus, (PKEY_VALUE_FULL_INFORMATION*)BusInformation);
                  }
               }
            }
         }

         /* Free the allocated memory */
         for (SubBusLoop = 0; SubBusLoop < 3; SubBusLoop++)
         {
            if (BusInformation[SubBusLoop])
            {
               ExFreePoolWithTag(BusInformation[SubBusLoop], TAG_IO_RESOURCE);
               BusInformation[SubBusLoop] = NULL;
            }
         }

         /* Exit the Loop if we found the bus */
         if (Query->BusNumber != NULL && *(Query->BusNumber) == *Bus)
         {
            ZwClose(SubRootKeyHandle);
            SubRootKeyHandle = NULL;
            continue;
         }
      }

      /* Enumerate the buses below us recursively if we haven't found the bus yet */
      Status = IopQueryBusDescription(Query, SubRootRegName, SubRootKeyHandle, Bus, !KeyIsRoot);

      /* Everything enumerated */
      if (Status == STATUS_NO_MORE_ENTRIES) Status = STATUS_SUCCESS;

      ZwClose(SubRootKeyHandle);
      SubRootKeyHandle = NULL;
   }

   /* Free the last remaining Allocated Memory */
   if (BasicInformation)
      ExFreePoolWithTag(BasicInformation, TAG_IO_RESOURCE);

   return Status;
}

NTSTATUS
NTAPI
IopFetchConfigurationInformation(OUT PWSTR * SymbolicLinkList,
                                 IN GUID Guid,
                                 IN ULONG ExpectedInterfaces,
                                 IN PULONG Interfaces)
{
    NTSTATUS Status;
    ULONG IntInterfaces = 0;
    PWSTR IntSymbolicLinkList;

    /* Get the associated enabled interfaces with the given GUID */
    Status = IoGetDeviceInterfaces(&Guid, NULL, 0, SymbolicLinkList);
    if (!NT_SUCCESS(Status))
    {
        /* Zero output and leave */
        if (SymbolicLinkList != 0)
        {
            *SymbolicLinkList = 0;
        }

        return STATUS_UNSUCCESSFUL;
    }

    IntSymbolicLinkList = *SymbolicLinkList;

    /* Count the number of enabled interfaces by counting the number of symbolic links */
    while (*IntSymbolicLinkList != UNICODE_NULL)
    {
        IntInterfaces++;
        IntSymbolicLinkList += wcslen(IntSymbolicLinkList) + (sizeof(UNICODE_NULL) / sizeof(WCHAR));
    }

    /* Matching result will define the result */
    Status = (IntInterfaces >= ExpectedInterfaces) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    /* Finally, give back to the caller the number of found interfaces */
    *Interfaces = IntInterfaces;

    return Status;
}

VOID
NTAPI
IopStoreSystemPartitionInformation(IN PUNICODE_STRING NtSystemPartitionDeviceName,
                                   IN PUNICODE_STRING OsLoaderPathName)
{
    NTSTATUS Status;
    UNICODE_STRING LinkTarget, KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE LinkHandle, RegistryHandle, KeyHandle;
    WCHAR LinkTargetBuffer[256];
    UNICODE_STRING CmRegistryMachineSystemName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM");

    ASSERT(NtSystemPartitionDeviceName->MaximumLength >= NtSystemPartitionDeviceName->Length + sizeof(WCHAR));
    ASSERT(NtSystemPartitionDeviceName->Buffer[NtSystemPartitionDeviceName->Length / sizeof(WCHAR)] == UNICODE_NULL);
    ASSERT(OsLoaderPathName->MaximumLength >= OsLoaderPathName->Length + sizeof(WCHAR));
    ASSERT(OsLoaderPathName->Buffer[OsLoaderPathName->Length / sizeof(WCHAR)] == UNICODE_NULL);

    /* First define needed stuff to open NtSystemPartitionDeviceName symbolic link */
    InitializeObjectAttributes(&ObjectAttributes,
                               NtSystemPartitionDeviceName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Open NtSystemPartitionDeviceName symbolic link */
    Status = ZwOpenSymbolicLinkObject(&LinkHandle,
                                      SYMBOLIC_LINK_QUERY,
                                      &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to open symlink %wZ, Status=%lx\n", NtSystemPartitionDeviceName, Status);
        return;
    }

    /* Prepare the string that will receive where symbolic link points to */
    LinkTarget.Length = 0;
    /* We will zero the end of the string after having received it */
    LinkTarget.MaximumLength = sizeof(LinkTargetBuffer) - sizeof(UNICODE_NULL);
    LinkTarget.Buffer = LinkTargetBuffer;

    /* Query target */
    Status = ZwQuerySymbolicLinkObject(LinkHandle,
                                       &LinkTarget,
                                       NULL);

    /* We are done with symbolic link */
    ObCloseHandle(LinkHandle, KernelMode);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed querying symlink %wZ, Status=%lx\n", NtSystemPartitionDeviceName, Status);
        return;
    }

    /* As promised, we zero the end */
    LinkTarget.Buffer[LinkTarget.Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Open registry to save data (HKLM\SYSTEM) */
    Status = IopOpenRegistryKeyEx(&RegistryHandle,
                                  NULL,
                                  &CmRegistryMachineSystemName,
                                  KEY_ALL_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to open HKLM\\SYSTEM, Status=%lx\n", Status);
        return;
    }

    /* Open or create the Setup subkey where we'll store in */
    RtlInitUnicodeString(&KeyName, L"Setup");

    Status = IopCreateRegistryKeyEx(&KeyHandle,
                                    RegistryHandle,
                                    &KeyName,
                                    KEY_ALL_ACCESS,
                                    REG_OPTION_NON_VOLATILE,
                                    NULL);

    /* We're done with HKLM\SYSTEM */
    ObCloseHandle(RegistryHandle, KernelMode);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed opening/creating Setup key, Status=%lx\n", Status);
        return;
    }

    /* Prepare first data writing... */
    RtlInitUnicodeString(&KeyName, L"SystemPartition");

    /* Write SystemPartition value which is the target of the symbolic link */
    Status = ZwSetValueKey(KeyHandle,
                           &KeyName,
                           0,
                           REG_SZ,
                           LinkTarget.Buffer,
                           LinkTarget.Length + sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed writing SystemPartition value, Status=%lx\n", Status);
    }

    /* Prepare for second data writing... */
    RtlInitUnicodeString(&KeyName, L"OsLoaderPath");

    /* Remove trailing slash if any (one slash only excepted) */
    if (OsLoaderPathName->Length > sizeof(WCHAR) &&
        OsLoaderPathName->Buffer[(OsLoaderPathName->Length / sizeof(WCHAR)) - 1] == OBJ_NAME_PATH_SEPARATOR)
    {
        OsLoaderPathName->Length -= sizeof(WCHAR);
        OsLoaderPathName->Buffer[OsLoaderPathName->Length / sizeof(WCHAR)] = UNICODE_NULL;
    }

    /* Then, write down data */
    Status = ZwSetValueKey(KeyHandle,
                           &KeyName,
                           0,
                           REG_SZ,
                           OsLoaderPathName->Buffer,
                           OsLoaderPathName->Length + sizeof(UNICODE_NULL));
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed writing OsLoaderPath value, Status=%lx\n", Status);
    }

    /* We're finally done! */
    ObCloseHandle(KeyHandle, KernelMode);
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
PCONFIGURATION_INFORMATION NTAPI
IoGetConfigurationInformation(VOID)
{
  return(&_SystemConfigurationInformation);
}

/*
 * @halfplemented
 */
static
BOOLEAN
IopCheckResourceDescriptor(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR ResDesc,
    IN PCM_RESOURCE_LIST ResourceList,
    IN BOOLEAN Silent,
    OUT OPTIONAL PCM_PARTIAL_RESOURCE_DESCRIPTOR ConflictingDescriptor)
{
   ULONG i, ii;
   BOOLEAN Result = FALSE;

   for (i = 0; i < ResourceList->Count; i++)
   {
      PCM_PARTIAL_RESOURCE_LIST ResList = &ResourceList->List[i].PartialResourceList;
      for (ii = 0; ii < ResList->Count; ii++)
      {
         PCM_PARTIAL_RESOURCE_DESCRIPTOR ResDesc2 = &ResList->PartialDescriptors[ii];

         /* We don't care about shared resources */
         if (ResDesc->ShareDisposition == CmResourceShareShared &&
             ResDesc2->ShareDisposition == CmResourceShareShared)
             continue;

         /* Make sure we're comparing the same types */
         if (ResDesc->Type != ResDesc2->Type)
             continue;

         switch (ResDesc->Type)
         {
             case CmResourceTypeMemory:
                 if (((ULONGLONG)ResDesc->u.Memory.Start.QuadPart < (ULONGLONG)ResDesc2->u.Memory.Start.QuadPart &&
                      (ULONGLONG)ResDesc->u.Memory.Start.QuadPart + ResDesc->u.Memory.Length >
                      (ULONGLONG)ResDesc2->u.Memory.Start.QuadPart) || ((ULONGLONG)ResDesc2->u.Memory.Start.QuadPart <
                      (ULONGLONG)ResDesc->u.Memory.Start.QuadPart && (ULONGLONG)ResDesc2->u.Memory.Start.QuadPart +
                      ResDesc2->u.Memory.Length > (ULONGLONG)ResDesc->u.Memory.Start.QuadPart))
                 {
                      if (!Silent)
                      {
                          DPRINT1("Resource conflict: Memory (0x%I64x to 0x%I64x vs. 0x%I64x to 0x%I64x)\n",
                                  ResDesc->u.Memory.Start.QuadPart, ResDesc->u.Memory.Start.QuadPart +
                                  ResDesc->u.Memory.Length, ResDesc2->u.Memory.Start.QuadPart,
                                  ResDesc2->u.Memory.Start.QuadPart + ResDesc2->u.Memory.Length);
                      }

                      Result = TRUE;

                      goto ByeBye;
                 }
                 break;

             case CmResourceTypePort:
                 if (((ULONGLONG)ResDesc->u.Port.Start.QuadPart < (ULONGLONG)ResDesc2->u.Port.Start.QuadPart &&
                      (ULONGLONG)ResDesc->u.Port.Start.QuadPart + ResDesc->u.Port.Length >
                      (ULONGLONG)ResDesc2->u.Port.Start.QuadPart) || ((ULONGLONG)ResDesc2->u.Port.Start.QuadPart <
                      (ULONGLONG)ResDesc->u.Port.Start.QuadPart && (ULONGLONG)ResDesc2->u.Port.Start.QuadPart +
                      ResDesc2->u.Port.Length > (ULONGLONG)ResDesc->u.Port.Start.QuadPart))
                 {
                      if (!Silent)
                      {
                          DPRINT1("Resource conflict: Port (0x%I64x to 0x%I64x vs. 0x%I64x to 0x%I64x)\n",
                                  ResDesc->u.Port.Start.QuadPart, ResDesc->u.Port.Start.QuadPart +
                                  ResDesc->u.Port.Length, ResDesc2->u.Port.Start.QuadPart,
                                  ResDesc2->u.Port.Start.QuadPart + ResDesc2->u.Port.Length);
                      }

                      Result = TRUE;

                      goto ByeBye;
                 }
                 break;

             case CmResourceTypeInterrupt:
                 if (ResDesc->u.Interrupt.Vector == ResDesc2->u.Interrupt.Vector)
                 {
                      if (!Silent)
                      {
                          DPRINT1("Resource conflict: IRQ (0x%x 0x%x vs. 0x%x 0x%x)\n",
                                  ResDesc->u.Interrupt.Vector, ResDesc->u.Interrupt.Level,
                                  ResDesc2->u.Interrupt.Vector, ResDesc2->u.Interrupt.Level);
                      }

                      Result = TRUE;

                      goto ByeBye;
                 }
                 break;

             case CmResourceTypeBusNumber:
                 if ((ResDesc->u.BusNumber.Start < ResDesc2->u.BusNumber.Start &&
                      ResDesc->u.BusNumber.Start + ResDesc->u.BusNumber.Length >
                      ResDesc2->u.BusNumber.Start) || (ResDesc2->u.BusNumber.Start <
                      ResDesc->u.BusNumber.Start && ResDesc2->u.BusNumber.Start +
                      ResDesc2->u.BusNumber.Length > ResDesc->u.BusNumber.Start))
                 {
                      if (!Silent)
                      {
                          DPRINT1("Resource conflict: Bus number (0x%x to 0x%x vs. 0x%x to 0x%x)\n",
                                  ResDesc->u.BusNumber.Start, ResDesc->u.BusNumber.Start +
                                  ResDesc->u.BusNumber.Length, ResDesc2->u.BusNumber.Start,
                                  ResDesc2->u.BusNumber.Start + ResDesc2->u.BusNumber.Length);
                      }

                      Result = TRUE;

                      goto ByeBye;
                 }
                 break;

             case CmResourceTypeDma:
                 if (ResDesc->u.Dma.Channel == ResDesc2->u.Dma.Channel)
                 {
                     if (!Silent)
                     {
                         DPRINT1("Resource conflict: Dma (0x%x 0x%x vs. 0x%x 0x%x)\n",
                                 ResDesc->u.Dma.Channel, ResDesc->u.Dma.Port,
                                 ResDesc2->u.Dma.Channel, ResDesc2->u.Dma.Port);
                     }

                     Result = TRUE;

                     goto ByeBye;
                 }
                 break;
         }
      }
   }

ByeBye:

   if (Result && ConflictingDescriptor)
   {
       RtlCopyMemory(ConflictingDescriptor,
                     ResDesc,
                     sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
   }

   return Result;
}

static
BOOLEAN
IopCheckForResourceConflict(
    IN PCM_RESOURCE_LIST ResourceList1,
    IN PCM_RESOURCE_LIST ResourceList2,
    IN BOOLEAN Silent,
    OUT OPTIONAL PCM_PARTIAL_RESOURCE_DESCRIPTOR ConflictingDescriptor)
{
   ULONG i, ii;
   BOOLEAN Result = FALSE;

   for (i = 0; i < ResourceList1->Count; i++)
   {
      PCM_PARTIAL_RESOURCE_LIST ResList = &ResourceList1->List[i].PartialResourceList;
      for (ii = 0; ii < ResList->Count; ii++)
      {
         PCM_PARTIAL_RESOURCE_DESCRIPTOR ResDesc = &ResList->PartialDescriptors[ii];

         Result = IopCheckResourceDescriptor(ResDesc,
                                             ResourceList2,
                                             Silent,
                                             ConflictingDescriptor);
         if (Result) goto ByeBye;
      }
   }

ByeBye:

   return Result;
}

NTSTATUS NTAPI
IopDetectResourceConflict(
    IN PCM_RESOURCE_LIST ResourceList,
    IN BOOLEAN Silent,
    OUT OPTIONAL PCM_PARTIAL_RESOURCE_DESCRIPTOR ConflictingDescriptor)
{
    UNICODE_STRING ResourceMapName = RTL_CONSTANT_STRING(IO_REG_KEY_RESOURCEMAP);
    UNICODE_STRING KeyName;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    PKEY_VALUE_BASIC_INFORMATION KeyNameInformation;
    PKEY_BASIC_INFORMATION KeyInformation;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ResourceMapKey = NULL;
    HANDLE ChildKey2 = NULL;
    HANDLE ChildKey3 = NULL;
    ULONG KeyValueInformationLength;
    ULONG KeyNameInformationLength;
    ULONG KeyInformationLength;
    ULONG RequiredLength;
    ULONG ChildKeyIndex1 = 0;
    ULONG ChildKeyIndex2 = 0;
    ULONG ChildKeyIndex3 = 0;
    NTSTATUS Status;

    InitializeObjectAttributes(&ObjectAttributes,
                               &ResourceMapName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&ResourceMapKey,
                       KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
       /* The key is missing which means we are the first device */
       return STATUS_SUCCESS;
    }

    while (TRUE)
    {
        Status = ZwEnumerateKey(ResourceMapKey,
                                ChildKeyIndex1,
                                KeyBasicInformation,
                                NULL,
                                0,
                                &RequiredLength);

        if (Status == STATUS_NO_MORE_ENTRIES) {
            break;
        }
        else if (Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL)
        {
            KeyInformationLength = RequiredLength;
            KeyInformation = ExAllocatePoolWithTag(PagedPool,
                                                   KeyInformationLength,
                                                   TAG_IO);
            if (!KeyInformation) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto cleanup;
            }

            Status = ZwEnumerateKey(ResourceMapKey,
                                    ChildKeyIndex1,
                                    KeyBasicInformation,
                                    KeyInformation,
                                    KeyInformationLength,
                                    &RequiredLength);
        }
        else {
           goto cleanup;
        }

        ChildKeyIndex1++;

        if (!NT_SUCCESS(Status)) {
            ExFreePoolWithTag(KeyInformation, TAG_IO);
            goto cleanup;
        }

        KeyName.Buffer = KeyInformation->Name;
        KeyName.MaximumLength = KeyName.Length = (USHORT)KeyInformation->NameLength;

        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   ResourceMapKey,
                                   NULL);

        Status = ZwOpenKey(&ChildKey2,
                           KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                           &ObjectAttributes);

        ExFreePoolWithTag(KeyInformation, TAG_IO);
        if (!NT_SUCCESS(Status)) {
            goto cleanup;
        }

        while (TRUE)
        {
            Status = ZwEnumerateKey(ChildKey2,
                                    ChildKeyIndex2,
                                    KeyBasicInformation,
                                    NULL,
                                    0,
                                    &RequiredLength);

            if (Status == STATUS_NO_MORE_ENTRIES){
                break;
            }
            else if (Status == STATUS_BUFFER_TOO_SMALL)
            {
                KeyInformationLength = RequiredLength;

                KeyInformation = ExAllocatePoolWithTag(PagedPool, KeyInformationLength, TAG_IO);
                if (!KeyInformation) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto cleanup;
                }

                Status = ZwEnumerateKey(ChildKey2,
                                        ChildKeyIndex2,
                                        KeyBasicInformation,
                                        KeyInformation,
                                        KeyInformationLength,
                                        &RequiredLength);
            }
            else {
                goto cleanup;
            }

            ChildKeyIndex2++;

            if (!NT_SUCCESS(Status)) {
                ExFreePoolWithTag(KeyInformation, TAG_IO);
                goto cleanup;
            }

            KeyName.Buffer = KeyInformation->Name;
            KeyName.MaximumLength = KeyName.Length = (USHORT)KeyInformation->NameLength;

            InitializeObjectAttributes(&ObjectAttributes,
                                       &KeyName,
                                       OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                       ChildKey2,
                                       NULL);

            Status = ZwOpenKey(&ChildKey3, KEY_QUERY_VALUE, &ObjectAttributes);
            ExFreePoolWithTag(KeyInformation, TAG_IO);
            if (!NT_SUCCESS(Status)) {
                goto cleanup;
            }

            while (TRUE)
            {
                Status = ZwEnumerateValueKey(ChildKey3,
                                             ChildKeyIndex3,
                                             KeyValuePartialInformation,
                                             NULL,
                                             0,
                                             &RequiredLength);

                if (Status == STATUS_NO_MORE_ENTRIES) {
                    break;
                }
                else if (Status == STATUS_BUFFER_TOO_SMALL)
                {
                    KeyValueInformationLength = RequiredLength;

                    KeyValueInformation = ExAllocatePoolWithTag(PagedPool, KeyValueInformationLength, TAG_IO);
                    if (!KeyValueInformation) {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto cleanup;
                    }

                    Status = ZwEnumerateValueKey(ChildKey3,
                                                 ChildKeyIndex3,
                                                 KeyValuePartialInformation,
                                                 KeyValueInformation,
                                                 KeyValueInformationLength,
                                                 &RequiredLength);
                }
                else
                    goto cleanup;
                if (!NT_SUCCESS(Status))
                {
                    ExFreePoolWithTag(KeyValueInformation, TAG_IO);
                    goto cleanup;
                }
                Status = ZwEnumerateValueKey(ChildKey3,
                                             ChildKeyIndex3,
                                             KeyValueBasicInformation,
                                             NULL,
                                             0,
                                             &RequiredLength);
                if (Status == STATUS_BUFFER_TOO_SMALL)
                {
                    KeyNameInformationLength = RequiredLength;
                    KeyNameInformation = ExAllocatePoolWithTag(PagedPool,
                                                               KeyNameInformationLength + sizeof(WCHAR),
                                                               TAG_IO);
                    if (!KeyNameInformation)
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto cleanup;
                    }
                    Status = ZwEnumerateValueKey(ChildKey3,
                                                 ChildKeyIndex3,
                                                 KeyValueBasicInformation,
                                                 KeyNameInformation,
                                                 KeyNameInformationLength,
                                                 &RequiredLength);
                }
                else
                    goto cleanup;
                ChildKeyIndex3++;
                if (!NT_SUCCESS(Status))
                {
                    ExFreePoolWithTag(KeyNameInformation, TAG_IO);
                    goto cleanup;
                }
                KeyNameInformation->Name[KeyNameInformation->NameLength / sizeof(WCHAR)] = UNICODE_NULL;
                /* Skip translated entries */
                if (wcsstr(KeyNameInformation->Name, L".Translated"))
                {
                    ExFreePoolWithTag(KeyNameInformation, TAG_IO);
                    ExFreePoolWithTag(KeyValueInformation, TAG_IO);
                    continue;
                }
                ExFreePoolWithTag(KeyNameInformation, TAG_IO);
                if (IopCheckForResourceConflict(ResourceList,
                                                (PCM_RESOURCE_LIST)KeyValueInformation->Data,
                                                Silent,
                                                ConflictingDescriptor))
                {
                    ExFreePoolWithTag(KeyValueInformation, TAG_IO);
                    Status = STATUS_CONFLICTING_ADDRESSES;
                    goto cleanup;
                }
                ExFreePoolWithTag(KeyValueInformation, TAG_IO);
            }
        }
    }

cleanup:

    if (ResourceMapKey != NULL) {
        ObCloseHandle(ResourceMapKey, KernelMode);
    }
    if (ChildKey2 != NULL) {
        ObCloseHandle(ChildKey2, KernelMode);
    }
    if (ChildKey3 != NULL) {
        ObCloseHandle(ChildKey3, KernelMode);
    }
    if (Status == STATUS_NO_MORE_ENTRIES) {
        Status = STATUS_SUCCESS;
    }

    return Status;
}

/*
 * @halfplemented
 */
NTSTATUS NTAPI
IoReportResourceUsage(PUNICODE_STRING DriverClassName,
		      PDRIVER_OBJECT DriverObject,
		      PCM_RESOURCE_LIST DriverList,
		      ULONG DriverListSize,
		      PDEVICE_OBJECT DeviceObject,
		      PCM_RESOURCE_LIST DeviceList,
		      ULONG DeviceListSize,
		      BOOLEAN OverrideConflict,
		      PBOOLEAN ConflictDetected)
     /*
      * FUNCTION: Reports hardware resources in the
      * \Registry\Machine\Hardware\ResourceMap tree, so that a subsequently
      * loaded driver cannot attempt to use the same resources.
      * ARGUMENTS:
      *       DriverClassName - The class of driver under which the resource
      *       information should be stored.
      *       DriverObject - The driver object that was input to the
      *       DriverEntry.
      *       DriverList - Resources that claimed for the driver rather than
      *       per-device.
      *       DriverListSize - Size in bytes of the DriverList.
      *       DeviceObject - The device object for which resources should be
      *       claimed.
      *       DeviceList - List of resources which should be claimed for the
      *       device.
      *       DeviceListSize - Size of the per-device resource list in bytes.
      *       OverrideConflict - True if the resources should be cliamed
      *       even if a conflict is found.
      *       ConflictDetected - Points to a variable that receives TRUE if
      *       a conflict is detected with another driver.
      */
{
    NTSTATUS Status;
    PCM_RESOURCE_LIST ResourceList;

    DPRINT1("IoReportResourceUsage is halfplemented!\n");

    if (!DriverList && !DeviceList)
        return STATUS_INVALID_PARAMETER;

    if (DeviceList)
        ResourceList = DeviceList;
    else
        ResourceList = DriverList;

    Status = IopDetectResourceConflict(ResourceList, FALSE, NULL);
    if (Status == STATUS_CONFLICTING_ADDRESSES)
    {
        *ConflictDetected = TRUE;

        if (!OverrideConflict)
        {
            DPRINT1("Denying an attempt to claim resources currently in use by another device!\n");
            return STATUS_CONFLICTING_ADDRESSES;
        }
        else
        {
            DPRINT1("Proceeding with conflicting resources\n");
        }
    }
    else if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* TODO: Claim resources in registry */

    *ConflictDetected = FALSE;

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoAssignResources(IN PUNICODE_STRING RegistryPath,
                  IN PUNICODE_STRING DriverClassName,
                  IN PDRIVER_OBJECT DriverObject,
                  IN PDEVICE_OBJECT DeviceObject,
                  IN PIO_RESOURCE_REQUIREMENTS_LIST RequestedResources,
                  IN OUT PCM_RESOURCE_LIST* AllocatedResources)
{
    PDEVICE_NODE DeviceNode;

    /* Do we have a DO? */
    if (DeviceObject)
    {
        /* Get its device node */
        DeviceNode = IopGetDeviceNode(DeviceObject);
        if ((DeviceNode) && !(DeviceNode->Flags & DNF_LEGACY_RESOURCE_DEVICENODE))
        {
            /* New drivers should not call this API */
            KeBugCheckEx(PNP_DETECTED_FATAL_ERROR,
                         0,
                         0,
                         (ULONG_PTR)DeviceObject,
                         (ULONG_PTR)DriverObject);
        }
    }

    /* Did the driver supply resources? */
    if (RequestedResources)
    {
        /* Make sure there's actually something useful in them */
        if (!(RequestedResources->AlternativeLists) || !(RequestedResources->List[0].Count))
        {
            /* Empty resources are no resources */
            RequestedResources = NULL;
        }
    }

    /* Initialize output if given */
    if (AllocatedResources) *AllocatedResources = NULL;

    /* Call internal helper function */
    return IopLegacyResourceAllocation(ArbiterRequestLegacyAssigned,
                                       DriverObject,
                                       DeviceObject,
                                       RequestedResources,
                                       AllocatedResources);
}

/*
 * FUNCTION:
 *     Reads and returns Hardware information from the appropriate hardware registry key.
 *
 * ARGUMENTS:
 *     BusType          - MCA, ISA, EISA...specifies the Bus Type
 *     BusNumber	- Which bus of above should be queried
 *     ControllerType	- Specifices the Controller Type
 *     ControllerNumber	- Which of the controllers to query.
 *     CalloutRoutine	- Which function to call for each valid query.
 *     Context          - Value to pass to the callback.
 *
 * RETURNS:
 *     Status
 *
 * STATUS:
 *     @implemented
 */

NTSTATUS NTAPI
IoQueryDeviceDescription(PINTERFACE_TYPE BusType OPTIONAL,
			 PULONG BusNumber OPTIONAL,
			 PCONFIGURATION_TYPE ControllerType OPTIONAL,
			 PULONG ControllerNumber OPTIONAL,
			 PCONFIGURATION_TYPE PeripheralType OPTIONAL,
			 PULONG PeripheralNumber OPTIONAL,
			 PIO_QUERY_DEVICE_ROUTINE CalloutRoutine,
			 PVOID Context)
{
   NTSTATUS Status;
   ULONG BusLoopNumber = -1; /* Root Bus */
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING RootRegKey;
   HANDLE RootRegHandle;
   IO_QUERY Query;

   /* Set up the String */
   RootRegKey.Length = 0;
   RootRegKey.MaximumLength = 2048;
   RootRegKey.Buffer = ExAllocatePoolWithTag(PagedPool, RootRegKey.MaximumLength, TAG_IO_RESOURCE);
   RtlAppendUnicodeToString(&RootRegKey, L"\\REGISTRY\\MACHINE\\HARDWARE\\DESCRIPTION\\SYSTEM");

   /* Open a handle to the Root Registry Key */
   InitializeObjectAttributes(
      &ObjectAttributes,
      &RootRegKey,
      OBJ_CASE_INSENSITIVE,
      NULL,
      NULL);

   Status = ZwOpenKey(&RootRegHandle, KEY_READ, &ObjectAttributes);

   if (NT_SUCCESS(Status))
   {
      /* Use a helper function to loop though this key and get the info */
      Query.BusType = BusType;
      Query.BusNumber = BusNumber;
      Query.ControllerType = ControllerType;
      Query.ControllerNumber = ControllerNumber;
      Query.PeripheralType = PeripheralType;
      Query.PeripheralNumber = PeripheralNumber;
      Query.CalloutRoutine = CalloutRoutine;
      Query.Context = Context;
      Status = IopQueryBusDescription(&Query, RootRegKey, RootRegHandle, &BusLoopNumber, TRUE);

      /* Close registry */
      ZwClose(RootRegHandle);
   }

   /* Free Memory */
   ExFreePoolWithTag(RootRegKey.Buffer, TAG_IO_RESOURCE);

   return Status;
}

NTSTATUS
NTAPI
HeadlessTerminalAddResources(
    _In_ PCM_RESOURCE_LIST List,
    _In_ ULONG ListSize,
    _In_ BOOLEAN IsTranslated,
    _Out_ PCM_RESOURCE_LIST *OutList,
    _Out_ PULONG OutSize)
{
    PCM_RESOURCE_LIST CmResource;
    PHYSICAL_ADDRESS TerminalPortAddress;
    PCM_FULL_RESOURCE_DESCRIPTOR CmFullDesc;
    PHYSICAL_ADDRESS TranslatedAddress;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor;

    DPRINT("HeadlessTerminalAddResources: List - %p, ListSize - %X, IsTranslated - %X\n",
           List, ListSize, IsTranslated);

    if (!HeadlessGlobals || HeadlessGlobals->IsNonLegacyDevice)
    {
        *OutList = NULL;
        *OutSize = 0;
        return STATUS_SUCCESS;
    }

    *OutSize = ListSize + sizeof(CM_FULL_RESOURCE_DESCRIPTOR);

    CmResource = ExAllocatePoolWithTag(PagedPool,
                                       ListSize + sizeof(CM_FULL_RESOURCE_DESCRIPTOR),
                                       'sldH');
    *OutList = CmResource;

    if (!CmResource)
    {
        ASSERT(FALSE);
        *OutSize = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(CmResource, List, ListSize);

    TerminalPortAddress.QuadPart = (ULONG_PTR)HeadlessGlobals->TerminalPortAddress;

    if (IsTranslated)
    {
        ULONG AddressSpace = 1;

        HalTranslateBusAddress(Internal,
                               0,
                               TerminalPortAddress,
                               &AddressSpace,
                               &TranslatedAddress);
    }
    else
    {
        TranslatedAddress = TerminalPortAddress;
    }

    (*OutList)->Count++;

    CmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)((ULONG_PTR)(*OutList) + ListSize);

    CmFullDesc->BusNumber = 0;
    CmFullDesc->InterfaceType = Isa;

    CmFullDesc->PartialResourceList.Count = 1;
    CmFullDesc->PartialResourceList.Revision = 0;
    CmFullDesc->PartialResourceList.Version = 0;

    CmDescriptor = &CmFullDesc->PartialResourceList.PartialDescriptors[0];

    CmDescriptor->Type = CmResourceTypePort;
    CmDescriptor->ShareDisposition = CmResourceShareDriverExclusive;
    CmDescriptor->Flags = CM_RESOURCE_PORT_IO;
    CmDescriptor->u.Port.Start.QuadPart = TranslatedAddress.QuadPart;
    CmDescriptor->u.Port.Length = 8;

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
IoReportHalResourceUsage(
    _In_ PUNICODE_STRING HalDescription,
    _In_ PCM_RESOURCE_LIST RawList,
    _In_ PCM_RESOURCE_LIST TranslatedList,
    _In_ ULONG ListSize)
/*
 * FUNCTION:
 *      Reports hardware resources of the HAL in the
 *      \Registry\Machine\Hardware\ResourceMap tree.
 * ARGUMENTS:
 *      HalDescription: Descriptive name of the HAL.
 *      RawList: List of raw (bus specific) resources which should be
 *               claimed for the HAL.
 *      TranslatedList: List of translated (system wide) resources which
 *                      should be claimed for the HAL.
 *      ListSize: Size in bytes of the raw and translated resource lists.
 *                Both lists have the same size.
 * RETURNS:
 *      Status.
 */
{
    PCM_RESOURCE_LIST HeadlessRawList;
    PCM_RESOURCE_LIST HeadlessTranslatedList;
    PCM_RESOURCE_LIST CmInitHalResource;
    UNICODE_STRING HalKeyName;
    UNICODE_STRING ValueName;
    HANDLE ResourceMapHandle;
    ULONG HeadlessListSize;
    NTSTATUS Status;
    UNICODE_STRING ResourceMapName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\HARDWARE\\RESOURCEMAP");

    PAGED_CODE();
    DPRINT("IoReportHalResourceUsage: HalDescription - %wZ, ListSize - %X\n",
           HalDescription, ListSize);

    RtlInitUnicodeString(&HalKeyName, L"Hardware Abstraction Layer");
    Status = IopCreateRegistryKeyEx(&ResourceMapHandle,
                                    0,
                                    &ResourceMapName,
                                    KEY_READ | KEY_WRITE,
                                    REG_OPTION_VOLATILE,
                                    NULL);

    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
        return Status;
    }

    Status = HeadlessTerminalAddResources(RawList,
                                          ListSize,
                                          FALSE,
                                          &HeadlessRawList,
                                          &HeadlessListSize);

    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
        goto Exit;
    }

    if (HeadlessRawList)
    {
        PipDumpCmResourceList(HeadlessRawList, 1);
        RawList = HeadlessRawList;
        ListSize = HeadlessListSize;
    }

    RtlInitUnicodeString(&ValueName, L".Raw");
    Status = IopWriteResourceList(ResourceMapHandle,
                                  &HalKeyName,
                                  HalDescription,
                                  &ValueName,
                                  RawList,
                                  ListSize);

    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
        goto Exit;
    }

    RtlInitUnicodeString(&ValueName, L".Translated");
    Status = HeadlessTerminalAddResources(TranslatedList,
                                          ListSize,
                                          TRUE,
                                          &HeadlessTranslatedList,
                                          &HeadlessListSize);

    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
        goto Exit;
    }

    if (HeadlessTranslatedList)
    {
        PipDumpCmResourceList(HeadlessRawList, 1);
        TranslatedList = HeadlessTranslatedList;
        ListSize = HeadlessListSize;
    }

    Status = IopWriteResourceList(ResourceMapHandle,
                                  &HalKeyName,
                                  HalDescription,
                                  &ValueName,
                                  TranslatedList,
                                  ListSize);

    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
    }

    if (HeadlessTranslatedList)
    {
        ExFreePoolWithTag(HeadlessTranslatedList, 'sldH');
    }

Exit:
    ZwClose(ResourceMapHandle);

    if (!NT_SUCCESS(Status))
    {
        if (HeadlessRawList)
        {
            ExFreePoolWithTag(HeadlessRawList, 'sldH');
        }

        return Status;
    }

    if (HeadlessRawList)
    {
        IopInitHalResources = HeadlessRawList;
        return Status;
    }

    CmInitHalResource = ExAllocatePoolWithTag(PagedPool, ListSize, '  pP');
    IopInitHalResources = CmInitHalResource;

    if (!CmInitHalResource)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(CmInitHalResource, RawList, ListSize);

    return Status;
}

/* EOF */
