
#include <rxgkrnl.h>
//#define NDEBUG
#include <debug.h>

extern PRXGK_PRIVATE_EXTENSION RxgkDriverExtension;


NTSTATUS
APIENTRY
RxgkVidPnGetTopology(
    _In_ const D3DKMDT_HVIDPN                              hVidPn,
    _Out_ D3DKMDT_HVIDPNTOPOLOGY*                          phVidPnTopology,
    _Outptr_ const DXGK_VIDPNTOPOLOGY_INTERFACE**           ppVidPnTopologyInterface)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
APIENTRY
RxgkVidPnAcquireSourceModeSet(
    _In_ const D3DKMDT_HVIDPN                                hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID                VidPnSourceId,
    _Out_ D3DKMDT_HVIDPNSOURCEMODESET *                      phVidPnSourceModeSet,
    _Outptr_ const DXGK_VIDPNSOURCEMODESET_INTERFACE**    ppVidPnSourceModeSetInterface)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
APIENTRY
RxgkVidPnReleaseSourceModeSet(
    _In_ const D3DKMDT_HVIDPN                                hVidPn,
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET                   hVidPnSourceModeSet)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
APIENTRY
RxgkVidPnCreateNewSourceModeSet(
    _In_ const D3DKMDT_HVIDPN                                hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID                VidPnSourceId,
    _Out_ D3DKMDT_HVIDPNSOURCEMODESET*                       phNewVidPnSourceModeSet,
    _Outptr_ const DXGK_VIDPNSOURCEMODESET_INTERFACE**       ppVidPnSourceModeSetInterface)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
APIENTRY
RxgkVidPnAssignSourceModeSet(
    _In_ D3DKMDT_HVIDPN                                      hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID                VidPnSourceId,
    _In_ const D3DKMDT_HVIDPNSOURCEMODESET                   hVidPnSourceModeSet)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
APIENTRY
RxgkVidPnAssignMultisamplingMethodSet(
    _In_ D3DKMDT_HVIDPN                                       hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_SOURCE_ID                 VidPnSourceId,
    _In_ const SIZE_T                                         NumMethods,
    _In_reads_(NumMethods) CONST D3DDDI_MULTISAMPLINGMETHOD*  pSupportedMethodSet)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
APIENTRY
RxgkVidPnAcquireTargetModeSet(
    _In_ const D3DKMDT_HVIDPN                                  hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID                  VidPnTargetId,
    _Out_ D3DKMDT_HVIDPNTARGETMODESET*                         phVidPnTargetModeSet,
    _Outptr_ const DXGK_VIDPNTARGETMODESET_INTERFACE**         ppVidPnTargetModeSetInterface)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
APIENTRY
RxgkVidPnReleaseTargetModeSet(
    _In_ const D3DKMDT_HVIDPN                                  hVidPn,
    _In_ const D3DKMDT_HVIDPNTARGETMODESET                     hVidPnTargetModeSet)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
APIENTRY
RxgkVidPnCreateNewTargetModeSet(
    _In_ const D3DKMDT_HVIDPN                               hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID               VidPnTargetId,
    _Out_ D3DKMDT_HVIDPNTARGETMODESET*                      phNewVidPnTargetModeSet,
    _Outptr_ const DXGK_VIDPNTARGETMODESET_INTERFACE**      ppVidPnTargetModeSetInterace)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
APIENTRY
RxgkVidPnAssignTargetModeSet(
    _In_ D3DKMDT_HVIDPN                                     hVidPn,
    _In_ const D3DDDI_VIDEO_PRESENT_TARGET_ID               VidPnTargetId,
    _In_ const D3DKMDT_HVIDPNTARGETMODESET                  hVidPnTargetModeSet)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
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
    DXGK_VIDPN_INTERFACE VidPnInterface;

    if (!ppVidPnInterface)
        return STATUS_INVALID_PARAMETER;

    VidPnInterface.Version = DXGK_VIDPN_INTERFACE_VERSION_V1;
    VidPnInterface.pfnGetTopology = RxgkVidPnGetTopology;
    VidPnInterface.pfnAcquireSourceModeSet = RxgkVidPnAcquireSourceModeSet;
    VidPnInterface.pfnReleaseSourceModeSet =  RxgkVidPnReleaseSourceModeSet;
    VidPnInterface.pfnCreateNewSourceModeSet =  RxgkVidPnCreateNewSourceModeSet;
    VidPnInterface.pfnAssignSourceModeSet =  RxgkVidPnAssignSourceModeSet;
    VidPnInterface.pfnAssignMultisamplingMethodSet = RxgkVidPnAssignMultisamplingMethodSet;
    VidPnInterface.pfnAcquireTargetModeSet =  RxgkVidPnAcquireTargetModeSet;
    VidPnInterface.pfnReleaseTargetModeSet =  RxgkVidPnReleaseTargetModeSet;
    VidPnInterface.pfnCreateNewTargetModeSet =  RxgkVidPnCreateNewTargetModeSet;
    VidPnInterface.pfnAssignTargetModeSet =  RxgkVidPnAssignTargetModeSet;
    *ppVidPnInterface = &VidPnInterface;
    return STATUS_SUCCESS;
}