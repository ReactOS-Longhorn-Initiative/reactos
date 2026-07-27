/*
 * PROJECT:     ReactOS NT User Mode Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ntdll ALPC message-attribute helper routines
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

/* DATA ***********************************************************************/

/*
 * The attribute payloads follow the ALPC_MESSAGE_ATTRIBUTES header in the
 * order listed here (highest presence bit first). A given attribute is only
 * present when its bit is set in AllocatedAttributes.
 */
typedef struct _ALPC_ATTRIBUTE_DESCRIPTOR
{
    ULONG Flag;
    ULONG Size;
} ALPC_ATTRIBUTE_DESCRIPTOR;

static const ALPC_ATTRIBUTE_DESCRIPTOR AlpcpAttributeLayout[] =
{
    { ALPC_MESSAGE_SECURITY_ATTRIBUTE, sizeof(ALPC_SECURITY_ATTR) },
    { ALPC_MESSAGE_VIEW_ATTRIBUTE,     sizeof(ALPC_DATA_VIEW_ATTR) },
    { ALPC_MESSAGE_CONTEXT_ATTRIBUTE,  sizeof(ALPC_CONTEXT_ATTR) },
    { ALPC_MESSAGE_HANDLE_ATTRIBUTE,   sizeof(ALPC_HANDLE_ATTR) },
    { ALPC_MESSAGE_TOKEN_ATTRIBUTE,    sizeof(ALPC_TOKEN_ATTR) },
};

#define ALPC_MESSAGE_ATTRIBUTE_MASK \
    (ALPC_MESSAGE_SECURITY_ATTRIBUTE | ALPC_MESSAGE_VIEW_ATTRIBUTE | \
     ALPC_MESSAGE_CONTEXT_ATTRIBUTE | ALPC_MESSAGE_HANDLE_ATTRIBUTE | \
     ALPC_MESSAGE_TOKEN_ATTRIBUTE)

/* FUNCTIONS ******************************************************************/

/**
 * @brief Returns the size of the attribute block needed for a set of attributes.
 *
 * @param[in] Flags
 * Combination of ALPC_MESSAGE_*_ATTRIBUTE presence bits.
 *
 * @return The size in bytes of the ALPC_MESSAGE_ATTRIBUTES header plus every
 * requested attribute payload. With no attributes this is the bare header.
 */
ULONG
NTAPI
AlpcGetHeaderSize(
    _In_ ULONG Flags)
{
    ULONG Size = sizeof(ALPC_MESSAGE_ATTRIBUTES);
    ULONG i;

    for (i = 0; i < RTL_NUMBER_OF(AlpcpAttributeLayout); i++)
    {
        if (Flags & AlpcpAttributeLayout[i].Flag)
            Size += AlpcpAttributeLayout[i].Size;
    }

    return Size;
}

/**
 * @brief Validates and initializes an ALPC message-attribute buffer.
 *
 * @param[in] AttributeFlags
 * The attributes to allocate room for (ALPC_MESSAGE_*_ATTRIBUTE).
 *
 * @param[out] Buffer
 * Buffer to initialize. May be NULL purely to query the required size.
 *
 * @param[in] BufferSize
 * Size of @p Buffer in bytes.
 *
 * @param[out] RequiredBufferSize
 * Receives the number of bytes required for @p AttributeFlags.
 *
 * @return STATUS_SUCCESS once the buffer is initialized, or
 * STATUS_BUFFER_TOO_SMALL if @p BufferSize is insufficient (with
 * @p RequiredBufferSize filled in).
 */
NTSTATUS
NTAPI
AlpcInitializeMessageAttribute(
    _In_ ULONG AttributeFlags,
    _Out_opt_ PALPC_MESSAGE_ATTRIBUTES Buffer,
    _In_ SIZE_T BufferSize,
    _Out_ PSIZE_T RequiredBufferSize)
{
    ULONG Allocated = AttributeFlags & ALPC_MESSAGE_ATTRIBUTE_MASK;
    SIZE_T Required = AlpcGetHeaderSize(Allocated);

    *RequiredBufferSize = Required;

    if (BufferSize < Required)
        return STATUS_BUFFER_TOO_SMALL;

    Buffer->AllocatedAttributes = Allocated;
    Buffer->ValidAttributes = 0;
    return STATUS_SUCCESS;
}

/**
 * @brief Returns a pointer to a single attribute within a message-attribute buffer.
 *
 * The attribute must be present in @p Buffer->AllocatedAttributes; this routine
 * deliberately keys off AllocatedAttributes rather than ValidAttributes.
 *
 * @param[in] Buffer
 * An attribute buffer previously set up by AlpcInitializeMessageAttribute.
 *
 * @param[in] AttributeFlag
 * The single attribute to locate (one ALPC_MESSAGE_*_ATTRIBUTE bit).
 *
 * @return A pointer to the attribute payload, or NULL if it is not allocated.
 */
PVOID
NTAPI
AlpcGetMessageAttribute(
    _In_ PALPC_MESSAGE_ATTRIBUTES Buffer,
    _In_ ULONG AttributeFlag)
{
    PUCHAR Payload;
    ULONG i;

    if (!(Buffer->AllocatedAttributes & AttributeFlag))
        return NULL;

    Payload = (PUCHAR)(Buffer + 1);
    for (i = 0; i < RTL_NUMBER_OF(AlpcpAttributeLayout); i++)
    {
        if (!(Buffer->AllocatedAttributes & AlpcpAttributeLayout[i].Flag))
            continue;

        if (AlpcpAttributeLayout[i].Flag == AttributeFlag)
            return Payload;

        Payload += AlpcpAttributeLayout[i].Size;
    }

    return NULL;
}

/**
 * @brief Dequeues the next message from a port's completion-list ring.
 *
 * The kernel publishes received messages into fixed-size slots starting at
 * ALPC_COMPLETION_LIST_HEADER.ListOffset and advances Head; this routine returns
 * the slot at Tail and advances it. The returned message stays valid until it is
 * released with AlpcFreeCompletionListMessage.
 *
 * @param[in] CompletionList
 * Base of the registered completion-list buffer.
 *
 * @return A pointer to the next PORT_MESSAGE, or NULL if the ring is empty.
 */
PVOID
NTAPI
AlpcGetMessageFromCompletionList(
    _In_ PVOID CompletionList,
    _Reserved_ PVOID Reserved)
{
    PALPC_COMPLETION_LIST_HEADER Header = (PALPC_COMPLETION_LIST_HEADER)CompletionList;
    PVOID Slot;
    ULONG Index;

    UNREFERENCED_PARAMETER(Reserved);

    if (Header == NULL || Header->StartMagic != ALPC_COMPLETION_LIST_START_MAGIC)
        return NULL;
    if (Header->SlotCount == 0)
        return NULL;
    if (Header->Tail == Header->Head)
        return NULL;

    Index = Header->Tail % Header->SlotCount;
    Slot = (PUCHAR)Header + Header->ListOffset + (SIZE_T)Index * Header->SlotSize;
    Header->Tail++;

    return Slot;
}

/**
 * @brief Releases a message returned by AlpcGetMessageFromCompletionList.
 *
 * The slot is returned to the ring (the dequeue already advanced the read
 * cursor), so this is a validation step.
 *
 * @return TRUE if @p Message lies within the completion-list buffer.
 */
BOOLEAN
NTAPI
AlpcFreeCompletionListMessage(
    _In_ PVOID CompletionList,
    _In_ PVOID Message)
{
    PALPC_COMPLETION_LIST_HEADER Header = (PALPC_COMPLETION_LIST_HEADER)CompletionList;
    PUCHAR Base;

    if (Header == NULL || Message == NULL ||
        Header->StartMagic != ALPC_COMPLETION_LIST_START_MAGIC)
    {
        return FALSE;
    }

    Base = (PUCHAR)Header + Header->ListOffset;
    if ((PUCHAR)Message < Base || (PUCHAR)Message >= Base + Header->ListSize)
        return FALSE;

    return TRUE;
}
