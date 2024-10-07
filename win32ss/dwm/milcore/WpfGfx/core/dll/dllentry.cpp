// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      MILCore.dll entry point
//
//

#include "precomp.hpp"
#define DbgBreakPoint __debugbreak
extern "C"
BOOL
__stdcall
DllMain(
    HINSTANCE   dllHandle,
    ULONG       reason,
    __in_ecount(1) CONTEXT* /* context */
    )
{
    DbgBreakPoint();
    return MILCoreDllMain(
        dllHandle,
        reason
        );
}


void __stdcall
IMILBitmapEffectConnector_IsConnected()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffectInputConnector_ConnectTo()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffectInputConnector_GetConnection()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffectOutputConnector_GetConnection()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffectOutputConnector_GetNumberConnections()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffectRenderContext_GetFinalTransform()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffectRenderContext_GetOutputDPI()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffectRenderContext_GetOutputPixelFormat()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffectRenderContext_SetInitialTransform()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffectRenderContext_SetOutputDPI()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffectRenderContext_SetOutputPixelFormat()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffectRenderContext_SetRegionOfInterest()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffectRenderContext_SetUseSoftwareRenderer()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffect_GetOutput()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffect_GetParentEffect()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffect_IMILBitmapEffectConnectionsInfo_GetNumberInputs()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffect_IMILBitmapEffectConnectionsInfo_GetNumberOutputs()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffect_IMILBitmapEffectConnections_GetInputConnector()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffect_IMILBitmapEffectConnections_GetOutputConnector()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffect_IMILBitmapEffectGroup_Add()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffect_IMILBitmapEffectGroup_GetInteriorInputConnector()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffect_IMILBitmapEffectGroup_GetInteriorOutputConnector()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffect_IMILBitmapEffectPrimitive_GetAffineMatrix()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffect_IMILBitmapEffectPrimitive_HasAffineTransform()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffect_IMILBitmapEffectPrimitive_HasInverseTransform()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffect_IMILBitmapEffectPrimitive_SetValue()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffect_IMILBitmapEffectPrimitive_TransformPoint()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffect_IMILBitmapEffectPrimitive_TransformRect()
{
    DbgBreakPoint();
}
void __stdcall
IMILBitmapEffect_SetInputSource()
{
    DbgBreakPoint();
}
void __stdcall
MIL3DCalcBrushToIdealSampleSpace()
{
    DbgBreakPoint();
}
void __stdcall
MILCreateBitmapEffectOuterPublic()
{
    DbgBreakPoint();
}
void __stdcall
MILFactoryCreateBitmapEffect()
{
    DbgBreakPoint();
}
void __stdcall
MILFactoryCreateBitmapEffectContext()
{
    DbgBreakPoint();
}
void __stdcall
MILFactoryCreateBitmapEffectOuter()
{
    DbgBreakPoint();
}
void __stdcall
MILInitializeBitmapEffectPublic()
{
    DbgBreakPoint();
}
void __stdcall
MILRegisterWmpFactory()
{
    DbgBreakPoint();
}
void __stdcall
MILStreamNotifyReadComplete()
{
    DbgBreakPoint();
}
void __stdcall
MilChannel_SendSyncCommand()
{
    DbgBreakPoint();
}
void __stdcall
MilCompositionEngine_GetFeedbackReader()
{
    DbgBreakPoint();
}
void __stdcall
MilConnection_RecordUCE()
{
    DbgBreakPoint();
}
void __stdcall
MilCoreClientIsDwm()
{
    DbgBreakPoint();
}
void __stdcall
MilGraphicsStream_Open()
{
    DbgBreakPoint();
}
void __stdcall
MilGraphicsStream_SetTransformHint()
{
    DbgBreakPoint();
}
void __stdcall
MilGraphicsStream_Close()
{
    DbgBreakPoint();
}
void __stdcall
MilGraphicsStream_GetTransformHint()
{
    DbgBreakPoint();
}
void __stdcall
MilSynchPacketTransport_Create()
{
    DbgBreakPoint();
}
void __stdcall
MilSynchPacketTransport_Present()
{
    DbgBreakPoint();
}
void __stdcall
MilTransport_AddRef()
{
    DbgBreakPoint();
}
void __stdcall
MilTransport_Create()
{
    DbgBreakPoint();
}
void __stdcall
MilTransport_CreateFromPacketTransport()
{
    DbgBreakPoint();
}
void __stdcall
MilTransport_CreateSurfaceManager()
{
    DbgBreakPoint();
}
void __stdcall
MilTransport_CreateTransportParameters()
{
    DbgBreakPoint();
}
void __stdcall
MilTransport_DisconnectTransport()
{
    DbgBreakPoint();
}
void __stdcall
MilTransport_InitializeConnectionManager()
{
    DbgBreakPoint();
}
void __stdcall
MilTransport_Release()
{
    DbgBreakPoint();
}
void __stdcall
MilTransport_ShutDownConnectionManager()
{
    DbgBreakPoint();
}
void __stdcall
VirtualChannelGetInstance()
{
    DbgBreakPoint();
}
