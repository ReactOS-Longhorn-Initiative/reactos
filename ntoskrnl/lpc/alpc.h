
#ifndef _NTOSKRNL_ALPC_H_
#define _NTOSKRNL_ALPC_H_

#include <ntoskrnl.h>
#include <ndk/rtltypes.h>
#include <ndk/rtlfuncs.h>
#include <ntdef.h>
#include <ntifs.h>

typedef struct _RTL_SRWLOCK {
  PVOID Ptr;
} RTL_SRWLOCK, *PRTL_SRWLOCK;

typedef HANDLE ALPC_HANDLE, *PALPC_HANDLE;

// Forward declarations
struct _ALPC_PORT;
struct _KALPC_MESSAGE;
struct _ALPC_MESSAGE_ATTRIBUTES;
struct _ALPC_HANDLE_TABLE;
struct _ALPC_COMMUNICATION_INFO;
struct _ALPC_COMPLETION_PACKET_LOOKASIDE;

/* Ob Objects? */
/* IoSetIoCompletionEx2 - win8 */
typedef struct _IO_MINI_COMPLETION_PACKET_USER
{
    struct _LIST_ENTRY ListEntry;                                           //0x0
    ULONG PacketType;                                                       //0x8
    VOID* KeyContext;                                                       //0xc
    VOID* ApcContext;                                                       //0x10
    LONG IoStatus;                                                          //0x14
    ULONG IoStatusInformation;                                              //0x18
    VOID (*MiniPacketCallback)(struct _IO_MINI_COMPLETION_PACKET_USER* arg1, VOID* arg2); //0x1c
    VOID* Context;                                                          //0x20
    UCHAR Allocated;                                                        //0x24
} IO_MINI_COMPLETION_PACKET_USER, *PIO_MINI_COMPLETION_PACKET_USER;

/* Win10? OB_DUPLICATE_OBJECT_STATE */
typedef struct _OB_DUPLICATE_OBJECT_STATE
{
  EPROCESS *SourceProcess;
  void *SourceHandle;
  void *Object;
  unsigned int TargetAccess;
  HANDLE_TABLE_ENTRY_INFO ObjectInfo;
  unsigned int HandleAttributes;
} OB_DUPLICATE_OBJECT_STATE, *POB_DUPLICATE_OBJECT_STATE;


#define ALPC_MSGFLG_REPLY_MESSAGE 0x1
#define ALPC_MSGFLG_LPC_MODE 0x2 // ?
#define ALPC_MSGFLG_RELEASE_MESSAGE 0x10000 // dbg
#define ALPC_MSGFLG_SYNC_REQUEST 0x20000 // dbg
#define ALPC_MSGFLG_WAIT_USER_MODE 0x100000
#define ALPC_MSGFLG_WAIT_ALERTABLE 0x200000
#define ALPC_MSGFLG_WOW64_CALL 0x80000000 // dbg
#define ALPC_MESSAGE_SECURITY_ATTRIBUTE 0x80000000
#define ALPC_MESSAGE_VIEW_ATTRIBUTE 0x40000000
#define ALPC_MESSAGE_CONTEXT_ATTRIBUTE 0x20000000
#define ALPC_MESSAGE_HANDLE_ATTRIBUTE 0x10000000
#define ALPC_COMPLETION_LIST_BUFFER_GRANULARITY_MASK 0x3f // dbg
#define ALPC_PORFLG_ALLOW_LPC_REQUESTS 0x20000 // rev
#define ALPC_PORFLG_WAITABLE_PORT 0x40000 // dbg
#define ALPC_PORFLG_SYSTEM_PROCESS 0x100000 // dbg
#define ALPC_HANDLEFLG_DUPLICATE_SAME_ACCESS 0x10000
#define ALPC_HANDLEFLG_DUPLICATE_SAME_ATTRIBUTES 0x20000
#define ALPC_HANDLEFLG_DUPLICATE_INHERIT 0x80000
#define ALPC_SECFLG_CREATE_HANDLE 0x20000 // dbg
#define ALPC_SECFLG_NOSECTIONHANDLE 0x40000
#define ALPC_VIEWFLG_NOT_SECURE 0x40000

typedef enum _ALPC_PORT_INFORMATION_CLASS
{
	AlpcBasicInformation, // q: out ALPC_BASIC_INFORMATION
	AlpcPortInformation, // s: in ALPC_PORT_ATTRIBUTES
	AlpcAssociateCompletionPortInformation, // s: in ALPC_PORT_ASSOCIATE_COMPLETION_PORT
	AlpcConnectedSIDInformation, // q: in SID
	AlpcServerInformation, // q: inout ALPC_SERVER_INFORMATION
	AlpcMessageZoneInformation, // s: in ALPC_PORT_MESSAGE_ZONE_INFORMATION
	AlpcRegisterCompletionListInformation, // s: in ALPC_PORT_COMPLETION_LIST_INFORMATION
	AlpcUnregisterCompletionListInformation, // s: VOID
	AlpcAdjustCompletionListConcurrencyCountInformation, // s: in ULONG
	AlpcRegisterCallbackInformation, // kernel-mode only
	AlpcCompletionListRundownInformation, // s: VOID
	AlpcWaitForPortReferences
} ALPC_PORT_INFORMATION_CLASS;

// private
typedef enum _ALPC_MESSAGE_INFORMATION_CLASS
{
	AlpcMessageSidInformation, // q: out SID
	AlpcMessageTokenModifiedIdInformation,  // q: out LUID
	AlpcMessageDirectStatusInformation,
	AlpcMessageHandleInformation, // ALPC_MESSAGE_HANDLE_INFORMATION
	MaxAlpcMessageInfoClass
} ALPC_MESSAGE_INFORMATION_CLASS, *PALPC_MESSAGE_INFORMATION_CLASS;

typedef union _KALPC_DIRECT_EVENT                                               // 4 / 4 elements; 0x0004 / 0x0008 Bytes
{
    UINT_PTR                    Value;                                          // 0x0000 / 0x0000; 0x0004 / 0x0008 Bytes
    UINT_PTR                    DirectType                                 : 1; // 0x0000 / 0x0000; Bit:   0
    UINT_PTR                    EventReferenced                            : 1; // 0x0000 / 0x0000; Bit:   1
#if defined(_M_AMD64)
    UINT64                      EventObjectBits                            :62; // ------ / 0x0000; Bits:  2 - 63
#else                                                                           // #if defined(_M_X64)
    ULONG32                     EventObjectBits                            :30; // 0x0000 / ------; Bits:  2 - 31
#endif                                                                          // #if defined(_M_X64)
} KALPC_DIRECT_EVENT, * PKALPC_DIRECT_EVENT;


typedef struct _PALPC_PORT_REFERENCE_WAIT_BLOCK
{
    KEVENT DesiredReferenceNoEvent;
    LONG DesiredReferenceNo;
} ALPC_PORT_REFERENCE_WAIT_BLOCK, *PALPC_PORT_REFERENCE_WAIT_BLOCK; 

/* Win10 1607 */
typedef struct _ALPC_WORK_ON_BEHALF_TICKET
{
    ULONG ThreadId;                                                         //0x0
    ULONG ThreadCreationTimeLow;                                            //0x4
} ALPC_WORK_ON_BEHALF_TICKET, *PALPC_WORK_ON_BEHALF_TICKET; 

typedef struct _KALPC_WORK_ON_BEHALF_DATA
{
    struct _ALPC_WORK_ON_BEHALF_TICKET Ticket;                       
} KALPC_WORK_ON_BEHALF_DATA, *PKALPC_WORK_ON_BEHALF_DATA; 
/* Win10 1607 */


typedef struct _KALPC_VIEW
{
    struct _LIST_ENTRY ViewListEntry;                                       //0x0
    struct _KALPC_REGION* Region;                                           //0x8
    struct _ALPC_PORT* OwnerPort;                                           //0xc
    struct _EPROCESS* OwnerProcess;                                         //0x10
    VOID* Address;                                                          //0x14
    ULONG Size;                                                             //0x18
    VOID* SecureViewHandle;                                                 //0x1c
    VOID* WriteAccessHandle;                                                //0x20
    union
    {
        struct
        {
            ULONG WriteAccess:1;                                            //0x24
            ULONG AutoRelease:1;                                            //0x24
            ULONG ForceUnlink:1;                                            //0x24
        } s1;                                                               //0x24
    } u1;                                                                   //0x24
    ULONG NumberOfOwnerMessages;                                            //0x28
    struct _LIST_ENTRY ProcessViewListEntry;                                //0x2c
} KALPC_VIEW, *PKALPC_VIEW, ALPC_VIEW, *PALPC_VIEW; 

typedef struct _KALPC_SECURITY_DATA
{
    struct _ALPC_HANDLE_TABLE* HandleTable;                                 //0x0
    VOID* ContextHandle;                                                    //0x4
    struct _EPROCESS* OwningProcess;                                        //0x8
    struct _ALPC_PORT* OwnerPort;                                           //0xc
    struct _SECURITY_CLIENT_CONTEXT DynamicSecurity;                        //0x10
    union
    {
        struct
        {
            ULONG Revoked:1;                                                //0x4c
            ULONG Impersonated:1;                                           //0x4c
        } s1;                                                               //0x4c
    } u1;                                                                   //0x4c
} KALPC_SECURITY_DATA, *PKALPC_SECURITY_DATA, ALPC_SECURITY_DATA, *PALPC_SECURITY_DATA; 

typedef struct _KALPC_SECTION
{
    VOID* SectionObject;                                                    //0x0
    ULONG Size;                                                             //0x4
    struct _ALPC_HANDLE_TABLE* HandleTable;                                 //0x8
    VOID* SectionHandle;                                                    //0xc
    struct _EPROCESS* OwnerProcess;                                         //0x10
    struct _ALPC_PORT* OwnerPort;                                           //0x14
    union
    {
        struct
        {
            ULONG Internal:1;                                               //0x18
            ULONG Secure:1;                                                 //0x18
        } s1;                                                               //0x18
    } u1;                                                                   //0x18
    ULONG NumberOfRegions;                                                  //0x1c
    struct _LIST_ENTRY RegionListHead;                                      //0x20
} KALPC_SECTION, *PKALPC_SECTION, ALPC_SECTION, *PALPC_SECTION; 

typedef struct _KALPC_RESERVE
{
    struct _ALPC_PORT* OwnerPort;                                           //0x0
    struct _ALPC_HANDLE_TABLE* HandleTable;                                 //0x4
    VOID* Handle;                                                           //0x8
    struct _KALPC_MESSAGE* Message;                                         //0xc
    LONG Active;                                                            //0x10
} KALPC_RESERVE, *PKALPC_RESERVE, ALPC_RESERVE, *PALPC_RESERVE;

typedef struct _KALPC_MESSAGE_ATTRIBUTES
{
    VOID* ClientContext;                                                    //0x0
    VOID* ServerContext;                                                    //0x4
    VOID* PortContext;                                                      //0x8
    VOID* CancelPortContext;                                               //0xc
    struct _KALPC_SECURITY_DATA* SecurityData;                              //0x10
    struct _KALPC_VIEW* View;                                               //0x14
    struct _KALPC_HANDLE_DATA* HandleData;                                  //0x18
    KALPC_DIRECT_EVENT DirectEvent;                                         //0x1c
    struct _KALPC_WORK_ON_BEHALF_DATA WorkOnBehalfData;                     //0x20
} KALPC_MESSAGE_ATTRIBUTES, *PKALPC_MESSAGE_ATTRIBUTES;

typedef struct _KALPC_MESSAGE
{
    struct _LIST_ENTRY Entry;                                               //0x0
    struct _ALPC_PORT* PortQueue;                                           //0x8
    struct _ALPC_PORT* OwnerPort;                                           //0xc
    struct _ETHREAD* WaitingThread;                                         //0x10
    union
    {
        struct
        {
            ULONG QueueType:3;                                              //0x14
            ULONG QueuePortType:4;                                          //0x14
            ULONG Canceled:1;                                               //0x14
            ULONG Ready:1;                                                  //0x14
            ULONG ReleaseMessage:1;                                         //0x14
            ULONG SharedQuota:1;                                            //0x14
            ULONG ReplyWaitReply:1;                                         //0x14
            ULONG OwnerPortReference:1;                                     //0x14
            ULONG ReserveReference:1;                                       //0x14
            ULONG ReceiverReference:1;                                      //0x14
            ULONG ViewAttributeRetrieved:1;                                 //0x14
            ULONG InDispatch:1;                                             //0x14
        } s1;                                                               //0x14
        ULONG State;                                                        //0x14
    } u1;                                                                   //0x14
    LONG SequenceNo;                                                        //0x18
    union
    {
        struct _EPROCESS* QuotaProcess;                                     //0x1c
        VOID* QuotaBlock;                                                   //0x1c
    };
    struct _ALPC_PORT* CancelSequencePort;                                  //0x20
    struct _ALPC_PORT* CancelQueuePort;                                     //0x24
    LONG CancelSequenceNo;                                                  //0x28
    struct _LIST_ENTRY CancelListEntry;                                     //0x2c
    struct _KALPC_RESERVE* Reserve;                                         //0x34
    struct _KALPC_MESSAGE_ATTRIBUTES MessageAttributes;                     //0x38
    VOID* DataUserVa;                                                       //0x60
    struct _ALPC_COMMUNICATION_INFO* CommunicationInfo;                     //0x64
    struct _ALPC_PORT* ConnectionPort;                                      //0x68
    struct _ETHREAD* ServerThread;                                          //0x6c
    VOID* WakeReference;                                                    //0x70
    VOID* ExtensionBuffer;                                                  //0x74
    ULONG ExtensionBufferSize;                                              //0x78
    struct _PORT_MESSAGE PortMessage;                                       //0x80
} KALPC_MESSAGE, *PKALPC_MESSAGE, ALPC_MESSAGE, *PALPC_MESSAGE;

typedef struct _KALPC_HANDLE_DATA
{
    ULONG ObjectType;                                                       //0x0
    ULONG Count;                                                            //0x4
    OB_DUPLICATE_OBJECT_STATE DuplicateContext;                             //0x8
} KALPC_HANDLE_DATA, *PKALPC_HANDLE_DATA;

typedef struct _ALPC_PROCESS_CONTEXT
{
    EX_PUSH_LOCK Lock;                                              //0x0
    LIST_ENTRY ViewListHead;                                        //0x4
    volatile ULONG PagedPoolQuotaCache;                             //0xc
} ALPC_PROCESS_CONTEXT, *PALPC_PROCESS_CONTEXT;

typedef struct _ALPC_PORT_ATTRIBUTES
{
    ULONG Flags;                                                            //0x0
    SECURITY_QUALITY_OF_SERVICE SecurityQos;                                //0x4
    ULONG MaxMessageLength;                                                 //0x10
    ULONG MemoryBandwidth;                                                  //0x14
    ULONG MaxPoolUsage;                                                     //0x18
    ULONG MaxSectionSize;                                                   //0x1c
    ULONG MaxViewSize;                                                      //0x20
    ULONG MaxTotalSectionSize;                                              //0x24
    ULONG DupObjectTypes;                                                   //0x28
} ALPC_PORT_ATTRIBUTES, *PALPC_PORT_ATTRIBUTES;

typedef struct _ALPC_PORT
{
    LIST_ENTRY PortListEntry;                                       //0x0
    struct _ALPC_COMMUNICATION_INFO* CommunicationInfo;                     //0x8
    EPROCESS* OwnerProcess;                                         //0xc
    VOID* CompletionPort;                                           //0x10
    VOID* CompletionKey;                                            //0x14
    struct _ALPC_COMPLETION_PACKET_LOOKASIDE* CompletionPacketLookaside;    //0x18
    VOID* PortContext;                                                      //0x1c
    SECURITY_CLIENT_CONTEXT StaticSecurity;                                 //0x20
    EX_PUSH_LOCK IncomingQueueLock;                                         //0x5c
    LIST_ENTRY MainQueue;                                                   //0x60
    LIST_ENTRY LargeMessageQueue;                                           //0x68
    EX_PUSH_LOCK PendingQueueLock;                                         //0x70
    LIST_ENTRY PendingQueue;                                               //0x74
    EX_PUSH_LOCK DirectQueueLock;                                          //0x7c
    LIST_ENTRY DirectQueue;                                                //0x80
    EX_PUSH_LOCK WaitQueueLock;                                           //0x88
    LIST_ENTRY WaitQueue;                                                 //0x8c
    union
    {
        KSEMAPHORE* Semaphore;                                            //0x94
        KEVENT* DummyEvent;                                               //0x94
    };
    ALPC_PORT_ATTRIBUTES PortAttributes;                                  //0x98
    EX_PUSH_LOCK ResourceListLock;                                        //0xc4
    LIST_ENTRY ResourceListHead;                                          //0xc8
    EX_PUSH_LOCK PortObjectLock;                                          //0xd0
    struct _ALPC_COMPLETION_LIST* CompletionList;                                 //0xd4
    CALLBACK_OBJECT* CallbackObject;                                      //0xd8
    VOID* CallbackContext;                                                //0xdc
    LIST_ENTRY CanceledQueue;                                             //0xe0
    LONG SequenceNo;                                                      //0xe8
    LONG ReferenceNo;                                                     //0xec
    PALPC_PORT_REFERENCE_WAIT_BLOCK* ReferenceNoWait;                   //0xf0
    union
    {
        struct
        {
            ULONG Initialized:1;                                            //0xf4
            ULONG Type:2;                                                   //0xf4
            ULONG ConnectionPending:1;                                      //0xf4
            ULONG ConnectionRefused:1;                                      //0xf4
            ULONG Disconnected:1;                                           //0xf4
            ULONG Closed:1;                                                 //0xf4
            ULONG NoFlushOnClose:1;                                         //0xf4
            ULONG ReturnExtendedInfo:1;                                     //0xf4
            ULONG Waitable:1;                                               //0xf4
            ULONG DynamicSecurity:1;                                        //0xf4
            ULONG Wow64CompletionList:1;                                    //0xf4
            ULONG Lpc:1;                                                    //0xf4
            ULONG LpcToLpc:1;                                               //0xf4
            ULONG HasCompletionList:1;                                      //0xf4
            ULONG HadCompletionList:1;                                      //0xf4
            ULONG EnableCompletionList:1;                                   //0xf4
        } s1;                                                               //0xf4
        ULONG State;                                                        //0xf4
    } u1;                                                                   //0xf4
    struct _ALPC_PORT* TargetQueuePort;                                     //0xf8
    struct _ALPC_PORT* TargetSequencePort;                                  //0xfc
    struct _KALPC_MESSAGE* CachedMessage;                                   //0x100
    ULONG CachedConnectionMessageId;                                        // cached MessageId of last connection request
    struct _ALPC_PORT* PendingClientPort;                                   // client port from last connection request
    ULONG MainQueueLength;                                                  //0x104
    ULONG LargeMessageQueueLength;                                          //0x108
    ULONG PendingQueueLength;                                               //0x10c
    ULONG DirectQueueLength;                                                //0x110
    ULONG CanceledQueueLength;                                              //0x114
    ULONG WaitQueueLength;                                                  //0x118
} ALPC_PORT, *PALPC_PORT;

typedef struct _ALPC_MESSAGE_ATTRIBUTES
{
    ULONG AllocatedAttributes;                                              //0x0
    ULONG ValidAttributes;                                                  //0x4
} ALPC_MESSAGE_ATTRIBUTES, *PALPC_MESSAGE_ATTRIBUTES;

/* Global message table is an executive HANDLE_TABLE in NT6+ */
extern PHANDLE_TABLE AlpcMessageTable;

/* Thread wake sentinels to communicate teardown/cancel events via ETHREAD->AlpcMessage */
#define ALPC_THREAD_WAKE_PORT_CLOSED     ((PVOID)(ULONG_PTR)0x1)
#define ALPC_THREAD_WAKE_CANCELED        ((PVOID)(ULONG_PTR)0x2)
/* Tags */
#define TAG_ALPC_COMM 'ALPC'
#define TAG_ALPC_SYNC 'SplA'
/* Synchronous reply receive helper (aligns with Win8 flow) */
NTSTATUS
NTAPI
AlpcpReceiveSynchronousReply(
    _In_ HANDLE PortHandle,
    _In_ ULONG ReplyToMessageId,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Out_ PPORT_MESSAGE ReceiveMessage
    );

/* Global initialization (Win8-style init entrypoints) */
NTSTATUS NTAPI AlpcpInitSystem(VOID);
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS NTAPI AlpcpInitializeMessageLog(_In_ ULONGLONG MessageCount, _In_ ULONGLONG SnapshotCount);

/* Per-thread accept context (internal) */
typedef struct _ALPC_ACCEPT_MAP_ENTRY
{
    LIST_ENTRY Link;
    PETHREAD Thread;
    struct _ALPC_PORT* Port;
} ALPC_ACCEPT_MAP_ENTRY, *PALPC_ACCEPT_MAP_ENTRY;

/* Internal helpers */
VOID NTAPI AlpcpSetThreadAcceptPort(_In_ PETHREAD Thread, _In_ struct _ALPC_PORT* Port);
struct _ALPC_PORT* NTAPI AlpcpGetAndClearThreadAcceptPort(_In_ PETHREAD Thread);


typedef struct _ALPC_HANDLE_ENTRY
{
    VOID* Object;                                                           //0x0
} ALPC_HANDLE_ENTRY, *PALPC_HANDLE_ENTRY;

typedef struct ALPC_HANDLE_TABLE
{
    PALPC_HANDLE_ENTRY Handles;                                             //0x0
    ULONG TotalHandles;                                                     //0x4
    ULONG Flags;                                                            //0x8
    EX_PUSH_LOCK Lock;                                                      //0xc
} ALPC_HANDLE_TABLE, *PALPC_HANDLE_TABLE;

typedef struct _ALPC_DISPATCH_CONTEXT
{
    ALPC_PORT* PortObject;                                          //0x0
    KALPC_MESSAGE* Message;                                         //0x4
    struct _ALPC_COMMUNICATION_INFO* CommunicationInfo;                     //0x8
    PETHREAD TargetThread;                                          //0xc
    ALPC_PORT* TargetPort;                                          //0x10
    KALPC_DIRECT_EVENT DirectEvent;                                  //0x14
    ULONG Flags;                                                            //0x18
    USHORT TotalLength;                                                     //0x1c
    USHORT Type;                                                            //0x1e
    USHORT DataInfoOffset;                                                  //0x20
/* win7+ */
    UCHAR SignalCompletion;                                                 //0x22
    UCHAR PostedToCompletionList;                                           //0x23
} ALPC_DISPATCH_CONTEXT, *PALPC_DISPATCH_CONTEXT;

typedef struct _ALPC_COMPLETION_PACKET_LOOKASIDE_ENTRY
{
    SINGLE_LIST_ENTRY ListEntry;                                    //0x0
    IO_MINI_COMPLETION_PACKET_USER* Packet;                         //0x4
    struct _ALPC_COMPLETION_PACKET_LOOKASIDE* Lookaside;                    //0x8
} ALPC_COMPLETION_PACKET_LOOKASIDE_ENTRY, *PALPC_COMPLETION_PACKET_LOOKASIDE_ENTRY;

/* Internal helpers/APIs used across compilation units */

/* Thread-centric ALPC wait helper (Windows-style: signal-and-wait with WrLpcReply) */
NTSTATUS
NTAPI
AlpcpSignalAndWait(
    _Inout_ PALPC_DISPATCH_CONTEXT DispatchContext,
    _Inout_ PKSEMAPHORE WaitSemaphore,
    _In_ KWAIT_REASON WaitReason,
    _In_ KPROCESSOR_MODE WaitMode,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ ULONG ArmWait
    );


NTSTATUS NTAPI AlpcpReceiveMessagePort(
    _Inout_ PALPC_DISPATCH_CONTEXT DispatchContext,
    _In_ KPROCESSOR_MODE WaitMode,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Outptr_result_maybenull_ PKALPC_MESSAGE *ReceiveMessagePointer,
    _In_ ULONG ReceiveAttributes
    );

NTSTATUS NTAPI AlpcpSendMessage(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE MessageHeader,
    _In_ BOOLEAN ReplyWaitReply
    );

NTSTATUS NTAPI AlpcpReplyLegacySynchronousRequest(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE RequestMessage
    );

VOID NTAPI AlpcpFreeMessage(_In_ PKALPC_MESSAGE Message);

NTSTATUS NTAPI AlpcpReferencePortByName(
    _In_ PUNICODE_STRING ObjectName,
    _Outptr_ PALPC_PORT *PortObject
    );

NTSTATUS NTAPI AlpcpCreatePortInternal(
    _In_ BOOLEAN ConnectionPort,
    _In_ BOOLEAN Waitable,
    _Outptr_ PALPC_PORT* PortObject
    );

NTSTATUS NTAPI AlpcpSendMessagePortObjects(
    _In_ PALPC_PORT SourcePort,
    _In_ PALPC_PORT TargetPort,
    _In_ PPORT_MESSAGE MessageHeader,
    _In_ BOOLEAN ReplyWaitReply,
    _In_opt_ PVOID ExtensionBuffer,
    _In_ ULONG ExtensionBufferSize);

/* Kernel-only: wait for a specific reply on a given port object */
NTSTATUS NTAPI AlpcpReceiveSynchronousReplyPortObject(
    _In_ PALPC_PORT Port,
    _In_ ULONG ReplyToMessageId,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Out_ PPORT_MESSAGE ReceiveMessage
    );

/* Kernel-only: minimal extension used to ferry LPC PORT_VIEW info during connect */
typedef struct _ALPC_LEGACY_CONNECT_VIEW_EXT
{
    PEPROCESS ClientProcess;     /* client process to write back ViewRemoteBase */
    PVOID ClientViewUser;        /* user pointer to client's PORT_VIEW struct */
    PVOID SectionObject;         /* referenced section object */
    SIZE_T ViewSize;             /* mapped size in client */
    LARGE_INTEGER SectionOffset; /* offset used */
    PVOID ClientViewBase;        /* mapped base in client */
} ALPC_LEGACY_CONNECT_VIEW_EXT, *PALPC_LEGACY_CONNECT_VIEW_EXT;

typedef struct _ALPC_COMPLETION_PACKET_LOOKASIDE
{
    ULONG Lock;                                                             //0x0
    ULONG Size;                                                             //0x4
    ULONG ActiveCount;                                                      //0x8
    ULONG PendingNullCount;                                                 //0xc
    ULONG PendingCheckCompletionListCount;                                  //0x10
    ULONG PendingDelete;                                                    //0x14
    SINGLE_LIST_ENTRY FreeListHead;                                         //0x18
    VOID* CompletionPort;                                                   //0x1c
    VOID* CompletionKey;                                                    //0x20
    ALPC_COMPLETION_PACKET_LOOKASIDE_ENTRY Entry[1];                //0x24
} ALPC_COMPLETION_PACKET_LOOKASIDE, *PALPC_COMPLETION_PACKET_LOOKASIDE; 

typedef struct _ALPC_COMPLETION_LIST_STATE
{
    union
    {
        struct
        {
            ULONGLONG Head:24;                                              //0x0
            ULONGLONG Tail:24;                                              //0x0
            ULONGLONG ActiveThreadCount:16;                                 //0x0
        } s1;                                                               //0x0
        LONGLONG Value;                                                     //0x0
    } u1;                                                                   //0x0
} ALPC_COMPLETION_LIST_STATE, *PALPC_COMPLETION_LIST_STATE;

typedef struct _ALPC_COMPLETION_LIST_HEADER
{
    ULONGLONG StartMagic;                                                   //0x0
    ULONG TotalSize;                                                        //0x8
    ULONG ListOffset;                                                       //0xc
    ULONG ListSize;                                                         //0x10
    ULONG BitmapOffset;                                                     //0x14
    ULONG BitmapSize;                                                       //0x18
    ULONG DataOffset;                                                       //0x1c
    ULONG DataSize;                                                         //0x20
    ULONG AttributeFlags;                                                   //0x24
    ULONG AttributeSize;                                                    //0x28
    volatile ALPC_COMPLETION_LIST_STATE State;                              //0x40
    volatile ULONG LastMessageId;                                           //0x48
    volatile ULONG LastCallbackId;                                          //0x4c
    volatile ULONG PostCount;                                               //0x80
    volatile ULONG ReturnCount;                                             //0xc0
    volatile ULONG LogSequenceNumber;                                       //0x100
    RTL_SRWLOCK UserLock;                                                   //0x140
    ULONGLONG EndMagic;                                                     //0x148
} ALPC_COMPLETION_LIST_HEADER, *PALPC_COMPLETION_LIST_HEADER;

typedef struct _ALPC_COMPLETION_LIST
{
    LIST_ENTRY Entry;                                               //0x0
    PEPROCESS OwnerProcess;                                         //0x8
    EX_PUSH_LOCK CompletionListLock;                                //0xc
    PMDL Mdl;                                                       //0x10
    VOID* UserVa;                                                           //0x14
    VOID* UserLimit;                                                        //0x18
    VOID* DataUserVa;                                                       //0x1c
    VOID* SystemVa;                                                         //0x20
    ULONG TotalSize;                                                        //0x24
    ALPC_COMPLETION_LIST_HEADER* Header;                            //0x28
    VOID* List;                                                             //0x2c
    ULONG ListSize;                                                         //0x30
    VOID* Bitmap;                                                           //0x34
    ULONG BitmapSize;                                                       //0x38
    VOID* Data;                                                             //0x3c
    ULONG DataSize;                                                         //0x40
    ULONG BitmapLimit;                                                      //0x44
    ULONG BitmapNextHint;                                                   //0x48
    ULONG ConcurrencyCount;                                                 //0x4c
    ULONG AttributeFlags;                                                   //0x50
    ULONG AttributeSize;                                                    //0x54
} ALPC_COMPLETION_LIST, *PALPC_COMPLETION_LIST;

typedef struct _ALPC_COMMUNICATION_INFO
{
    PALPC_PORT ConnectionPort;                                      //0x0
    PALPC_PORT ServerCommunicationPort;                             //0x4
    PALPC_PORT ClientCommunicationPort;                             //0x8
    LIST_ENTRY CommunicationList;                                   //0xc
    ALPC_HANDLE_TABLE HandleTable;                                  //0x14
// win8+?
    KALPC_MESSAGE* CloseMessage;                                    //0x24
} ALPC_COMMUNICATION_INFO, *PALPC_COMMUNICATION_INFO;

#endif // _NTOSKRNL_ALPC_H_
