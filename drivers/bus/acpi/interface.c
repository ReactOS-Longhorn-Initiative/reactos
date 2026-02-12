#include "precomp.h"

#define NDEBUG
#include <debug.h>

typedef struct _ACPI_GPE_CONNECTION
{
  ACPI_HANDLE GpeDevice;
  UINT32 GpeNumber;
  PGPE_SERVICE_ROUTINE ServiceRoutine;
  PVOID ServiceContext;
} ACPI_GPE_CONNECTION, *PACPI_GPE_CONNECTION;

static
__inline
PPDO_DEVICE_DATA
AcpiPdoDataFromInterfaceContext(_In_ PVOID Context)
{
  PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)Context;

  if (!DeviceObject)
    return NULL;

  return (PPDO_DEVICE_DATA)DeviceObject->DeviceExtension;
}

static
NTSTATUS
AcpiStatusToNtStatus(_In_ ACPI_STATUS Status)
{
  if (ACPI_SUCCESS(Status))
    return STATUS_SUCCESS;

  switch (Status)
  {
    case AE_NO_MEMORY:
      return STATUS_INSUFFICIENT_RESOURCES;
    case AE_BAD_PARAMETER:
      return STATUS_INVALID_PARAMETER;
    case AE_NOT_FOUND:
      return STATUS_NOT_FOUND;
    case AE_ALREADY_EXISTS:
      return STATUS_OBJECT_NAME_COLLISION;
    default:
      return STATUS_UNSUCCESSFUL;
  }
}

static
UINT32
AcpiGpeHandlerThunk(
  _In_ ACPI_HANDLE GpeDevice,
  _In_ UINT32 GpeNumber,
  _In_opt_ PVOID Context)
{
  PACPI_GPE_CONNECTION Connection = (PACPI_GPE_CONNECTION)Context;
  BOOLEAN Handled;

  UNREFERENCED_PARAMETER(GpeDevice);
  UNREFERENCED_PARAMETER(GpeNumber);

  if (!Connection || !Connection->ServiceRoutine)
    return ACPI_INTERRUPT_NOT_HANDLED | ACPI_REENABLE_GPE;

  Handled = Connection->ServiceRoutine((PVOID)Connection, Connection->ServiceContext);
  return (Handled ? ACPI_INTERRUPT_HANDLED : ACPI_INTERRUPT_NOT_HANDLED) | ACPI_REENABLE_GPE;
}

static
VOID
AcpiPdoNotifyHandlerThunk(
  _In_ ACPI_HANDLE Device,
  _In_ UINT32 Value,
  _In_opt_ PVOID Context)
{
  PPDO_DEVICE_DATA PdoData = (PPDO_DEVICE_DATA)Context;
  PACPI_NOTIFY_ENTRY *Snapshot = NULL;
  ULONG Count = 0;
  ULONG Index = 0;

  UNREFERENCED_PARAMETER(Device);

  if (!PdoData)
    return;

  ExAcquireFastMutex(&PdoData->NotifyLock);

  for (PLIST_ENTRY Entry = PdoData->NotifyList.Flink;
     Entry != &PdoData->NotifyList;
     Entry = Entry->Flink)
  {
    Count++;
  }

  if (Count)
  {
    Snapshot = ExAllocatePoolWithTag(NonPagedPool,
                     Count * sizeof(*Snapshot),
                     'NpcA');
    if (Snapshot)
    {
      for (PLIST_ENTRY Entry = PdoData->NotifyList.Flink;
         Entry != &PdoData->NotifyList && Index < Count;
         Entry = Entry->Flink)
      {
        Snapshot[Index++] = CONTAINING_RECORD(Entry, ACPI_NOTIFY_ENTRY, Link);
      }
      Count = Index;
    }
    else
    {
      Count = 0;
    }
  }

  ExReleaseFastMutex(&PdoData->NotifyLock);

  for (Index = 0; Index < Count; Index++)
  {
    if (Snapshot[Index] && Snapshot[Index]->Callback)
      Snapshot[Index]->Callback(Snapshot[Index]->CallbackContext, (ULONG)Value);
  }

  if (Snapshot)
    ExFreePoolWithTag(Snapshot, 'NpcA');
}

VOID
AcpiInterfaceCleanupPdoNotifications(
  _Inout_ PPDO_DEVICE_DATA PdoData)
{
  if (!PdoData)
    return;

  ExAcquireFastMutex(&PdoData->NotifyLock);

  while (!IsListEmpty(&PdoData->NotifyList))
  {
    PLIST_ENTRY Link = RemoveHeadList(&PdoData->NotifyList);
    ExFreePoolWithTag(CONTAINING_RECORD(Link, ACPI_NOTIFY_ENTRY, Link), 'NpcA');
  }

  if (PdoData->NotifyHandlerInstalled && PdoData->AcpiHandle)
  {
    (void)AcpiRemoveNotifyHandler(PdoData->AcpiHandle,
                    ACPI_ALL_NOTIFY,
                    AcpiPdoNotifyHandlerThunk);
    PdoData->NotifyHandlerInstalled = FALSE;
  }

  ExReleaseFastMutex(&PdoData->NotifyLock);
}

VOID
NTAPI
AcpiInterfaceReference(PVOID Context)
{
  PPDO_DEVICE_DATA PdoData = AcpiPdoDataFromInterfaceContext(Context);

  if (!PdoData)
    return;

  InterlockedIncrement((volatile LONG *)&PdoData->InterfaceRefCount);
  ObReferenceObject(PdoData->Common.Self);
}

VOID
NTAPI
AcpiInterfaceDereference(PVOID Context)
{
  PPDO_DEVICE_DATA PdoData = AcpiPdoDataFromInterfaceContext(Context);

  if (!PdoData)
      return;

  ObDereferenceObject(PdoData->Common.Self);
  InterlockedDecrement((volatile LONG *)&PdoData->InterfaceRefCount);
}

NTSTATUS
NTAPI
AcpiInterfaceConnectVector(PDEVICE_OBJECT Context,
                           ULONG GpeNumber,
                           KINTERRUPT_MODE Mode,
                           BOOLEAN Shareable,
                           PGPE_SERVICE_ROUTINE ServiceRoutine,
                           PVOID ServiceContext,
               PVOID *ObjectContext)
{
  PPDO_DEVICE_DATA PdoData;
  PACPI_GPE_CONNECTION Connection;
  UINT32 Type;
  ACPI_STATUS Status;

  UNREFERENCED_PARAMETER(Shareable);

  if (!Context || !ObjectContext || !ServiceRoutine)
    return STATUS_INVALID_PARAMETER;

  *ObjectContext = NULL;

  PdoData = (PPDO_DEVICE_DATA)Context->DeviceExtension;
  if (!PdoData)
    return STATUS_INVALID_PARAMETER;

  if (!PdoData->AcpiHandle)
    return STATUS_NOT_SUPPORTED;

  Connection = ExAllocatePoolWithTag(NonPagedPool,
                   sizeof(*Connection),
                   'GpcA');
  if (!Connection)
    return STATUS_INSUFFICIENT_RESOURCES;

  RtlZeroMemory(Connection, sizeof(*Connection));
  Connection->GpeDevice = PdoData->AcpiHandle;
  Connection->GpeNumber = (UINT32)GpeNumber;
  Connection->ServiceRoutine = ServiceRoutine;
  Connection->ServiceContext = ServiceContext;

  Type = (Mode == LevelSensitive) ? ACPI_GPE_LEVEL_TRIGGERED : ACPI_GPE_EDGE_TRIGGERED;
  Status = AcpiInstallGpeHandler(Connection->GpeDevice,
                 Connection->GpeNumber,
                 Type,
                 AcpiGpeHandlerThunk,
                 Connection);
  if (ACPI_FAILURE(Status))
  {
    ExFreePoolWithTag(Connection, 'GpcA');
    return AcpiStatusToNtStatus(Status);
  }

  *ObjectContext = (PVOID)Connection;
  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
AcpiInterfaceConnectVector2(
  _In_ PVOID Context,
  _In_ ULONG GpeNumber,
  _In_ KINTERRUPT_MODE Mode,
  _In_ BOOLEAN Shareable,
  _In_ PGPE_SERVICE_ROUTINE ServiceRoutine,
  _In_opt_ PVOID ServiceContext,
  _Outptr_ PVOID *ObjectContext)
{
  return AcpiInterfaceConnectVector((PDEVICE_OBJECT)Context,
                    GpeNumber,
                    Mode,
                    Shareable,
                    ServiceRoutine,
                    ServiceContext,
                    ObjectContext);
}

NTSTATUS
NTAPI
AcpiInterfaceDisconnectVector(PVOID ObjectContext)
{
  PACPI_GPE_CONNECTION Connection = (PACPI_GPE_CONNECTION)ObjectContext;
  ACPI_STATUS Status;

  if (!Connection)
      return STATUS_INVALID_PARAMETER;

  Status = AcpiRemoveGpeHandler(Connection->GpeDevice,
                                Connection->GpeNumber,
                                AcpiGpeHandlerThunk);
  ExFreePoolWithTag(Connection, 'GpcA');
  return AcpiStatusToNtStatus(Status);
}

NTSTATUS
NTAPI
AcpiInterfaceDisconnectVector2(
  _In_ PVOID Context,
  _In_ PVOID ObjectContext)
{
  UNREFERENCED_PARAMETER(Context);
  return AcpiInterfaceDisconnectVector(ObjectContext);
}

NTSTATUS
NTAPI
AcpiInterfaceEnableEvent(PDEVICE_OBJECT Context,
                         PVOID ObjectContext)
{
  PACPI_GPE_CONNECTION Connection = (PACPI_GPE_CONNECTION)ObjectContext;
  ACPI_STATUS Status;

  UNREFERENCED_PARAMETER(Context);

  if (!Connection)
      return STATUS_INVALID_PARAMETER;

  Status = AcpiEnableGpe(Connection->GpeDevice, Connection->GpeNumber);
  return AcpiStatusToNtStatus(Status);
}

NTSTATUS
NTAPI
AcpiInterfaceEnableEvent2(
  _In_ PVOID Context,
  _In_ PVOID ObjectContext)
{
  return AcpiInterfaceEnableEvent((PDEVICE_OBJECT)Context, ObjectContext);
}

NTSTATUS
NTAPI
AcpiInterfaceDisableEvent(PDEVICE_OBJECT Context,
                          PVOID ObjectContext)
{
  PACPI_GPE_CONNECTION Connection = (PACPI_GPE_CONNECTION)ObjectContext;
  ACPI_STATUS Status;

  UNREFERENCED_PARAMETER(Context);

  if (!Connection)
      return STATUS_INVALID_PARAMETER;

  Status = AcpiDisableGpe(Connection->GpeDevice, Connection->GpeNumber);
  return AcpiStatusToNtStatus(Status);
}

NTSTATUS
NTAPI
AcpiInterfaceDisableEvent2(
  _In_ PVOID Context,
  _In_ PVOID ObjectContext)
{
  return AcpiInterfaceDisableEvent((PDEVICE_OBJECT)Context, ObjectContext);
}

NTSTATUS
NTAPI
AcpiInterfaceClearStatus(PDEVICE_OBJECT Context,
                         PVOID ObjectContext)
{
  PACPI_GPE_CONNECTION Connection = (PACPI_GPE_CONNECTION)ObjectContext;
  ACPI_STATUS Status;

  UNREFERENCED_PARAMETER(Context);

  if (!Connection)
      return STATUS_INVALID_PARAMETER;

  Status = AcpiClearGpe(Connection->GpeDevice, Connection->GpeNumber);
  return AcpiStatusToNtStatus(Status);
}

NTSTATUS
NTAPI
AcpiInterfaceClearStatus2(
  _In_ PVOID Context,
  _In_ PVOID ObjectContext)
{
  return AcpiInterfaceClearStatus((PDEVICE_OBJECT)Context, ObjectContext);
}

NTSTATUS
NTAPI
AcpiInterfaceNotificationsRegister(PDEVICE_OBJECT Context,
                                   PDEVICE_NOTIFY_CALLBACK NotificationHandler,
                                   PVOID NotificationContext)
{
  PPDO_DEVICE_DATA PdoData;
  PACPI_NOTIFY_ENTRY Entry;
  ACPI_STATUS Status;

  if (!Context || !NotificationHandler)
    return STATUS_INVALID_PARAMETER;

  PdoData = (PPDO_DEVICE_DATA)Context->DeviceExtension;
  if (!PdoData)
    return STATUS_INVALID_PARAMETER;

  if (!PdoData->AcpiHandle)
    return STATUS_NOT_SUPPORTED;

  Entry = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Entry), 'NpcA');
  if (!Entry)
    return STATUS_INSUFFICIENT_RESOURCES;

  RtlZeroMemory(Entry, sizeof(*Entry));
  Entry->Callback = NotificationHandler;
  Entry->CallbackContext = NotificationContext;

  ExAcquireFastMutex(&PdoData->NotifyLock);

  if (!PdoData->NotifyHandlerInstalled)
  {
    Status = AcpiInstallNotifyHandler(PdoData->AcpiHandle,
                    ACPI_ALL_NOTIFY,
                    AcpiPdoNotifyHandlerThunk,
                    PdoData);
    if (ACPI_FAILURE(Status))
    {
      ExReleaseFastMutex(&PdoData->NotifyLock);
      ExFreePoolWithTag(Entry, 'NpcA');
      return AcpiStatusToNtStatus(Status);
    }
    PdoData->NotifyHandlerInstalled = TRUE;
  }

  InsertTailList(&PdoData->NotifyList, &Entry->Link);

  ExReleaseFastMutex(&PdoData->NotifyLock);
  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
AcpiInterfaceNotificationsRegister2(
  _In_ PVOID Context,
  _In_ PDEVICE_NOTIFY_CALLBACK2 NotificationHandler,
  _In_opt_ PVOID NotificationContext)
{
  return AcpiInterfaceNotificationsRegister((PDEVICE_OBJECT)Context,
                        (PDEVICE_NOTIFY_CALLBACK)NotificationHandler,
                        NotificationContext);
}

VOID
NTAPI
AcpiInterfaceNotificationsUnregister(PDEVICE_OBJECT Context,
                                     PDEVICE_NOTIFY_CALLBACK NotificationHandler)
{
  PPDO_DEVICE_DATA PdoData;
  BOOLEAN Empty;

  if (!Context || !NotificationHandler)
    return;

  PdoData = (PPDO_DEVICE_DATA)Context->DeviceExtension;
  if (!PdoData)
    return;

  ExAcquireFastMutex(&PdoData->NotifyLock);

  for (PLIST_ENTRY Link = PdoData->NotifyList.Flink; Link != &PdoData->NotifyList; Link = Link->Flink)
  {
    PACPI_NOTIFY_ENTRY Entry = CONTAINING_RECORD(Link, ACPI_NOTIFY_ENTRY, Link);
    if (Entry->Callback == NotificationHandler)
    {
      RemoveEntryList(&Entry->Link);
      ExFreePoolWithTag(Entry, 'NpcA');
      break;
    }
  }

  Empty = IsListEmpty(&PdoData->NotifyList);
  if (Empty && PdoData->NotifyHandlerInstalled && PdoData->AcpiHandle)
  {
    (void)AcpiRemoveNotifyHandler(PdoData->AcpiHandle,
                  ACPI_ALL_NOTIFY,
                  AcpiPdoNotifyHandlerThunk);
    PdoData->NotifyHandlerInstalled = FALSE;
  }

  ExReleaseFastMutex(&PdoData->NotifyLock);
}

VOID
NTAPI
AcpiInterfaceNotificationsUnregister2(
  _In_ PVOID Context)
{
  PPDO_DEVICE_DATA PdoData = AcpiPdoDataFromInterfaceContext(Context);

  if (!PdoData)
    return;

  AcpiInterfaceCleanupPdoNotifications(PdoData);
}

NTSTATUS
Bus_PDO_QueryInterface(PPDO_DEVICE_DATA DeviceData,
                       PIRP Irp)
{
  PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
  PACPI_INTERFACE_STANDARD AcpiInterface;
  PACPI_INTERFACE_STANDARD2 AcpiInterface2;
  NTSTATUS Status;

  if (!DeviceData || !IrpSp)
    return STATUS_INVALID_PARAMETER;

  if (RtlCompareMemory(IrpSp->Parameters.QueryInterface.InterfaceType,
                        &GUID_ACPI_INTERFACE_STANDARD, sizeof(GUID)) == sizeof(GUID))
  {
      DPRINT("GUID_ACPI_INTERFACE_STANDARD\n");

    switch (IrpSp->Parameters.QueryInterface.Version)
    {
      case 1:
        if (IrpSp->Parameters.QueryInterface.Size < sizeof(ACPI_INTERFACE_STANDARD))
        {
          DPRINT1("Buffer too small! (%d)\n", IrpSp->Parameters.QueryInterface.Size);
          Status = STATUS_BUFFER_TOO_SMALL;
          break;
        }

        AcpiInterface = (PACPI_INTERFACE_STANDARD)IrpSp->Parameters.QueryInterface.Interface;

        AcpiInterface->Size = sizeof(ACPI_INTERFACE_STANDARD);
        AcpiInterface->Version = 1;
        AcpiInterface->Context = DeviceData->Common.Self;
        AcpiInterface->InterfaceReference = AcpiInterfaceReference;
        AcpiInterface->InterfaceDereference = AcpiInterfaceDereference;
        AcpiInterface->GpeConnectVector = AcpiInterfaceConnectVector;
        AcpiInterface->GpeDisconnectVector = AcpiInterfaceDisconnectVector;
        AcpiInterface->GpeEnableEvent = AcpiInterfaceEnableEvent;
        AcpiInterface->GpeDisableEvent = AcpiInterfaceDisableEvent;
        AcpiInterface->GpeClearStatus = AcpiInterfaceClearStatus;
        AcpiInterface->RegisterForDeviceNotifications = AcpiInterfaceNotificationsRegister;
        AcpiInterface->UnregisterForDeviceNotifications = AcpiInterfaceNotificationsUnregister;

        AcpiInterfaceReference(DeviceData->Common.Self);
        Status = STATUS_SUCCESS;
        break;

      case 2:
        if (IrpSp->Parameters.QueryInterface.Size < sizeof(ACPI_INTERFACE_STANDARD2))
        {
          DPRINT1("Buffer too small! (%d)\n", IrpSp->Parameters.QueryInterface.Size);
          Status = STATUS_BUFFER_TOO_SMALL;
          break;
        }

        AcpiInterface2 = (PACPI_INTERFACE_STANDARD2)IrpSp->Parameters.QueryInterface.Interface;
        AcpiInterface2->Size = sizeof(ACPI_INTERFACE_STANDARD2);
        AcpiInterface2->Version = 2;
        AcpiInterface2->Context = DeviceData->Common.Self;
        AcpiInterface2->InterfaceReference = AcpiInterfaceReference;
        AcpiInterface2->InterfaceDereference = AcpiInterfaceDereference;
              AcpiInterface2->GpeConnectVector = AcpiInterfaceConnectVector2;
              AcpiInterface2->GpeDisconnectVector = AcpiInterfaceDisconnectVector2;
              AcpiInterface2->GpeEnableEvent = AcpiInterfaceEnableEvent2;
              AcpiInterface2->GpeDisableEvent = AcpiInterfaceDisableEvent2;
              AcpiInterface2->GpeClearStatus = AcpiInterfaceClearStatus2;
              AcpiInterface2->RegisterForDeviceNotifications = AcpiInterfaceNotificationsRegister2;
              AcpiInterface2->UnregisterForDeviceNotifications = AcpiInterfaceNotificationsUnregister2;

        AcpiInterfaceReference(DeviceData->Common.Self);
        Status = STATUS_SUCCESS;
        break;

      default:
        DPRINT1("Invalid version number: %d\n",
            IrpSp->Parameters.QueryInterface.Version);
        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    Irp->IoStatus.Status = Status;
    return Status;
  }
  else
  {
      DPRINT1("Invalid GUID\n");
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    return STATUS_NOT_SUPPORTED;
  }
}
