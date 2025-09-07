/*
 * ReactOS ALPC Kernel Structures and Prototypes (initial draft)
 * Based on Windows 10 decompilation and public sources
 */

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
// Forward declarations
struct _ALPC_PORT;
struct _KALPC_MESSAGE;
struct _ALPC_MESSAGE_ATTRIBUTES;

#define ALPC_MSGFLG_REPLY_MESSAGE 0x1
#define ALPC_MSGFLG_LPC_MODE 0x2 // ?
#define ALPC_MSGFLG_RELEASE_MESSAGE 0x10000 // dbg
#define ALPC_MSGFLG_SYNC_REQUEST 0x20000 // dbg
#define ALPC_MSGFLG_WAIT_USER_MODE 0x100000
#define ALPC_MSGFLG_WAIT_ALERTABLE 0x200000
#define ALPC_MSGFLG_WOW64_CALL 0x80000000 // dbg


// Asynchronous Local Inter-process Communication

// rev
typedef HANDLE ALPC_HANDLE, *PALPC_HANDLE;

#define ALPC_PORFLG_ALLOW_LPC_REQUESTS 0x20000 // rev
#define ALPC_PORFLG_WAITABLE_PORT 0x40000 // dbg
#define ALPC_PORFLG_SYSTEM_PROCESS 0x100000 // dbg

// symbols
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
#ifdef _WIN64
	ULONG Reserved;
#endif
} ALPC_PORT_ATTRIBUTES, *PALPC_PORT_ATTRIBUTES;

// begin_rev
#define ALPC_MESSAGE_SECURITY_ATTRIBUTE 0x80000000
#define ALPC_MESSAGE_VIEW_ATTRIBUTE 0x40000000
#define ALPC_MESSAGE_CONTEXT_ATTRIBUTE 0x20000000
#define ALPC_MESSAGE_HANDLE_ATTRIBUTE 0x10000000
// end_rev


// symbols
typedef struct _ALPC_COMPLETION_LIST_STATE
{
	union
	{
		struct
		{
			ULONG64 Head : 24;
			ULONG64 Tail : 24;
			ULONG64 ActiveThreadCount : 16;
		} s1;
		ULONG64 Value;
	} u1;
} ALPC_COMPLETION_LIST_STATE, *PALPC_COMPLETION_LIST_STATE;

#define ALPC_COMPLETION_LIST_BUFFER_GRANULARITY_MASK 0x3f // dbg

// symbols
typedef struct DECLSPEC_ALIGN(128) _ALPC_COMPLETION_LIST_HEADER
{
	ULONG64 StartMagic;

	ULONG TotalSize;
	ULONG ListOffset;
	ULONG ListSize;
	ULONG BitmapOffset;
	ULONG BitmapSize;
	ULONG DataOffset;
	ULONG DataSize;
	ULONG AttributeFlags;
	ULONG AttributeSize;

	DECLSPEC_ALIGN(128) ALPC_COMPLETION_LIST_STATE State;
	ULONG LastMessageId;
	ULONG LastCallbackId;
	DECLSPEC_ALIGN(128) ULONG PostCount;
	DECLSPEC_ALIGN(128) ULONG ReturnCount;
	DECLSPEC_ALIGN(128) ULONG LogSequenceNumber;
	DECLSPEC_ALIGN(128) RTL_SRWLOCK UserLock;

	ULONG64 EndMagic;
} ALPC_COMPLETION_LIST_HEADER, *PALPC_COMPLETION_LIST_HEADER;

// private
typedef struct _ALPC_CONTEXT_ATTR
{
	PVOID PortContext;
	PVOID MessageContext;
	ULONG Sequence;
	ULONG MessageId;
	ULONG CallbackId;
} ALPC_CONTEXT_ATTR, *PALPC_CONTEXT_ATTR;

// begin_rev
#define ALPC_HANDLEFLG_DUPLICATE_SAME_ACCESS 0x10000
#define ALPC_HANDLEFLG_DUPLICATE_SAME_ATTRIBUTES 0x20000
#define ALPC_HANDLEFLG_DUPLICATE_INHERIT 0x80000
// end_rev

// private
typedef struct _ALPC_HANDLE_ATTR32
{
	ULONG Flags;
	ULONG Reserved0;
	ULONG SameAccess;
	ULONG SameAttributes;
	ULONG Indirect;
	ULONG Inherit;
	ULONG Reserved1;
	ULONG Handle;
	ULONG ObjectType; // ObjectTypeCode, not ObjectTypeIndex
	ULONG DesiredAccess;
	ULONG GrantedAccess;
} ALPC_HANDLE_ATTR32, *PALPC_HANDLE_ATTR32;

// private
typedef struct _ALPC_HANDLE_ATTR
{
	ULONG Flags;
	ULONG Reserved0;
	ULONG SameAccess;
	ULONG SameAttributes;
	ULONG Indirect;
	ULONG Inherit;
	ULONG Reserved1;
	HANDLE Handle;
	PALPC_HANDLE_ATTR32 HandleAttrArray;
	ULONG ObjectType; // ObjectTypeCode, not ObjectTypeIndex
	ULONG HandleCount;
	ACCESS_MASK DesiredAccess;
	ACCESS_MASK GrantedAccess;
} ALPC_HANDLE_ATTR, *PALPC_HANDLE_ATTR;

#define ALPC_SECFLG_CREATE_HANDLE 0x20000 // dbg
#define ALPC_SECFLG_NOSECTIONHANDLE 0x40000
// private
typedef struct _ALPC_SECURITY_ATTR
{
	ULONG Flags;
	PSECURITY_QUALITY_OF_SERVICE QoS;
	ALPC_HANDLE ContextHandle; // dbg
} ALPC_SECURITY_ATTR, *PALPC_SECURITY_ATTR;

// begin_rev
#define ALPC_VIEWFLG_NOT_SECURE 0x40000
// end_rev

// private
typedef struct _ALPC_DATA_VIEW_ATTR
{
	ULONG Flags;
	ALPC_HANDLE SectionHandle;
	PVOID ViewBase; // must be zero on input
	SIZE_T ViewSize;
} ALPC_DATA_VIEW_ATTR, *PALPC_DATA_VIEW_ATTR;

// private
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
typedef struct _ALPC_BASIC_INFORMATION
{
	ULONG Flags;
	ULONG SequenceNo;
	PVOID PortContext;
} ALPC_BASIC_INFORMATION, *PALPC_BASIC_INFORMATION;

// private
typedef struct _ALPC_PORT_ASSOCIATE_COMPLETION_PORT
{
	PVOID CompletionKey;
	HANDLE CompletionPort;
} ALPC_PORT_ASSOCIATE_COMPLETION_PORT, *PALPC_PORT_ASSOCIATE_COMPLETION_PORT;

// private
typedef struct _ALPC_SERVER_INFORMATION
{
	union
	{
		struct
		{
			HANDLE ThreadHandle;
		} In;
		struct
		{
			BOOLEAN ThreadBlocked;
			HANDLE ConnectedProcessId;
			UNICODE_STRING ConnectionPortName;
		} Out;
	};
} ALPC_SERVER_INFORMATION, *PALPC_SERVER_INFORMATION;

// private
typedef struct _ALPC_PORT_MESSAGE_ZONE_INFORMATION
{
	PVOID Buffer;
	ULONG Size;
} ALPC_PORT_MESSAGE_ZONE_INFORMATION, *PALPC_PORT_MESSAGE_ZONE_INFORMATION;

// private
typedef struct _ALPC_PORT_COMPLETION_LIST_INFORMATION
{
	PVOID Buffer; // PALPC_COMPLETION_LIST_HEADER
	ULONG Size;
	ULONG ConcurrencyCount;
	ULONG AttributeFlags;
} ALPC_PORT_COMPLETION_LIST_INFORMATION, *PALPC_PORT_COMPLETION_LIST_INFORMATION;

// private
typedef enum _ALPC_MESSAGE_INFORMATION_CLASS
{
	AlpcMessageSidInformation, // q: out SID
	AlpcMessageTokenModifiedIdInformation,  // q: out LUID
	AlpcMessageDirectStatusInformation,
	AlpcMessageHandleInformation, // ALPC_MESSAGE_HANDLE_INFORMATION
	MaxAlpcMessageInfoClass
} ALPC_MESSAGE_INFORMATION_CLASS, *PALPC_MESSAGE_INFORMATION_CLASS;

typedef struct _ALPC_MESSAGE_HANDLE_INFORMATION
{
	ULONG Index;
	ULONG Flags;
	ULONG Handle;
	ULONG ObjectType;
	ACCESS_MASK GrantedAccess;
} ALPC_MESSAGE_HANDLE_INFORMATION, *PALPC_MESSAGE_HANDLE_INFORMATION;

typedef struct _KALPC_MESSAGE_ATTRIBUTES
{
    VOID* ClientContext;                                                    //0x0
    VOID* ServerContext;                                                    //0x4
    VOID* PortContext;                                                      //0x8
    VOID* CancelPortContext;                                                //0xc
#if 0
    TODO:
    struct _KALPC_SECURITY_DATA* SecurityData;                              //0x10
    struct _KALPC_VIEW* View;                                               //0x14
    struct _KALPC_HANDLE_DATA* HandleData;                                  //0x18
    union _KALPC_DIRECT_EVENT DirectEvent;                                  //0x1c
    struct _KALPC_WORK_ON_BEHALF_DATA WorkOnBehalfData;                     //0x20
#endif
} ALPC_MESSAGE_ATTRIBUTES, *PALPC_MESSAGE_ATTRIBUTES; 

// ALPC Port (expanded, with refcount and basic fields)
typedef struct _ALPC_PORT {
    LONG RefCount;
    HANDLE PortHandle;
    ALPC_PORT_ATTRIBUTES PortAttributes;
    PVOID OwnerProcess;
    PVOID CommunicationInfo;
    LIST_ENTRY PortListEntry;
    // TODO: Add message queues, security, etc.
} ALPC_PORT, *PALPC_PORT;

struct _ALPC_HANDLE_ENTRY
{
    VOID* Object;                                                           //0x0
}; 

typedef struct _ALPC_HANDLE_TABLE
{
    struct _ALPC_HANDLE_ENTRY* Handles;                                     //0x0
    ULONG TotalHandles;                                                     //0x4
    ULONG Flags;                                                            //0x8
    struct _EX_PUSH_LOCK Lock;                                              //0xc
} ALPC_HANDLE_TABLE, *PALPC_HANDLE_TABLE;

struct _ALPC_COMMUNICATION_INFO
{
    struct _ALPC_PORT* ConnectionPort;                                      //0x0
    struct _ALPC_PORT* ServerCommunicationPort;                             //0x4
    struct _ALPC_PORT* ClientCommunicationPort;                             //0x8
    struct _LIST_ENTRY CommunicationList;                                   //0xc
    struct _ALPC_HANDLE_TABLE HandleTable;                                  //0x14
    struct _KALPC_MESSAGE* CloseMessage;                                    //0x24
 }; 


typedef struct _KALPC_RESERVE
{
    ALPC_PORT* OwnerPort;                                           //0x0
    ALPC_HANDLE_TABLE* HandleTable;                                 //0x4
    VOID* Handle;                                                   //0x8
    struct _KALPC_MESSAGE* Message;                                       //0xc
    LONG Active;                                                    //0x10
} KALPC_RESERVE, *PKALPC_RESERVE;
// Global ALPC port list (for demo, real impl will use object manager)
extern LIST_ENTRY AlpcPortList;
extern FAST_MUTEX AlpcPortListLock;

// Reference management
PALPC_PORT AlpcReferencePort(PALPC_PORT Port);
void AlpcDereferencePort(PALPC_PORT Port);

// Port allocation and system init
PALPC_PORT AlpcAllocatePort(PALPC_PORT_ATTRIBUTES PortAttributes);
VOID AlpcpInitSystem(VOID);

// KALPC Message (expanded, partial)
typedef struct _KALPC_MESSAGE
{
    LIST_ENTRY Entry;                                               //0x0
    ALPC_PORT* PortQueue;                                           //0x8
    ALPC_PORT* OwnerPort;                                           //0xc
    ETHREAD* WaitingThread;                                         //0x10
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
    ALPC_PORT* CancelSequencePort;                                  //0x20
    ALPC_PORT* CancelQueuePort;                                     //0x24
    LONG CancelSequenceNo;                                          //0x28
    LIST_ENTRY CancelListEntry;                                     //0x2c
    KALPC_RESERVE* Reserve;                                         //0x34
    struct _KALPC_MESSAGE_ATTRIBUTES MessageAttributes;                     //0x38
    VOID* DataUserVa;                                               //0x60
    struct _ALPC_COMMUNICATION_INFO* CommunicationInfo;                     //0x64
    ALPC_PORT* ConnectionPort;                                      //0x68
    ETHREAD* ServerThread;                                          //0x6c
    VOID* WakeReference;                                            //0x70
    VOID* ExtensionBuffer;                                          //0x74
    ULONG ExtensionBufferSize;                                      //0x78
    PORT_MESSAGE PortMessage;                                       //0x80
} KALPC_MESSAGE, *PKALPC_MESSAGE, ALPC_MESSAGE, *PALPC_MESSAGE;

// ALPC Port View (partial)
typedef struct _ALPC_PORT_VIEW {
    ULONG Length;
    HANDLE SectionHandle;
    ULONG SectionOffset;
    SIZE_T ViewSize;
    PVOID ViewBase;
    PVOID ViewRemoteBase;
} ALPC_PORT_VIEW, *PALPC_PORT_VIEW;

// ALPC Remote Port View (partial)
typedef struct _ALPC_REMOTE_PORT_VIEW {
    ULONG Length;
    SIZE_T ViewSize;
    PVOID ViewBase;
} ALPC_REMOTE_PORT_VIEW, *PALPC_REMOTE_PORT_VIEW;

// Function prototypes (core ALPC kernel API)
char *AlpcGetMessageAttribute(PALPC_MESSAGE_ATTRIBUTES Buffer, ULONG AttributeFlag);
int AlpcInitializeMessageAttribute(ULONG AttributeFlags, PALPC_MESSAGE_ATTRIBUTES Buffer, ULONG BufferSize, PULONG RequiredBufferSize);
ULONG AlpcGetHeaderSize(ULONG Flags);

NTSTATUS AlpcCreatePort(PHANDLE PortHandle, POBJECT_ATTRIBUTES ObjectAttributes, PALPC_PORT_ATTRIBUTES PortAttributes);
NTSTATUS AlpcConnectPort(PHANDLE PortHandle, PUNICODE_STRING PortName, PALPC_PORT_ATTRIBUTES PortAttributes, PALPC_PORT_VIEW PortView, PALPC_REMOTE_PORT_VIEW RemotePortView, PVOID ConnectionInformation, PULONG ConnectionInformationLength);
NTSTATUS AlpcSendWaitReceivePort(HANDLE PortHandle, ULONG Flags, PVOID SendMessage, PALPC_MESSAGE_ATTRIBUTES SendMessageAttributes, PVOID ReceiveMessage, PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes, PLARGE_INTEGER Timeout);
NTSTATUS AlpcAcceptConnectPort(PHANDLE PortHandle, HANDLE ConnectionPortHandle, ULONG Flags, PALPC_PORT_ATTRIBUTES PortAttributes, PALPC_PORT_VIEW PortView, PALPC_REMOTE_PORT_VIEW RemotePortView, PVOID ConnectionInformation, PULONG ConnectionInformationLength);

#endif // _NTOSKRNL_ALPC_H_
