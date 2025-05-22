#pragma once

typedef struct IDwmWindowList IDwmWindowList;

typedef struct _WindowListVtbl
{
  HRESULT (WINAPI* WindowListCreateWindow)( CompositedWindow*);
  HRESULT (WINAPI* WindowListDestroyWindow)(CompositedWindow*);
  HRESULT (WINAPI* WindowListCreateSprite)(CompositedWindow*);
  HRESULT (WINAPI* WindowListDestroySprite)(CompositedWindow*);
  HRESULT (WINAPI* WindowListShowHide)(CompositedWindow*);
  HRESULT (WINAPI* WindowListMoveSize)(CompositedWindow*);
  HRESULT (WINAPI* WindowListZOrder)(CompositedWindow*, CompositedWindow*);
  HRESULT (WINAPI* WindowListStyleChange)(CompositedWindow*);
  HRESULT (WINAPI* WindowListOwnerChange)(CompositedWindow*, CompositedWindow*);
  HRESULT (WINAPI* WindowListClientMarginsChange)(CompositedWindow*);
  HRESULT (WINAPI* WindowListClientGlassChange)(CompositedWindow*);
  HRESULT (WINAPI* WindowListActivationChange)(CompositedWindow*);
  HRESULT (WINAPI* WindowListAlphaChange)(CompositedWindow*);
  HRESULT (WINAPI* WindowListBlurBehindChange)( CompositedWindow*, DWM_BLURBEHIND*);
  HRESULT (WINAPI* WindowListClipChange)(CompositedWindow*);
  HRESULT (WINAPI* WindowListDXContentChange)(CompositedWindow*);
  HRESULT (WINAPI* WindowListGDISurfaceChange)(CompositedWindow*);
  HRESULT (WINAPI* WindowListGhostChange)(CompositedWindow*);
  HRESULT (WINAPI* WindowListForceIconicRepresentationChange)(CompositedWindow*);
  HRESULT (WINAPI* WindowListFlip3DWindowPolicyChange)( CompositedWindow*);
  HRESULT (WINAPI* WindowListForceDisconnectClientNode)( CompositedWindow*);
  HRESULT (WINAPI* WindowListGetWindowBounds)( HWND, RECT*);
  HRESULT (WINAPI* WindowListProcessAsyncDwmMessage)( RWM_COMMANDS, PVOID, UINT32, BOOLEAN);
  HRESULT (WINAPI* WindowListProcessSyncDwmMessage)( RWM_COMMANDS, PVOID, UINT32, BOOLEAN, UINT32, REMOTE_PORT_VIEW*, HRESULT*, UINT32*);
  HRESULT (WINAPI* WindowListProcessBackChannelMessage)(MIL_MESSAGE*);
  HRESULT (WINAPI* WindowListUpdateScene)();
} WindowListVtbl;

typedef struct IDwmWindowList 
{
  WindowListVtbl* lpVtbl;
} IDwmWindowList;
