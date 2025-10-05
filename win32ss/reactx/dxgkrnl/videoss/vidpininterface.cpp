
#include <rxgkrnl.h>
//#define NDEBUG
#include <debug.h>

extern PRXGK_PRIVATE_EXTENSION RxgkDriverExtension;

// Internal minimal VidPn manager (non-persistent across reboots, per process lifetime)
// Provides simple containers for VidPn, Topology, SourceModeSet, TargetModeSet and their nodes

typedef struct _RXGK_VIDPN_SOURCE_MODE_NODE
{
    LIST_ENTRY Link;
    D3DKMDT_VIDPN_SOURCE_MODE Mode;
    BOOLEAN AddedToSet;
    ULONG Signature; // 'SRCS'
} RXGK_VIDPN_SOURCE_MODE_NODE, *PRXGK_VIDPN_SOURCE_MODE_NODE;

typedef struct _RXGK_VIDPN_TARGET_MODE_NODE
{
    LIST_ENTRY Link;
    D3DKMDT_VIDPN_TARGET_MODE Mode;
    BOOLEAN AddedToSet;
    ULONG Signature; // 'TSTS'
} RXGK_VIDPN_TARGET_MODE_NODE, *PRXGK_VIDPN_TARGET_MODE_NODE;

typedef struct _RXGK_VIDPN_PRESENT_PATH_NODE
{
    LIST_ENTRY Link;
    D3DKMDT_VIDPN_PRESENT_PATH Path;
    BOOLEAN InTopology;
    ULONG Signature; // 'PATH'
} RXGK_VIDPN_PRESENT_PATH_NODE, *PRXGK_VIDPN_PRESENT_PATH_NODE;

typedef struct _RXGK_VIDPN_SOURCE_MODE_SET
{
    LIST_ENTRY Link;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID SourceId;
    LIST_ENTRY ModeList;
    D3DKMDT_VIDEO_PRESENT_SOURCE_MODE_ID PinnedId;
    ULONG NextModeId;
} RXGK_VIDPN_SOURCE_MODE_SET, *PRXGK_VIDPN_SOURCE_MODE_SET;

typedef struct _RXGK_VIDPN_TARGET_MODE_SET
{
    LIST_ENTRY Link;
    D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId;
    LIST_ENTRY ModeList;
    D3DKMDT_VIDEO_PRESENT_TARGET_MODE_ID PinnedId;
    ULONG NextModeId;
} RXGK_VIDPN_TARGET_MODE_SET, *PRXGK_VIDPN_TARGET_MODE_SET;

typedef struct _RXGK_VIDPN_TOPOLOGY
{
    LIST_ENTRY PathList;
    ULONG Signature; // 'TOPL'
} RXGK_VIDPN_TOPOLOGY, *PRXGK_VIDPN_TOPOLOGY;

typedef struct _RXGK_VIDPN
{
    LIST_ENTRY SourceModeSets; // RXGK_VIDPN_SOURCE_MODE_SET
    LIST_ENTRY TargetModeSets; // RXGK_VIDPN_TARGET_MODE_SET
    PRXGK_VIDPN_TOPOLOGY Topology;
    ULONG Signature; // 'VIDP'
} RXGK_VIDPN, *PRXGK_VIDPN;

#define RXGK_TAG_VIDPN 'ndPV'
#define RXGK_TAG_PATH  'htaP'
#define RXGK_TAG_SRCM  'mcrS'
#define RXGK_TAG_TGTM  'mcrT'

// Simple handle validation helpers: in this implementation, handles are direct pointers
static __forceinline PRXGK_VIDPN RxgkFromVidPnHandle(_In_ D3DKMDT_HVIDPN hVidPn)
{
    PRXGK_VIDPN p = (PRXGK_VIDPN)hVidPn;
    if (!p || p->Signature != 'VIDP')
        return NULL;
    return p;
}

static __forceinline PRXGK_VIDPN_TOPOLOGY RxgkFromTopologyHandle(_In_ D3DKMDT_HVIDPNTOPOLOGY hTopology)
{
    PRXGK_VIDPN_TOPOLOGY t = (PRXGK_VIDPN_TOPOLOGY)hTopology;
    if (!t || t->Signature != 'TOPL')
        return NULL;
    return t;
}

static PRXGK_VIDPN RxgkAllocateVidPn()
{
    PRXGK_VIDPN VidPn = (PRXGK_VIDPN)ExAllocatePoolWithTag(NonPagedPool, sizeof(RXGK_VIDPN), RXGK_TAG_VIDPN);
    if (!VidPn)
        return NULL;
    InitializeListHead(&VidPn->SourceModeSets);
    InitializeListHead(&VidPn->TargetModeSets);
    VidPn->Topology = (PRXGK_VIDPN_TOPOLOGY)ExAllocatePoolWithTag(NonPagedPool, sizeof(RXGK_VIDPN_TOPOLOGY), RXGK_TAG_VIDPN);
    if (!VidPn->Topology)
    {
        ExFreePoolWithTag(VidPn, RXGK_TAG_VIDPN);
        return NULL;
    }
    InitializeListHead(&VidPn->Topology->PathList);
    VidPn->Topology->Signature = 'TOPL';
    VidPn->Signature = 'VIDP';
    return VidPn;
}

NTSTATUS
NTAPI
RxgkCreateVidPn(_Out_ D3DKMDT_HVIDPN* phVidPn)
{
    if (!phVidPn)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN VidPn = RxgkAllocateVidPn();
    if (!VidPn)
        return STATUS_NO_MEMORY;
    *phVidPn = (D3DKMDT_HVIDPN)VidPn;
    return STATUS_SUCCESS;
}

VOID
NTAPI
RxgkDestroyVidPn(_In_ D3DKMDT_HVIDPN hVidPn)
{
    PRXGK_VIDPN VidPn = (PRXGK_VIDPN)hVidPn;
    if (!VidPn || VidPn->Signature != 'VIDP')
        return;

    while (!IsListEmpty(&VidPn->Topology->PathList))
    {
        PLIST_ENTRY e = RemoveHeadList(&VidPn->Topology->PathList);
        PRXGK_VIDPN_PRESENT_PATH_NODE n = CONTAINING_RECORD(e, RXGK_VIDPN_PRESENT_PATH_NODE, Link);
        ExFreePoolWithTag(n, RXGK_TAG_PATH);
    }

    while (!IsListEmpty(&VidPn->SourceModeSets))
    {
        PLIST_ENTRY e = RemoveHeadList(&VidPn->SourceModeSets);
        PRXGK_VIDPN_SOURCE_MODE_SET s = CONTAINING_RECORD(e, RXGK_VIDPN_SOURCE_MODE_SET, Link);
        while (!IsListEmpty(&s->ModeList))
        {
            PLIST_ENTRY me = RemoveHeadList(&s->ModeList);
            PRXGK_VIDPN_SOURCE_MODE_NODE mn = CONTAINING_RECORD(me, RXGK_VIDPN_SOURCE_MODE_NODE, Link);
            ExFreePoolWithTag(mn, RXGK_TAG_SRCM);
        }
        ExFreePoolWithTag(s, RXGK_TAG_SRCM);
    }

    while (!IsListEmpty(&VidPn->TargetModeSets))
    {
        PLIST_ENTRY e = RemoveHeadList(&VidPn->TargetModeSets);
        PRXGK_VIDPN_TARGET_MODE_SET s = CONTAINING_RECORD(e, RXGK_VIDPN_TARGET_MODE_SET, Link);
        while (!IsListEmpty(&s->ModeList))
        {
            PLIST_ENTRY me = RemoveHeadList(&s->ModeList);
            PRXGK_VIDPN_TARGET_MODE_NODE mn = CONTAINING_RECORD(me, RXGK_VIDPN_TARGET_MODE_NODE, Link);
            ExFreePoolWithTag(mn, RXGK_TAG_TGTM);
        }
        ExFreePoolWithTag(s, RXGK_TAG_TGTM);
    }

    ExFreePoolWithTag(VidPn->Topology, RXGK_TAG_VIDPN);
    ExFreePoolWithTag(VidPn, RXGK_TAG_VIDPN);
}

static PRXGK_VIDPN_SOURCE_MODE_SET RxgkFindSourceModeSet(_In_ PRXGK_VIDPN VidPn, _In_ D3DDDI_VIDEO_PRESENT_SOURCE_ID SourceId)
{
    for (PLIST_ENTRY e = VidPn->SourceModeSets.Flink; e != &VidPn->SourceModeSets; e = e->Flink)
    {
        PRXGK_VIDPN_SOURCE_MODE_SET s = CONTAINING_RECORD(e, RXGK_VIDPN_SOURCE_MODE_SET, Link);
        if (s->SourceId == SourceId)
            return s;
    }
    return NULL;
}

static PRXGK_VIDPN_SOURCE_MODE_SET RxgkEnsureSourceModeSet(_In_ PRXGK_VIDPN VidPn, _In_ D3DDDI_VIDEO_PRESENT_SOURCE_ID SourceId)
{
    PRXGK_VIDPN_SOURCE_MODE_SET Set = RxgkFindSourceModeSet(VidPn, SourceId);
    if (Set)
        return Set;
    Set = (PRXGK_VIDPN_SOURCE_MODE_SET)ExAllocatePoolWithTag(NonPagedPool, sizeof(RXGK_VIDPN_SOURCE_MODE_SET), RXGK_TAG_SRCM);
    if (!Set)
        return NULL;
    Set->SourceId = SourceId;
    InitializeListHead(&Set->ModeList);
    Set->PinnedId = 0;
    Set->NextModeId = 1;
    InsertTailList(&VidPn->SourceModeSets, &Set->Link);
    return Set;
}

static PRXGK_VIDPN_TARGET_MODE_SET RxgkFindTargetModeSet(_In_ PRXGK_VIDPN VidPn, _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId)
{
    for (PLIST_ENTRY e = VidPn->TargetModeSets.Flink; e != &VidPn->TargetModeSets; e = e->Flink)
    {
        PRXGK_VIDPN_TARGET_MODE_SET s = CONTAINING_RECORD(e, RXGK_VIDPN_TARGET_MODE_SET, Link);
        if (s->TargetId == TargetId)
            return s;
    }
    return NULL;
}

static PRXGK_VIDPN_TARGET_MODE_SET RxgkEnsureTargetModeSet(_In_ PRXGK_VIDPN VidPn, _In_ D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId)
{
    PRXGK_VIDPN_TARGET_MODE_SET Set = RxgkFindTargetModeSet(VidPn, TargetId);
    if (Set)
        return Set;
    Set = (PRXGK_VIDPN_TARGET_MODE_SET)ExAllocatePoolWithTag(NonPagedPool, sizeof(RXGK_VIDPN_TARGET_MODE_SET), RXGK_TAG_TGTM);
    if (!Set)
        return NULL;
    Set->TargetId = TargetId;
    InitializeListHead(&Set->ModeList);
    Set->PinnedId = 0;
    Set->NextModeId = 1;
    InsertTailList(&VidPn->TargetModeSets, &Set->Link);
    return Set;
}

// Helpers: count and iteration
static SIZE_T RxgkCountList(_In_ const LIST_ENTRY* Head)
{
    SIZE_T n = 0;
    for (const LIST_ENTRY* e = Head->Flink; e != Head; e = e->Flink)
        ++n;
    return n;
}

// Global interface singletons
static DXGK_VIDPNSOURCEMODESET_INTERFACE g_SourceModeSetInterface;
static DXGK_VIDPNTARGETMODESET_INTERFACE g_TargetModeSetInterface;
static DXGK_VIDPNTOPOLOGY_INTERFACE g_TopologyInterface;
static DXGK_VIDPN_INTERFACE g_VidPnInterface;
static BOOLEAN g_InterfacesInitialized = FALSE;

// Forward declarations of interface methods
// Forward declarations of VidPn interface entry points
NTSTATUS
APIENTRY
RxgkVidPnGetTopology(
    _In_ const D3DKMDT_HVIDPN                              hVidPn,
    _Out_ D3DKMDT_HVIDPNTOPOLOGY*                          phVidPnTopology,
    _Outptr_ const DXGK_VIDPNTOPOLOGY_INTERFACE**          ppVidPnTopologyInterface);

NTSTATUS
APIENTRY
RxgkVidPnAcquireSourceModeSet(
    _In_ const D3DKMDT_HVIDPN                                hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID                VidPnSourceId,
    _Out_ D3DKMDT_HVIDPNSOURCEMODESET *                      phVidPnSourceModeSet,
    _Outptr_ const DXGK_VIDPNSOURCEMODESET_INTERFACE**       ppVidPnSourceModeSetInterface);

NTSTATUS
APIENTRY
RxgkVidPnReleaseSourceModeSet(
    _In_ const D3DKMDT_HVIDPN                                hVidPn,
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET                   hVidPnSourceModeSet);

NTSTATUS
APIENTRY
RxgkVidPnCreateNewSourceModeSet(
    _In_ const D3DKMDT_HVIDPN                                hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID                VidPnSourceId,
    _Out_ D3DKMDT_HVIDPNSOURCEMODESET*                       phNewVidPnSourceModeSet,
    _Outptr_ const DXGK_VIDPNSOURCEMODESET_INTERFACE**       ppVidPnSourceModeSetInterface);

NTSTATUS
APIENTRY
RxgkVidPnAssignSourceModeSet(
    _In_ D3DKMDT_HVIDPN                                      hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID                VidPnSourceId,
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET                   hVidPnSourceModeSet);

NTSTATUS
APIENTRY
RxgkVidPnAssignMultisamplingMethodSet(
    _In_ D3DKMDT_HVIDPN                                       hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID                 VidPnSourceId,
    _In_ const SIZE_T                                         NumMethods,
    _In_reads_(NumMethods) CONST D3DDDI_MULTISAMPLINGMETHOD*  pSupportedMethodSet);

NTSTATUS
APIENTRY
RxgkVidPnAcquireTargetModeSet(
    _In_ const D3DKMDT_HVIDPN                                  hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID                  VidPnTargetId,
    _Out_ D3DKMDT_HVIDPNTARGETMODESET*                         phVidPnTargetModeSet,
    _Outptr_ const DXGK_VIDPNTARGETMODESET_INTERFACE**         ppVidPnTargetModeSetInterface);

NTSTATUS
APIENTRY
RxgkVidPnReleaseTargetModeSet(
    _In_ const D3DKMDT_HVIDPN                                  hVidPn,
    _In_ const D3DKMDT_HVIDPNTARGETMODESET                     hVidPnTargetModeSet);

NTSTATUS
APIENTRY
RxgkVidPnCreateNewTargetModeSet(
    _In_ const D3DKMDT_HVIDPN                               hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID               VidPnTargetId,
    _Out_ D3DKMDT_HVIDPNTARGETMODESET*                      phNewVidPnTargetModeSet,
    _Outptr_ const DXGK_VIDPNTARGETMODESET_INTERFACE**      ppVidPnTargetModeSetInterace);

NTSTATUS
APIENTRY
RxgkVidPnAssignTargetModeSet(
    _In_ D3DKMDT_HVIDPN                                     hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID               VidPnTargetId,
    _In_ const D3DKMDT_HVIDPNTARGETMODESET                  hVidPnTargetModeSet);

// Internal set/topology method prototypes
static NTSTATUS APIENTRY Rxgk_SourceModeSet_GetNumModes(_In_ const D3DKMDT_HVIDPNSOURCEMODESET hSet, _Out_ SIZE_T* const pNumSourceModes);
static NTSTATUS APIENTRY Rxgk_SourceModeSet_AcquireFirstModeInfo(_In_ const D3DKMDT_HVIDPNSOURCEMODESET hSet, _Outptr_ const D3DKMDT_VIDPN_SOURCE_MODE** ppFirst);
static NTSTATUS APIENTRY Rxgk_SourceModeSet_AcquireNextModeInfo(_In_ const D3DKMDT_HVIDPNSOURCEMODESET hSet, _In_ const D3DKMDT_VIDPN_SOURCE_MODE* pCurrent, _Outptr_ const D3DKMDT_VIDPN_SOURCE_MODE** ppNext);
static NTSTATUS APIENTRY Rxgk_SourceModeSet_AcquirePinnedModeInfo(_In_ const D3DKMDT_HVIDPNSOURCEMODESET hSet, _Outptr_ const D3DKMDT_VIDPN_SOURCE_MODE** ppPinned);
static NTSTATUS APIENTRY Rxgk_SourceModeSet_ReleaseModeInfo(_In_ const D3DKMDT_HVIDPNSOURCEMODESET hSet, _In_ const D3DKMDT_VIDPN_SOURCE_MODE* pMode);
static NTSTATUS APIENTRY Rxgk_SourceModeSet_CreateNewModeInfo(_In_ const D3DKMDT_HVIDPNSOURCEMODESET hSet, _Outptr_ const D3DKMDT_VIDPN_SOURCE_MODE** ppNew);
static NTSTATUS APIENTRY Rxgk_SourceModeSet_AddMode(_In_ D3DKMDT_HVIDPNSOURCEMODESET hSet, _In_ D3DKMDT_VIDPN_SOURCE_MODE* pMode);
static NTSTATUS APIENTRY Rxgk_SourceModeSet_PinMode(_In_ D3DKMDT_HVIDPNSOURCEMODESET hSet, _In_ const D3DKMDT_VIDEO_PRESENT_SOURCE_MODE_ID VidPnSourceModeId);

static NTSTATUS APIENTRY Rxgk_TargetModeSet_GetNumModes(_In_ const D3DKMDT_HVIDPNTARGETMODESET hSet, _Out_ SIZE_T* const pNumTargetModes);
static NTSTATUS APIENTRY Rxgk_TargetModeSet_AcquireFirstModeInfo(_In_ const D3DKMDT_HVIDPNTARGETMODESET hSet, _Outptr_ const D3DKMDT_VIDPN_TARGET_MODE** ppFirst);
static NTSTATUS APIENTRY Rxgk_TargetModeSet_AcquireNextModeInfo(_In_ const D3DKMDT_HVIDPNTARGETMODESET hSet, _In_ const D3DKMDT_VIDPN_TARGET_MODE* pCurrent, _Outptr_ const D3DKMDT_VIDPN_TARGET_MODE** ppNext);
static NTSTATUS APIENTRY Rxgk_TargetModeSet_AcquirePinnedModeInfo(_In_ const D3DKMDT_HVIDPNTARGETMODESET hSet, _Outptr_ const D3DKMDT_VIDPN_TARGET_MODE** ppPinned);
static NTSTATUS APIENTRY Rxgk_TargetModeSet_ReleaseModeInfo(_In_ const D3DKMDT_HVIDPNTARGETMODESET hSet, _In_ const D3DKMDT_VIDPN_TARGET_MODE* pMode);
static NTSTATUS APIENTRY Rxgk_TargetModeSet_CreateNewModeInfo(_In_ const D3DKMDT_HVIDPNTARGETMODESET hSet, _Outptr_ const D3DKMDT_VIDPN_TARGET_MODE** ppNew);
static NTSTATUS APIENTRY Rxgk_TargetModeSet_AddMode(_In_ D3DKMDT_HVIDPNTARGETMODESET hSet, _In_ D3DKMDT_VIDPN_TARGET_MODE* pMode);
static NTSTATUS APIENTRY Rxgk_TargetModeSet_PinMode(_In_ D3DKMDT_HVIDPNTARGETMODESET hSet, _In_ const D3DKMDT_VIDEO_PRESENT_TARGET_MODE_ID VidPnTargetModeId);

static NTSTATUS APIENTRY Rxgk_Topology_GetNumPaths(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _Out_ PSIZE_T pNumPaths);
static NTSTATUS APIENTRY Rxgk_Topology_GetNumPathsFromSource(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId, _Out_ PSIZE_T pNumPathsFromSource);
static NTSTATUS APIENTRY Rxgk_Topology_EnumPathTargetsFromSource(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId, _In_ const D3DKMDT_VIDPN_PRESENT_PATH_INDEX VidPnPresentPathIndex, _Out_ D3DDDI_VIDEO_PRESENT_TARGET_ID* pVidPnTargetId);
static NTSTATUS APIENTRY Rxgk_Topology_GetPathSourceFromTarget(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId, _Out_ D3DDDI_VIDEO_PRESENT_SOURCE_ID* pVidPnSourceId);
static NTSTATUS APIENTRY Rxgk_Topology_AcquirePathInfo(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId, _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId, _Outptr_ const D3DKMDT_VIDPN_PRESENT_PATH** ppPathInfo);
static NTSTATUS APIENTRY Rxgk_Topology_AcquireFirstPathInfo(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _Outptr_ const D3DKMDT_VIDPN_PRESENT_PATH** ppFirstPathInfo);
static NTSTATUS APIENTRY Rxgk_Topology_AcquireNextPathInfo(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _In_ const D3DKMDT_VIDPN_PRESENT_PATH* pCurrent, _Outptr_ const D3DKMDT_VIDPN_PRESENT_PATH** ppNext);
static NTSTATUS APIENTRY Rxgk_Topology_UpdatePathSupportInfo(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _In_ const D3DKMDT_VIDPN_PRESENT_PATH* pPathInfo);
static NTSTATUS APIENTRY Rxgk_Topology_ReleasePathInfo(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _In_ const D3DKMDT_VIDPN_PRESENT_PATH* pPathInfo);
static NTSTATUS APIENTRY Rxgk_Topology_CreateNewPathInfo(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _Outptr_ D3DKMDT_VIDPN_PRESENT_PATH** ppNewPathInfo);
static NTSTATUS APIENTRY Rxgk_Topology_AddPath(_In_ D3DKMDT_HVIDPNTOPOLOGY hTopology, _In_ D3DKMDT_VIDPN_PRESENT_PATH* pPathInfo);
static NTSTATUS APIENTRY Rxgk_Topology_RemovePath(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId, _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId);

static VOID RxgkInitializeVidPnInterfaces()
{
    if (g_InterfacesInitialized)
        return;

    // SourceModeSet
    g_SourceModeSetInterface.pfnGetNumModes = Rxgk_SourceModeSet_GetNumModes;
    g_SourceModeSetInterface.pfnAcquireFirstModeInfo = Rxgk_SourceModeSet_AcquireFirstModeInfo;
    g_SourceModeSetInterface.pfnAcquireNextModeInfo = Rxgk_SourceModeSet_AcquireNextModeInfo;
    g_SourceModeSetInterface.pfnAcquirePinnedModeInfo = Rxgk_SourceModeSet_AcquirePinnedModeInfo;
    g_SourceModeSetInterface.pfnReleaseModeInfo = Rxgk_SourceModeSet_ReleaseModeInfo;
    g_SourceModeSetInterface.pfnCreateNewModeInfo = Rxgk_SourceModeSet_CreateNewModeInfo;
    g_SourceModeSetInterface.pfnAddMode = Rxgk_SourceModeSet_AddMode;
    g_SourceModeSetInterface.pfnPinMode = Rxgk_SourceModeSet_PinMode;

    // TargetModeSet
    g_TargetModeSetInterface.pfnGetNumModes = Rxgk_TargetModeSet_GetNumModes;
    g_TargetModeSetInterface.pfnAcquireFirstModeInfo = Rxgk_TargetModeSet_AcquireFirstModeInfo;
    g_TargetModeSetInterface.pfnAcquireNextModeInfo = Rxgk_TargetModeSet_AcquireNextModeInfo;
    g_TargetModeSetInterface.pfnAcquirePinnedModeInfo = Rxgk_TargetModeSet_AcquirePinnedModeInfo;
    g_TargetModeSetInterface.pfnReleaseModeInfo = Rxgk_TargetModeSet_ReleaseModeInfo;
    g_TargetModeSetInterface.pfnCreateNewModeInfo = Rxgk_TargetModeSet_CreateNewModeInfo;
    g_TargetModeSetInterface.pfnAddMode = Rxgk_TargetModeSet_AddMode;
    g_TargetModeSetInterface.pfnPinMode = Rxgk_TargetModeSet_PinMode;

    // Topology
    g_TopologyInterface.pfnGetNumPaths = Rxgk_Topology_GetNumPaths;
    g_TopologyInterface.pfnGetNumPathsFromSource = Rxgk_Topology_GetNumPathsFromSource;
    g_TopologyInterface.pfnEnumPathTargetsFromSource = Rxgk_Topology_EnumPathTargetsFromSource;
    g_TopologyInterface.pfnGetPathSourceFromTarget = Rxgk_Topology_GetPathSourceFromTarget;
    g_TopologyInterface.pfnAcquirePathInfo = Rxgk_Topology_AcquirePathInfo;
    g_TopologyInterface.pfnAcquireFirstPathInfo = Rxgk_Topology_AcquireFirstPathInfo;
    g_TopologyInterface.pfnAcquireNextPathInfo = Rxgk_Topology_AcquireNextPathInfo;
    g_TopologyInterface.pfnUpdatePathSupportInfo = Rxgk_Topology_UpdatePathSupportInfo;
    g_TopologyInterface.pfnReleasePathInfo = Rxgk_Topology_ReleasePathInfo;
    g_TopologyInterface.pfnCreateNewPathInfo = Rxgk_Topology_CreateNewPathInfo;
    g_TopologyInterface.pfnAddPath = Rxgk_Topology_AddPath;
    g_TopologyInterface.pfnRemovePath = Rxgk_Topology_RemovePath;

    // VidPn Interface
    g_VidPnInterface.Version = DXGK_VIDPN_INTERFACE_VERSION_V1;
    g_VidPnInterface.pfnGetTopology = RxgkVidPnGetTopology;
    g_VidPnInterface.pfnAcquireSourceModeSet = RxgkVidPnAcquireSourceModeSet;
    g_VidPnInterface.pfnReleaseSourceModeSet = RxgkVidPnReleaseSourceModeSet;
    g_VidPnInterface.pfnCreateNewSourceModeSet = RxgkVidPnCreateNewSourceModeSet;
    g_VidPnInterface.pfnAssignSourceModeSet = RxgkVidPnAssignSourceModeSet;
    g_VidPnInterface.pfnAssignMultisamplingMethodSet = RxgkVidPnAssignMultisamplingMethodSet;
    g_VidPnInterface.pfnAcquireTargetModeSet = RxgkVidPnAcquireTargetModeSet;
    g_VidPnInterface.pfnReleaseTargetModeSet = RxgkVidPnReleaseTargetModeSet;
    g_VidPnInterface.pfnCreateNewTargetModeSet = RxgkVidPnCreateNewTargetModeSet;
    g_VidPnInterface.pfnAssignTargetModeSet = RxgkVidPnAssignTargetModeSet;

    g_InterfacesInitialized = TRUE;
}

// ========================= VidPn Interface implementation =========================

NTSTATUS
APIENTRY
RxgkVidPnGetTopology(
    _In_ const D3DKMDT_HVIDPN                              hVidPn,
    _Out_ D3DKMDT_HVIDPNTOPOLOGY*                          phVidPnTopology,
    _Outptr_ const DXGK_VIDPNTOPOLOGY_INTERFACE**           ppVidPnTopologyInterface)
{
    if (!phVidPnTopology || !ppVidPnTopologyInterface)
        return STATUS_INVALID_PARAMETER;

    RxgkInitializeVidPnInterfaces();

    PRXGK_VIDPN VidPn = RxgkFromVidPnHandle(hVidPn);
    if (!VidPn)
        return STATUS_NO_MEMORY;

    *phVidPnTopology = (D3DKMDT_HVIDPNTOPOLOGY)VidPn->Topology;
    *ppVidPnTopologyInterface = &g_TopologyInterface;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
RxgkVidPnAcquireSourceModeSet(
    _In_ const D3DKMDT_HVIDPN                                hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID                VidPnSourceId,
    _Out_ D3DKMDT_HVIDPNSOURCEMODESET *                      phVidPnSourceModeSet,
    _Outptr_ const DXGK_VIDPNSOURCEMODESET_INTERFACE**    ppVidPnSourceModeSetInterface)
{
    if (!phVidPnSourceModeSet || !ppVidPnSourceModeSetInterface)
        return STATUS_INVALID_PARAMETER;

    RxgkInitializeVidPnInterfaces();
    PRXGK_VIDPN VidPn = RxgkFromVidPnHandle(hVidPn);
    if (!VidPn)
        return STATUS_NOT_FOUND;

    PRXGK_VIDPN_SOURCE_MODE_SET Set = RxgkFindSourceModeSet(VidPn, VidPnSourceId);
    if (!Set)
        return STATUS_NOT_FOUND;

    *phVidPnSourceModeSet = (D3DKMDT_HVIDPNSOURCEMODESET)Set;
    *ppVidPnSourceModeSetInterface = &g_SourceModeSetInterface;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
RxgkVidPnReleaseSourceModeSet(
    _In_ const D3DKMDT_HVIDPN                                hVidPn,
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET                   hVidPnSourceModeSet)
{
    UNREFERENCED_PARAMETER(hVidPn);
    UNREFERENCED_PARAMETER(hVidPnSourceModeSet);
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
RxgkVidPnCreateNewSourceModeSet(
    _In_ const D3DKMDT_HVIDPN                                hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID                VidPnSourceId,
    _Out_ D3DKMDT_HVIDPNSOURCEMODESET*                       phNewVidPnSourceModeSet,
    _Outptr_ const DXGK_VIDPNSOURCEMODESET_INTERFACE**       ppVidPnSourceModeSetInterface)
{
    if (!phNewVidPnSourceModeSet || !ppVidPnSourceModeSetInterface)
        return STATUS_INVALID_PARAMETER;

    RxgkInitializeVidPnInterfaces();
    PRXGK_VIDPN VidPn = RxgkFromVidPnHandle(hVidPn);
    if (!VidPn)
        return STATUS_NO_MEMORY;

    PRXGK_VIDPN_SOURCE_MODE_SET Set = (PRXGK_VIDPN_SOURCE_MODE_SET)ExAllocatePoolWithTag(NonPagedPool, sizeof(RXGK_VIDPN_SOURCE_MODE_SET), RXGK_TAG_SRCM);
    if (!Set)
        return STATUS_NO_MEMORY;
    Set->SourceId = VidPnSourceId;
    InitializeListHead(&Set->ModeList);
    Set->PinnedId = 0;
    Set->NextModeId = 1;
    *phNewVidPnSourceModeSet = (D3DKMDT_HVIDPNSOURCEMODESET)Set;
    *ppVidPnSourceModeSetInterface = &g_SourceModeSetInterface;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
RxgkVidPnAssignSourceModeSet(
    _In_ D3DKMDT_HVIDPN                                      hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID                VidPnSourceId,
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET                   hVidPnSourceModeSet)
{
    PRXGK_VIDPN VidPn = RxgkFromVidPnHandle(hVidPn);
    if (!VidPn)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_SOURCE_MODE_SET NewSet = (PRXGK_VIDPN_SOURCE_MODE_SET)hVidPnSourceModeSet;
    if (!NewSet)
        return STATUS_INVALID_PARAMETER;

    // Replace or attach in VidPn container
    PRXGK_VIDPN_SOURCE_MODE_SET Existing = RxgkFindSourceModeSet(VidPn, VidPnSourceId);
    if (!Existing)
    {
        InsertTailList(&VidPn->SourceModeSets, &NewSet->Link);
    }
    else
    {
        // Replace link; do not free to avoid races
        Existing->SourceId = NewSet->SourceId;
        Existing->ModeList = NewSet->ModeList;
        Existing->PinnedId = NewSet->PinnedId;
        Existing->NextModeId = NewSet->NextModeId;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
RxgkVidPnAssignMultisamplingMethodSet(
    _In_ D3DKMDT_HVIDPN                                       hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID                 VidPnSourceId,
    _In_ const SIZE_T                                         NumMethods,
    _In_reads_(NumMethods) CONST D3DDDI_MULTISAMPLINGMETHOD*  pSupportedMethodSet)
{
    UNREFERENCED_PARAMETER(hVidPn);
    UNREFERENCED_PARAMETER(VidPnSourceId);
    UNREFERENCED_PARAMETER(NumMethods);
    UNREFERENCED_PARAMETER(pSupportedMethodSet);
    return STATUS_SUCCESS;
}


NTSTATUS
APIENTRY
RxgkVidPnAcquireTargetModeSet(
    _In_ const D3DKMDT_HVIDPN                                  hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID                  VidPnTargetId,
    _Out_ D3DKMDT_HVIDPNTARGETMODESET*                         phVidPnTargetModeSet,
    _Outptr_ const DXGK_VIDPNTARGETMODESET_INTERFACE**         ppVidPnTargetModeSetInterface)
{
    if (!phVidPnTargetModeSet || !ppVidPnTargetModeSetInterface)
        return STATUS_INVALID_PARAMETER;

    RxgkInitializeVidPnInterfaces();
    PRXGK_VIDPN VidPn = RxgkFromVidPnHandle(hVidPn);
    if (!VidPn)
        return STATUS_NOT_FOUND;

    PRXGK_VIDPN_TARGET_MODE_SET Set = RxgkFindTargetModeSet(VidPn, VidPnTargetId);
    if (!Set)
        return STATUS_NOT_FOUND;

    *phVidPnTargetModeSet = (D3DKMDT_HVIDPNTARGETMODESET)Set;
    *ppVidPnTargetModeSetInterface = &g_TargetModeSetInterface;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
RxgkVidPnReleaseTargetModeSet(
    _In_ const D3DKMDT_HVIDPN                                  hVidPn,
    _In_ const D3DKMDT_HVIDPNTARGETMODESET                     hVidPnTargetModeSet)
{
    UNREFERENCED_PARAMETER(hVidPn);
    UNREFERENCED_PARAMETER(hVidPnTargetModeSet);
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
RxgkVidPnCreateNewTargetModeSet(
    _In_ const D3DKMDT_HVIDPN                               hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID               VidPnTargetId,
    _Out_ D3DKMDT_HVIDPNTARGETMODESET*                      phNewVidPnTargetModeSet,
    _Outptr_ const DXGK_VIDPNTARGETMODESET_INTERFACE**      ppVidPnTargetModeSetInterace)
{
    if (!phNewVidPnTargetModeSet || !ppVidPnTargetModeSetInterace)
        return STATUS_INVALID_PARAMETER;

    RxgkInitializeVidPnInterfaces();
    PRXGK_VIDPN VidPn = RxgkFromVidPnHandle(hVidPn);
    if (!VidPn)
        return STATUS_NO_MEMORY;

    PRXGK_VIDPN_TARGET_MODE_SET Set = (PRXGK_VIDPN_TARGET_MODE_SET)ExAllocatePoolWithTag(NonPagedPool, sizeof(RXGK_VIDPN_TARGET_MODE_SET), RXGK_TAG_TGTM);
    if (!Set)
        return STATUS_NO_MEMORY;
    Set->TargetId = VidPnTargetId;
    InitializeListHead(&Set->ModeList);
    Set->PinnedId = 0;
    Set->NextModeId = 1;
    *phNewVidPnTargetModeSet = (D3DKMDT_HVIDPNTARGETMODESET)Set;
    *ppVidPnTargetModeSetInterace = &g_TargetModeSetInterface;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
RxgkVidPnAssignTargetModeSet(
    _In_ D3DKMDT_HVIDPN                                     hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID               VidPnTargetId,
    _In_ const D3DKMDT_HVIDPNTARGETMODESET                  hVidPnTargetModeSet)
{
    PRXGK_VIDPN VidPn = RxgkFromVidPnHandle(hVidPn);
    if (!VidPn)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_TARGET_MODE_SET NewSet = (PRXGK_VIDPN_TARGET_MODE_SET)hVidPnTargetModeSet;
    if (!NewSet)
        return STATUS_INVALID_PARAMETER;

    PRXGK_VIDPN_TARGET_MODE_SET Existing = RxgkFindTargetModeSet(VidPn, VidPnTargetId);
    if (!Existing)
    {
        InsertTailList(&VidPn->TargetModeSets, &NewSet->Link);
    }
    else
    {
        Existing->TargetId = NewSet->TargetId;
        Existing->ModeList = NewSet->ModeList;
        Existing->PinnedId = NewSet->PinnedId;
        Existing->NextModeId = NewSet->NextModeId;
    }
    return STATUS_SUCCESS;
}

/*
 *  Just provide the VidPn Interface to the KMD miniport driver
 */
NTSTATUS
APIENTRY
CALLBACK
RxgkCbQueryVidPnInterface(_In_ const D3DKMDT_HVIDPN                             hVidPn,
                          _In_ const DXGK_VIDPN_INTERFACE_VERSION               VidPnInterfaceVersion,
                          _Outptr_ const DXGK_VIDPN_INTERFACE**                  ppVidPnInterface)

{
    if (!ppVidPnInterface)
        return STATUS_INVALID_PARAMETER;
    UNREFERENCED_PARAMETER(hVidPn);
    UNREFERENCED_PARAMETER(VidPnInterfaceVersion);

    RxgkInitializeVidPnInterfaces();
    *ppVidPnInterface = &g_VidPnInterface;
    return STATUS_SUCCESS;
}

// ========================= SourceModeSet methods =========================

static NTSTATUS APIENTRY Rxgk_SourceModeSet_GetNumModes(_In_ const D3DKMDT_HVIDPNSOURCEMODESET hSet, _Out_ SIZE_T* const pNumSourceModes)
{
    if (!pNumSourceModes || !hSet)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_SOURCE_MODE_SET Set = (PRXGK_VIDPN_SOURCE_MODE_SET)hSet;
    *pNumSourceModes = RxgkCountList(&Set->ModeList);
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_SourceModeSet_AcquireFirstModeInfo(_In_ const D3DKMDT_HVIDPNSOURCEMODESET hSet, _Outptr_ const D3DKMDT_VIDPN_SOURCE_MODE** ppFirst)
{
    if (!ppFirst || !hSet)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_SOURCE_MODE_SET Set = (PRXGK_VIDPN_SOURCE_MODE_SET)hSet;
    if (IsListEmpty(&Set->ModeList))
        return STATUS_NOT_FOUND;
    PRXGK_VIDPN_SOURCE_MODE_NODE Node = CONTAINING_RECORD(Set->ModeList.Flink, RXGK_VIDPN_SOURCE_MODE_NODE, Link);
    *ppFirst = &Node->Mode;
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_SourceModeSet_AcquireNextModeInfo(_In_ const D3DKMDT_HVIDPNSOURCEMODESET hSet, _In_ const D3DKMDT_VIDPN_SOURCE_MODE* pCurrent, _Outptr_ const D3DKMDT_VIDPN_SOURCE_MODE** ppNext)
{
    UNREFERENCED_PARAMETER(hSet);
    if (!ppNext || !pCurrent)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_SOURCE_MODE_NODE Cur = CONTAINING_RECORD(pCurrent, RXGK_VIDPN_SOURCE_MODE_NODE, Mode);
    if (Cur->Link.Flink == NULL || Cur->Link.Flink == Cur->Link.Blink)
        return STATUS_NOT_FOUND;
    if (Cur->Link.Flink == &Cur->Link)
        return STATUS_NOT_FOUND;
    PRXGK_VIDPN_SOURCE_MODE_NODE Next = CONTAINING_RECORD(Cur->Link.Flink, RXGK_VIDPN_SOURCE_MODE_NODE, Link);
    *ppNext = &Next->Mode;
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_SourceModeSet_AcquirePinnedModeInfo(_In_ const D3DKMDT_HVIDPNSOURCEMODESET hSet, _Outptr_ const D3DKMDT_VIDPN_SOURCE_MODE** ppPinned)
{
    if (!ppPinned || !hSet)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_SOURCE_MODE_SET Set = (PRXGK_VIDPN_SOURCE_MODE_SET)hSet;
    if (Set->PinnedId == 0)
        return STATUS_NOT_FOUND;
    for (PLIST_ENTRY e = Set->ModeList.Flink; e != &Set->ModeList; e = e->Flink)
    {
        PRXGK_VIDPN_SOURCE_MODE_NODE Node = CONTAINING_RECORD(e, RXGK_VIDPN_SOURCE_MODE_NODE, Link);
        if (Node->Mode.Id == Set->PinnedId)
        {
            *ppPinned = &Node->Mode;
            return STATUS_SUCCESS;
        }
    }
    return STATUS_NOT_FOUND;
}

static NTSTATUS APIENTRY Rxgk_SourceModeSet_ReleaseModeInfo(_In_ const D3DKMDT_HVIDPNSOURCEMODESET hSet, _In_ const D3DKMDT_VIDPN_SOURCE_MODE* pMode)
{
    UNREFERENCED_PARAMETER(hSet);
    if (!pMode)
        return STATUS_INVALID_PARAMETER;
    // Free only if the node was created via CreateNew and not added
    PRXGK_VIDPN_SOURCE_MODE_NODE Node = CONTAINING_RECORD(pMode, RXGK_VIDPN_SOURCE_MODE_NODE, Mode);
    if (!Node->AddedToSet && Node->Signature == 'SRCS')
    {
        ExFreePoolWithTag(Node, RXGK_TAG_SRCM);
    }
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_SourceModeSet_CreateNewModeInfo(_In_ const D3DKMDT_HVIDPNSOURCEMODESET hSet, _Outptr_ const D3DKMDT_VIDPN_SOURCE_MODE** ppNew)
{
    if (!ppNew || !hSet)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_SOURCE_MODE_NODE Node = (PRXGK_VIDPN_SOURCE_MODE_NODE)ExAllocatePoolWithTag(NonPagedPool, sizeof(RXGK_VIDPN_SOURCE_MODE_NODE), RXGK_TAG_SRCM);
    if (!Node)
        return STATUS_NO_MEMORY;
    RtlZeroMemory(Node, sizeof(*Node));
    Node->AddedToSet = FALSE;
    Node->Signature = 'SRCS';
    // Provide a default sane mode type
    Node->Mode.Id = 0; // will be assigned on AddMode
    *ppNew = &Node->Mode;
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_SourceModeSet_AddMode(_In_ D3DKMDT_HVIDPNSOURCEMODESET hSet, _In_ D3DKMDT_VIDPN_SOURCE_MODE* pMode)
{
    if (!hSet || !pMode)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_SOURCE_MODE_SET Set = (PRXGK_VIDPN_SOURCE_MODE_SET)hSet;
    PRXGK_VIDPN_SOURCE_MODE_NODE Node = CONTAINING_RECORD(pMode, RXGK_VIDPN_SOURCE_MODE_NODE, Mode);
    if (Node->Signature != 'SRCS')
    {
        // Copy from foreign buffer
        Node = (PRXGK_VIDPN_SOURCE_MODE_NODE)ExAllocatePoolWithTag(NonPagedPool, sizeof(RXGK_VIDPN_SOURCE_MODE_NODE), RXGK_TAG_SRCM);
        if (!Node)
            return STATUS_NO_MEMORY;
        RtlZeroMemory(Node, sizeof(*Node));
        Node->Mode = *pMode;
        Node->Signature = 'SRCS';
    }
    if (Node->Mode.Id == 0)
        Node->Mode.Id = (D3DKMDT_VIDEO_PRESENT_SOURCE_MODE_ID)Set->NextModeId++;
    Node->AddedToSet = TRUE;
    InsertTailList(&Set->ModeList, &Node->Link);
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_SourceModeSet_PinMode(_In_ D3DKMDT_HVIDPNSOURCEMODESET hSet, _In_ const D3DKMDT_VIDEO_PRESENT_SOURCE_MODE_ID VidPnSourceModeId)
{
    if (!hSet)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_SOURCE_MODE_SET Set = (PRXGK_VIDPN_SOURCE_MODE_SET)hSet;
    Set->PinnedId = VidPnSourceModeId;
    return STATUS_SUCCESS;
}

// ========================= TargetModeSet methods =========================

static NTSTATUS APIENTRY Rxgk_TargetModeSet_GetNumModes(_In_ const D3DKMDT_HVIDPNTARGETMODESET hSet, _Out_ SIZE_T* const pNumTargetModes)
{
    if (!pNumTargetModes || !hSet)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_TARGET_MODE_SET Set = (PRXGK_VIDPN_TARGET_MODE_SET)hSet;
    *pNumTargetModes = RxgkCountList(&Set->ModeList);
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_TargetModeSet_AcquireFirstModeInfo(_In_ const D3DKMDT_HVIDPNTARGETMODESET hSet, _Outptr_ const D3DKMDT_VIDPN_TARGET_MODE** ppFirst)
{
    if (!ppFirst || !hSet)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_TARGET_MODE_SET Set = (PRXGK_VIDPN_TARGET_MODE_SET)hSet;
    if (IsListEmpty(&Set->ModeList))
        return STATUS_NOT_FOUND;
    PRXGK_VIDPN_TARGET_MODE_NODE Node = CONTAINING_RECORD(Set->ModeList.Flink, RXGK_VIDPN_TARGET_MODE_NODE, Link);
    *ppFirst = &Node->Mode;
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_TargetModeSet_AcquireNextModeInfo(_In_ const D3DKMDT_HVIDPNTARGETMODESET hSet, _In_ const D3DKMDT_VIDPN_TARGET_MODE* pCurrent, _Outptr_ const D3DKMDT_VIDPN_TARGET_MODE** ppNext)
{
    UNREFERENCED_PARAMETER(hSet);
    if (!ppNext || !pCurrent)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_TARGET_MODE_NODE Cur = CONTAINING_RECORD(pCurrent, RXGK_VIDPN_TARGET_MODE_NODE, Mode);
    if (Cur->Link.Flink == NULL || Cur->Link.Flink == Cur->Link.Blink)
        return STATUS_NOT_FOUND;
    if (Cur->Link.Flink == &Cur->Link)
        return STATUS_NOT_FOUND;
    PRXGK_VIDPN_TARGET_MODE_NODE Next = CONTAINING_RECORD(Cur->Link.Flink, RXGK_VIDPN_TARGET_MODE_NODE, Link);
    *ppNext = &Next->Mode;
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_TargetModeSet_AcquirePinnedModeInfo(_In_ const D3DKMDT_HVIDPNTARGETMODESET hSet, _Outptr_ const D3DKMDT_VIDPN_TARGET_MODE** ppPinned)
{
    if (!ppPinned || !hSet)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_TARGET_MODE_SET Set = (PRXGK_VIDPN_TARGET_MODE_SET)hSet;
    if (Set->PinnedId == 0)
        return STATUS_NOT_FOUND;
    for (PLIST_ENTRY e = Set->ModeList.Flink; e != &Set->ModeList; e = e->Flink)
    {
        PRXGK_VIDPN_TARGET_MODE_NODE Node = CONTAINING_RECORD(e, RXGK_VIDPN_TARGET_MODE_NODE, Link);
        if (Node->Mode.Id == Set->PinnedId)
        {
            *ppPinned = &Node->Mode;
            return STATUS_SUCCESS;
        }
    }
    return STATUS_NOT_FOUND;
}

static NTSTATUS APIENTRY Rxgk_TargetModeSet_ReleaseModeInfo(_In_ const D3DKMDT_HVIDPNTARGETMODESET hSet, _In_ const D3DKMDT_VIDPN_TARGET_MODE* pMode)
{
    UNREFERENCED_PARAMETER(hSet);
    if (!pMode)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_TARGET_MODE_NODE Node = CONTAINING_RECORD(pMode, RXGK_VIDPN_TARGET_MODE_NODE, Mode);
    if (!Node->AddedToSet && Node->Signature == 'TSTS')
    {
        ExFreePoolWithTag(Node, RXGK_TAG_TGTM);
    }
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_TargetModeSet_CreateNewModeInfo(_In_ const D3DKMDT_HVIDPNTARGETMODESET hSet, _Outptr_ const D3DKMDT_VIDPN_TARGET_MODE** ppNew)
{
    if (!ppNew || !hSet)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_TARGET_MODE_NODE Node = (PRXGK_VIDPN_TARGET_MODE_NODE)ExAllocatePoolWithTag(NonPagedPool, sizeof(RXGK_VIDPN_TARGET_MODE_NODE), RXGK_TAG_TGTM);
    if (!Node)
        return STATUS_NO_MEMORY;
    RtlZeroMemory(Node, sizeof(*Node));
    Node->AddedToSet = FALSE;
    Node->Signature = 'TSTS';
    Node->Mode.Id = 0; // to be assigned on Add
    *ppNew = &Node->Mode;
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_TargetModeSet_AddMode(_In_ D3DKMDT_HVIDPNTARGETMODESET hSet, _In_ D3DKMDT_VIDPN_TARGET_MODE* pMode)
{
    if (!hSet || !pMode)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_TARGET_MODE_SET Set = (PRXGK_VIDPN_TARGET_MODE_SET)hSet;
    PRXGK_VIDPN_TARGET_MODE_NODE Node = CONTAINING_RECORD(pMode, RXGK_VIDPN_TARGET_MODE_NODE, Mode);
    if (Node->Signature != 'TSTS')
    {
        Node = (PRXGK_VIDPN_TARGET_MODE_NODE)ExAllocatePoolWithTag(NonPagedPool, sizeof(RXGK_VIDPN_TARGET_MODE_NODE), RXGK_TAG_TGTM);
        if (!Node)
            return STATUS_NO_MEMORY;
        RtlZeroMemory(Node, sizeof(*Node));
        Node->Mode = *pMode;
        Node->Signature = 'TSTS';
    }
    if (Node->Mode.Id == 0)
        Node->Mode.Id = (D3DKMDT_VIDEO_PRESENT_TARGET_MODE_ID)Set->NextModeId++;
    Node->AddedToSet = TRUE;
    InsertTailList(&Set->ModeList, &Node->Link);
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_TargetModeSet_PinMode(_In_ D3DKMDT_HVIDPNTARGETMODESET hSet, _In_ const D3DKMDT_VIDEO_PRESENT_TARGET_MODE_ID VidPnTargetModeId)
{
    if (!hSet)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_TARGET_MODE_SET Set = (PRXGK_VIDPN_TARGET_MODE_SET)hSet;
    Set->PinnedId = VidPnTargetModeId;
    return STATUS_SUCCESS;
}

// ========================= Topology methods =========================

static NTSTATUS APIENTRY Rxgk_Topology_GetNumPaths(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _Out_ PSIZE_T pNumPaths)
{
    if (!pNumPaths || !hTopology)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_TOPOLOGY Topo = RxgkFromTopologyHandle(hTopology);
    *pNumPaths = (SIZE_T)RxgkCountList(&Topo->PathList);
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_Topology_GetNumPathsFromSource(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId, _Out_ PSIZE_T pNumPathsFromSource)
{
    if (!pNumPathsFromSource || !hTopology)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_TOPOLOGY Topo = RxgkFromTopologyHandle(hTopology);
    SIZE_T n = 0;
    for (PLIST_ENTRY e = Topo->PathList.Flink; e != &Topo->PathList; e = e->Flink)
    {
        PRXGK_VIDPN_PRESENT_PATH_NODE Node = CONTAINING_RECORD(e, RXGK_VIDPN_PRESENT_PATH_NODE, Link);
        if (Node->Path.VidPnSourceId == VidPnSourceId)
            ++n;
    }
    *pNumPathsFromSource = n;
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_Topology_EnumPathTargetsFromSource(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId, _In_ const D3DKMDT_VIDPN_PRESENT_PATH_INDEX VidPnPresentPathIndex, _Out_ D3DDDI_VIDEO_PRESENT_TARGET_ID* pVidPnTargetId)
{
    if (!pVidPnTargetId || !hTopology)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_TOPOLOGY Topo = RxgkFromTopologyHandle(hTopology);
    SIZE_T idx = 0;
    for (PLIST_ENTRY e = Topo->PathList.Flink; e != &Topo->PathList; e = e->Flink)
    {
        PRXGK_VIDPN_PRESENT_PATH_NODE Node = CONTAINING_RECORD(e, RXGK_VIDPN_PRESENT_PATH_NODE, Link);
        if (Node->Path.VidPnSourceId == VidPnSourceId)
        {
            if (idx == VidPnPresentPathIndex)
            {
                *pVidPnTargetId = Node->Path.VidPnTargetId;
                return STATUS_SUCCESS;
            }
            ++idx;
        }
    }
    return STATUS_NOT_FOUND;
}

static NTSTATUS APIENTRY Rxgk_Topology_GetPathSourceFromTarget(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId, _Out_ D3DDDI_VIDEO_PRESENT_SOURCE_ID* pVidPnSourceId)
{
    if (!pVidPnSourceId || !hTopology)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_TOPOLOGY Topo = RxgkFromTopologyHandle(hTopology);
    for (PLIST_ENTRY e = Topo->PathList.Flink; e != &Topo->PathList; e = e->Flink)
    {
        PRXGK_VIDPN_PRESENT_PATH_NODE Node = CONTAINING_RECORD(e, RXGK_VIDPN_PRESENT_PATH_NODE, Link);
        if (Node->Path.VidPnTargetId == VidPnTargetId)
        {
            *pVidPnSourceId = Node->Path.VidPnSourceId;
            return STATUS_SUCCESS;
        }
    }
    return STATUS_NOT_FOUND;
}

static NTSTATUS APIENTRY Rxgk_Topology_AcquirePathInfo(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId, _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId, _Outptr_ const D3DKMDT_VIDPN_PRESENT_PATH** ppPathInfo)
{
    if (!ppPathInfo || !hTopology)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_TOPOLOGY Topo = (PRXGK_VIDPN_TOPOLOGY)hTopology;
    for (PLIST_ENTRY e = Topo->PathList.Flink; e != &Topo->PathList; e = e->Flink)
    {
        PRXGK_VIDPN_PRESENT_PATH_NODE Node = CONTAINING_RECORD(e, RXGK_VIDPN_PRESENT_PATH_NODE, Link);
        if (Node->Path.VidPnSourceId == VidPnSourceId && Node->Path.VidPnTargetId == VidPnTargetId)
        {
            *ppPathInfo = &Node->Path;
            return STATUS_SUCCESS;
        }
    }
    return STATUS_NOT_FOUND;
}

static NTSTATUS APIENTRY Rxgk_Topology_AcquireFirstPathInfo(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _Outptr_ const D3DKMDT_VIDPN_PRESENT_PATH** ppFirstPathInfo)
{
    if (!ppFirstPathInfo || !hTopology)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_TOPOLOGY Topo = (PRXGK_VIDPN_TOPOLOGY)hTopology;
    if (IsListEmpty(&Topo->PathList))
        return STATUS_NOT_FOUND;
    PRXGK_VIDPN_PRESENT_PATH_NODE Node = CONTAINING_RECORD(Topo->PathList.Flink, RXGK_VIDPN_PRESENT_PATH_NODE, Link);
    *ppFirstPathInfo = &Node->Path;
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_Topology_AcquireNextPathInfo(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _In_ const D3DKMDT_VIDPN_PRESENT_PATH* pCurrent, _Outptr_ const D3DKMDT_VIDPN_PRESENT_PATH** ppNext)
{
    UNREFERENCED_PARAMETER(hTopology);
    if (!ppNext || !pCurrent)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_PRESENT_PATH_NODE Cur = CONTAINING_RECORD(pCurrent, RXGK_VIDPN_PRESENT_PATH_NODE, Path);
    if (Cur->Link.Flink == NULL || Cur->Link.Flink == Cur->Link.Blink)
        return STATUS_NOT_FOUND;
    if (Cur->Link.Flink == &Cur->Link)
        return STATUS_NOT_FOUND;
    PRXGK_VIDPN_PRESENT_PATH_NODE Next = CONTAINING_RECORD(Cur->Link.Flink, RXGK_VIDPN_PRESENT_PATH_NODE, Link);
    *ppNext = &Next->Path;
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_Topology_UpdatePathSupportInfo(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _In_ const D3DKMDT_VIDPN_PRESENT_PATH* pPathInfo)
{
    UNREFERENCED_PARAMETER(hTopology);
    UNREFERENCED_PARAMETER(pPathInfo);
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_Topology_ReleasePathInfo(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _In_ const D3DKMDT_VIDPN_PRESENT_PATH* pPathInfo)
{
    UNREFERENCED_PARAMETER(hTopology);
    if (!pPathInfo)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_PRESENT_PATH_NODE Node = CONTAINING_RECORD(pPathInfo, RXGK_VIDPN_PRESENT_PATH_NODE, Path);
    if (!Node->InTopology && Node->Signature == 'PATH')
    {
        ExFreePoolWithTag(Node, RXGK_TAG_PATH);
    }
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_Topology_CreateNewPathInfo(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _Outptr_ D3DKMDT_VIDPN_PRESENT_PATH** ppNewPathInfo)
{
    if (!ppNewPathInfo || !hTopology)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_PRESENT_PATH_NODE Node = (PRXGK_VIDPN_PRESENT_PATH_NODE)ExAllocatePoolWithTag(NonPagedPool, sizeof(RXGK_VIDPN_PRESENT_PATH_NODE), RXGK_TAG_PATH);
    if (!Node)
        return STATUS_NO_MEMORY;
    RtlZeroMemory(Node, sizeof(*Node));
    Node->InTopology = FALSE;
    Node->Signature = 'PATH';
    *ppNewPathInfo = &Node->Path;
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_Topology_AddPath(_In_ D3DKMDT_HVIDPNTOPOLOGY hTopology, _In_ D3DKMDT_VIDPN_PRESENT_PATH* pPathInfo)
{
    if (!hTopology || !pPathInfo)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_TOPOLOGY Topo = (PRXGK_VIDPN_TOPOLOGY)hTopology;
    PRXGK_VIDPN_PRESENT_PATH_NODE Node = CONTAINING_RECORD(pPathInfo, RXGK_VIDPN_PRESENT_PATH_NODE, Path);
    if (Node->Signature != 'PATH')
    {
        Node = (PRXGK_VIDPN_PRESENT_PATH_NODE)ExAllocatePoolWithTag(NonPagedPool, sizeof(RXGK_VIDPN_PRESENT_PATH_NODE), RXGK_TAG_PATH);
        if (!Node)
            return STATUS_NO_MEMORY;
        RtlZeroMemory(Node, sizeof(*Node));
        Node->Path = *pPathInfo;
        Node->Signature = 'PATH';
    }
    Node->InTopology = TRUE;
    InsertTailList(&Topo->PathList, &Node->Link);
    return STATUS_SUCCESS;
}

static NTSTATUS APIENTRY Rxgk_Topology_RemovePath(_In_ const D3DKMDT_HVIDPNTOPOLOGY hTopology, _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId, _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId)
{
    if (!hTopology)
        return STATUS_INVALID_PARAMETER;
    PRXGK_VIDPN_TOPOLOGY Topo = (PRXGK_VIDPN_TOPOLOGY)hTopology;
    for (PLIST_ENTRY e = Topo->PathList.Flink; e != &Topo->PathList; e = e->Flink)
    {
        PRXGK_VIDPN_PRESENT_PATH_NODE Node = CONTAINING_RECORD(e, RXGK_VIDPN_PRESENT_PATH_NODE, Link);
        if (Node->Path.VidPnSourceId == VidPnSourceId && Node->Path.VidPnTargetId == VidPnTargetId)
        {
            RemoveEntryList(&Node->Link);
            ExFreePoolWithTag(Node, RXGK_TAG_PATH);
            return STATUS_SUCCESS;
        }
    }
    return STATUS_NOT_FOUND;
}