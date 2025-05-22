#pragma once

typedef enum _FLIP3DPOLCIY
{
    FLIP3D_DEFAULT =      0,
    FLIP3D_IGNOREBELOW = 1,
    FLIP3D_IGNOREABOVE = 2,
} FLIP3DPOLCIY;

typedef enum _MIL_SOURCE_FLAGS
{
    In =    0,
    Out =   1,
    InOut = 2,
    None =  3,
} MIL_SOURCE_FLAGS;

#define VISTA_DWM = 0x1

/*
 * When DWM calls our CreateWindow it gives us an interface that is formated similar to this.
 * This CompositedWindow is an object to represent windows that RWM is going to deal with.
 */
class CompositedWindow
{
public:
    virtual VOID         SetClientData(PVOID Data);
    virtual PVOID        GetClientData();
    virtual HSPRITE      GetSpriteHandle();
    virtual HWND         GetWindowHandle();
    virtual UINT32       GetStyle();
    virtual UINT32       GetExStyle();
    virtual UINT32       GetClsStyle();
    virtual BOOLEAN      IsVisible();
#ifdef VISTA_DWM
    virtual VOID         GetRect();
#else
    virtual VOID         GetRect(RECT* Rect);
#endif
#ifdef WIN7_DWM
    virtual VOID         GetWindowRect(RECT* Rect);
#endif
    virtual VOID         GetClientMargins(MARGINS* Margins );
    virtual INT32        GetBorderThickness();
    virtual BOOLEAN      IsDpiAware();
    virtual BOOLEAN      IsForeground();
#ifdef WIN7_DWM
    virtual UINT8        GetSourceConstantAlpha();
#endif
#ifdef VISTA_DWM
    virtual UINT8        GetAlpha();
#endif
    virtual VOID         GetClientGlassMargins(MARGINS* Margins);
    virtual BOOLEAN      ShouldNcRender();
    virtual BOOLEAN      IsNonClientRTLLayout();
    virtual BOOLEAN      HasDXContent();
#ifdef WIN7_DWM
    virtual BOOLEAN      HasProtectedContent();
    virtual BOOLEAN      IsGDIContentOpaque();
#else /* VISTA_DWM */
    virtual BOOLEAN      IsContentOpaque();
#endif
    virtual BOOLEAN      ShouldForceIconicRepresentation();
    virtual FLIP3DPOLCIY GetFlip3DWindowPolicy();
#ifdef WIN7_DWM
    virtual BOOLEAN      HasIconicBitmap();
    virtual BOOLEAN      ShouldForceTransitionsDisabled();
    virtual BOOLEAN      IsExcludedFromLivePreview();
    virtual BOOLEAN      IsForceActiveWindowApperenace();
    virtual BOOLEAN      IsPeekDisallowed();
    virtual BOOLEAN      IsDeviceBitmap();
#endif
    virtual HRESULT      GetClientNode(MIL_CHANNEL Channel, UINT32* Node);
    virtual HRESULT      GetClientNodeClone(MIL_CHANNEL Channel, UINT32* node);
    virtual HRESULT      GetBlurBehindGeometry(HRGN Gemoetry);
    virtual HRESULT      GetClipGeometry(MIL_CHANNEL Channel, UINT32* Clip);
    virtual HRESULT      GetGDISurface(MIL_CHANNEL Channel, UINT32* Surface);
    virtual VOID         GetSourceModifications(MIL_SOURCE_FLAGS* MilSourceFlags, UINT32* Color);
#ifdef WIN7_DWM
    virtual UINT32       GetExStyle2();
    virtual BOOLEAN      IsRenderForCapture();
    virtual BOOLEAN      GetNeedsContextualizedOpacity();
    virtual ULONG        GetContextualizedOpacity();
#endif
};
