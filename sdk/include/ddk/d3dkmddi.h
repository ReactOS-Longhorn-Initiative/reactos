/*
 * PROJECT:     ReactOS Display Driver Model
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Header file for WDDM style DDIs
 * COPYRIGHT:   Copyright 2024 Justin Miller <justin.miller@reactos.org>
 */

#ifndef _D3DKMDDI_H_
#define _D3DKMDDI_H_

#include <d3dkmdt.h>

typedef enum _DXGK_HANDLE_TYPE
{
    DXGK_HANDLE_ALLOCATION  = 1,
    DXGK_HANDLE_RESOURCE    = 2,
} DXGK_HANDLE_TYPE;

typedef enum _DXGK_INTERRUPT_TYPE
{
    DXGK_INTERRUPT_DMA_COMPLETED                = 1,
    DXGK_INTERRUPT_DMA_PREEMPTED                = 2,
    DXGK_INTERRUPT_CRTC_VSYNC                   = 3,
    DXGK_INTERRUPT_DMA_FAULTED                  = 4,

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    DXGK_INTERRUPT_DISPLAYONLY_VSYNC            = 5,
    DXGK_INTERRUPT_DISPLAYONLY_PRESENT_PROGRESS = 6,
    DXGK_INTERRUPT_CRTC_VSYNC_WITH_MULTIPLANE_OVERLAY = 7,
#endif // DXGKDDI_INTERFACE_VERSION

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    DXGK_INTERRUPT_MICACAST_CHUNK_PROCESSING_COMPLETE = 8,
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    DXGK_INTERRUPT_DMA_PAGE_FAULTED = 9,
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
    DXGK_INTERRUPT_CRTC_VSYNC_WITH_MULTIPLANE_OVERLAY2 = 10,
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
    DXGK_INTERRUPT_MONITORED_FENCE_SIGNALED = 11,
    DXGK_INTERRUPT_HWQUEUE_PAGE_FAULTED = 12,
    DXGK_INTERRUPT_HWCONTEXTLIST_SWITCH_COMPLETED = 13,
    DXGK_INTERRUPT_PERIODIC_MONITORED_FENCE_SIGNALED = 14,
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
    DXGK_INTERRUPT_SCHEDULING_LOG_INTERRUPT = 15,
    DXGK_INTERRUPT_GPU_ENGINE_TIMEOUT = 16,
    DXGK_INTERRUPT_SUSPEND_CONTEXT_COMPLETED = 17,
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
    DXGK_INTERRUPT_CRTC_VSYNC_WITH_MULTIPLANE_OVERLAY3 = 18,
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_1)
    DXGK_INTERRUPT_NATIVE_FENCE_SIGNALED = 19,
    DXGK_INTERRUPT_GPU_ENGINE_STATE_CHANGE = 20
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_1)

} DXGK_INTERRUPT_TYPE;

typedef enum _DXGK_MONITOR_INTERFACE_VERSION
{
    DXGK_MONITOR_INTERFACE_VERSION_UNINITIALIZED = 0,
    DXGK_MONITOR_INTERFACE_VERSION_V1            = 1,
    DXGK_MONITOR_INTERFACE_VERSION_V2            = 2,
} DXGK_MONITOR_INTERFACE_VERSION;

typedef enum _DXGK_VIDPN_INTERFACE_VERSION
{
    DXGK_VIDPN_INTERFACE_VERSION_UNINITIALIZED = 0,
    DXGK_VIDPN_INTERFACE_VERSION_V1            = 1,

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
    DXGK_VIDPN_INTERFACE_VERSION_V2            = 2,
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
} DXGK_VIDPN_INTERFACE_VERSION;

typedef enum _DXGK_ENGINE_STATE
{
    DXGK_ENGINE_STATE_ACTIVE = 0,
    DXGK_ENGINE_STATE_IDLE = 1,
    DXGK_ENGINE_STATE_HUNG = 2
} DXGK_ENGINE_STATE;

typedef enum _DXGK_QUERYADAPTERINFOTYPE
{
    DXGKQAITYPE_UMDRIVERPRIVATE           = 0,
    DXGKQAITYPE_DRIVERCAPS                = 1,
    DXGKQAITYPE_QUERYSEGMENT              = 2,
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7)
    DXGKQAITYPE_RESERVED                  = 3,
    DXGKQAITYPE_QUERYSEGMENT2             = 4, 
#endif // DXGKDDI_INTERFACE_VERSION_WIN7
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    DXGKQAITYPE_QUERYSEGMENT3             = 5,
    DXGKQAITYPE_NUMPOWERCOMPONENTS        = 6,
    DXGKQAITYPE_POWERCOMPONENTINFO        = 7,
    DXGKQAITYPE_PREFERREDGPUNODE          = 8,
#endif // DXGKDDI_INTERFACE_VERSION_WIN8
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    DXGKQAITYPE_POWERCOMPONENTPSTATEINFO  = 9,
    DXGKQAITYPE_HISTORYBUFFERPRECISION    = 10,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM1_3
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    DXGKQAITYPE_QUERYSEGMENT4             = 11,
    DXGKQAITYPE_SEGMENTMEMORYSTATE        = 12,
    DXGKQAITYPE_GPUMMUCAPS                = 13,
    DXGKQAITYPE_PAGETABLELEVELDESC        = 14,
    DXGKQAITYPE_PHYSICALADAPTERCAPS       = 15,
    DXGKQAITYPE_DISPLAY_DRIVERCAPS_EXTENSION    = 16,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_0
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
    DXGKQAITYPE_INTEGRATED_DISPLAY_DESCRIPTOR   = 17,
    DXGKQAITYPE_UEFIFRAMEBUFFERRANGES           = 18,
    DXGKQAITYPE_QUERYCOLORIMETRYOVERRIDES       = 19,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_2
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_3)
    DXGKQAITYPE_DISPLAYID_DESCRIPTOR   = 20,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_3
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
    DXGKQAITYPE_FRAMEBUFFERSAVESIZE    = 21,
    DXGKQAITYPE_HARDWARERESERVEDRANGES = 22,
    DXGKQAITYPE_INTEGRATED_DISPLAY_DESCRIPTOR2  = 23,
    DXGKQAITYPE_NODEPERFDATA            = 24,
    DXGKQAITYPE_ADAPTERPERFDATA         = 25,
    DXGKQAITYPE_ADAPTERPERFDATA_CAPS    = 26,
    DXGKQAITYPE_GPUVERSION              = 27,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_4
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_5)
    DXGKQAITYPE_DEVICE_TYPE_CAPS        = 28,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_5
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_6)
    DXGKQAITYPE_WDDMDEVICECAPS          = 29,
    DXGKQAITYPE_GPUPCAPS                = 30,
    DXGKQAITYPE_QUERYTARGETGAMMACAPS    = 31,
    DXGKQAITYPE_SCANOUT_CAPS            = 33,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_6
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
    DXGKQAITYPE_PHYSICAL_MEMORY_CAPS   = 34,
    DXGKQAITYPE_IOMMU_CAPS             = 35,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM2_9
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_1)
    DXGKQAITYPE_HARDWARERESERVEDRANGES2 = 36,
    DXGKQAITYPE_NATIVE_FENCE_CAPS       = 37,
    DXGKQAITYPE_USERMODESUBMISSION_CAPS = 38,
#endif // DXGKDDI_INTERFACE_VERSION_WDDM3_1

} DXGK_QUERYADAPTERINFOTYPE;

typedef struct _DXGKCB_GETHANDLEDATAFLAGS
{
    union
    {
        struct
        {
            UINT                DeviceSpecific  : 1;
            UINT                Reserved        :31;
        };
        UINT Value;
    };
} DXGKCB_GETHANDLEDATAFLAGS;

typedef struct _DXGK_ALLOCATIONINFOFLAGS_WDDM2_0
{
    union
    {
        struct
        {
            UINT    CpuVisible                      : 1;
            UINT    PermanentSysMem                 : 1;
            UINT    Cached                          : 1;
            UINT    Protected                       : 1;
            UINT    ExistingSysMem                  : 1;
            UINT    ExistingKernelSysMem            : 1;
            UINT    FromEndOfSegment                : 1;
            UINT    DisableLargePageMapping         : 1;
            UINT    Overlay                         : 1;
            UINT    Capture                         : 1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
            UINT    CreateInVpr                     : 1;
#else
            UINT    Reserved00                      : 1;
#endif
            UINT    DXGK_ALLOC_RESERVED17           : 1;
            UINT    Reserved02                      : 1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
            UINT    MapApertureCpuVisible           : 1;
#else
            UINT    Reserved03                      : 1;
#endif
            UINT    HistoryBuffer                   : 1;
            UINT    AccessedPhysically              : 1;
            UINT    ExplicitResidencyNotification   : 1;
            UINT    HardwareProtected               : 1;
            UINT    CpuVisibleOnDemand              : 1;
            UINT    DXGK_ALLOC_RESERVED16           : 1;
            UINT    DXGK_ALLOC_RESERVED15           : 1;
            UINT    DXGK_ALLOC_RESERVED14           : 1;
            UINT    DXGK_ALLOC_RESERVED13           : 1;
            UINT    DXGK_ALLOC_RESERVED12           : 1;
            UINT    DXGK_ALLOC_RESERVED11           : 1;
            UINT    DXGK_ALLOC_RESERVED10           : 1;
            UINT    DXGK_ALLOC_RESERVED9            : 1;
            UINT    DXGK_ALLOC_RESERVED4            : 1;
            UINT    DXGK_ALLOC_RESERVED3            : 1;
            UINT    DXGK_ALLOC_RESERVED2            : 1;
            UINT    DXGK_ALLOC_RESERVED1            : 1;
            UINT    DXGK_ALLOC_RESERVED0            : 1;
        };
        UINT Value;
    };
} DXGK_ALLOCATIONINFOFLAGS_WDDM2_0;

typedef struct _DXGK_SEGMENTPREFERENCE
{
    union
    {
        struct
        {
            UINT SegmentId0 : 5;
            UINT Direction0 : 1;
            UINT SegmentId1 : 5;
            UINT Direction1 : 1;
            UINT SegmentId2 : 5;
            UINT Direction2 : 1;
            UINT SegmentId3 : 5;
            UINT Direction3 : 1;
            UINT SegmentId4 : 5;
            UINT Direction4 : 1;
            UINT Reserved   : 2;
        };
        UINT Value;
    };
} DXGK_SEGMENTPREFERENCE, *PDXGK_SEGMENTPREFERENCE;


typedef struct _DXGK_SEGMENTBANKPREFERENCE
{
    union
    {
        struct
        {
            UINT Bank0          : 7;
            UINT Direction0     : 1;
            UINT Bank1          : 7;
            UINT Direction1     : 1;
            UINT Bank2          : 7;
            UINT Direction2     : 1;
            UINT Bank3          : 7;
            UINT Direction3     : 1;
        };
        UINT Value;
    };
} DXGK_SEGMENTBANKPREFERENCE;
typedef struct _DXGK_ALLOCATIONINFOFLAGS
{
    union
    {
        struct
        {
            UINT    CpuVisible              : 1;
            UINT    PermanentSysMem         : 1;
            UINT    Cached                  : 1;
            UINT    Protected               : 1;
            UINT    ExistingSysMem          : 1;
            UINT    ExistingKernelSysMem    : 1;
            UINT    FromEndOfSegment        : 1;
            UINT    Swizzled                : 1;
            UINT    Overlay                 : 1;
            UINT    Capture                 : 1;
            UINT    UseAlternateVA          : 1;
            UINT    SynchronousPaging       : 1;
            UINT    LinkMirrored            : 1;
            UINT    LinkInstanced           : 1;
            UINT    HistoryBuffer           : 1;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
            UINT    AccessedPhysically      : 1;
            UINT    ExplicitResidencyNotification : 1;
            UINT    HardwareProtected       : 1;
            UINT    CpuVisibleOnDemand      : 1;
#else
            UINT    Reserved                : 4;
#endif
            UINT    DXGK_ALLOC_RESERVED16   : 1;
            UINT    DXGK_ALLOC_RESERVED15   : 1;
            UINT    DXGK_ALLOC_RESERVED14   : 1;
            UINT    DXGK_ALLOC_RESERVED13   : 1;
            UINT    DXGK_ALLOC_RESERVED12   : 1;
            UINT    DXGK_ALLOC_RESERVED11   : 1;
            UINT    DXGK_ALLOC_RESERVED10   : 1;
            UINT    DXGK_ALLOC_RESERVED9    : 1;
            UINT    DXGK_ALLOC_RESERVED4    : 1;
            UINT    DXGK_ALLOC_RESERVED3    : 1;
            UINT    DXGK_ALLOC_RESERVED2    : 1;
            UINT    DXGK_ALLOC_RESERVED1    : 1;
            UINT    DXGK_ALLOC_RESERVED0    : 1;
        };
        UINT Value;
    };
} DXGK_ALLOCATIONINFOFLAGS;
typedef struct _DXGK_ALLOCATIONUSAGEINFO1
{
    union
    {
        struct
        {
            UINT        PrivateFormat  : 1;
            UINT        Swizzled       : 1;
            UINT        MipMap         : 1;
            UINT        Cube           : 1;
            UINT        Volume         : 1;
            UINT        Vertex         : 1;
            UINT        Index          : 1;
            UINT        Reserved       : 25;
        };
        UINT            Value;
    } Flags;
    union
    {
        D3DDDIFORMAT    Format;
        UINT            PrivateFormat;
    };
    UINT                SwizzledFormat;
    UINT                ByteOffset;
    UINT                Width;
    UINT                Height;
    UINT                Pitch;
    UINT                Depth;
    UINT                SlicePitch;
} DXGK_ALLOCATIONUSAGEINFO1;
typedef struct _DXGK_ALLOCATIONUSAGEHINT
{
    UINT                            Version;
    DXGK_ALLOCATIONUSAGEINFO1       v1;
} DXGK_ALLOCATIONUSAGEHINT;
typedef struct _DXGK_ALLOCATIONINFO
{
    VOID*                             pPrivateDriverData;
    UINT                              PrivateDriverDataSize;
    UINT                              Alignment;
    SIZE_T                            Size;
    SIZE_T                            PitchAlignedSize;
    DXGK_SEGMENTBANKPREFERENCE        HintedBank;
    DXGK_SEGMENTPREFERENCE            PreferredSegment;
    UINT                              SupportedReadSegmentSet;
    UINT                              SupportedWriteSegmentSet;
    UINT                              EvictionSegmentSet;
    union
    {
        UINT                          MaximumRenamingListLength;
        UINT                          PhysicalAdapterIndex;
    };
    HANDLE                            hAllocation;
    union
    {
        DXGK_ALLOCATIONINFOFLAGS            Flags;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
        DXGK_ALLOCATIONINFOFLAGS_WDDM2_0    FlagsWddm2;
#endif
    };
    DXGK_ALLOCATIONUSAGEHINT*         pAllocationUsageHint;
    UINT                              AllocationPriority;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_0)
    DXGK_ALLOCATIONINFOFLAGS2         Flags2;
#endif
} DXGK_ALLOCATIONINFO;

typedef struct _DXGKCB_NOTIFY_INTERRUPT_DATA_FLAGS
{
    union
    {
        struct
        {
            UINT            ValidPhysicalAdapterMask : 1; 
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
            UINT            HsyncFlipCompletion      : 1; 
            UINT            Reserved                 :30; 
#else
            UINT            Reserved                 :31;  
#endif
        };
        UINT                Value;
    };
} DXGKCB_NOTIFY_INTERRUPT_DATA_FLAGS;

typedef struct _DXGKARGCB_NOTIFY_INTERRUPT_DATA
{
    DXGK_INTERRUPT_TYPE  InterruptType;
    union
    {
        struct
        {
            UINT             SubmissionFenceId;
            UINT             NodeOrdinal;
            UINT             EngineOrdinal;
        } DmaCompleted;
        struct
        {
            UINT             PreemptionFenceId;    
            UINT             LastCompletedFenceId; 
            UINT             NodeOrdinal;          
            UINT             EngineOrdinal;        
        } DmaPreempted;
        struct
        {
            UINT             FaultedFenceId;     
            NTSTATUS         Status;             
            UINT             NodeOrdinal;        
            UINT             EngineOrdinal;      
        } DmaFaulted;
        struct
        {
            D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId;       
            PHYSICAL_ADDRESS               PhysicalAddress;     
            UINT                           PhysicalAdapterMask; 
                                                                
        } CrtcVsync;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
        struct
        {
            D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId;    
        } DisplayOnlyVsync;
        struct
        {
            D3DDDI_VIDEO_PRESENT_TARGET_ID     VidPnTargetId;
            UINT                               PhysicalAdapterMask;
            UINT                               MultiPlaneOverlayVsyncInfoCount;
            DXGK_MULTIPLANE_OVERLAY_VSYNC_INFO *pMultiPlaneOverlayVsyncInfo;
        } CrtcVsyncWithMultiPlaneOverlay;

        DXGKARGCB_PRESENT_DISPLAYONLY_PROGRESS DisplayOnlyPresentProgress;
#endif // DXGKDDI_INTERFACE_VERSION

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
        struct
        {
            D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId;
            DXGK_MIRACAST_CHUNK_INFO       ChunkInfo;
            PVOID                          pPrivateDriverData;
            UINT                           PrivateDataDriverSize;
            NTSTATUS                       Status;
        } MiracastEncodeChunkCompleted;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
        struct
        {
            UINT                        FaultedFenceId;        
                                                               
            UINT64                      FaultedPrimitiveAPISequenceNumber; 
            DXGK_RENDER_PIPELINE_STAGE  FaultedPipelineStage;  
            UINT                        FaultedBindTableEntry; 
            DXGK_PAGE_FAULT_FLAGS       PageFaultFlags;       
            D3DGPU_VIRTUAL_ADDRESS      FaultedVirtualAddress; 
            UINT                        NodeOrdinal;          
            UINT                        EngineOrdinal;        
            UINT                        PageTableLevel;        
            DXGK_FAULT_ERROR_CODE       FaultErrorCode;       
            HANDLE                      FaultedProcessHandle;  
                                                            
        } DmaPageFaulted;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)
        struct
        {
            D3DDDI_VIDEO_PRESENT_TARGET_ID                VidPnTargetId;
            UINT                                          PhysicalAdapterMask;
            UINT                                          MultiPlaneOverlayVsyncInfoCount;
            _Field_size_(MultiPlaneOverlayVsyncInfoCount) DXGK_MULTIPLANE_OVERLAY_VSYNC_INFO2 *pMultiPlaneOverlayVsyncInfo;
            ULONGLONG                                     GpuFrequency;
            ULONGLONG                                     GpuClockCounter;
        } CrtcVsyncWithMultiPlaneOverlay2;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_1)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
        struct
        {
            UINT    NodeOrdinal;          
            UINT    EngineOrdinal;        
        } MonitoredFenceSignaled;

        struct
        {
            UINT    NodeOrdinal;            
            UINT    EngineOrdinal;          
            UINT64  ContextSwitchFence;     
        } HwContextListSwitchCompleted;

        struct
        {
            UINT64                      FaultedFenceId;       
                                                              
            D3DGPU_VIRTUAL_ADDRESS      FaultedVirtualAddress;
            UINT64                      FaultedPrimitiveAPISequenceNumber; 

            union
            {
                HANDLE                  FaultedHwQueue;         
                                                                
                HANDLE                  FaultedHwContext;       
                                                                
                HANDLE                  FaultedProcessHandle;   
                                                                
            };

            UINT                        NodeOrdinal;        
            UINT                        EngineOrdinal;      

            DXGK_RENDER_PIPELINE_STAGE  FaultedPipelineStage;  
            UINT                        FaultedBindTableEntry; 
            DXGK_PAGE_FAULT_FLAGS       PageFaultFlags;         
            UINT                        PageTableLevel;         
            DXGK_FAULT_ERROR_CODE       FaultErrorCode;         
        } HwQueuePageFaulted;

        struct
        {
            D3DDDI_VIDEO_PRESENT_TARGET_ID    VidPnTargetId;    
            UINT                              NotificationID;   
        } PeriodicMonitoredFenceSignaled;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
        struct
        {
            UINT    NodeOrdinal;             
            UINT    EngineOrdinal;           
        } SchedulingLogInterrupt;

        struct
        {
            UINT    NodeOrdinal;             
            UINT    EngineOrdinal;           
        } GpuEngineTimeout;

        struct
        {
            HANDLE  hContext;                
            UINT64  ContextSuspendFence;     
        } SuspendContextCompleted;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)
        struct
        {
            D3DDDI_VIDEO_PRESENT_TARGET_ID                VidPnTargetId;
            UINT                                          PhysicalAdapterMask;
            UINT                                          MultiPlaneOverlayVsyncInfoCount;
            _Field_size_(MultiPlaneOverlayVsyncInfoCount) DXGK_MULTIPLANE_OVERLAY_VSYNC_INFO3 *pMultiPlaneOverlayVsyncInfo;
            ULONGLONG                                     GpuFrequency;
            ULONGLONG                                     GpuClockCounter;
        } CrtcVsyncWithMultiPlaneOverlay3;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_9)

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_1)
        struct
        {
            UINT    NodeOrdinal;                
            UINT    EngineOrdinal;              

                                                 
                                                 
            UINT    SignaledNativeFenceCount;    
            _Field_size_(SignaledNativeFenceCount)
            HANDLE* pSignaledNativeFenceArray;   
        } NativeFenceSignaled;

        struct
        {
            UINT              NodeOrdinal;    
            UINT              EngineOrdinal;  
            DXGK_ENGINE_STATE NewState;
        } EngineStateChange;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM3_1)

        struct
        {
            UINT            Reserved[16];
        } Reserved;
    };
    DXGKCB_NOTIFY_INTERRUPT_DATA_FLAGS Flags;     
} DXGKARGCB_NOTIFY_INTERRUPT_DATA ,*PDXGKARGCB_NOTIFY_INTERRUPT_DATA;

typedef struct _DXGKARGCB_GETHANDLEDATA
{
    D3DKMT_HANDLE           hObject;
    DXGK_HANDLE_TYPE        Type;
    DXGKCB_GETHANDLEDATAFLAGS Flags;
} DXGKARGCB_GETHANDLEDATA , *PDXGKARGCB_GETHANDLEDATA;

typedef struct _DXGKARGCB_GETCAPTUREADDRESS
{
    D3DKMT_HANDLE      hAllocation;          
    UINT               SegmentId;            
    PHYSICAL_ADDRESS   PhysicalAddress;      
} DXGKARGCB_GETCAPTUREADDRESS, *PDXGKARGCB_GETCAPTUREADDRESS;

typedef struct _DXGKARGCB_ENUMHANDLECHILDREN
{
    D3DKMT_HANDLE   hObject;
    UINT            Index;
} DXGKARGCB_ENUMHANDLECHILDREN, *PDXGKARGCB_ENUMHANDLECHILDREN;

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNSOURCEMODESET_GETNUMMODES)(
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET          hVidPnSourceModeSet,
    _Out_ SIZE_T* const                           pNumSourceModes);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNSOURCEMODESET_ACQUIREFIRSTMODEINFO)(
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET         hVidPnSourceModeSet,
    _Outptr_ const D3DKMDT_VIDPN_SOURCE_MODE**     ppFirstVidPnSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNSOURCEMODESET_ACQUIRENEXTMODEINFO)(
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET          hVidPnSourceModeSet,
    _In_ const D3DKMDT_VIDPN_SOURCE_MODE*          pVidPnSourceModeInfo,
    _Outptr_ const D3DKMDT_VIDPN_SOURCE_MODE**     ppNextVidPnSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNSOURCEMODESET_ACQUIREPINNEDMODEINFO)(
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET        hVidPnSourceModeSet,
    _Outptr_ const D3DKMDT_VIDPN_SOURCE_MODE**    ppPinnedVidPnSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNSOURCEMODESET_RELEASEMODEINFO)(
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET          hVidPnSourceModeSet,
    _In_ const D3DKMDT_VIDPN_SOURCE_MODE*           pVidPnSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNSOURCEMODESET_CREATENEWMODEINFO)(
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET           hVidPnSourceModeSet,
    _Outptr_ const D3DKMDT_VIDPN_SOURCE_MODE**       ppNewVidPnSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNSOURCEMODESET_ADDMODE)(
    _In_ D3DKMDT_HVIDPNSOURCEMODESET          hVidPnSourceModeSet,
    _In_ D3DKMDT_VIDPN_SOURCE_MODE*           pVidPnSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNSOURCEMODESET_PINMODE)(
    _In_ D3DKMDT_HVIDPNSOURCEMODESET                   hVidPnSourceModeSet,
    _In_ const D3DKMDT_VIDEO_PRESENT_SOURCE_MODE_ID    VidPnSourceModeId);

typedef struct _DXGK_VIDPNSOURCEMODESET_INTERFACE
{
    DXGKDDI_VIDPNSOURCEMODESET_GETNUMMODES              pfnGetNumModes;
    DXGKDDI_VIDPNSOURCEMODESET_ACQUIREFIRSTMODEINFO     pfnAcquireFirstModeInfo;
    DXGKDDI_VIDPNSOURCEMODESET_ACQUIRENEXTMODEINFO      pfnAcquireNextModeInfo;
    DXGKDDI_VIDPNSOURCEMODESET_ACQUIREPINNEDMODEINFO    pfnAcquirePinnedModeInfo;
    DXGKDDI_VIDPNSOURCEMODESET_RELEASEMODEINFO          pfnReleaseModeInfo;
    DXGKDDI_VIDPNSOURCEMODESET_CREATENEWMODEINFO        pfnCreateNewModeInfo;
    DXGKDDI_VIDPNSOURCEMODESET_ADDMODE                  pfnAddMode;
    DXGKDDI_VIDPNSOURCEMODESET_PINMODE                  pfnPinMode;
} DXGK_VIDPNSOURCEMODESET_INTERFACE;

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTARGETMODESET_GETNUMMODES)(
    _In_ const D3DKMDT_HVIDPNTARGETMODESET           hVidPnTargetModeSet,
    _Out_ SIZE_T*   const                            pNumTargetModes);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTARGETMODESET_ACQUIREFIRSTMODEINFO)(
    _In_ const D3DKMDT_HVIDPNTARGETMODESET           hVidPnTargetModeSet,
    _Outptr_ const D3DKMDT_VIDPN_TARGET_MODE**       ppFirstVidPnTargetModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTARGETMODESET_ACQUIRENEXTMODEINFO)(
    _In_ const D3DKMDT_HVIDPNTARGETMODESET           hVidPnTargetModeSet,
    _In_ const D3DKMDT_VIDPN_TARGET_MODE*            pVidPnTargetModeInfo,
    _Outptr_ const D3DKMDT_VIDPN_TARGET_MODE**       ppNextVidPnTargetModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTARGETMODESET_ACQUIREPINNEDMODEINFO)(
    _In_ const D3DKMDT_HVIDPNTARGETMODESET           hVidPnTargetModeSet,
    _Outptr_ const D3DKMDT_VIDPN_TARGET_MODE**       ppPinnedVidPnTargetModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTARGETMODESET_RELEASEMODEINFO)(
    _In_ const D3DKMDT_HVIDPNTARGETMODESET           hVidPnTargetModeSet,
    _In_ const D3DKMDT_VIDPN_TARGET_MODE*            pVidPnTargetModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTARGETMODESET_CREATENEWMODEINFO)(
    _In_ const D3DKMDT_HVIDPNTARGETMODESET           hVidPnTargetModeSet,
    _Outptr_ const D3DKMDT_VIDPN_TARGET_MODE**       ppNewVidPnTargetModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTARGETMODESET_ADDMODE)(
    _In_ D3DKMDT_HVIDPNTARGETMODESET                  hVidPnTargetModeSet,
    _In_ D3DKMDT_VIDPN_TARGET_MODE*                   pVidPnTargetModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTARGETMODESET_PINMODE)(
    _In_ D3DKMDT_HVIDPNTARGETMODESET                 hVidPnTargetModeSet,
    _In_ const D3DKMDT_VIDEO_PRESENT_TARGET_MODE_ID  VidPnTargetModeId);

typedef struct _DXGK_VIDPNTARGETMODESET_INTERFACE
{
    DXGKDDI_VIDPNTARGETMODESET_GETNUMMODES              pfnGetNumModes;
    DXGKDDI_VIDPNTARGETMODESET_ACQUIREFIRSTMODEINFO     pfnAcquireFirstModeInfo;
    DXGKDDI_VIDPNTARGETMODESET_ACQUIRENEXTMODEINFO      pfnAcquireNextModeInfo;
    DXGKDDI_VIDPNTARGETMODESET_ACQUIREPINNEDMODEINFO    pfnAcquirePinnedModeInfo;
    DXGKDDI_VIDPNTARGETMODESET_RELEASEMODEINFO          pfnReleaseModeInfo;
    DXGKDDI_VIDPNTARGETMODESET_CREATENEWMODEINFO        pfnCreateNewModeInfo;
    DXGKDDI_VIDPNTARGETMODESET_ADDMODE                  pfnAddMode;
    DXGKDDI_VIDPNTARGETMODESET_PINMODE                  pfnPinMode;
} DXGK_VIDPNTARGETMODESET_INTERFACE;

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_GETNUMPATHS)(
    _In_ const D3DKMDT_HVIDPNTOPOLOGY  hVidPnTopology,
    _Out_ PSIZE_T                      pNumPaths);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_GETNUMPATHSFROMSOURCE)(
    _In_ const D3DKMDT_HVIDPNTOPOLOGY            hVidPnTopology,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID    VidPnSourceId,
    _Out_ PSIZE_T                                pNumPathsFromSource);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_ENUMPATHTARGETSFROMSOURCE)(
    _In_ const D3DKMDT_HVIDPNTOPOLOGY            hVidPnTopology,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID    VidPnSourceId,
    _In_ const D3DKMDT_VIDPN_PRESENT_PATH_INDEX  VidPnPresentPathIndex,
    _Out_ D3DDDI_VIDEO_PRESENT_TARGET_ID*        pVidPnTargetId);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_GETPATHSOURCEFROMTARGET)(
    _In_ const D3DKMDT_HVIDPNTOPOLOGY            hVidTopology,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID    VidPnTargetId,
    _Out_ D3DDDI_VIDEO_PRESENT_SOURCE_ID*        pVidPnSourceId);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_ACQUIREPATHINFO)(
    _In_ const D3DKMDT_HVIDPNTOPOLOGY              hVidPnTopology,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID      VidPnSourceId,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID      VidPnTargetId,
    _Outptr_ const D3DKMDT_VIDPN_PRESENT_PATH**   ppVidPnPresentPathInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_ACQUIREFIRSTPATHINFO)(
    _In_ const D3DKMDT_HVIDPNTOPOLOGY              hVidPnTopology,
    _Outptr_ const D3DKMDT_VIDPN_PRESENT_PATH**    ppFirstVidPnPresentPathInfo);


typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_ACQUIRENEXTPATHINFO)(
    _In_ const D3DKMDT_HVIDPNTOPOLOGY              hVidPnTopology,
    _In_ const D3DKMDT_VIDPN_PRESENT_PATH*   pVidPnPresentPathInfo,
    _Outptr_ const D3DKMDT_VIDPN_PRESENT_PATH** ppNextVidPnPresentPathInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_UPDATEPATHSUPPORTINFO)(
     _In_ const D3DKMDT_HVIDPNTOPOLOGY              i_hVidPnTopology,
     _In_ const D3DKMDT_VIDPN_PRESENT_PATH*         i_pVidPnPresentPathInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_RELEASEPATHINFO)(
     _In_ const D3DKMDT_HVIDPNTOPOLOGY             hVidPnTopology,
     _In_ const D3DKMDT_VIDPN_PRESENT_PATH*        pVidPnPresentPathInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_CREATENEWPATHINFO)(
     _In_ const D3DKMDT_HVIDPNTOPOLOGY             hVidPnTopology,
     _Outptr_ D3DKMDT_VIDPN_PRESENT_PATH**         ppNewVidPnPresentPathInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_ADDPATH)(
     _In_ D3DKMDT_HVIDPNTOPOLOGY                   hVidPnTopology,
     _In_ D3DKMDT_VIDPN_PRESENT_PATH*              pVidPnPresentPath);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPNTOPOLOGY_REMOVEPATH)(
    _In_ const D3DKMDT_HVIDPNTOPOLOGY           hVidPnTopology,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID  VidPnTargetId);

typedef struct _DXGK_VIDPNTOPOLOGY_INTERFACE
{
    DXGKDDI_VIDPNTOPOLOGY_GETNUMPATHS                pfnGetNumPaths;
    DXGKDDI_VIDPNTOPOLOGY_GETNUMPATHSFROMSOURCE      pfnGetNumPathsFromSource;
    DXGKDDI_VIDPNTOPOLOGY_ENUMPATHTARGETSFROMSOURCE  pfnEnumPathTargetsFromSource;
    DXGKDDI_VIDPNTOPOLOGY_GETPATHSOURCEFROMTARGET    pfnGetPathSourceFromTarget;
    DXGKDDI_VIDPNTOPOLOGY_ACQUIREPATHINFO            pfnAcquirePathInfo;
    DXGKDDI_VIDPNTOPOLOGY_ACQUIREFIRSTPATHINFO       pfnAcquireFirstPathInfo;
    DXGKDDI_VIDPNTOPOLOGY_ACQUIRENEXTPATHINFO        pfnAcquireNextPathInfo;
    DXGKDDI_VIDPNTOPOLOGY_UPDATEPATHSUPPORTINFO      pfnUpdatePathSupportInfo;
    DXGKDDI_VIDPNTOPOLOGY_RELEASEPATHINFO            pfnReleasePathInfo;
    DXGKDDI_VIDPNTOPOLOGY_CREATENEWPATHINFO          pfnCreateNewPathInfo;
    DXGKDDI_VIDPNTOPOLOGY_ADDPATH                    pfnAddPath;
    DXGKDDI_VIDPNTOPOLOGY_REMOVEPATH                 pfnRemovePath;
} DXGK_VIDPNTOPOLOGY_INTERFACE, *PDXGK_VIDPNTOPOLOGY_INTERFACE;

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_GETTOPOLOGY)(
    _In_ const D3DKMDT_HVIDPN                              hVidPn,
    _Out_ D3DKMDT_HVIDPNTOPOLOGY*                          phVidPnTopology,
    _Outptr_ const PDXGK_VIDPNTOPOLOGY_INTERFACE           ppVidPnTopologyInterface);


typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_ACQUIRESOURCEMODESET)(
    _In_ const D3DKMDT_HVIDPN                                hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID                VidPnSourceId,
    _Out_ D3DKMDT_HVIDPNSOURCEMODESET *                      phVidPnSourceModeSet,
    _Outptr_ const DXGK_VIDPNSOURCEMODESET_INTERFACE**    ppVidPnSourceModeSetInterface);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_RELEASESOURCEMODESET)(
    _In_ const D3DKMDT_HVIDPN                                hVidPn,
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET                   hVidPnSourceModeSet);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_CREATENEWSOURCEMODESET)(
    _In_ const D3DKMDT_HVIDPN                                hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID                VidPnSourceId,
    _Out_ D3DKMDT_HVIDPNSOURCEMODESET*                       phNewVidPnSourceModeSet,
    _Outptr_ const DXGK_VIDPNSOURCEMODESET_INTERFACE**       ppVidPnSourceModeSetInterface);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_ASSIGNSOURCEMODESET)(
    _In_ D3DKMDT_HVIDPN                                      hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID                VidPnSourceId,
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET                   hVidPnSourceModeSet
    );

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_ASSIGNMULTISAMPLINGMETHODSET)(
    _In_ D3DKMDT_HVIDPN                                       hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID                 VidPnSourceId,
    _In_ const SIZE_T                                         NumMethods,
    _In_reads_(NumMethods) CONST D3DDDI_MULTISAMPLINGMETHOD* pSupportedMethodSet
    );


typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_ACQUIRETARGETMODESET)(
    _In_ const D3DKMDT_HVIDPN                                  hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID                  VidPnTargetId,
    _Out_ D3DKMDT_HVIDPNTARGETMODESET*                         phVidPnTargetModeSet,
    _Outptr_ const DXGK_VIDPNTARGETMODESET_INTERFACE**      ppVidPnTargetModeSetInterface
    );

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_RELEASETARGETMODESET)(
    _In_ const D3DKMDT_HVIDPN                                  hVidPn,
    _In_ const D3DKMDT_HVIDPNTARGETMODESET                     hVidPnTargetModeSet
    );

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_CREATENEWTARGETMODESET)(
    _In_ const D3DKMDT_HVIDPN                               hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID               VidPnTargetId,
    _Out_ D3DKMDT_HVIDPNTARGETMODESET*                      phNewVidPnTargetModeSet,
    _Outptr_ const DXGK_VIDPNTARGETMODESET_INTERFACE**   ppVidPnTargetModeSetInterace
    );

typedef
NTSTATUS
(APIENTRY *DXGKDDI_VIDPN_ASSIGNTARGETMODESET)(
    _In_ D3DKMDT_HVIDPN                                     hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID               VidPnTargetId,
    _In_ const D3DKMDT_HVIDPNTARGETMODESET                  hVidPnTargetModeSet);

typedef struct _DXGK_VIDPN_INTERFACE
{
    DXGK_VIDPN_INTERFACE_VERSION                 Version;
    DXGKDDI_VIDPN_GETTOPOLOGY                    pfnGetTopology;

    // Source modality
    DXGKDDI_VIDPN_ACQUIRESOURCEMODESET           pfnAcquireSourceModeSet;
    DXGKDDI_VIDPN_RELEASESOURCEMODESET           pfnReleaseSourceModeSet;
    DXGKDDI_VIDPN_CREATENEWSOURCEMODESET         pfnCreateNewSourceModeSet;
    DXGKDDI_VIDPN_ASSIGNSOURCEMODESET            pfnAssignSourceModeSet;
    DXGKDDI_VIDPN_ASSIGNMULTISAMPLINGMETHODSET   pfnAssignMultisamplingMethodSet;

    // Target modality
    DXGKDDI_VIDPN_ACQUIRETARGETMODESET           pfnAcquireTargetModeSet;
    DXGKDDI_VIDPN_RELEASETARGETMODESET           pfnReleaseTargetModeSet;
    DXGKDDI_VIDPN_CREATENEWTARGETMODESET         pfnCreateNewTargetModeSet;
    DXGKDDI_VIDPN_ASSIGNTARGETMODESET            pfnAssignTargetModeSet;
} DXGK_VIDPN_INTERFACE, *PDXGK_VIDPN_INTERFACE;

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORSOURCEMODESET_GETNUMMODES)(
    _In_ const D3DKMDT_HMONITORSOURCEMODESET                 hMonitorSourceModeSet,
    _Out_ const PSIZE_T                                      pNumMonitorSourceModes);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORSOURCEMODESET_ACQUIREPREFERREDMODEINFO)(
    _In_ const D3DKMDT_HMONITORSOURCEMODESET                hMonitorSourceModeSet,
    _Outptr_ const D3DKMDT_MONITOR_SOURCE_MODE**            ppFirstMonitorSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORSOURCEMODESET_ACQUIREFIRSTMODEINFO)(
    _In_ const D3DKMDT_HMONITORSOURCEMODESET                  hMonitorSourceModeSet,
    _Outptr_ const D3DKMDT_MONITOR_SOURCE_MODE**              ppFirstMonitorSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORSOURCEMODESET_ACQUIRENEXTMODEINFO)(
    _In_ const D3DKMDT_HMONITORSOURCEMODESET                  hMonitorSourceModeSet,
    _In_ const D3DKMDT_MONITOR_SOURCE_MODE*                   pMonitorSourceModeInfo,
    _Outptr_ const D3DKMDT_MONITOR_SOURCE_MODE**              ppNextMonitorSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORSOURCEMODESET_CREATENEWMODEINFO)(
    _In_ const D3DKMDT_HMONITORSOURCEMODESET                hMonitorSourceModeSet,
    _Outptr_  D3DKMDT_MONITOR_SOURCE_MODE**                 ppNewMonitorSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORSOURCEMODESET_ADDMODE)(
    _In_ const D3DKMDT_HMONITORSOURCEMODESET                 hMonitorSourceModeSet,
    _In_ const D3DKMDT_MONITOR_SOURCE_MODE*                  pMonitorSourceModeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORSOURCEMODESET_RELEASEMODEINFO)(
     _In_ const D3DKMDT_HMONITORSOURCEMODESET                 hMonitorSourceModeSet,
     _In_ const D3DKMDT_MONITOR_SOURCE_MODE*                  pMonitorSourceModeInfo);

typedef struct _DXGK_MONITORSOURCEMODESET_INTERFACE
{
    DXGKDDI_MONITORSOURCEMODESET_GETNUMMODES               pfnGetNumModes;
    DXGKDDI_MONITORSOURCEMODESET_ACQUIREPREFERREDMODEINFO  pfnAcquirePreferredModeInfo;
    DXGKDDI_MONITORSOURCEMODESET_ACQUIREFIRSTMODEINFO      pfnAcquireFirstModeInfo;
    DXGKDDI_MONITORSOURCEMODESET_ACQUIRENEXTMODEINFO       pfnAcquireNextModeInfo;
    DXGKDDI_MONITORSOURCEMODESET_CREATENEWMODEINFO         pfnCreateNewModeInfo;
    DXGKDDI_MONITORSOURCEMODESET_ADDMODE                   pfnAddMode;
    DXGKDDI_MONITORSOURCEMODESET_RELEASEMODEINFO           pfnReleaseModeInfo;
} DXGK_MONITORSOURCEMODESET_INTERFACE, *PDXGK_MONITORSOURCEMODESET_INTERFACE;

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORFREQUENCYRANGESET_GETNUMFREQUENCYRANGES)(
    _In_ const D3DKMDT_HMONITORFREQUENCYRANGESET    hMonitorFrequencyRangeSet,
    _Out_ const PSIZE_T                             pNumMonitorFrequencyRanges);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORFREQUENCYRANGESET_ACQUIREFIRSTFREQUENCYRANGEINFO)(
    _In_ const D3DKMDT_HMONITORFREQUENCYRANGESET          hMonitorFrequencyRangeSet,
    _Outptr_ const D3DKMDT_MONITOR_FREQUENCY_RANGE**   ppFirstMonitorFrequencyRangeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORFREQUENCYRANGESET_ACQUIRENEXTFREQUENCYRANGEINFO)(
    _In_ const D3DKMDT_HMONITORFREQUENCYRANGESET          hMonitorFrequencyRangeSet,
    _In_ const D3DKMDT_MONITOR_FREQUENCY_RANGE*           pMonitorFrequencyRangeInfo,
    _Outptr_ const D3DKMDT_MONITOR_FREQUENCY_RANGE**      ppNextMonitorFrequencyRangeInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORFREQUENCYRANGESET_RELEASEFREQUENCYRANGEINFO)(
     _In_ const D3DKMDT_HMONITORFREQUENCYRANGESET         hMonitorFrequencyRangeSet,
     _In_ const D3DKMDT_MONITOR_FREQUENCY_RANGE*          pMonitorFrequencyRangeInfo);

typedef struct _DXGK_MONITORFREQUENCYRANGESET_INTERFACE
{
    DXGKDDI_MONITORFREQUENCYRANGESET_GETNUMFREQUENCYRANGES           pfnGetNumFrequencyRanges;
    DXGKDDI_MONITORFREQUENCYRANGESET_ACQUIREFIRSTFREQUENCYRANGEINFO  pfnAcquireFirstFrequencyRangeInfo;
    DXGKDDI_MONITORFREQUENCYRANGESET_ACQUIRENEXTFREQUENCYRANGEINFO   pfnAcquireNextFrequencyRangeInfo;
    DXGKDDI_MONITORFREQUENCYRANGESET_RELEASEFREQUENCYRANGEINFO       pfnReleaseFrequencyRangeInfo;
} DXGK_MONITORFREQUENCYRANGESET_INTERFACE, *PDXGK_MONITORFREQUENCYRANGESET_INTERFACE;

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORDESCRIPTORSET_GETNUMDESCRIPTORS)(
    _In_ const D3DKMDT_HMONITORDESCRIPTORSET            hMonitorDescriptorSet,
    _Out_ const PSIZE_T                                 pNumMonitorDescriptors);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORDESCRIPTORSET_ACQUIREFIRSTDESCRIPTORINFO)(
    _In_ const D3DKMDT_HMONITORDESCRIPTORSET              hMonitorDescriptorSet,
    _Outptr_ const D3DKMDT_MONITOR_DESCRIPTOR**        ppFirstMonitorDescriptorInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORDESCRIPTORSET_ACQUIRENEXTDESCRIPTORINFO)(
    _In_ const D3DKMDT_HMONITORDESCRIPTORSET              hMonitorDescriptorSet,
    _In_ const D3DKMDT_MONITOR_DESCRIPTOR*                pMonitorDescriptorInfo,
    _Outptr_ const D3DKMDT_MONITOR_DESCRIPTOR**        ppNextMonitorDescriptorInfo);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITORDESCRIPTORSET_RELEASEDESCRIPTORINFO)(
     _In_ const D3DKMDT_HMONITORDESCRIPTORSET             hMonitorDescriptorSet,
     _In_ const D3DKMDT_MONITOR_DESCRIPTOR*               pMonitorDescriptorInfo);

typedef struct _DXGK_MONITORDESCRIPTORSET_INTERFACE
{
    DXGKDDI_MONITORDESCRIPTORSET_GETNUMDESCRIPTORS           pfnGetNumDescriptors;
    DXGKDDI_MONITORDESCRIPTORSET_ACQUIREFIRSTDESCRIPTORINFO  pfnAcquireFirstDescriptorInfo;
    DXGKDDI_MONITORDESCRIPTORSET_ACQUIRENEXTDESCRIPTORINFO   pfnAcquireNextDescriptorInfo;
    DXGKDDI_MONITORDESCRIPTORSET_RELEASEDESCRIPTORINFO       pfnReleaseDescriptorInfo;
} DXGK_MONITORDESCRIPTORSET_INTERFACE, *PDXGK_MONITORDESCRIPTORSET_INTERFACE;

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITOR_ACQUIREMONITORSOURCEMODESET)(
    _In_ const D3DKMDT_ADAPTER                              hAdapter,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID               VideoPresentTargetId,
    _Out_ D3DKMDT_HMONITORSOURCEMODESET*                    phMonitorSourceModeSet,
    _Outptr_ const PDXGK_MONITORSOURCEMODESET_INTERFACE*    ppMonitorSourceModeSetInterface);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITOR_RELEASEMONITORSOURCEMODESET)(
    _In_ const D3DKMDT_ADAPTER                hAdapter,
    _In_ const D3DKMDT_HMONITORSOURCEMODESET  hMonitorSourceModeSet);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITOR_GETMONITORFREQUENCYRANGESET)(
    _In_ const D3DKMDT_ADAPTER                                  hAdapter,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID                   VideoPresentTargetId,
    _Out_ D3DKMDT_HMONITORFREQUENCYRANGESET*                    phMonitorFrequencyRangeSet,
    _Outptr_ const  PDXGK_MONITORFREQUENCYRANGESET_INTERFACE*   ppMonitorFrequencyRangeSetInterface);

typedef
NTSTATUS
(APIENTRY *DXGKDDI_MONITOR_GETMONITORDESCRIPTORSET)(
    _In_ const D3DKMDT_ADAPTER                                  hAdapter,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID                   VideoPresentTargetId,
    _Out_ D3DKMDT_HMONITORDESCRIPTORSET*                        phMonitorDescriptorSet,
    _Outptr_ const  PDXGK_MONITORDESCRIPTORSET_INTERFACE*       ppMonitorDescriptorSetInterface);

typedef struct _DXGK_MONITOR_INTERFACE
{
    DXGK_MONITOR_INTERFACE_VERSION                 Version;
    DXGKDDI_MONITOR_ACQUIREMONITORSOURCEMODESET    pfnAcquireMonitorSourceModeSet;
    DXGKDDI_MONITOR_RELEASEMONITORSOURCEMODESET    pfnReleaseMonitorSourceModeSet;
    DXGKDDI_MONITOR_GETMONITORFREQUENCYRANGESET    pfnGetMonitorFrequencyRangeSet;
    DXGKDDI_MONITOR_GETMONITORDESCRIPTORSET        pfnGetMonitorDescriptorSet;
}
DXGK_MONITOR_INTERFACE, *PDXGK_MONITOR_INTERFACE;

typedef
VOID*
(APIENTRY CALLBACK *DXGKCB_GETHANDLEDATA)(_In_ const PDXGKARGCB_GETHANDLEDATA);

typedef
D3DKMT_HANDLE
(APIENTRY CALLBACK *DXGKCB_GETHANDLEPARENT)(_In_ D3DKMT_HANDLE hAllocation);

typedef
D3DKMT_HANDLE
(APIENTRY CALLBACK *DXGKCB_ENUMHANDLECHILDREN)(_In_ const PDXGKARGCB_ENUMHANDLECHILDREN);

typedef
VOID (APIENTRY CALLBACK *DXGKCB_NOTIFY_INTERRUPT)(
    _In_ const HANDLE hAdapter, _In_ const PDXGKARGCB_NOTIFY_INTERRUPT_DATA);

typedef
VOID
(APIENTRY CALLBACK *DXGKCB_NOTIFY_DPC)(
    _In_ const HANDLE hAdapter);

typedef
NTSTATUS
(APIENTRY CALLBACK *DXGKCB_QUERYVIDPNINTERFACE)(
    _In_ const D3DKMDT_HVIDPN                                 hVidPn,
    _In_ const DXGK_VIDPN_INTERFACE_VERSION                   VidPnInterfaceVersion,
    _Outptr_ const PDXGK_VIDPN_INTERFACE                  ppVidPnInterface);

typedef
NTSTATUS
(APIENTRY CALLBACK *DXGKCB_QUERYMONITORINTERFACE)(
    _In_ const HANDLE                          hAdapter,
    _In_ const DXGK_MONITOR_INTERFACE_VERSION  MonitorInterfaceVersion,
    _Outptr_ const PDXGK_MONITOR_INTERFACE*    ppMonitorInterface);

typedef
NTSTATUS
(APIENTRY CALLBACK *DXGKCB_GETCAPTUREADDRESS)(_Inout_ PDXGKARGCB_GETCAPTUREADDRESS);

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
typedef struct _DXGK_QUERYADAPTERINFOFLAGS
{
    union
    {
        struct
        {
            UINT    VirtualMachineData          : 1; 
            UINT    SecureVirtualMachine        : 1; 
            UINT    Reserved                    :30; 
        };
        UINT Value;
    };
 } DXGK_QUERYADAPTERINFOFLAGS;
 #endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)


typedef struct _DXGKARG_QUERYADAPTERINFO
{
    DXGK_QUERYADAPTERINFOTYPE   Type;
    VOID*                       pInputData;
    UINT                        InputDataSize;
    VOID*                       pOutputData;
    UINT                        OutputDataSize;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
    DXGK_QUERYADAPTERINFOFLAGS  Flags;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_2)
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
    HANDLE                      hKmdProcessHandle; 
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4)
} DXGKARG_QUERYADAPTERINFO, *PDXGKARG_QUERYADAPTERINFO;

typedef struct _DXGK_DEVICEINFOFLAGS
{
    union
    {
        struct
        {
            UINT    GuaranteedDmaBufferContract : 1;
            UINT    Reserved                    :31;
        };
        UINT Value;
    };
 } DXGK_DEVICEINFOFLAGS;

typedef struct _DXGK_DEVICEINFO
{
    UINT        DmaBufferSize;
    UINT        DmaBufferSegmentSet;
    UINT        DmaBufferPrivateDataSize;
    UINT        AllocationListSize;
    UINT        PatchLocationListSize;
    DXGK_DEVICEINFOFLAGS Flags;
} DXGK_DEVICEINFO;

typedef struct _DXGK_CREATEDEVICEFLAGS
{
    union
    {
        struct
        {
            UINT    SystemDevice            :  1;
            UINT    GdiDevice               :  1;
            UINT    Reserved                : 29;
            UINT    DXGK_DEVICE_RESERVED0   :  1;
        };
        UINT Value;
    };
} DXGK_CREATEDEVICEFLAGS;

typedef struct _DXGKARG_CREATEDEVICE
{
    HANDLE               hDevice;    
    union
    {
        DXGK_CREATEDEVICEFLAGS Flags;
        DXGK_DEVICEINFO*       pInfo;
    };
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    ULONG   Pasid;
    HANDLE  hKmdProcess;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
} DXGKARG_CREATEDEVICE, *PDXGKARG_CREATEDEVICE;

typedef enum _DXGK_RECOMMENDFUNCTIONALVIDPN_REASON
{
    DXGK_RFVR_UNINITIALIZED  = 0,
    DXGK_RFVR_HOTKEY         = 1,
    DXGK_RFVR_USERMODE       = 2,
    DXGK_RFVR_FIRMWARE       = 3,
}
DXGK_RECOMMENDFUNCTIONALVIDPN_REASON;

typedef struct _DXGKARG_RECOMMENDFUNCTIONALVIDPN
{
    IN  UINT                                    NumberOfVidPnTargets;
    IN  CONST D3DDDI_VIDEO_PRESENT_TARGET_ID*   pVidPnTargetPrioritizationVector;
    IN  D3DKMDT_HVIDPN                          hRecommendedFunctionalVidPn;
    IN  DXGK_RECOMMENDFUNCTIONALVIDPN_REASON    RequestReason;
    IN  OPTIONAL VOID*                          pPrivateDriverData;
    IN  UINT                                    PrivateDriverDataSize;
} DXGKARG_RECOMMENDFUNCTIONALVIDPN, *PDXGKARG_RECOMMENDFUNCTIONALVIDPN;

typedef struct _DXGK_ENUM_PIVOT
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;
    D3DDDI_VIDEO_PRESENT_TARGET_ID  VidPnTargetId;
} DXGK_ENUM_PIVOT;

typedef struct _DXGKARG_ENUMVIDPNCOFUNCMODALITY
{
    IN  D3DKMDT_HVIDPN                         hConstrainingVidPn;
    IN  D3DKMDT_ENUMCOFUNCMODALITY_PIVOT_TYPE  EnumPivotType;
    IN  DXGK_ENUM_PIVOT                        EnumPivot;
} DXGKARG_ENUMVIDPNCOFUNCMODALITY, *PDXGKARG_ENUMVIDPNCOFUNCMODALITY;

typedef
NTSTATUS
APIENTRY
DXGKDDI_QUERYADAPTERINFO(
    _In_ const HANDLE                         hAdapter,
    _In_ const PDXGKARG_QUERYADAPTERINFO      pQueryAdapterInfo);


typedef
NTSTATUS
APIENTRY
DXGKDDI_CREATEDEVICE(_In_ const HANDLE                 hAdapter,
                     _Inout_ PDXGKARG_CREATEDEVICE     pCreateDevice);

typedef
NTSTATUS
APIENTRY
DXGKDDI_RECOMMENDFUNCTIONALVIDPN(
    _In_ const HANDLE                                     hAdapter,
    _In_ const PDXGKARG_RECOMMENDFUNCTIONALVIDPN  pRecommendFunctionalVidPn);


typedef
NTSTATUS
APIENTRY
DXGKDDI_ENUMVIDPNCOFUNCMODALITY(
    _In_ const HANDLE                                     hAdapter,
    _In_ const PDXGKARG_ENUMVIDPNCOFUNCMODALITY           pEnumCofuncModality);



typedef DXGKDDI_QUERYADAPTERINFO                *PDXGKDDI_QUERYADAPTERINFO;
typedef DXGKDDI_CREATEDEVICE                    *PDXGKDDI_CREATEDEVICE;
// TODO: Lazy
#if 0
typedef DXGKDDI_CREATEALLOCATION                *PDXGKDDI_CREATEALLOCATION;
typedef DXGKDDI_DESTROYALLOCATION               *PDXGKDDI_DESTROYALLOCATION;
typedef DXGKDDI_DESCRIBEALLOCATION              *PDXGKDDI_DESCRIBEALLOCATION;
typedef DXGKDDI_GETSTANDARDALLOCATIONDRIVERDATA *PDXGKDDI_GETSTANDARDALLOCATIONDRIVERDATA;
typedef DXGKDDI_ACQUIRESWIZZLINGRANGE           *PDXGKDDI_ACQUIRESWIZZLINGRANGE;
typedef DXGKDDI_RELEASESWIZZLINGRANGE           *PDXGKDDI_RELEASESWIZZLINGRANGE;
typedef DXGKDDI_PATCH                           *PDXGKDDI_PATCH;
typedef DXGKDDI_SUBMITCOMMAND                   *PDXGKDDI_SUBMITCOMMAND;
typedef DXGKDDI_PREEMPTCOMMAND                  *PDXGKDDI_PREEMPTCOMMAND;
typedef DXGKDDI_CANCELCOMMAND                   *PDXGKDDI_CANCELCOMMAND;
typedef DXGKDDI_BUILDPAGINGBUFFER               *PDXGKDDI_BUILDPAGINGBUFFER;
typedef DXGKDDI_SETPALETTE                      *PDXGKDDI_SETPALETTE;
typedef DXGKDDI_SETPOINTERPOSITION              *PDXGKDDI_SETPOINTERPOSITION;
typedef DXGKDDI_SETPOINTERSHAPE                 *PDXGKDDI_SETPOINTERSHAPE;
typedef DXGKDDI_RESETFROMTIMEOUT                *PDXGKDDI_RESETFROMTIMEOUT;
typedef DXGKDDI_RESTARTFROMTIMEOUT              *PDXGKDDI_RESTARTFROMTIMEOUT;
typedef DXGKDDI_ESCAPE                          *PDXGKDDI_ESCAPE;
typedef DXGKDDI_COLLECTDBGINFO                  *PDXGKDDI_COLLECTDBGINFO;
typedef DXGKDDI_QUERYCURRENTFENCE               *PDXGKDDI_QUERYCURRENTFENCE;
typedef DXGKDDI_ISSUPPORTEDVIDPN                *PDXGKDDI_ISSUPPORTEDVIDPN;
typedef DXGKDDI_RECOMMENDFUNCTIONALVIDPN        *PDXGKDDI_RECOMMENDFUNCTIONALVIDPN;
typedef DXGKDDI_ENUMVIDPNCOFUNCMODALITY         *PDXGKDDI_ENUMVIDPNCOFUNCMODALITY;
typedef DXGKDDI_SETVIDPNSOURCEADDRESS           *PDXGKDDI_SETVIDPNSOURCEADDRESS;
typedef DXGKDDI_SETVIDPNSOURCEVISIBILITY        *PDXGKDDI_SETVIDPNSOURCEVISIBILITY;
typedef DXGKDDI_COMMITVIDPN                     *PDXGKDDI_COMMITVIDPN;
typedef DXGKDDI_UPDATEACTIVEVIDPNPRESENTPATH    *PDXGKDDI_UPDATEACTIVEVIDPNPRESENTPATH;
typedef DXGKDDI_RECOMMENDMONITORMODES           *PDXGKDDI_RECOMMENDMONITORMODES;
typedef DXGKDDI_RECOMMENDVIDPNTOPOLOGY          *PDXGKDDI_RECOMMENDVIDPNTOPOLOGY;
typedef DXGKDDI_GETSCANLINE                     *PDXGKDDI_GETSCANLINE;
typedef DXGKDDI_STOPCAPTURE                     *PDXGKDDI_STOPCAPTURE;
typedef DXGKDDI_CONTROLINTERRUPT                *PDXGKDDI_CONTROLINTERRUPT;
typedef DXGKDDI_CREATEOVERLAY                   *PDXGKDDI_CREATEOVERLAY;
typedef DXGKDDI_DESTROYDEVICE                   *PDXGKDDI_DESTROYDEVICE;
typedef DXGKDDI_OPENALLOCATIONINFO              *PDXGKDDI_OPENALLOCATIONINFO;
typedef DXGKDDI_CLOSEALLOCATION                 *PDXGKDDI_CLOSEALLOCATION;
typedef DXGKDDI_RENDER                          *PDXGKDDI_RENDER;
typedef DXGKDDI_PRESENT                         *PDXGKDDI_PRESENT;
typedef DXGKDDI_UPDATEOVERLAY                   *PDXGKDDI_UPDATEOVERLAY;
typedef DXGKDDI_FLIPOVERLAY                     *PDXGKDDI_FLIPOVERLAY;
typedef DXGKDDI_DESTROYOVERLAY                  *PDXGKDDI_DESTROYOVERLAY;
typedef DXGKDDI_CREATECONTEXT                   *PDXGKDDI_CREATECONTEXT;
typedef DXGKDDI_DESTROYCONTEXT                  *PDXGKDDI_DESTROYCONTEXT;
typedef DXGKDDI_SETDISPLAYPRIVATEDRIVERFORMAT   *PDXGKDDI_SETDISPLAYPRIVATEDRIVERFORMAT;
#endif
typedef UINT32 *PDXGKDDI_CREATEALLOCATION;
typedef UINT32 *PDXGKDDI_DESTROYALLOCATION;
typedef UINT32 *PDXGKDDI_DESCRIBEALLOCATION;
typedef UINT32 *PDXGKDDI_GETSTANDARDALLOCATIONDRIVERDATA;
typedef UINT32 *PDXGKDDI_ACQUIRESWIZZLINGRANGE;
typedef UINT32 *PDXGKDDI_RELEASESWIZZLINGRANGE;
typedef UINT32 *PDXGKDDI_PATCH;
typedef UINT32 *PDXGKDDI_SUBMITCOMMAND;
typedef UINT32 *PDXGKDDI_PREEMPTCOMMAND;
typedef UINT32 *PDXGKDDI_CANCELCOMMAND;
typedef UINT32 *PDXGKDDI_BUILDPAGINGBUFFER;
typedef UINT32 *PDXGKDDI_SETPALETTE;
typedef UINT32 *PDXGKDDI_SETPOINTERPOSITION;
typedef UINT32 *PDXGKDDI_SETPOINTERSHAPE;
typedef UINT32 *PDXGKDDI_RESETFROMTIMEOUT;
typedef UINT32 *PDXGKDDI_RESTARTFROMTIMEOUT;
typedef UINT32 *PDXGKDDI_ESCAPE;
typedef UINT32 *PDXGKDDI_COLLECTDBGINFO;
typedef UINT32 *PDXGKDDI_QUERYCURRENTFENCE;
typedef UINT32 *PDXGKDDI_ISSUPPORTEDVIDPN;
typedef DXGKDDI_RECOMMENDFUNCTIONALVIDPN        *PDXGKDDI_RECOMMENDFUNCTIONALVIDPN;
typedef DXGKDDI_ENUMVIDPNCOFUNCMODALITY         *PDXGKDDI_ENUMVIDPNCOFUNCMODALITY;
typedef UINT32 *PDXGKDDI_SETVIDPNSOURCEADDRESS;
typedef UINT32 *PDXGKDDI_SETVIDPNSOURCEVISIBILITY;
typedef UINT32 *PDXGKDDI_COMMITVIDPN;
typedef UINT32 *PDXGKDDI_UPDATEACTIVEVIDPNPRESENTPATH;
typedef UINT32 *PDXGKDDI_RECOMMENDMONITORMODES;
typedef UINT32 *PDXGKDDI_RECOMMENDVIDPNTOPOLOGY;
typedef UINT32 *PDXGKDDI_GETSCANLINE;
typedef UINT32 *PDXGKDDI_STOPCAPTURE;
typedef UINT32 *PDXGKDDI_CONTROLINTERRUPT;
typedef UINT32 *PDXGKDDI_CREATEOVERLAY;
typedef UINT32 *PDXGKDDI_DESTROYDEVICE;
typedef UINT32 *PDXGKDDI_OPENALLOCATIONINFO;
typedef UINT32 *PDXGKDDI_CLOSEALLOCATION;
typedef UINT32 *PDXGKDDI_RENDER;
typedef UINT32 *PDXGKDDI_PRESENT;
typedef UINT32 *PDXGKDDI_UPDATEOVERLAY;
typedef UINT32 *PDXGKDDI_FLIPOVERLAY;
typedef UINT32 *PDXGKDDI_DESTROYOVERLAY;
typedef UINT32 *PDXGKDDI_CREATECONTEXT;
typedef UINT32 *PDXGKDDI_DESTROYCONTEXT;
typedef UINT32 *PDXGKDDI_SETDISPLAYPRIVATEDRIVERFORMAT;
#endif // _D3DKMDDI_H_