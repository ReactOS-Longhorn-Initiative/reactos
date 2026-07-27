/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Private ALPC kernel definitions (ports, messages, views, ...)
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * Layouts mirror the Windows kernel (ntkrpamp / x86). Public ALPC types
 * (ALPC_PORT_ATTRIBUTES, ALPC_MESSAGE_ATTRIBUTES, the information classes and
 * the flag macros) come from <ndk/lpctypes.h> and are intentionally not
 * redefined here.
 */

#ifndef _NTOSKRNL_ALPC_H_
#define _NTOSKRNL_ALPC_H_

/* Completion-list buffer granularity (not part of the public ABI). */
#define ALPC_COMPLETION_LIST_BUFFER_GRANULARITY_MASK 0x3f

/* IoSetIoCompletionEx2 mini packet (used by completion ports). */
typedef struct _IO_MINI_COMPLETION_PACKET_USER
{
    LIST_ENTRY ListEntry;
    ULONG PacketType;
    PVOID KeyContext;
    PVOID ApcContext;
    LONG IoStatus;
    ULONG IoStatusInformation;
    VOID (*MiniPacketCallback)(struct _IO_MINI_COMPLETION_PACKET_USER *, PVOID);
    PVOID Context;
    UCHAR Allocated;
} IO_MINI_COMPLETION_PACKET_USER, *PIO_MINI_COMPLETION_PACKET_USER;

typedef struct _OB_DUPLICATE_OBJECT_STATE
{
    PEPROCESS SourceProcess;
    PVOID SourceHandle;
    PVOID Object;
    ULONG TargetAccess;
    HANDLE_TABLE_ENTRY_INFO ObjectInfo;
    ULONG HandleAttributes;
} OB_DUPLICATE_OBJECT_STATE, *POB_DUPLICATE_OBJECT_STATE;

typedef union _KALPC_DIRECT_EVENT
{
    UINT_PTR Value;
    struct
    {
        UINT_PTR DirectType:1;
        UINT_PTR EventReferenced:1;
#if defined(_M_AMD64)
        UINT64 EventObjectBits:62;
#else
        ULONG32 EventObjectBits:30;
#endif
    };
} KALPC_DIRECT_EVENT, *PKALPC_DIRECT_EVENT;

typedef struct _ALPC_PORT_REFERENCE_WAIT_BLOCK
{
    KEVENT DesiredReferenceNoEvent;
    LONG DesiredReferenceNo;
} ALPC_PORT_REFERENCE_WAIT_BLOCK, *PALPC_PORT_REFERENCE_WAIT_BLOCK;

typedef struct _ALPC_WORK_ON_BEHALF_TICKET
{
    ULONG ThreadId;
    ULONG ThreadCreationTimeLow;
} ALPC_WORK_ON_BEHALF_TICKET, *PALPC_WORK_ON_BEHALF_TICKET;

typedef struct _KALPC_WORK_ON_BEHALF_DATA
{
    ALPC_WORK_ON_BEHALF_TICKET Ticket;
} KALPC_WORK_ON_BEHALF_DATA, *PKALPC_WORK_ON_BEHALF_DATA;

typedef struct _ALPC_HANDLE_ENTRY
{
    PVOID Object;
} ALPC_HANDLE_ENTRY, *PALPC_HANDLE_ENTRY;

typedef struct _ALPC_HANDLE_TABLE
{
    PALPC_HANDLE_ENTRY Handles;
    ULONG TotalHandles;
    ULONG Flags;
    EX_PUSH_LOCK Lock;
} ALPC_HANDLE_TABLE, *PALPC_HANDLE_TABLE;

typedef struct _KALPC_VIEW
{
    LIST_ENTRY ViewListEntry;
    struct _KALPC_REGION *Region;
    struct _ALPC_PORT *OwnerPort;
    PEPROCESS OwnerProcess;
    PVOID Address;
    ULONG Size;
    PVOID SecureViewHandle;
    PVOID WriteAccessHandle;
    struct
    {
        ULONG WriteAccess:1;
        ULONG AutoRelease:1;
        ULONG ForceUnlink:1;
    } u1;
    ULONG NumberOfOwnerMessages;
    LIST_ENTRY ProcessViewListEntry;
} KALPC_VIEW, *PKALPC_VIEW;

typedef struct _KALPC_SECURITY_DATA
{
    PALPC_HANDLE_TABLE HandleTable;
    PVOID ContextHandle;
    PEPROCESS OwningProcess;
    struct _ALPC_PORT *OwnerPort;
    SECURITY_CLIENT_CONTEXT DynamicSecurity;
    struct
    {
        ULONG Revoked:1;
        ULONG Impersonated:1;
    } u1;
} KALPC_SECURITY_DATA, *PKALPC_SECURITY_DATA;

typedef struct _KALPC_SECTION
{
    PVOID SectionObject;
    ULONG Size;
    PALPC_HANDLE_TABLE HandleTable;
    PVOID SectionHandle;
    PEPROCESS OwnerProcess;
    struct _ALPC_PORT *OwnerPort;
    struct
    {
        ULONG Internal:1;
        ULONG Secure:1;
    } u1;
    ULONG NumberOfRegions;
    LIST_ENTRY RegionListHead;
} KALPC_SECTION, *PKALPC_SECTION;

typedef struct _KALPC_RESERVE
{
    struct _ALPC_PORT *OwnerPort;
    PALPC_HANDLE_TABLE HandleTable;
    PVOID Handle;
    struct _KALPC_MESSAGE *Message;
    LONG Active;
} KALPC_RESERVE, *PKALPC_RESERVE;

typedef struct _KALPC_HANDLE_DATA
{
    ULONG ObjectType;
    ULONG Count;
    OB_DUPLICATE_OBJECT_STATE DuplicateContext;
} KALPC_HANDLE_DATA, *PKALPC_HANDLE_DATA;

typedef struct _KALPC_MESSAGE_ATTRIBUTES
{
    PVOID ClientContext;
    PVOID ServerContext;
    PVOID PortContext;
    PVOID CancelPortContext;
    PKALPC_SECURITY_DATA SecurityData;
    PKALPC_VIEW View;
    PKALPC_HANDLE_DATA HandleData;
    KALPC_DIRECT_EVENT DirectEvent;
    KALPC_WORK_ON_BEHALF_DATA WorkOnBehalfData;
} KALPC_MESSAGE_ATTRIBUTES, *PKALPC_MESSAGE_ATTRIBUTES;

typedef struct _KALPC_MESSAGE
{
    LIST_ENTRY Entry;
    struct _ALPC_PORT *PortQueue;
    struct _ALPC_PORT *OwnerPort;
    PETHREAD WaitingThread;
    union
    {
        struct
        {
            ULONG QueueType:3;
            ULONG QueuePortType:4;
            ULONG Canceled:1;
            ULONG Ready:1;
            ULONG ReleaseMessage:1;
            ULONG SharedQuota:1;
            ULONG ReplyWaitReply:1;
            ULONG OwnerPortReference:1;
            ULONG ReserveReference:1;
            ULONG ReceiverReference:1;
            ULONG ViewAttributeRetrieved:1;
            ULONG InDispatch:1;
        };
        ULONG State;
    } u1;
    LONG SequenceNo;
    union
    {
        PEPROCESS QuotaProcess;
        PVOID QuotaBlock;
    };
    struct _ALPC_PORT *CancelSequencePort;
    struct _ALPC_PORT *CancelQueuePort;
    LONG CancelSequenceNo;
    LIST_ENTRY CancelListEntry;
    PKALPC_RESERVE Reserve;
    KALPC_MESSAGE_ATTRIBUTES MessageAttributes;
    PVOID DataUserVa;
    struct _ALPC_COMMUNICATION_INFO *CommunicationInfo;
    struct _ALPC_PORT *ConnectionPort;
    PETHREAD ServerThread;
    PVOID WakeReference;
    PVOID ExtensionBuffer;
    ULONG ExtensionBufferSize;
    PORT_MESSAGE PortMessage;
} KALPC_MESSAGE, *PKALPC_MESSAGE;

typedef struct _ALPC_PROCESS_CONTEXT
{
    EX_PUSH_LOCK Lock;
    LIST_ENTRY ViewListHead;
    volatile ULONG PagedPoolQuotaCache;
} ALPC_PROCESS_CONTEXT, *PALPC_PROCESS_CONTEXT;

typedef struct _ALPC_PORT
{
    LIST_ENTRY PortListEntry;
    struct _ALPC_COMMUNICATION_INFO *CommunicationInfo;
    PEPROCESS OwnerProcess;
    PVOID CompletionPort;
    PVOID CompletionKey;
    struct _ALPC_COMPLETION_PACKET_LOOKASIDE *CompletionPacketLookaside;
    PVOID PortContext;
    SECURITY_CLIENT_CONTEXT StaticSecurity;
    EX_PUSH_LOCK IncomingQueueLock;
    LIST_ENTRY MainQueue;
    LIST_ENTRY LargeMessageQueue;
    EX_PUSH_LOCK PendingQueueLock;
    LIST_ENTRY PendingQueue;
    EX_PUSH_LOCK DirectQueueLock;
    LIST_ENTRY DirectQueue;
    EX_PUSH_LOCK WaitQueueLock;
    LIST_ENTRY WaitQueue;
    union
    {
        PKSEMAPHORE Semaphore;
        PKEVENT DummyEvent;
    };
    ALPC_PORT_ATTRIBUTES PortAttributes;
    EX_PUSH_LOCK ResourceListLock;
    LIST_ENTRY ResourceListHead;
    EX_PUSH_LOCK PortObjectLock;
    struct _ALPC_COMPLETION_LIST *CompletionList;
    PVOID CallbackObject;
    PVOID CallbackContext;
    LIST_ENTRY CanceledQueue;
    LONG SequenceNo;
    LONG ReferenceNo;
    PALPC_PORT_REFERENCE_WAIT_BLOCK *ReferenceNoWait;
    union
    {
        struct
        {
            ULONG Initialized:1;
            ULONG Type:2;
            ULONG ConnectionPending:1;
            ULONG ConnectionRefused:1;
            ULONG Disconnected:1;
            ULONG Closed:1;
            ULONG NoFlushOnClose:1;
            ULONG ReturnExtendedInfo:1;
            ULONG Waitable:1;
            ULONG DynamicSecurity:1;
            ULONG Wow64CompletionList:1;
            ULONG Lpc:1;
            ULONG LpcToLpc:1;
            ULONG HasCompletionList:1;
            ULONG HadCompletionList:1;
            ULONG EnableCompletionList:1;
        };
        ULONG State;
    } u1;
    struct _ALPC_PORT *TargetQueuePort;
    struct _ALPC_PORT *TargetSequencePort;
    struct _KALPC_MESSAGE *CachedMessage;
    ULONG CachedConnectionMessageId;
    struct _ALPC_PORT *PendingClientPort;
    ULONG MainQueueLength;
    ULONG LargeMessageQueueLength;
    ULONG PendingQueueLength;
    ULONG DirectQueueLength;
    ULONG CanceledQueueLength;
    ULONG WaitQueueLength;
    /* Legacy LPC connection shared-section view (the client's "write" section,
     * mapped into both the client and the server so CSR-style clients can pass
     * capture buffers). Tracked on the client communication port. */
    PVOID LpcViewSection;
    PVOID LpcClientViewBase;
    PVOID LpcServerViewBase;
    SIZE_T LpcViewSize;
} ALPC_PORT, *PALPC_PORT;

typedef struct _ALPC_COMPLETION_LIST
{
    PMDL Mdl;               /* locks the user buffer pages */
    PVOID SystemVa;         /* kernel mapping of the buffer (== Header) */
    PVOID UserVa;           /* user view base the caller polls through */
    SIZE_T Size;
    struct _ALPC_COMPLETION_LIST_HEADER *Header;
} ALPC_COMPLETION_LIST, *PALPC_COMPLETION_LIST;

typedef struct _ALPC_COMMUNICATION_INFO
{
    PALPC_PORT ConnectionPort;
    PALPC_PORT ServerCommunicationPort;
    PALPC_PORT ClientCommunicationPort;
    LIST_ENTRY CommunicationList;
    ALPC_HANDLE_TABLE HandleTable;
    PKALPC_MESSAGE CloseMessage;
} ALPC_COMMUNICATION_INFO, *PALPC_COMMUNICATION_INFO;

/* Pool tags. */
#define TAG_ALPC_PORT    'PclA'  /* AlpC */
#define TAG_ALPC_COMM    'CclA'
#define TAG_ALPC_HANDLE  'HclA'
#define TAG_ALPC_MESSAGE 'MclA'

/* GLOBALS ******************************************************************/

extern POBJECT_TYPE AlpcPortObjectType;
extern PKEVENT AlpcpDummyEvent;
extern LIST_ENTRY AlpcpPortList;
extern KGUARDED_MUTEX AlpcpPortListLock;
extern KGUARDED_MUTEX AlpcpLock;
extern LONG AlpcpNextMessageId;

/* Sentinel stored in ETHREAD.AlpcMessage to wake a synchronous sender because
 * its pending request was cancelled (rather than replied to). */
#define ALPC_WAKE_CANCELED ((PVOID)(ULONG_PTR)2)

/* FUNCTIONS ****************************************************************/

CODE_SEG("INIT") NTSTATUS NTAPI AlpcpInitSystem(VOID);

/* Port initialization shared by connection and communication ports (alpcport.c). */
NTSTATUS NTAPI AlpcpInitializePort(_Inout_ PALPC_PORT Port, _In_ ULONG Type, _In_ BOOLEAN Waitable);

/* Message allocation (alpcmsg.c). The payload follows PortMessage. */
PKALPC_MESSAGE NTAPI AlpcpAllocateMessage(_In_ ULONG DataLength);
VOID NTAPI AlpcpFreeMessage(_In_ PKALPC_MESSAGE Message);
FORCEINLINE PVOID AlpcpGetMessageData(_In_ PKALPC_MESSAGE Message)
{
    return (PUCHAR)&Message->PortMessage + sizeof(PORT_MESSAGE);
}

NTSTATUS
NTAPI
AlpcpReceiveMessage(
    _In_ PALPC_PORT Port,
    _Out_ PPORT_MESSAGE ReceiveMessage,
    _Inout_ PSIZE_T BufferLength,
    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes,
    _Out_opt_ PVOID *OutServerContext,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_opt_ PLARGE_INTEGER Timeout);

/* Pop one waiting receiver thread off a port (call with AlpcpLock held). */
PETHREAD NTAPI AlpcpDequeueReceiver(_In_ PALPC_PORT Port);

/* ALPC send/reply cores, also used by the legacy LPC layer (alpcmsg.c). */
NTSTATUS
NTAPI
AlpcpSendRequest(
    _In_ PALPC_PORT SourcePort,
    _In_ PPORT_MESSAGE Header,
    _In_reads_bytes_opt_(DataLength) PVOID Data,
    _In_ ULONG DataLength,
    _In_ ULONG Flags,
    _In_opt_ PALPC_MESSAGE_ATTRIBUTES SendMessageAttributes,
    _Out_opt_ PPORT_MESSAGE ReceiveMessage,
    _Inout_opt_ PSIZE_T BufferLength,
    _In_ SIZE_T UserBufferLength,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_opt_ PLARGE_INTEGER Timeout);

NTSTATUS
NTAPI
AlpcpSendReply(
    _In_ PALPC_PORT ReplyPort,
    _In_ PPORT_MESSAGE Header,
    _In_reads_bytes_opt_(DataLength) PVOID Data,
    _In_ ULONG DataLength);

/* Create a communication (client/server) port bound to a connection port
 * (alpccon.c); used by both the ALPC and legacy LPC connect/accept paths. */
NTSTATUS
NTAPI
AlpcpCreateCommunicationPort(
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ PALPC_PORT ConnectionPort,
    _Outptr_ PALPC_PORT *OutPort);

/* Per-port handle table for reserves, sections and security contexts
 * (alpchandle.c). Handles are small opaque non-NULL values. */
NTSTATUS NTAPI AlpcpInitializeHandleTable(_Out_ PALPC_HANDLE_TABLE Table);
ALPC_HANDLE NTAPI AlpcpInsertHandle(_In_ PALPC_HANDLE_TABLE Table, _In_ PVOID Object);
PVOID NTAPI AlpcpReferenceHandle(_In_ PALPC_HANDLE_TABLE Table, _In_ ALPC_HANDLE Handle);
PVOID NTAPI AlpcpRemoveHandle(_In_ PALPC_HANDLE_TABLE Table, _In_ ALPC_HANDLE Handle);

/* Allocate a port's communication info (with handle table). For a connection
 * port pass ConnectionPort == Port; for a communication port pass the owning
 * connection port (a routing reference is taken on it). */
NTSTATUS NTAPI AlpcpAllocateCommunicationInfo(_Inout_ PALPC_PORT Port, _In_ PALPC_PORT ConnectionPort);

/* VIEW message-attribute transfer (alpcview.c). */
PVOID NTAPI AlpcpReferenceViewSection(_In_ PALPC_PORT Port, _In_ PVOID ViewBase, _Out_ PULONG OutSize);
NTSTATUS NTAPI AlpcpMapReceivedView(_In_ PALPC_PORT Port, _In_ PVOID SectionObject, _In_ ULONG Size, _Out_ PVOID *OutViewBase, _Out_ PSIZE_T OutViewSize);

/* Claim a reserve referenced by a tagged handle during send (alpcrsrc.c). */
NTSTATUS NTAPI AlpcpConsumeReserve(_In_ PALPC_PORT Port, _In_ ULONG TaggedHandle);

/* Completion-list registration and ring delivery (alpccl.c). */
NTSTATUS NTAPI AlpcpRegisterCompletionList(_Inout_ PALPC_PORT Port, _In_ PVOID Buffer, _In_ ULONG Size, _In_ KPROCESSOR_MODE PreviousMode);
NTSTATUS NTAPI AlpcpUnregisterCompletionList(_Inout_ PALPC_PORT Port);
BOOLEAN NTAPI AlpcpInsertCompletionList(_In_ PALPC_PORT Port, _In_ CLIENT_ID ClientId, _In_reads_bytes_opt_(DataLength) PVOID Data, _In_ ULONG DataLength);

/* Impersonate the sender of a pending message by id (alpcsec.c). */
NTSTATUS NTAPI AlpcpImpersonateMessage(_In_ PALPC_PORT Port, _In_ ULONG MessageId);

VOID NTAPI AlpcpDeletePort(_In_ PVOID Object);

VOID
NTAPI
AlpcpClosePort(
    _In_opt_ PEPROCESS Process,
    _In_ PVOID Object,
    _In_ ACCESS_MASK GrantedAccess,
    _In_ ULONG ProcessHandleCount,
    _In_ ULONG SystemHandleCount);

NTSTATUS
NTAPI
AlpcpCreateConnectionPort(
    _Out_ PHANDLE PortHandle,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes,
    _In_ ULONG MaxMessageLength,
    _In_ BOOLEAN Waitable,
    _In_ BOOLEAN LegacyPort);

#endif /* _NTOSKRNL_ALPC_H_ */
