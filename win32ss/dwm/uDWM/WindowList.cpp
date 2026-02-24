#include "uDWM.h"
#include <debug.h>
#include "RwmVistaSp1MilCmd.h"
#include <uxtheme.h>
#include <math.h>
extern DwmDesktop* DwmDesktopInstance;

static LIST_ENTRY g_RwmWindowListHead;
static BOOLEAN g_RwmWindowListInitialized = FALSE;
static UINT g_RwmWindowCount = 0;
static ULONG g_uDwmUpdateSceneCount = 0;
static BOOLEAN g_RwmRebuildChildren = TRUE;

/* Vista DWMWindow theme parts/states (from Vista SP1 uDWM). */
#define DWMWP_TOPFRAME      (57)
#define DWMTFS_ACTIVE       (1)
#define DWMTFS_INACTIVE     (2)
#define DWMTFS_MAXACTIVE    (3)
#define DWMTFS_MAXINACTIVE  (4)

/*
 * IMPORTANT:
 * Do not rely on ReactOS' `vssym32.h` for these values; keep the Vista-validated
 * numeric IDs from the reference code.
 */
#define UDWM_TMT_SIZINGMARGINS       (3601)
#define UDWM_TMT_CAPTIONBARHEIGHT    (1205)
#define UDWM_TMT_ROUNDCORNERWIDTH    (2404)
#define UDWM_TMT_ROUNDCORNERHEIGHT   (2405)
#define UDWM_TMT_COLORIZATIONCOLOR   (2431)
#define UDWM_TMT_COLORIZATIONOPACITY (2432)

static HTHEME g_hDwmTheme = NULL;

static __forceinline HTHEME uDwmGetDwmWindowTheme(VOID)
{
    if (!g_hDwmTheme)
    {
        /* Vista reference uses hwnd=0. */
        g_hDwmTheme = OpenThemeData(NULL, L"DWMWindow");
    }
    return g_hDwmTheme;
}

static __forceinline int uDwmGetTopFrameThemeState(_In_ PRWM_WINDOWDATA_VISTA_SP1 pData)
{
    if (!pData || !pData->hWnd || !pData->Window)
        return DWMTFS_ACTIVE;

    const BOOL maximized = IsZoomed(pData->hWnd) ? TRUE : FALSE;
    const BOOL active = pData->Window->IsForeground() ? TRUE : FALSE;
    if (maximized)
        return active ? DWMTFS_MAXACTIVE : DWMTFS_MAXINACTIVE;
    return active ? DWMTFS_ACTIVE : DWMTFS_INACTIVE;
}

typedef struct _UDWM_THEME_METRICS
{
    MARGINS Frame;              /* sizing margins (frame thickness) */
    int CaptionHeight;          /* caption height */
    int CornerRadius;           /* approximate radius in px */
    float ColorizationOpacity;  /* 0..1 */
} UDWM_THEME_METRICS;

static VOID uDwmQueryThemeMetrics(_In_opt_ PRWM_WINDOWDATA_VISTA_SP1 pData, _Out_ UDWM_THEME_METRICS* pOut)
{
    if (!pOut)
        return;
    RtlZeroMemory(pOut, sizeof(*pOut));

    /* Sensible defaults if theme APIs fail. */
    pOut->Frame.cxLeftWidth = GetSystemMetrics(SM_CXSIZEFRAME);
    pOut->Frame.cxRightWidth = GetSystemMetrics(SM_CXSIZEFRAME);
    pOut->Frame.cyTopHeight = GetSystemMetrics(SM_CYSIZEFRAME);
    pOut->Frame.cyBottomHeight = GetSystemMetrics(SM_CYSIZEFRAME);
    pOut->CaptionHeight = GetSystemMetrics(SM_CYCAPTION);
    pOut->CornerRadius = 6;
    pOut->ColorizationOpacity = 1.0f;

    HTHEME hTheme = uDwmGetDwmWindowTheme();
    if (!hTheme)
        return;

    /* Frame sizing margins: Vista uses TMT_SIZINGMARGINS (3601). */
    MARGINS mar = {};
    if (SUCCEEDED(GetThemeMargins(hTheme, NULL, DWMWP_TOPFRAME, 0, UDWM_TMT_SIZINGMARGINS, NULL, &mar)))
        pOut->Frame = mar;

    /* Caption height: theme-global size property. */
    int cap = 0;
    if (SUCCEEDED(GetThemeInt(hTheme, 0, 0, UDWM_TMT_CAPTIONBARHEIGHT, &cap)) && cap > 0)
        pOut->CaptionHeight = cap;

    /* Corner radius: approximate from round-corner ellipse size. */
    const int st = uDwmGetTopFrameThemeState(pData);
    int rw = 0, rh = 0;
    if (SUCCEEDED(GetThemeInt(hTheme, DWMWP_TOPFRAME, st, UDWM_TMT_ROUNDCORNERWIDTH, &rw)) &&
        SUCCEEDED(GetThemeInt(hTheme, DWMWP_TOPFRAME, st, UDWM_TMT_ROUNDCORNERHEIGHT, &rh)))
    {
        int r = rw;
        if (rh > 0 && rh < r) r = rh;
        if (r > 0)
            pOut->CornerRadius = r / 2;
    }

    /* Colorization opacity: store as int percent in the theme (2432). */
    int op = 0;
    if (SUCCEEDED(GetThemeInt(hTheme, DWMWP_TOPFRAME, st, UDWM_TMT_COLORIZATIONOPACITY, &op)))
    {
        if (op < 0) op = 0;
        if (op > 100) op = 100;
        pOut->ColorizationOpacity = (float)op / 100.0f;
    }
}

static __forceinline float uDwmSrgbToScrgb(_In_ float c)
{
    if (c <= 0.0f) return 0.0f;
    if (c >= 1.0f) return 1.0f;
    if (c <= 0.04045f)
        return c / 12.92f;
    return (float)pow((c + 0.055f) / 1.055f, 2.4f);
}

static VOID uDwmGetEffectiveClientMargins(_In_ PRWM_WINDOWDATA_VISTA_SP1 pData, _Out_ MARGINS* pMargins)
{
    if (!pMargins)
        return;

    /*
     * IMPORTANT:
     * CompositedWindow::GetClientMargins / GetClientGlassMargins are *client-area* insets
     * (e.g. DwmExtendFrameIntoClientArea), used to compute ContentRect within ClientRect.
     *
     * Do NOT mix theme frame/caption metrics into these margins; that double-counts
     * non-client and makes the composed content drift (overlap/gaps vs glass).
     */
    MARGINS m = {};
    if (pData && pData->Window)
        pData->Window->GetClientMargins(&m);

    /* Use the window-provided client margins (Vista UpdateSurfaceOffsets uses this). */
    *pMargins = m;

    /* Clamp negatives (Vista uses 0x7fffffff for "extend"; we clamp to avoid poisoning milcore). */
    if (pMargins->cxLeftWidth < 0) pMargins->cxLeftWidth = 0;
    if (pMargins->cxRightWidth < 0) pMargins->cxRightWidth = 0;
    if (pMargins->cyTopHeight < 0) pMargins->cyTopHeight = 0;
    if (pMargins->cyBottomHeight < 0) pMargins->cyBottomHeight = 0;
}

static VOID uDwmComputeNonClientMarginsFromRects(_In_ const RECT* prcWindowScreen,
                                                _In_ const RECT* prcClientScreen,
                                                _Out_ MARGINS* pMargins)
{
    if (!pMargins)
        return;
    RtlZeroMemory(pMargins, sizeof(*pMargins));
    if (!prcWindowScreen || !prcClientScreen)
        return;

    LONG l = prcClientScreen->left - prcWindowScreen->left;
    LONG t = prcClientScreen->top - prcWindowScreen->top;
    LONG r = prcWindowScreen->right - prcClientScreen->right;
    LONG b = prcWindowScreen->bottom - prcClientScreen->bottom;

    if (l < 0) l = 0;
    if (t < 0) t = 0;
    if (r < 0) r = 0;
    if (b < 0) b = 0;

    pMargins->cxLeftWidth = l;
    pMargins->cyTopHeight = t;
    pMargins->cxRightWidth = r;
    pMargins->cyBottomHeight = b;
}

static HRESULT uDwmUpdateNcColorizationResource(_In_ PRWM_WINDOWDATA_VISTA_SP1 pData)
{
    if (!pData || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !pData->hNcColorization)
        return E_UNEXPECTED;

    DWORD dwColor = 0;
    DWORD dwOpaqueBlend = 0;

    HKEY hKey = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\DWM", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        DWORD cb = sizeof(DWORD);
        (void)RegQueryValueExW(hKey, L"ColorizationColor", 0, NULL, (LPBYTE)&dwColor, &cb);
        cb = sizeof(DWORD);
        (void)RegQueryValueExW(hKey, L"ColorizationOpaqueBlend", 0, NULL, (LPBYTE)&dwOpaqueBlend, &cb);
        RegCloseKey(hKey);
    }

    /* Theme base color as fallback / blend base. */
    DWORD dwBase = 0xFF000000u;
    HTHEME hTheme = uDwmGetDwmWindowTheme();
    if (hTheme)
    {
        int base = 0;
        if (SUCCEEDED(GetThemeInt(hTheme, 0, 0, UDWM_TMT_COLORIZATIONCOLOR, &base)))
            dwBase = (DWORD)base;
    }

    /* If registry is missing, fall back entirely to theme base. */
    if (dwColor == 0)
        dwColor = dwBase;

    /* Normalize sRGB colors from 0xAARRGGBB. */
    const float baseB = (float)((dwBase >> 0) & 0xFF) / 255.0f;
    const float baseG = (float)((dwBase >> 8) & 0xFF) / 255.0f;
    const float baseR = (float)((dwBase >> 16) & 0xFF) / 255.0f;
    const float baseA = (float)((dwBase >> 24) & 0xFF) / 255.0f;

    const float srcB0 = (float)((dwColor >> 0) & 0xFF) / 255.0f;
    const float srcG0 = (float)((dwColor >> 8) & 0xFF) / 255.0f;
    const float srcR0 = (float)((dwColor >> 16) & 0xFF) / 255.0f;
    float srcA = (float)((dwColor >> 24) & 0xFF) / 255.0f;
    if (dwOpaqueBlend)
        srcA = 1.0f;

    UDWM_THEME_METRICS tm = {};
    uDwmQueryThemeMetrics(pData, &tm);
    const float opacity = tm.ColorizationOpacity;

    const float blendA = srcA * opacity;

    /* Premultiplied "over" blend, then unpremultiply (matches Vista intent). */
    const float baseRp = baseR * baseA;
    const float baseGp = baseG * baseA;
    const float baseBp = baseB * baseA;
    const float srcRp = srcR0 * blendA;
    const float srcGp = srcG0 * blendA;
    const float srcBp = srcB0 * blendA;

    const float outA = (1.0f - blendA) * baseA + blendA;
    float outR = (1.0f - blendA) * baseRp + srcRp;
    float outG = (1.0f - blendA) * baseGp + srcGp;
    float outB = (1.0f - blendA) * baseBp + srcBp;

    if (outA > 0.00001f)
    {
        outR /= outA;
        outG /= outA;
        outB /= outA;
    }
    else
    {
        outR = outG = outB = 0.0f;
    }

    typedef struct _MILCMD_COLORRESOURCE_VSP1_LOCAL
    {
        MILCMD Type;          /* 19 */
        HMIL_RESOURCE Handle; /* ColorResource */
        float Color[4];       /* scRGB: {r,g,b,a} */
    } MILCMD_COLORRESOURCE_VSP1_LOCAL;
    static_assert(sizeof(MILCMD_COLORRESOURCE_VSP1_LOCAL) == 0x18, "Unexpected ColorResource cmd size");

    MILCMD_COLORRESOURCE_VSP1_LOCAL cr = {};
    cr.Type = (MILCMD)RWM_MILCMD_VSP1_COLORRESOURCE;
    cr.Handle = (HMIL_RESOURCE)pData->hNcColorization;
    cr.Color[0] = uDwmSrgbToScrgb(outR);
    cr.Color[1] = uDwmSrgbToScrgb(outG);
    cr.Color[2] = uDwmSrgbToScrgb(outB);
    cr.Color[3] = outA;

    return MilResource_SendCommand(&cr, sizeof(cr), DwmDesktopInstance->GlobalChannel);
}

#pragma pack(push, 4)
typedef struct _MILCMD_WINDOWNODE_SETBOUNDS_VSP1
{
    MILCMD Type;          /* 58 */
    HMIL_RESOURCE Handle; /* WindowNode handle */
    RECT WindowRect;
    RECT ClientRect;
    RECT ContentRect;
} MILCMD_WINDOWNODE_SETBOUNDS_VSP1;

typedef struct _MILCMD_WINDOWNODE_UPDATESPRITEHANDLE_VSP1
{
    MILCMD Type;          /* 60 */
    HMIL_RESOURCE Handle; /* WindowNode handle */
    HSPRITE hSprite;
    UINT32 Unknown0;      /* always 0 in dwmredir */
} MILCMD_WINDOWNODE_UPDATESPRITEHANDLE_VSP1;

typedef struct _MILCMD_WINDOWNODE_SETALPHAMARGINS_VSP1
{
    MILCMD Type;          /* 67 */
    HMIL_RESOURCE Handle; /* WindowNode handle */
    MARGINS Margins;
} MILCMD_WINDOWNODE_SETALPHAMARGINS_VSP1;

typedef struct _MILCMD_WINDOWNODE_CREATE_VSP1
{
    MILCMD Type;          /* 54 */
    HMIL_RESOURCE Handle; /* WindowNode handle */
    UINT32 Sprite;        /* HSPRITE (DWORD) */
    UINT32 Unknown0;      /* always 0 in dwmredir */
    UINT32 Hwnd;          /* HWND (DWORD) */
    UINT32 Unknown1;      /* always 0 in dwmredir */
    UINT32 CachingMode;   /* MILWindowTargetCachingMode (DWORD) */
} MILCMD_WINDOWNODE_CREATE_VSP1;

typedef struct _MILCMD_TARGET_SETROOT_VSP1
{
    MILCMD Type;          /* 77 */
    HMIL_RESOURCE Handle; /* target */
    HMIL_RESOURCE hRoot;  /* root node */
} MILCMD_TARGET_SETROOT_VSP1;

typedef struct _MILCMD_WINDOWNODE_SETSPRITEIMAGE_VSP1
{
    MILCMD Type;              /* 62 */
    HMIL_RESOURCE Handle;     /* WindowNode handle */
    HMIL_RESOURCE hSpriteImage; /* typically a redirection/GDI surface handle */
} MILCMD_WINDOWNODE_SETSPRITEIMAGE_VSP1;

/*
 * Vista SP1 WindowNode UpdateProperties follow-ups (dwmredir DuceHelper::WindowNode_UpdateProperties):
 * - cmd.Type = 63, sizeof=0x0C: set DX surface handle (may be 0)
 * - cmd.Type = 65, sizeof=0x0C: set DX clip handle (may be 0)
 */
typedef struct _MILCMD_WINDOWNODE_SETDXSURFACE_VSP1
{
    MILCMD Type;          /* 63 */
    HMIL_RESOURCE Handle; /* WindowNode handle */
    HMIL_RESOURCE hDxSurface;
} MILCMD_WINDOWNODE_SETDXSURFACE_VSP1;

/*
 * Vista SP1 WindowNode sprite clip command (dwmredir DuceHelper::WindowNode_SetSpriteClip):
 * - cmd.Type = 64, sizeof=0x10
 * Layout (from dwmredir.dll.c):
 *   UINT32 Type(64), UINT32 Handle, UINT32 fSomething, UINT32 hClip
 */
typedef struct _MILCMD_WINDOWNODE_SETSPRITECLIP_VSP1
{
    MILCMD Type;              /* 64 */
    HMIL_RESOURCE Handle;     /* WindowNode handle */
    UINT32 Flags;             /* dwmredir passes !m_fUsingWindowRgn */
    HMIL_RESOURCE hClip;      /* geometry handle (PATHGEOMETRY) or 0 */
} MILCMD_WINDOWNODE_SETSPRITECLIP_VSP1;

typedef struct _MILCMD_WINDOWNODE_SETDXCLIP_VSP1
{
    MILCMD Type;          /* 65 */
    HMIL_RESOURCE Handle; /* WindowNode handle */
    HMIL_RESOURCE hDxClip;
} MILCMD_WINDOWNODE_SETDXCLIP_VSP1;

/*
 * Vista SP1 RefreshNodeProperties ends with cmd.Type=66, sizeof=0x2C.
 * It incorporates "source modifications" (flags + scRGB color) from the window context.
 */
typedef struct _MILCMD_WINDOWNODE_SETSOURCEMODS_VSP1
{
    MILCMD Type;              /* 66 */
    HMIL_RESOURCE Handle;     /* WindowNode handle */
    UINT32 SourceFlags;       /* MIL_SOURCE_FLAGS bitmask */
    float ModColor[4];        /* scRGB from GetSourceModifications() */
    float Color[4];           /* Vista copies RGB then forces A=1.0 */
} MILCMD_WINDOWNODE_SETSOURCEMODS_VSP1;
static_assert(sizeof(MILCMD_WINDOWNODE_SETSOURCEMODS_VSP1) == 0x2C, "Unexpected SetSourceMods cmd size");

typedef struct _MILCMD_PATHGEOMETRY_VSP1
{
    MILCMD Type;          /* 173 */
    HMIL_RESOURCE Handle; /* geometry */
    UINT32 Unknown0;      /* 0 */
    UINT32 Unknown1;      /* 0 */
    UINT32 cbDataSize;    /* extra data bytes */
} MILCMD_PATHGEOMETRY_VSP1;

/* Vista SP1 uDWM RenderData update command (cmd.Type = 29, sizeof=0x0c). */
typedef struct _MILCMD_RENDERDATA_VSP1
{
    MILCMD Type;          /* 29 */
    HMIL_RESOURCE Handle; /* RenderData handle */
    UINT32 cbData;        /* extra data */
} MILCMD_RENDERDATA_VSP1;

/* Vista SP1 uDWM SolidColorBrush update command (cmd.Type = 174, sizeof=0x30). */
typedef struct _MILCMD_SOLIDCOLORBRUSH_VSP1
{
    MILCMD Type;              /* 174 */
    HMIL_RESOURCE Handle;     /* SolidColorBrush handle */
    double Opacity;
    MilColorF Color;
    HMIL_RESOURCE hOpacityAnimations;
    HMIL_RESOURCE hTransform;
    HMIL_RESOURCE hRelativeTransform;
    HMIL_RESOURCE hColorAnimations;
} MILCMD_SOLIDCOLORBRUSH_VSP1;

/* Vista SP1 ColorResource update (DesktopManager::SetColorizationColorResource), cmd.Type = 19, sizeof=0x18. */
typedef struct _MILCMD_COLORRESOURCE_VSP1
{
    MILCMD Type;          /* 19 */
    HMIL_RESOURCE Handle; /* ColorResource */
    float Color[4];       /* scRGB: {r,g,b,a} */
} MILCMD_COLORRESOURCE_VSP1;

/*
 * Vista SP1 RectangleGeometry update (ResourceHelper::CreateRectangleGeometry),
 * cmd.Type = 169, sizeof=0x48.
 */
typedef struct _MILCMD_RECTANGLEGEOMETRY_VSP1
{
    MILCMD Type;          /* 169 */
    HMIL_RESOURCE Handle; /* RectangleGeometry */
    double RadiusX;
    double RadiusY;
    double X;
    double Y;
    double Width;
    double Height;
    HMIL_RESOURCE Unknown0;
    HMIL_RESOURCE Unknown1;
    HMIL_RESOURCE Unknown2;
    HMIL_RESOURCE Unknown3;
} MILCMD_RECTANGLEGEOMETRY_VSP1;

/* RenderData "draw rectangle" instruction payload (matches Vista's 111 instruction). */
typedef struct _RWM_RENDATA_DRAWRECT_VSP1
{
    UINT32 cbInstruction; /* sizeof(this) */
    UINT32 Type;          /* 111 */
    double X;
    double Y;
    double Width;
    double Height;
    UINT32 hBrush;
    UINT32 hPen;
} RWM_RENDATA_DRAWRECT_VSP1;

/*
 * Vista uDWM CDrawGlassInstruction render-data instruction.
 * From DwmReversing\\Vista\\uDWM.dll.c: cbInstruction = 44, Type = 105.
 */
typedef struct _RWM_RENDATA_DRAWGLASS_VSP1
{
    UINT32 cbInstruction; /* 44 */
    UINT32 Type;          /* 105 */
    UINT32 hGeomTop;
    UINT32 hGeomLeft;
    UINT32 hGeomRight;
    UINT32 hGeomBottom;
    float f0;
    float f1;
    float f2;
    float f3;
    UINT32 hColorization;
} RWM_RENDATA_DRAWGLASS_VSP1;

/*
 * Vista SP1 translate transform command as used by dwmredir:
 * DuceHelper::CreateTranslateTransform:
 * - CreateResource(TYPE_TRANSLATETRANSFORM=68)
 * - Send cmd.Type=163, sizeof=0x20
 * Layout (from dwmredir.dll.c):
 *   UINT32 Type(163), UINT32 Handle, double X, double Y, UINT32 unk0, UINT32 unk1
 */
typedef struct _MILCMD_TRANSLATETRANSFORM_VSP1
{
    MILCMD Type;              /* 163 */
    HMIL_RESOURCE Handle;     /* transform handle */
    double X;
    double Y;
    UINT32 Unknown0;
    UINT32 Unknown1;
} MILCMD_TRANSLATETRANSFORM_VSP1;

/* Visual_SetTransform (also used on WindowNode handles in Vista) */
typedef struct _MILCMD_VISUAL_SETTRANSFORM_VSP1
{
    MILCMD Type;              /* 35 */
    HMIL_RESOURCE Handle;     /* visual/window node */
    HMIL_RESOURCE hTransform; /* transform handle or 0 */
} MILCMD_VISUAL_SETTRANSFORM_VSP1;

/* Vista SP1 Visual_SetClip command (DuceHelper::Visual_SetClip), cmd.Type = 36, sizeof=0x0C. */
typedef struct _MILCMD_VISUAL_SETCLIP_VSP1
{
    MILCMD Type;              /* 36 */
    HMIL_RESOURCE Handle;     /* visual/window node */
    HMIL_RESOURCE hClip;      /* geometry handle or 0 */
} MILCMD_VISUAL_SETCLIP_VSP1;
#pragma pack(pop)

/* Forward decls for helpers used before their definition. */
static __forceinline VOID uDwmMarkRebuildNeeded(VOID);
static HRESULT uDwmCreateOrUpdateGeometryFromHrgn(_In_ HRGN hrgn, _Inout_ HMIL_RESOURCE* phGeometry);

static __forceinline VOID uDwmInitializeWindowTracking(VOID)
{
    if (!g_RwmWindowListInitialized)
    {
        InitializeListHead(&g_RwmWindowListHead);
        g_RwmWindowListInitialized = TRUE;
        g_RwmWindowCount = 0;
    }
}

static UINT uDwmGetInsertedChildCount(VOID)
{
    if (!g_RwmWindowListInitialized)
        return 0;

    UINT c = 0;
    for (PLIST_ENTRY e = g_RwmWindowListHead.Flink; e != &g_RwmWindowListHead; e = e->Flink)
    {
        PRWM_WINDOWDATA_VISTA_SP1 pData = CONTAINING_RECORD(e, RWM_WINDOWDATA_VISTA_SP1, ListEntry);
        if (pData && (pData->Flags & RWM_WD_VISUAL_INSERTED))
            c++;
    }
    return c;
}

static UINT uDwmGetInsertedIndexForWindow(_In_ const RWM_WINDOWDATA_VISTA_SP1* pTarget)
{
    if (!g_RwmWindowListInitialized || !pTarget)
        return 0;

    UINT idx = 0;
    for (PLIST_ENTRY e = g_RwmWindowListHead.Flink; e != &g_RwmWindowListHead; e = e->Flink)
    {
        const RWM_WINDOWDATA_VISTA_SP1* pData = CONTAINING_RECORD(e, RWM_WINDOWDATA_VISTA_SP1, ListEntry);
        if (pData == pTarget)
            break;
        if (pData && (pData->Flags & RWM_WD_VISUAL_INSERTED))
            idx++;
    }
    return idx;
}

static VOID uDwmSetDesiredVisible(PRWM_WINDOWDATA_VISTA_SP1 pData, BOOLEAN visible)
{
    if (!pData)
        return;

    pData->Flags |= RWM_WD_HAS_DESIRED_VISIBLE;
    if (visible)
        pData->Flags |= RWM_WD_DESIRED_VISIBLE;
    else
        pData->Flags &= ~RWM_WD_DESIRED_VISIBLE;
}

static BOOLEAN uDwmGetDesiredVisible(const RWM_WINDOWDATA_VISTA_SP1* pData)
{
    if (!pData)
        return FALSE;
    if (!(pData->Flags & RWM_WD_HAS_DESIRED_VISIBLE))
        return TRUE; /* default: visible until told otherwise */
    return (pData->Flags & RWM_WD_DESIRED_VISIBLE) ? TRUE : FALSE;
}

static __forceinline BOOLEAN uDwmComputeVisibleFromHwnd(_In_opt_ HWND hWnd)
{
    if (!hWnd)
        return FALSE;

    /*
     * Visibility is surprisingly tricky during startup:
     * - IsWindowVisible() can transiently return FALSE during reparenting/show sequences.
     * - GetWindowLongPtrW() can fail (returns 0) and must be error-checked.
     *
     * We bias toward "visible" when queries are unreliable to avoid the classic symptom:
     * "renders briefly, then disappears forever".
     */
    if (!IsWindow(hWnd))
        return FALSE;

    BOOLEAN visWin32 = IsWindowVisible(hWnd) ? TRUE : FALSE;

    SetLastError(0);
    const LONG_PTR style = GetWindowLongPtrW(hWnd, GWL_STYLE);
    const DWORD gle = GetLastError();

    BOOLEAN visStyle = FALSE;
    if (!(style == 0 && gle != 0))
        visStyle = (style & WS_VISIBLE) ? TRUE : FALSE;

    /* If either method says visible, treat as visible (unless minimized). */
    BOOLEAN vis = (visWin32 || visStyle) ? TRUE : FALSE;
    if (vis && IsIconic(hWnd))
        vis = FALSE;

    /*
     * If both checks say "not visible" but style query failed, keep it visible.
     * This avoids incorrect hides when GetWindowLongPtrW() is temporarily failing.
     */
    if (!vis && (style == 0 && gle != 0))
        vis = TRUE;

    return vis;
}

static __forceinline BOOLEAN uDwmIsTopmostWindow(_In_opt_ HWND hWnd)
{
    if (!hWnd || !IsWindow(hWnd))
        return FALSE;

    /* Shell tray should behave topmost for our purposes. */
    WCHAR cls[64] = {0};
    if (GetClassNameW(hWnd, cls, ARRAYSIZE(cls)) > 0)
    {
        if (_wcsicmp(cls, L"Shell_TrayWnd") == 0)
            return TRUE;
    }

    SetLastError(0);
    const LONG_PTR exStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
    const DWORD gle = GetLastError();
    if (exStyle == 0 && gle != 0)
        return FALSE; /* unknown -> not topmost */
    return (exStyle & WS_EX_TOPMOST) ? TRUE : FALSE;
}

static VOID uDwmLogWindowBrief(_In_ const char* tag, _In_opt_ HWND hWnd)
{
    if (!hWnd || !IsWindow(hWnd))
        return;

    WCHAR cls[64] = {0};
    (void)GetClassNameW(hWnd, cls, ARRAYSIZE(cls));

    /* Only spam for key shell windows (tray/taskmgr). */
    if (_wcsicmp(cls, L"Shell_TrayWnd") != 0 &&
        _wcsicmp(cls, L"TaskManagerWindow") != 0)
        return;

    /*
     * IMPORTANT:
     * Do NOT call GetWindowTextW here; it can block by sending WM_GETTEXT to a
     * potentially hung foreign-thread window, stalling the entire compositor.
     */
    WCHAR text[96] = {0};
    DWORD_PTR smto = 0;
    if (!SendMessageTimeoutW(hWnd,
                             WM_GETTEXT,
                             (WPARAM)ARRAYSIZE(text),
                             (LPARAM)text,
                             SMTO_ABORTIFHUNG | SMTO_BLOCK,
                             50,
                             &smto))
    {
        text[0] = 0;
    }

    DWORD pid = 0;
    (void)GetWindowThreadProcessId(hWnd, &pid);
    SetLastError(0);
    const LONG_PTR style = GetWindowLongPtrW(hWnd, GWL_STYLE);
    const DWORD gleStyle = GetLastError();
    SetLastError(0);
    const LONG_PTR exStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
    const DWORD gleEx = GetLastError();

    DPRINT1("%s: %s hwnd=0x%p cls=%S text=%S pid=%lu vis=%u top=%u style=0x%Ix%s ex=0x%Ix%s\n",
            tag,
            "uDWM",
            hWnd,
            cls,
            text,
            (ULONG)pid,
            (ULONG)(IsWindowVisible(hWnd) ? 1 : 0),
            (ULONG)(uDwmIsTopmostWindow(hWnd) ? 1 : 0),
            (ULONG_PTR)style,
            (style == 0 && gleStyle != 0) ? "(!)" : "",
            (ULONG_PTR)exStyle,
            (exStyle == 0 && gleEx != 0) ? "(!)" : "");
}

static __forceinline PRWM_WINDOWDATA_VISTA_SP1 uDwmFindWindowDataByHwnd(_In_opt_ HWND hWnd)
{
    if (!g_RwmWindowListInitialized || !hWnd)
        return NULL;

    for (PLIST_ENTRY e = g_RwmWindowListHead.Flink; e != &g_RwmWindowListHead; e = e->Flink)
    {
        PRWM_WINDOWDATA_VISTA_SP1 pData = CONTAINING_RECORD(e, RWM_WINDOWDATA_VISTA_SP1, ListEntry);
        if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE && pData->hWnd == hWnd)
            return pData;
    }
    return NULL;
}

static __forceinline HMIL_RESOURCE uDwmGetRootChildHandle(_In_ const RWM_WINDOWDATA_VISTA_SP1* pData)
{
    if (!pData)
        return 0;

    /*
     * DWM redirection already provides a per-window composition node (WindowNode).
     * Parent that node directly under the desktop root.
     *
     * Do not invent extra per-window "container" visuals here; those are not
     * required for correctness and can break Vista milcore batch processing.
     */
    return (HMIL_RESOURCE)pData->ClientNode;
}

static HRESULT uDwmEnsureNcSubtree(_In_ PRWM_WINDOWDATA_VISTA_SP1 pData)
{
    if (!pData || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !pData->ClientNode)
        return E_UNEXPECTED;

    HRESULT hr = S_OK;

    if (!pData->hNcVisual)
    {
        HMIL_RESOURCE hVis = 0;
        hr = MilResource_CreateOrAddRefOnChannel(DwmDesktopInstance->GlobalChannel,
                                                 (MIL_RESOURCE_TYPE)RWM_MILRT_VSP1_VISUAL,
                                                 &hVis);
        if (FAILED(hr) || !hVis)
            return FAILED(hr) ? hr : E_FAIL;
        pData->hNcVisual = hVis;
        pData->Flags &= ~RWM_WD_NC_VISUAL_INSERTED;
    }

    /*
     * Vista-like layering:
     * Attach the NC visual as a child of the WindowNode so it draws on top of the
     * redirected surface, while still using WindowNode's local coordinate space.
     */
    if (!(pData->Flags & RWM_WD_NC_VISUAL_INSERTED))
    {
        MILCMD_VISUAL_INSERTCHILDAT insertChild = {};
        insertChild.Type = (MILCMD)RWM_MILCMD_VSP1_VISUAL_INSERTCHILDAT;
        insertChild.Handle = (HMIL_RESOURCE)pData->ClientNode;
        insertChild.hChild = (HMIL_RESOURCE)pData->hNcVisual;
        /* Use a conservative index; Vista manages ordering via a richer subtree. */
        insertChild.index = 0;

        HRESULT hrIns = MilResource_SendCommand(&insertChild, sizeof(insertChild), DwmDesktopInstance->GlobalChannel);
        if (SUCCEEDED(hrIns))
            pData->Flags |= RWM_WD_NC_VISUAL_INSERTED;
        else
            DPRINT1("uDwmEnsureNcSubtree: InsertChildAt failed hr=0x%08lx node=0x%lx nc=0x%lx hwnd=0x%p\n",
                    hrIns, (ULONG)pData->ClientNode, (ULONG)pData->hNcVisual, pData->hWnd);
    }

    if (!pData->hNcRenderData)
    {
        HMIL_RESOURCE hRD = 0;
        hr = MilResource_CreateOrAddRefOnChannel(DwmDesktopInstance->GlobalChannel,
                                                 (MIL_RESOURCE_TYPE)RWM_MILRT_VSP1_RENDERDATA,
                                                 &hRD);
        if (FAILED(hr) || !hRD)
            return FAILED(hr) ? hr : E_FAIL;

        pData->hNcRenderData = hRD;

        /* Bind render data to NC visual. */
        MILCMD_VISUAL_SETCONTENT setContent = {};
        setContent.Type = (MILCMD)RWM_MILCMD_VSP1_VISUAL_SETCONTENT;
        setContent.Handle = (HMIL_RESOURCE)pData->hNcVisual;
        setContent.hContent = (HMIL_RESOURCE)pData->hNcRenderData;
        (void)MilResource_SendCommand(&setContent, sizeof(setContent), DwmDesktopInstance->GlobalChannel);
    }

    /* Create brushes (caption + border). */
    if (!pData->hNcCaptionBrush)
    {
        (void)MilResource_CreateOrAddRefOnChannel(DwmDesktopInstance->GlobalChannel,
                                                  (MIL_RESOURCE_TYPE)RWM_MILRT_VSP1_SOLIDCOLORBRUSH,
                                                  &pData->hNcCaptionBrush);
    }
    if (!pData->hNcBorderBrush)
    {
        (void)MilResource_CreateOrAddRefOnChannel(DwmDesktopInstance->GlobalChannel,
                                                  (MIL_RESOURCE_TYPE)RWM_MILRT_VSP1_SOLIDCOLORBRUSH,
                                                  &pData->hNcBorderBrush);
    }

    if (pData->hNcCaptionBrush)
    {
        MILCMD_SOLIDCOLORBRUSH_VSP1 br = {};
        br.Type = (MILCMD)RWM_MILCMD_VSP1_SOLIDCOLORBRUSH;
        br.Handle = pData->hNcCaptionBrush;
        br.Opacity = 0.75;
        br.Color = {0.15f, 0.05f, 0.40f, 0.85f}; /* A,R,G,B-ish; good enough for now */
        br.hOpacityAnimations = 0;
        br.hTransform = 0;
        br.hRelativeTransform = 0;
        br.hColorAnimations = 0;
        (void)MilResource_SendCommand(&br, sizeof(br), DwmDesktopInstance->GlobalChannel);
    }
    if (pData->hNcBorderBrush)
    {
        MILCMD_SOLIDCOLORBRUSH_VSP1 br = {};
        br.Type = (MILCMD)RWM_MILCMD_VSP1_SOLIDCOLORBRUSH;
        br.Handle = pData->hNcBorderBrush;
        br.Opacity = 0.85;
        br.Color = {0.85f, 0.00f, 0.00f, 0.00f};
        br.hOpacityAnimations = 0;
        br.hTransform = 0;
        br.hRelativeTransform = 0;
        br.hColorAnimations = 0;
        (void)MilResource_SendCommand(&br, sizeof(br), DwmDesktopInstance->GlobalChannel);
    }

    /* Ensure glass geometry resources (Vista: HRGN->Geometry => PathGeometry) and colorization resource exist. */
    if (!pData->hNcGlassGeomTop)
        (void)MilResource_CreateOrAddRefOnChannel(DwmDesktopInstance->GlobalChannel,
                                                  (MIL_RESOURCE_TYPE)RWM_MILRT_VSP1_PATHGEOMETRY,
                                                  &pData->hNcGlassGeomTop);
    if (!pData->hNcGlassGeomLeft)
        (void)MilResource_CreateOrAddRefOnChannel(DwmDesktopInstance->GlobalChannel,
                                                  (MIL_RESOURCE_TYPE)RWM_MILRT_VSP1_PATHGEOMETRY,
                                                  &pData->hNcGlassGeomLeft);
    if (!pData->hNcGlassGeomRight)
        (void)MilResource_CreateOrAddRefOnChannel(DwmDesktopInstance->GlobalChannel,
                                                  (MIL_RESOURCE_TYPE)RWM_MILRT_VSP1_PATHGEOMETRY,
                                                  &pData->hNcGlassGeomRight);
    if (!pData->hNcGlassGeomBottom)
        (void)MilResource_CreateOrAddRefOnChannel(DwmDesktopInstance->GlobalChannel,
                                                  (MIL_RESOURCE_TYPE)RWM_MILRT_VSP1_PATHGEOMETRY,
                                                  &pData->hNcGlassGeomBottom);
    if (!pData->hNcColorization)
        (void)MilResource_CreateOrAddRefOnChannel(DwmDesktopInstance->GlobalChannel,
                                                  (MIL_RESOURCE_TYPE)RWM_MILRT_VSP1_COLORRESOURCE,
                                                  &pData->hNcColorization);

    if (pData->hNcColorization)
    {
        (void)uDwmUpdateNcColorizationResource(pData);
    }

    return S_OK;
}

static HRESULT uDwmUpdateNcGlassRenderData(_In_ PRWM_WINDOWDATA_VISTA_SP1 pData)
{
    if (!pData || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel)
        return E_UNEXPECTED;
    if (!pData->hNcVisual || !pData->hNcRenderData)
        return S_FALSE;

    const LONG w = pData->WindowRect.right - pData->WindowRect.left;
    const LONG h = pData->WindowRect.bottom - pData->WindowRect.top;
    if (w <= 0 || h <= 0)
        return S_FALSE;

    if (!pData->hNcGlassGeomTop || !pData->hNcColorization)
        return S_FALSE;

    /*
     * Vista: DrawGlass geometry is built in SCREEN space, matching WindowNode_SetBounds.
     * Use cached screen rects from WindowNode_SetBounds for consistency.
     */
    const RECT rcWindow = pData->WindowRect;
    const RECT rcClient = pData->ClientMarginsRect;
    const RECT rcContent = pData->ContentRectLocal;

    auto updateBandGeomFromHrgn = [&](HMIL_RESOURCE* phGeom, HRGN hrgnBand, HRGN hrgnOuter) -> HRESULT
    {
        if (!phGeom || !*phGeom || !hrgnBand)
            return E_INVALIDARG;

        if (hrgnOuter)
            (void)CombineRgn(hrgnBand, hrgnBand, hrgnOuter, RGN_AND);

        return uDwmCreateOrUpdateGeometryFromHrgn(hrgnBand, phGeom);
    };

    /*
     * Match Vista's intent:
     * - Border bands are defined by the CLIENT rect boundary (non-client outside client).
     * - The TOP band can extend down to ContentRect.top so the "glass strip" between
     *   caption and content is included (Aero look).
     */
    const LONG topBandBottom = (rcContent.top > rcClient.top) ? rcContent.top : rcClient.top;
    const RECT rcTop = { rcWindow.left, rcWindow.top, rcWindow.right, topBandBottom };
    const RECT rcLeft = { rcWindow.left, rcWindow.top, rcClient.left, rcWindow.bottom };
    const RECT rcRight = { rcClient.right, rcWindow.top, rcWindow.right, rcWindow.bottom };
    const RECT rcBottom = { rcWindow.left, rcClient.bottom, rcWindow.right, rcWindow.bottom };

    const BOOL maximized = (pData->hWnd && IsZoomed(pData->hWnd)) ? TRUE : FALSE;
    UDWM_THEME_METRICS tm = {};
    uDwmQueryThemeMetrics(pData, &tm);
    const int radius = maximized ? 0 : tm.CornerRadius;
    HRGN hrgnOuter = (radius > 0) ? CreateRoundRectRgn(rcWindow.left, rcWindow.top, rcWindow.right, rcWindow.bottom, radius, radius)
                                  : CreateRectRgn(rcWindow.left, rcWindow.top, rcWindow.right, rcWindow.bottom);
    HRGN hrgnTop = CreateRectRgn(rcTop.left, rcTop.top, rcTop.right, rcTop.bottom);
    HRGN hrgnLeft = CreateRectRgn(rcLeft.left, rcLeft.top, rcLeft.right, rcLeft.bottom);
    HRGN hrgnRight = CreateRectRgn(rcRight.left, rcRight.top, rcRight.right, rcRight.bottom);
    HRGN hrgnBottom = CreateRectRgn(rcBottom.left, rcBottom.top, rcBottom.right, rcBottom.bottom);

    if (hrgnOuter && hrgnTop && hrgnLeft && hrgnRight && hrgnBottom)
    {
        (void)updateBandGeomFromHrgn(&pData->hNcGlassGeomTop, hrgnTop, hrgnOuter);
        (void)updateBandGeomFromHrgn(&pData->hNcGlassGeomLeft, hrgnLeft, hrgnOuter);
        (void)updateBandGeomFromHrgn(&pData->hNcGlassGeomRight, hrgnRight, hrgnOuter);
        (void)updateBandGeomFromHrgn(&pData->hNcGlassGeomBottom, hrgnBottom, hrgnOuter);
    }

    if (hrgnTop) DeleteObject(hrgnTop);
    if (hrgnLeft) DeleteObject(hrgnLeft);
    if (hrgnRight) DeleteObject(hrgnRight);
    if (hrgnBottom) DeleteObject(hrgnBottom);
    if (hrgnOuter) DeleteObject(hrgnOuter);

    /*
     * Do NOT clip the glass visual to only the top band.
     * Vista's DrawGlass instruction is parameterized by 4 geometries (top/left/right/bottom).
     * Clipping here breaks left/right/bottom glass rendering and makes the frame look wrong.
     */

    const UINT32 cbData = sizeof(RWM_RENDATA_DRAWGLASS_VSP1);
    RWM_RENDATA_DRAWGLASS_VSP1 instr = {};
    instr.cbInstruction = sizeof(RWM_RENDATA_DRAWGLASS_VSP1);
    instr.Type = 105;
    instr.hGeomTop = (UINT32)pData->hNcGlassGeomTop;
    instr.hGeomLeft = (UINT32)pData->hNcGlassGeomLeft;
    instr.hGeomRight = (UINT32)pData->hNcGlassGeomRight;
    instr.hGeomBottom = (UINT32)pData->hNcGlassGeomBottom;
    instr.f0 = 0.0f;
    instr.f1 = 0.0f;
    instr.f2 = 0.0f;
    instr.f3 = 0.0f;
    instr.hColorization = (UINT32)pData->hNcColorization;

    MILCMD_RENDERDATA_VSP1 cmd = {};
    cmd.Type = (MILCMD)RWM_MILCMD_VSP1_RENDERDATA;
    cmd.Handle = (HMIL_RESOURCE)pData->hNcRenderData;
    cmd.cbData = cbData;

    HRESULT hr = MilChannel_BeginCommand(DwmDesktopInstance->GlobalChannel, &cmd, sizeof(cmd), cbData);
    if (SUCCEEDED(hr))
    {
        hr = MilChannel_AppendCommandData(DwmDesktopInstance->GlobalChannel, &instr, cbData);
        (void)MilChannel_EndCommand(DwmDesktopInstance->GlobalChannel);
    }
    else
    {
        DPRINT1("uDwmUpdateNcGlassRenderData: BeginCommand failed hr=0x%08lx cbData=%u hwnd=0x%p\n",
                hr, cbData, pData->hWnd);
    }

    return hr;
}

static HRESULT uDwmInsertChildAtRoot(_In_ PRWM_WINDOWDATA_VISTA_SP1 pData, _Inout_ UINT* pIndex)
{
    if (!pData || !pIndex || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !DwmDesktopInstance->hRootNode)
        return E_UNEXPECTED;

    HMIL_RESOURCE hChild = uDwmGetRootChildHandle(pData);
    if (!hChild)
        return E_INVALIDARG;
    if (hChild == (HMIL_RESOURCE)DwmDesktopInstance->hRootNode)
        return S_FALSE;

    MILCMD_VISUAL_INSERTCHILDAT insertChild = {};
    insertChild.Type = (MILCMD)RWM_MILCMD_VSP1_VISUAL_INSERTCHILDAT;
    insertChild.Handle = (HMIL_RESOURCE)DwmDesktopInstance->hRootNode;
    insertChild.hChild = hChild;
    insertChild.index = *pIndex;

    HRESULT hrIns = MilResource_SendCommand(&insertChild, sizeof(insertChild), DwmDesktopInstance->GlobalChannel);
    if (SUCCEEDED(hrIns))
    {
        pData->Flags |= RWM_WD_VISUAL_INSERTED;
        (*pIndex)++;
    }
    else
    {
        DPRINT1("uDwmRebuildRootChildren: InsertChildAt failed hr=0x%08lx root=0x%lx child=0x%lx idx=%u hwnd=0x%p\n",
                hrIns,
                (ULONG)DwmDesktopInstance->hRootNode,
                (ULONG)hChild,
                insertChild.index,
                pData->hWnd);
    }
    return hrIns;
}

static __forceinline BOOLEAN uDwmNeedsExampleTitlebar(_In_ const RWM_WINDOWDATA_VISTA_SP1* pData)
{
    if (!pData || !pData->hWnd)
        return FALSE;
    if (pData->hWnd == GetDesktopWindow())
        return FALSE;

    const LONG_PTR style = GetWindowLongPtrW(pData->hWnd, GWL_STYLE);
    return ((style & WS_CAPTION) != 0) ? TRUE : FALSE;
}

/*
 * Reference behavior (Vista uDWM): Z-order and show/hide are modeled on a higher-level
 * VisualCollection and become MIL commands during RenderRecursive().
 *
 * Our uDWM implementation talks directly to the MIL channel. To avoid "remove without
 * successful reinsert" scenarios (which present as "shows briefly then disappears forever"),
 * we rebuild the root's child list in UpdateScene whenever the logical order/visibility changes.
 */
static __forceinline VOID uDwmMarkRebuildNeeded(VOID)
{
    g_RwmRebuildChildren = TRUE;
}

static VOID uDwmRebuildRootChildren(VOID)
{
    if (!g_RwmWindowListInitialized || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !DwmDesktopInstance->hRootNode)
        return;

    if (!g_RwmRebuildChildren)
        return;

    UINT cVisible = 0;
    const UINT cTotal = g_RwmWindowCount;

    /*
     * Pass 1: clear the root child collection.
     *
     * IMPORTANT:
     * During heavy z-order churn (Start menu, task switching), issuing dozens of
     * RemoveChild calls can desync state and trigger Vista milcore batch failures.
     * Prefer a single RemoveAllChildren on the root, then rebuild deterministically.
     */
    {
        MILCMD_VISUAL_REMOVEALLCHILDREN clear = {};
        clear.Type = (MILCMD)RWM_MILCMD_VSP1_VISUAL_REMOVEALLCHILDREN;
        clear.Handle = (HMIL_RESOURCE)DwmDesktopInstance->hRootNode;
        HRESULT hrClear = MilResource_SendCommand(&clear, sizeof(clear), DwmDesktopInstance->GlobalChannel);
        if (FAILED(hrClear))
        {
            DPRINT1("uDwmRebuildRootChildren: RemoveAllChildren failed hr=0x%08lx root=0x%lx\n",
                    hrClear, (ULONG)DwmDesktopInstance->hRootNode);
        }
    }

    /* Reset local insertion flags after clearing root. */
    for (PLIST_ENTRY e = g_RwmWindowListHead.Flink; e != &g_RwmWindowListHead; e = e->Flink)
    {
        PRWM_WINDOWDATA_VISTA_SP1 pData = CONTAINING_RECORD(e, RWM_WINDOWDATA_VISTA_SP1, ListEntry);
        if (!pData || pData->Signature != RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
            continue;
        /* Root child collection was cleared. */
        pData->Flags &= ~RWM_WD_VISUAL_INSERTED;
    }

    /*
     * Pass 2: refresh visibility + insert visible windows.
     *
     * Vista reference:
     * - `CWindowList::ZOrder` maintains an internal list and applies `ZOrderAfter` to the root visual's
     *   VisualCollection (see `DwmReversing\\Vista\\uDWM.dll.c`, `CWindowList::ZOrder`).
     *
     * Our uDWM talks to milcore directly (InsertChildAt/RemoveChild), which makes incremental reorder fragile
     * early-on. For correctness, rebuild the root's children in *actual Win32 z-order* so that topmost/bands
     * are honored deterministically (this matches what Vista ultimately composes from).
     */
    UINT idx = 0;

    /* Refresh desired visibility for all tracked windows up-front. */
    for (PLIST_ENTRY e = g_RwmWindowListHead.Flink; e != &g_RwmWindowListHead; e = e->Flink)
    {
        PRWM_WINDOWDATA_VISTA_SP1 pData = CONTAINING_RECORD(e, RWM_WINDOWDATA_VISTA_SP1, ListEntry);
        if (!pData || pData->Signature != RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
            continue;
        if (pData->hWnd)
        {
            uDwmSetDesiredVisible(pData, uDwmComputeVisibleFromHwnd(pData->hWnd));
            uDwmLogWindowBrief("rebuild", pData->hWnd);
        }
    }

    /* Insert tracked windows in actual Win32 z-order (bottom -> top). */
    UINT cZ = 0;
    for (HWND w = GetTopWindow(NULL); w; w = GetWindow(w, GW_HWNDNEXT))
        cZ++;

    if (cZ)
    {
        HWND* z = (HWND*)HeapAlloc(GetProcessHeap(), 0, sizeof(HWND) * cZ);
        if (z)
        {
            UINT k = 0;
            for (HWND w = GetTopWindow(NULL); w && k < cZ; w = GetWindow(w, GW_HWNDNEXT))
                z[k++] = w;

            while (k)
            {
                HWND w = z[--k]; /* bottom -> top */
                PRWM_WINDOWDATA_VISTA_SP1 pData = uDwmFindWindowDataByHwnd(w);
                if (!pData)
                    continue;
                if (!uDwmGetDesiredVisible(pData))
                    continue;
                if (!uDwmGetRootChildHandle(pData))
                    continue;
                if (pData->Flags & RWM_WD_VISUAL_INSERTED)
                    continue;

                cVisible++;
                (void)uDwmInsertChildAtRoot(pData, &idx);
            }

            HeapFree(GetProcessHeap(), 0, z);
        }
    }

    /* Fallback: insert any remaining visible tracked windows not present in z-order enumeration. */
    for (PLIST_ENTRY e = g_RwmWindowListHead.Flink; e != &g_RwmWindowListHead; e = e->Flink)
    {
        PRWM_WINDOWDATA_VISTA_SP1 pData = CONTAINING_RECORD(e, RWM_WINDOWDATA_VISTA_SP1, ListEntry);
        if (!pData || pData->Signature != RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
            continue;
        if (!uDwmGetDesiredVisible(pData) || !uDwmGetRootChildHandle(pData))
            continue;
        if (pData->Flags & RWM_WD_VISUAL_INSERTED)
            continue;

        cVisible++;
        (void)uDwmInsertChildAtRoot(pData, &idx);
    }

    /*
     * Breadcrumb (rate limited): if everything disappears, this tells us whether
     * we're rebuilding to zero children due to visibility state.
     */
    static ULONG s_rebuildPrint = 0;
    if ((++s_rebuildPrint % 60) == 1 || idx == 0)
    {
        DPRINT1("uDwmRebuildRootChildren: root=0x%lx inserted=%u visible=%u total=%u\n",
                (ULONG)DwmDesktopInstance->hRootNode, idx, cVisible, cTotal);
    }

    g_RwmRebuildChildren = FALSE;
}

static HRESULT uDwmWindowNode_SetBounds(PRWM_WINDOWDATA_VISTA_SP1 pData)
{
    if (!pData || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !pData->ClientNode || !pData->hWnd)
        return E_UNEXPECTED;

    RECT rcWindowScreen = {};
    RECT rcClientScreen = {};
    RECT rcContentScreen = {};

    /*
     * Root safety:
     * When the render-target root is the DesktopWindow clone, never allow an empty/odd
     * DesktopWindow rect to clip the entire subtree (classic "everything appears briefly then vanishes").
     */
    if (DwmDesktopInstance->RootIsDesktopClone &&
        pData->hWnd == GetDesktopWindow() &&
        (HMIL_RESOURCE)pData->ClientNode == (HMIL_RESOURCE)DwmDesktopInstance->hRootNode)
    {
        /*
         * Vista uses desktop/virtual-screen coordinates for the desktop root.
         * Use virtual screen origin + size, not just primary monitor 0..CX/CY.
         */
        const LONG vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
        const LONG vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
        const LONG vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        const LONG vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

        rcWindowScreen.left = vx;
        rcWindowScreen.top = vy;
        rcWindowScreen.right = vx + ((vw > 0) ? vw : 0);
        rcWindowScreen.bottom = vy + ((vh > 0) ? vh : 0);

        rcClientScreen = rcWindowScreen;
        rcContentScreen = rcWindowScreen;
    }
    else
    {
        if (!GetWindowRect(pData->hWnd, &rcWindowScreen))
            return HRESULT_FROM_WIN32(GetLastError());

        RECT rcClientWin = {};
        if (!GetClientRect(pData->hWnd, &rcClientWin))
            return HRESULT_FROM_WIN32(GetLastError());

        /*
         * Vista dwmredir:
         * - WindowRect / ClientRect / ContentRect are all in SCREEN coordinates
         * - Top-level WindowNodes typically use identity transforms
         */
        POINT ptTL = { 0, 0 };
        POINT ptBR = { rcClientWin.right, rcClientWin.bottom };
        (void)ClientToScreen(pData->hWnd, &ptTL);
        (void)ClientToScreen(pData->hWnd, &ptBR);

        rcClientScreen.left = ptTL.x;
        rcClientScreen.top = ptTL.y;
        rcClientScreen.right = ptBR.x;
        rcClientScreen.bottom = ptBR.y;

        /*
         * Vista's CMilWindowContext::GetContentRect():
         * it starts from the client rect and applies per-window content insets.
         */
        rcContentScreen = rcClientScreen;
        if (pData->Window)
        {
            MARGINS m = {};
            uDwmGetEffectiveClientMargins(pData, &m);

            rcContentScreen.left += m.cxLeftWidth;
            rcContentScreen.right -= m.cxRightWidth;
            rcContentScreen.top += m.cyTopHeight;
            rcContentScreen.bottom -= m.cyBottomHeight;

            /* Clamp to non-inverted rect. */
            if (rcContentScreen.right < rcContentScreen.left)
                rcContentScreen.right = rcContentScreen.left;
            if (rcContentScreen.bottom < rcContentScreen.top)
                rcContentScreen.bottom = rcContentScreen.top;
        }
    }

    /* Cache rects (screen-space, Vista-like). */
    pData->WindowRect = rcWindowScreen;
    pData->ClientMarginsRect = rcClientScreen;
    pData->ContentRectLocal = rcContentScreen;

    MILCMD_WINDOWNODE_SETBOUNDS_VSP1 cmd = {};
    cmd.Type = (MILCMD)RWM_MILCMD_VSP1_WINDOWNODE_SETBOUNDS;
    cmd.Handle = (HMIL_RESOURCE)pData->ClientNode;
    cmd.WindowRect = rcWindowScreen;
    cmd.ClientRect = rcClientScreen;
    cmd.ContentRect = rcContentScreen;

    HRESULT hr = MilResource_SendCommand(&cmd, sizeof(cmd), DwmDesktopInstance->GlobalChannel);
    if (FAILED(hr))
        DPRINT1("uDwmWindowNode_SetBounds failed hr=0x%08lx node=0x%lx hwnd=0x%p\n", hr, pData->ClientNode, pData->hWnd);
    return hr;
}

static HRESULT uDwmWindowNode_UpdateSpriteHandle(PRWM_WINDOWDATA_VISTA_SP1 pData, HSPRITE hSprite)
{
    if (!pData || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !pData->ClientNode)
        return E_UNEXPECTED;

    MILCMD_WINDOWNODE_UPDATESPRITEHANDLE_VSP1 cmd = {};
    cmd.Type = (MILCMD)RWM_MILCMD_VSP1_WINDOWNODE_UPDATESPRITE;
    cmd.Handle = (HMIL_RESOURCE)pData->ClientNode;
    cmd.hSprite = hSprite;
    cmd.Unknown0 = 0;

    HRESULT hr = MilResource_SendCommand(&cmd, sizeof(cmd), DwmDesktopInstance->GlobalChannel);
    if (FAILED(hr))
        DPRINT1("uDwmWindowNode_UpdateSpriteHandle failed hr=0x%08lx node=0x%lx\n", hr, pData->ClientNode);
    return hr;
}

static HRESULT uDwmWindowNode_SetAlphaMargins(PRWM_WINDOWDATA_VISTA_SP1 pData)
{
    if (!pData || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !pData->ClientNode || !pData->Window)
        return E_UNEXPECTED;

    /*
     * Vista's alpha inset comes from window context state (not theme frame metrics).
     * We approximate with the window-provided client margins (same as ContentRect insets),
     * which keeps sprite/content alignment stable.
     */
    MARGINS m = {};
    uDwmGetEffectiveClientMargins(pData, &m);
    /* Sanitize: avoid negative/oversized margins poisoning milcore. */
    const LONG w = pData->WindowRect.right - pData->WindowRect.left;
    const LONG h = pData->WindowRect.bottom - pData->WindowRect.top;
    if (m.cxLeftWidth < 0) m.cxLeftWidth = 0;
    if (m.cxRightWidth < 0) m.cxRightWidth = 0;
    if (m.cyTopHeight < 0) m.cyTopHeight = 0;
    if (m.cyBottomHeight < 0) m.cyBottomHeight = 0;
    if (w > 0)
    {
        if (m.cxLeftWidth > w) m.cxLeftWidth = w;
        if (m.cxRightWidth > w) m.cxRightWidth = w;
        if (m.cxLeftWidth + m.cxRightWidth > w)
        {
            const LONG half = w / 2;
            if (m.cxLeftWidth > half) m.cxLeftWidth = half;
            if (m.cxRightWidth > w - m.cxLeftWidth) m.cxRightWidth = w - m.cxLeftWidth;
        }
    }
    if (h > 0)
    {
        if (m.cyTopHeight > h) m.cyTopHeight = h;
        if (m.cyBottomHeight > h) m.cyBottomHeight = h;
        if (m.cyTopHeight + m.cyBottomHeight > h)
        {
            const LONG half = h / 2;
            if (m.cyTopHeight > half) m.cyTopHeight = half;
            if (m.cyBottomHeight > h - m.cyTopHeight) m.cyBottomHeight = h - m.cyTopHeight;
        }
    }

    MILCMD_WINDOWNODE_SETALPHAMARGINS_VSP1 cmd = {};
    cmd.Type = (MILCMD)RWM_MILCMD_VSP1_WINDOWNODE_SETALPHAMARGINS;
    cmd.Handle = (HMIL_RESOURCE)pData->ClientNode;
    cmd.Margins = m;

    HRESULT hr = MilResource_SendCommand(&cmd, sizeof(cmd), DwmDesktopInstance->GlobalChannel);
    if (FAILED(hr))
        DPRINT1("uDwmWindowNode_SetAlphaMargins failed hr=0x%08lx node=0x%lx\n", hr, pData->ClientNode);
    return hr;
}

static HRESULT uDwmWindowNode_SetSpriteImage(PRWM_WINDOWDATA_VISTA_SP1 pData, UINT32 hSurface)
{
    if (!pData || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !pData->ClientNode)
        return E_UNEXPECTED;

    /* Cache last surface to avoid redundant commands. */
    if (pData->LastSpriteImageSurface == hSurface)
        return S_OK;

    MILCMD_WINDOWNODE_SETSPRITEIMAGE_VSP1 cmd = {};
    cmd.Type = (MILCMD)RWM_MILCMD_VSP1_WINDOWNODE_SETSPRITEIMAGE;
    cmd.Handle = (HMIL_RESOURCE)pData->ClientNode;
    cmd.hSpriteImage = (HMIL_RESOURCE)hSurface;

    HRESULT hr = MilResource_SendCommand(&cmd, sizeof(cmd), DwmDesktopInstance->GlobalChannel);
    if (SUCCEEDED(hr))
        pData->LastSpriteImageSurface = hSurface;
    else
        DPRINT1("uDwmWindowNode_SetSpriteImage failed hr=0x%08lx node=0x%lx surf=0x%lx\n",
                hr, (ULONG)pData->ClientNode, (ULONG)hSurface);
    return hr;
}

static HRESULT uDwmWindowNode_UpdateProperties(PRWM_WINDOWDATA_VISTA_SP1 pData, UINT32 hRedirectionSurface)
{
    if (!pData || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !pData->ClientNode)
        return E_UNEXPECTED;

    /*
     * Vista DuceHelper::WindowNode_UpdateProperties:
     * - WindowNode_SetSpriteImage(pChannel, hWindowNode, hRedirectionSurface)
     * - cmd 63: set DX surface handle (a4)
     * - cmd 65: set DX clip handle (a5)
     */
    HRESULT hr = uDwmWindowNode_SetSpriteImage(pData, hRedirectionSurface);
    if (FAILED(hr))
        return hr;

    /*
     * On real Vista SP1, the IDwmWindow we receive is the dwmredir window context.
     * It *does* have distinct DX surface + DX clip handles, but they are not exposed
     * through the public-ish IDwmWindow/CompositedWindow vtable in our header.
     *
     * We are explicitly testing uDWM against Vista SP1's dwmredir+milcore, so read the
     * known fields directly (from DwmReversing\\Vista\\dwmredir.dll.c):
     *  - *((DWORD*)this + 32) = m_hDxSurface
     *  - *((DWORD*)this + 60) = m_hLastValidDxSurface (preferred when present)
     *  - *((DWORD*)this + 44) = m_hDxClip
     *
     * Guard with SEH so we don't crash if the object isn't the expected type.
     */
    UINT32 hDxSurface = 0;
    UINT32 hDxClip = 0;
    if (pData->Window && pData->Window->HasDXContent())
    {
        __try
        {
            /*
             * Vista dwmredir object layout nuance:
             * The IDwmWindow interface is an embedded base subobject at +8 bytes inside
             * CMilWindowContext (see dwmredir.dll.c: calls use (char*)this + 8, and vptr at this+2).
             * Our CompositedWindow* points at that embedded base, so subtract 8 to get the real
             * CMilWindowContext base before applying the field indices from the reference.
             */
            const BYTE* pBase = ((const BYTE*)pData->Window) - 8;
            const UINT32* p = (const UINT32*)pBase;
            const UINT32 dx = p[32];    /* m_hDxSurface */
            const UINT32 last = p[60];  /* m_hLastValidDxSurface */
            const UINT32 clip = p[44];  /* m_hDxClip */
            hDxSurface = last ? last : dx;
            hDxClip = clip;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            hDxSurface = 0;
            hDxClip = 0;
        }

        /* Fallback: if dwmredir doesn't expose the DX handle (unexpected), reuse redirection surface. */
        if (!hDxSurface)
            hDxSurface = hRedirectionSurface;
    }

    MILCMD_WINDOWNODE_SETDXSURFACE_VSP1 dxs = {};
    dxs.Type = (MILCMD)RWM_MILCMD_VSP1_WINDOWNODE_SETDXSURFACE;
    dxs.Handle = (HMIL_RESOURCE)pData->ClientNode;
    dxs.hDxSurface = (HMIL_RESOURCE)hDxSurface;
    (void)MilResource_SendCommand(&dxs, sizeof(dxs), DwmDesktopInstance->GlobalChannel);

    MILCMD_WINDOWNODE_SETDXCLIP_VSP1 dxc = {};
    dxc.Type = (MILCMD)RWM_MILCMD_VSP1_WINDOWNODE_SETDXCLIP;
    dxc.Handle = (HMIL_RESOURCE)pData->ClientNode;
    dxc.hDxClip = (HMIL_RESOURCE)hDxClip;
    (void)MilResource_SendCommand(&dxc, sizeof(dxc), DwmDesktopInstance->GlobalChannel);

    /*
     * Vista RefreshNodeProperties ends with cmd 66 (size 0x2C) which encodes "source
     * modifications" (flags + scRGB). The IDwmWindow method is at vtable slot 26 (offset 104)
     * with signature: GetSourceModifications(DWORD* pFlags, float* pScRgb4).
     *
     * Our local CompositedWindow.hpp signature doesn't match Vista; call via vtable.
     */
    if (pData->Window)
    {
        typedef int(__thiscall* PFN_GETSOURCEMODS)(void* self, DWORD* pFlags, float* pScRgb4);
        PFN_GETSOURCEMODS pfn = NULL;

        __try
        {
            void** vtbl = *(void***)pData->Window;
            pfn = (PFN_GETSOURCEMODS)vtbl[26];
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            pfn = NULL;
        }

        if (pfn)
        {
            DWORD flags = 0;
            float sc[4] = { 0, 0, 0, 0 };
            __try
            {
                (void)pfn((void*)pData->Window, &flags, sc);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                flags = 0;
                sc[0] = sc[1] = sc[2] = sc[3] = 0.0f;
            }

            MILCMD_WINDOWNODE_SETSOURCEMODS_VSP1 sm = {};
            sm.Type = (MILCMD)RWM_MILCMD_VSP1_WINDOWNODE_SETSOURCEMODS;
            sm.Handle = (HMIL_RESOURCE)pData->ClientNode;
            sm.SourceFlags = (UINT32)flags;
            sm.ModColor[0] = sc[0];
            sm.ModColor[1] = sc[1];
            sm.ModColor[2] = sc[2];
            sm.ModColor[3] = sc[3];
            sm.Color[0] = sc[0];
            sm.Color[1] = sc[1];
            sm.Color[2] = sc[2];
            sm.Color[3] = 1.0f; /* Vista forces alpha to 1.0 here */
            (void)MilResource_SendCommand(&sm, sizeof(sm), DwmDesktopInstance->GlobalChannel);
        }
    }

    return S_OK;
}

static HRESULT uDwmTryAttachClientSurface(PRWM_WINDOWDATA_VISTA_SP1 pData)
{
    if (!pData || !pData->Window || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !pData->ClientNode)
        return E_UNEXPECTED;

    /* dwmredir uses UpdateSurfaceOffsets() to keep surface margins in sync with NC/client. */
    typedef struct _MILCMD_WNREDIR_UPDATEMARGINS_VSP1
    {
        MILCMD Type;
        HMIL_RESOURCE Handle;
        MARGINS Margins;
    } MILCMD_WNREDIR_UPDATEMARGINS_VSP1;
    static_assert(sizeof(MILCMD_WNREDIR_UPDATEMARGINS_VSP1) == 0x18, "Unexpected UpdateMargins cmd size");

    UINT32 hSurface = 0;
    HRESULT hrSurf = pData->Window->GetGDISurface(DwmDesktopInstance->GlobalChannel, &hSurface);
    if (FAILED(hrSurf) || !hSurface)
        return FAILED(hrSurf) ? hrSurf : S_FALSE;

    /* Best-effort: update redirection surface margins to match current client margins. */
    if (pData->Window)
    {
        /*
         * Vista UpdateSurfaceOffsets passes CMilWindowContext::GetClientMargins() to
         * WindowRedirection_*_UpdateMargins. These are client-area content insets.
         */
        MARGINS m = {};
        uDwmGetEffectiveClientMargins(pData, &m);
        /* Sanitize for UpdateMargins; Vista expects non-negative and reasonable values. */
        const LONG w = pData->WindowRect.right - pData->WindowRect.left;
        const LONG h = pData->WindowRect.bottom - pData->WindowRect.top;
        if (m.cxLeftWidth < 0) m.cxLeftWidth = 0;
        if (m.cxRightWidth < 0) m.cxRightWidth = 0;
        if (m.cyTopHeight < 0) m.cyTopHeight = 0;
        if (m.cyBottomHeight < 0) m.cyBottomHeight = 0;
        if (w > 0)
        {
            if (m.cxLeftWidth > w) m.cxLeftWidth = w;
            if (m.cxRightWidth > w) m.cxRightWidth = w;
            if (m.cxLeftWidth + m.cxRightWidth > w)
            {
                const LONG half = w / 2;
                if (m.cxLeftWidth > half) m.cxLeftWidth = half;
                if (m.cxRightWidth > w - m.cxLeftWidth) m.cxRightWidth = w - m.cxLeftWidth;
            }
        }
        if (h > 0)
        {
            if (m.cyTopHeight > h) m.cyTopHeight = h;
            if (m.cyBottomHeight > h) m.cyBottomHeight = h;
            if (m.cyTopHeight + m.cyBottomHeight > h)
            {
                const LONG half = h / 2;
                if (m.cyTopHeight > half) m.cyTopHeight = half;
                if (m.cyBottomHeight > h - m.cyTopHeight) m.cyBottomHeight = h - m.cyTopHeight;
            }
        }

        MILCMD_WNREDIR_UPDATEMARGINS_VSP1 um = {};
        um.Type = (MILCMD)(pData->Window->HasDXContent()
                               ? RWM_MILCMD_VSP1_WNREDIR_FLIPCHAIN_UPDATEMARGINS
                               : RWM_MILCMD_VSP1_WNREDIR_GDISPRITE_UPDATEMARGINS);
        um.Handle = (HMIL_RESOURCE)hSurface;
        um.Margins = m;
        (void)MilResource_SendCommand(&um, sizeof(um), DwmDesktopInstance->GlobalChannel);
    }

    return uDwmWindowNode_UpdateProperties(pData, hSurface);
}

static HRESULT uDwmUpdateWindowTransform(_In_ PRWM_WINDOWDATA_VISTA_SP1 pData)
{
    if (!pData || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !pData->ClientNode)
        return E_UNEXPECTED;

    /*
     * Vista dwmredir:
     * - Top-level windows: Visual_SetTransform(WindowNode, 0) (identity)
     * - Transforms are used for linked windows, which we do not model yet.
     */
    MILCMD_VISUAL_SETTRANSFORM_VSP1 set = {};
    set.Type = (MILCMD)RWM_MILCMD_VSP1_VISUAL_SETTRANSFORM;
    set.Handle = (HMIL_RESOURCE)pData->ClientNode;
    set.hTransform = 0;

    HRESULT hr = MilResource_SendCommand(&set, sizeof(set), DwmDesktopInstance->GlobalChannel);
    if (SUCCEEDED(hr) && pData->hWindowTransform)
    {
        (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, pData->hWindowTransform, NULL);
        pData->hWindowTransform = 0;
        pData->OffsetX = 0.0;
        pData->OffsetY = 0.0;
    }
    return hr;
}

static HRESULT uDwmCreateOrUpdateClipGeometryFromRegionData(_In_ const RGNDATA* pRgnData,
                                                           _Inout_ HMIL_RESOURCE* phGeometry)
{
    if (!pRgnData || !phGeometry || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel)
        return E_INVALIDARG;

    HMIL_RESOURCE h = *phGeometry;
    HRESULT hr = S_OK;

    if (!h)
    {
        hr = MilResource_CreateOrAddRefOnChannel(DwmDesktopInstance->GlobalChannel,
                                                 (MIL_RESOURCE_TYPE)RWM_MILRT_VSP1_PATHGEOMETRY,
                                                 &h);
        if (FAILED(hr) || !h)
            return FAILED(hr) ? hr : E_FAIL;
    }

    const UINT32 n = (UINT32)pRgnData->rdh.nCount;
    const UINT32 cbFigure = (UINT32)(sizeof(MilPathFigure) + sizeof(MilSegmentPoly) + (3u * sizeof(MilPoint2D)));
    const UINT32 cbData = (UINT32)sizeof(MilPathGeometry) + (n * cbFigure);

    MILCMD_PATHGEOMETRY_VSP1 cmd = {};
    cmd.Type = (MILCMD)RWM_MILCMD_VSP1_PATHGEOMETRY;
    cmd.Handle = h;
    cmd.Unknown0 = 0;
    cmd.Unknown1 = 0;
    cmd.cbDataSize = cbData;

    BOOL began = FALSE;
    hr = MilChannel_BeginCommand(DwmDesktopInstance->GlobalChannel, &cmd, sizeof(cmd), cbData);
    if (SUCCEEDED(hr))
        began = TRUE;
    else
    {
        DPRINT1("uDwmClip: BeginCommand failed hr=0x%08lx cbData=%u n=%u\n", hr, cbData, n);
        goto Cleanup;
    }

    {
        MilPathGeometry pg = {};
        pg.Size = cbData;
        pg.Flags = 0x00000002u /* BoundsValid */ | 0x00000010u /* IsRegionData */;
        pg.Bounds.left = (double)pRgnData->rdh.rcBound.left;
        pg.Bounds.top = (double)pRgnData->rdh.rcBound.top;
        pg.Bounds.right = (double)pRgnData->rdh.rcBound.right;
        pg.Bounds.bottom = (double)pRgnData->rdh.rcBound.bottom;
        pg.FigureCount = n;
        pg.ForcePacking = 0;

        hr = MilChannel_AppendCommandData(DwmDesktopInstance->GlobalChannel, &pg, sizeof(pg));
        if (FAILED(hr))
            goto CleanupEnd;
    }

    const RECT* prc = (const RECT*)((const BYTE*)pRgnData + pRgnData->rdh.dwSize);
    UINT32 prevFigureSize = 0; /* Vista: BackSize points to previous figure size (0 for first). */
    for (UINT32 i = 0; i < n; ++i)
    {
        const RECT r = prc[i];

        MilPathFigure fig = {};
        fig.BackSize = prevFigureSize;
        fig.Flags = 0x00000004u /* IsClosed */ | 0x00000008u /* IsFillable */ | 0x00000010u /* IsRectangleData */;
        fig.Count = 1; /* one segment (polyline) */
        fig.Size = cbFigure;
        fig.StartPoint.X = (double)r.left;
        fig.StartPoint.Y = (double)r.top;
        fig.OffsetToLastSegment = (UINT32)sizeof(MilPathFigure);
        fig.ForcePacking = 0;

        hr = MilChannel_AppendCommandData(DwmDesktopInstance->GlobalChannel, &fig, sizeof(fig));
        if (FAILED(hr))
            goto CleanupEnd;

        MilSegmentPoly seg = {};
        /* Vista: polyLineSegment = { Type=5, Flags=0, BackSize=0, Count=3 } */
        seg.Type = MilSegmentType::PolyLine;
        seg.Flags = 0;
        seg.BackSize = 0;
        seg.Count = 3;

        hr = MilChannel_AppendCommandData(DwmDesktopInstance->GlobalChannel, &seg, sizeof(seg));
        if (FAILED(hr))
            goto CleanupEnd;

        MilPoint2D pts[3] = {};
        pts[0].X = (double)r.right;  pts[0].Y = (double)r.top;
        pts[1].X = (double)r.right;  pts[1].Y = (double)r.bottom;
        pts[2].X = (double)r.left;   pts[2].Y = (double)r.bottom;

        hr = MilChannel_AppendCommandData(DwmDesktopInstance->GlobalChannel, pts, sizeof(pts));
        if (FAILED(hr))
            goto CleanupEnd;

        /* Next figure's BackSize should point to this figure's size. */
        prevFigureSize = cbFigure;
    }

CleanupEnd:
    /* EndCommand even on failure to keep channel state sane. */
    if (began)
        (void)MilChannel_EndCommand(DwmDesktopInstance->GlobalChannel);

Cleanup:
    if (SUCCEEDED(hr))
        *phGeometry = h;
    else if (!*phGeometry && h)
        (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, h, NULL);

    return hr;
}

static HRESULT uDwmCreateOrUpdateGeometryFromHrgn(_In_ HRGN hrgn, _Inout_ HMIL_RESOURCE* phGeometry)
{
    if (!hrgn || !phGeometry)
        return E_INVALIDARG;

    DWORD cb = GetRegionData(hrgn, 0, NULL);
    if (!cb)
        return HRESULT_FROM_WIN32(GetLastError());

    RGNDATA* pDataRgn = (RGNDATA*)HeapAlloc(GetProcessHeap(), 0, cb);
    if (!pDataRgn)
        return E_OUTOFMEMORY;

    HRESULT hr = S_OK;
    if (GetRegionData(hrgn, cb, pDataRgn))
    {
        hr = uDwmCreateOrUpdateClipGeometryFromRegionData(pDataRgn, phGeometry);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    HeapFree(GetProcessHeap(), 0, pDataRgn);
    return hr;
}

static HRESULT uDwmUpdateWindowSpriteClip(_In_ PRWM_WINDOWDATA_VISTA_SP1 pData)
{
    if (!pData || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel || !pData->ClientNode)
        return E_UNEXPECTED;

    if (!pData->hWnd || !IsWindow(pData->hWnd))
        return S_FALSE;

    HRGN hrgn = CreateRectRgn(0, 0, 0, 0);
    if (!hrgn)
        return E_OUTOFMEMORY;

    int rgnType = GetWindowRgn(pData->hWnd, hrgn);
    const BOOL usingWindowRgn = (rgnType == SIMPLEREGION || rgnType == COMPLEXREGION);

    HRESULT hr = S_OK;
    if (usingWindowRgn)
    {
        /* GetWindowRgn returns WINDOW-local coords; convert to SCREEN-space like Vista. */
        (void)OffsetRgn(hrgn, pData->WindowRect.left, pData->WindowRect.top);

        DWORD cb = GetRegionData(hrgn, 0, NULL);
        if (cb)
        {
            RGNDATA* pDataRgn = (RGNDATA*)HeapAlloc(GetProcessHeap(), 0, cb);
            if (pDataRgn)
            {
                if (GetRegionData(hrgn, cb, pDataRgn))
                    hr = uDwmCreateOrUpdateClipGeometryFromRegionData(pDataRgn, &pData->hClipGeometry);
                HeapFree(GetProcessHeap(), 0, pDataRgn);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    else
    {
        /* No window region: drop any previous clip geometry. */
        if (pData->hClipGeometry)
        {
            (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, pData->hClipGeometry, NULL);
            pData->hClipGeometry = 0;
        }
    }

    /* Apply clip (or clear it). */
    MILCMD_WINDOWNODE_SETSPRITECLIP_VSP1 clip = {};
    clip.Type = (MILCMD)RWM_MILCMD_VSP1_WINDOWNODE_SETSPRITECLIP;
    clip.Handle = (HMIL_RESOURCE)pData->ClientNode;
    clip.Flags = usingWindowRgn ? 0u : 1u; /* dwmredir passes !m_fUsingWindowRgn */
    clip.hClip = usingWindowRgn ? pData->hClipGeometry : 0;
    (void)MilResource_SendCommand(&clip, sizeof(clip), DwmDesktopInstance->GlobalChannel);

    DeleteObject(hrgn);
    return hr;
}

static HRESULT uDwmEnsureWindowVisual(PRWM_WINDOWDATA_VISTA_SP1 pData)
{
    if (!pData || !DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel)
        return E_UNEXPECTED;

    HRESULT hr = S_OK;
    const BOOL LogClientNodeOnce = (pData->Flags & RWM_WD_LOGGED_CLIENTNODE) ? FALSE : TRUE;

    /*
     * On Vista, dwmredir may call CreateWindow before the per-window client
     * composition node has been fully established. Keep probing until we get a
     * non-zero node handle.
     */
    if (!pData->ClientNode && pData->Window)
    {
        UINT32 node = 0;
        HRESULT hrNode = pData->Window->GetClientNode(DwmDesktopInstance->GlobalChannel, &node);
        if (SUCCEEDED(hrNode) && node)
        {
            pData->ClientNode = node;
        }
        else
        {
            /*
             * Vista dwmredir's GetClientNode duplicates a pre-existing WindowRootNode
             * handle from its source channel. If that node isn't ready yet, it can
             * fail or return 0.
             *
             * Try GetClientNodeClone as a more self-contained fallback; it is known
             * to create a usable node graph in dwmredir.
             */
            UINT32 clone = 0;
            HRESULT hrClone = pData->Window->GetClientNodeClone(DwmDesktopInstance->GlobalChannel, &clone);
            if (SUCCEEDED(hrClone) && clone)
            {
                if ((HMIL_RESOURCE)clone == (HMIL_RESOURCE)DwmDesktopInstance->hRootNode)
                {
                    if (LogClientNodeOnce)
                        DPRINT1("uDwmEnsureWindowVisual: rejecting ClientNodeClone=0x%lx for hwnd=0x%p (collides with root/fallback)\n",
                                (ULONG)clone, pData->hWnd);
                }
                else
                {
                    pData->ClientNode = clone;
                    if (LogClientNodeOnce)
                        DPRINT1("uDwmEnsureWindowVisual: got ClientNodeClone=0x%lx for hwnd=0x%p\n",
                                (ULONG)pData->ClientNode, pData->hWnd);
                }
            }
            else if (LogClientNodeOnce)
            {
                DPRINT1("uDwmEnsureWindowVisual: GetClientNode failed/empty hr=0x%08lx node=0x%lx; clone hr=0x%08lx clone=0x%lx hwnd=0x%p\n",
                        hrNode, (ULONG)node, hrClone, (ULONG)clone, pData->hWnd);
            }
        }
    }
    if (LogClientNodeOnce && pData->ClientNode)
        pData->Flags |= RWM_WD_LOGGED_CLIENTNODE;

    /*
     * If this is the DesktopWindow, switch the desktop render target root over
     * to its node. This matches Vista's "root at desktop window context" model
     * much better than keeping an empty fallback root.
     */
    if (pData->ClientNode &&
        DwmDesktopInstance &&
        DwmDesktopInstance->hDesktopTarget &&
        pData->hWnd == GetDesktopWindow() &&
        (HMIL_RESOURCE)pData->ClientNode != DwmDesktopInstance->hRootNode)
    {
        HMIL_RESOURCE oldRoot = DwmDesktopInstance->hRootNode;
        const BOOLEAN oldWasDesktop = DwmDesktopInstance->RootIsDesktopClone;

        DwmDesktopInstance->hRootNode = (HMIL_RESOURCE)pData->ClientNode;
        DwmDesktopInstance->RootIsDesktopClone = TRUE;

        MILCMD_TARGET_SETROOT_VSP1 setRoot = {};
        setRoot.Type = (MILCMD)RWM_MILCMD_VSP1_TARGET_SETROOT;
        setRoot.Handle = (HMIL_RESOURCE)DwmDesktopInstance->hDesktopTarget;
        setRoot.hRoot = (HMIL_RESOURCE)DwmDesktopInstance->hRootNode;
        (void)MilResource_SendCommand(&setRoot, sizeof(setRoot), DwmDesktopInstance->GlobalChannel);

        /*
         * Critical: ensure the new root has non-empty bounds.
         * We always set bounds on the initial fallback root in EnsureDesktopTargetAndRoot(),
         * but when we switch the root to the DesktopWindow's node we must do it again,
         * otherwise milcore may clip the entire subtree to empty and everything "disappears"
         * after the root swap.
         */
        (void)uDwmWindowNode_SetBounds(pData);

        /* Force a reinsert pass for all windows against the new root. */
        if (g_RwmWindowListInitialized)
        {
            for (PLIST_ENTRY e = g_RwmWindowListHead.Flink; e != &g_RwmWindowListHead; e = e->Flink)
            {
                PRWM_WINDOWDATA_VISTA_SP1 wd = CONTAINING_RECORD(e, RWM_WINDOWDATA_VISTA_SP1, ListEntry);
                if (wd && wd->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
                {
                    wd->Flags &= ~RWM_WD_VISUAL_INSERTED;
                    /* NC visuals are children of WindowNodes; do not reset per-window child insertion here. */
                }
            }
        }
        uDwmMarkRebuildNeeded();

        DPRINT1("uDwmEnsureWindowVisual: switched root to DesktopWindow node=0x%lx (old root=0x%lx)\n",
                (ULONG)DwmDesktopInstance->hRootNode, (ULONG)oldRoot);

        /* Release the old fallback root if we owned it. */
        if (oldRoot && !oldWasDesktop)
            (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, oldRoot, NULL);

        /* Don't parent the desktop root into itself. */
        return S_OK;
    }

    /*
     * Vista dwmredir does NOT use Visual_SetContent(WindowNode) in the desktop tree.
     * It builds a graph of WindowNodes and uses Visual_InsertChildAt/RemoveChild
     * on these graph nodes. Treat the ClientNode as the child to parent directly.
     */

    /* Parenting into the root is handled in uDwmRebuildRootChildren() during UpdateScene. */

    if (pData->ClientNode)
    {
        (void)uDwmWindowNode_SetBounds(pData);
        (void)uDwmWindowNode_SetAlphaMargins(pData);
        /* Position the WindowNode in the desktop tree. */
        (void)uDwmUpdateWindowTransform(pData);
        if (uDwmNeedsExampleTitlebar(pData))
        {
            (void)uDwmEnsureNcSubtree(pData);
            (void)uDwmUpdateNcGlassRenderData(pData);
        }
        if (pData->hSprite)
            (void)uDwmWindowNode_UpdateSpriteHandle(pData, pData->hSprite);

        /* Best-effort: bind the current client surface for visible pixels. */
        (void)uDwmTryAttachClientSurface(pData);
    }

    return hr;
}

HRESULT
WINAPI
uDwmCreateWindow(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmCreateWindow Called\n");
    if (!DwmWindowInterface)
        return E_INVALIDARG;
    if (!DwmDesktopInstance)
        return E_UNEXPECTED;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    uDwmInitializeWindowTracking();

    if (DwmWindowInterface->GetClientData())
    {
        LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
        return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
    }

    PRWM_WINDOWDATA_VISTA_SP1 pData =
        (PRWM_WINDOWDATA_VISTA_SP1)HeapAlloc(GetProcessHeap(),
                                             HEAP_ZERO_MEMORY,
                                             sizeof(RWM_WINDOWDATA_VISTA_SP1));
    if (!pData)
    {
        LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
        return E_OUTOFMEMORY;
    }

    pData->cbSize = sizeof(RWM_WINDOWDATA_VISTA_SP1);
    pData->Signature = RWM_WINDOWDATA_VISTA_SP1_SIGNATURE;
    pData->Window = DwmWindowInterface;
    pData->hWnd = DwmWindowInterface->GetWindowHandle();
    uDwmLogWindowBrief("create", pData->hWnd);
    pData->hSprite = DwmWindowInterface->GetSpriteHandle();
    /* Default visible until ShowHide tells us otherwise. */

    /* Best-effort: capture the window's MIL node (Vista uses this heavily). */
    if (DwmDesktopInstance->GlobalChannel)
    {
        UINT32 node = 0;
        HRESULT hrNode = DwmWindowInterface->GetClientNode(DwmDesktopInstance->GlobalChannel, &node);
        if (SUCCEEDED(hrNode))
            pData->ClientNode = node;
    }

    /*
     * Attach the per-window data to the window object. This must happen before
     * dwmredir starts issuing further per-window callbacks.
     */
    DwmWindowInterface->SetClientData((PVOID)pData);

    /* Track this window internally so UpdateScene can walk it. */
    InsertTailList(&g_RwmWindowListHead, &pData->ListEntry);
    uDwmMarkRebuildNeeded();
    /* Create per-window visual and hook up content. */
    (void)uDwmEnsureWindowVisual(pData);
    g_RwmWindowCount++;

    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmDestroyWindow(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmDestroyWindow Called\n");
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);

    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    uDwmLogWindowBrief("destroy", pData->hWnd);
    if (!pData || pData->Signature != RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
        return E_INVALIDARG;
    }

    if (g_RwmWindowListInitialized)
    {
        RemoveEntryList(&pData->ListEntry);
        if (g_RwmWindowCount)
            g_RwmWindowCount--;
    }
    uDwmMarkRebuildNeeded();

    HMIL_RESOURCE hRootChild = uDwmGetRootChildHandle(pData);
    if ((pData->Flags & RWM_WD_VISUAL_INSERTED) && DwmDesktopInstance->hRootNode && hRootChild)
    {
        MILCMD_VISUAL_REMOVECHILD removeChild = {};
        removeChild.Type =
#if UDWM_TARGET_VISTA_SP1_MILCORE
            (MILCMD)RWM_MILCMD_VSP1_VISUAL_REMOVECHILD;
#else
            MilCmdVisualRemoveChild;
#endif
        removeChild.Handle = DwmDesktopInstance->hRootNode;
        removeChild.hChild = hRootChild;
        (void)MilResource_SendCommand(&removeChild, sizeof(removeChild), DwmDesktopInstance->GlobalChannel);
        pData->Flags &= ~RWM_WD_VISUAL_INSERTED;
    }

    /* Remove NC glass visual from the WindowNode (best-effort). */
    if ((pData->Flags & RWM_WD_NC_VISUAL_INSERTED) && pData->ClientNode && pData->hNcVisual)
    {
        MILCMD_VISUAL_REMOVECHILD removeChild = {};
        removeChild.Type = (MILCMD)RWM_MILCMD_VSP1_VISUAL_REMOVECHILD;
        removeChild.Handle = (HMIL_RESOURCE)pData->ClientNode;
        removeChild.hChild = (HMIL_RESOURCE)pData->hNcVisual;
        (void)MilResource_SendCommand(&removeChild, sizeof(removeChild), DwmDesktopInstance->GlobalChannel);
        pData->Flags &= ~RWM_WD_NC_VISUAL_INSERTED;
    }

    if (pData->hWindowTransform)
    {
        (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, pData->hWindowTransform, NULL);
        pData->hWindowTransform = 0;
    }

    if (pData->hClipGeometry)
    {
        (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, pData->hClipGeometry, NULL);
        pData->hClipGeometry = 0;
    }

    if (pData->hNcRenderData)
    {
        (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, pData->hNcRenderData, NULL);
        pData->hNcRenderData = 0;
    }
    if (pData->hNcCaptionBrush)
    {
        (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, pData->hNcCaptionBrush, NULL);
        pData->hNcCaptionBrush = 0;
    }
    if (pData->hNcBorderBrush)
    {
        (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, pData->hNcBorderBrush, NULL);
        pData->hNcBorderBrush = 0;
    }
    if (pData->hNcGlassGeomTop)
    {
        (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, pData->hNcGlassGeomTop, NULL);
        pData->hNcGlassGeomTop = 0;
    }
    if (pData->hNcGlassGeomLeft)
    {
        (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, pData->hNcGlassGeomLeft, NULL);
        pData->hNcGlassGeomLeft = 0;
    }
    if (pData->hNcGlassGeomRight)
    {
        (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, pData->hNcGlassGeomRight, NULL);
        pData->hNcGlassGeomRight = 0;
    }
    if (pData->hNcGlassGeomBottom)
    {
        (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, pData->hNcGlassGeomBottom, NULL);
        pData->hNcGlassGeomBottom = 0;
    }
    if (pData->hNcColorization)
    {
        (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, pData->hNcColorization, NULL);
        pData->hNcColorization = 0;
    }
    if (pData->hNcVisual)
    {
        (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, pData->hNcVisual, NULL);
        pData->hNcVisual = 0;
    }

    if (pData->hWindowVisual)
    {
        (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel, pData->hWindowVisual, NULL);
        pData->hWindowVisual = 0;
        pData->Flags &= ~RWM_WD_FRAME_CHILD_INSERTED;
    }

    /*
     * Release per-window MIL resources that are cloned/owned on our channel.
     * (Reference: Vista tears down the per-window visual tree on destroy.)
     *
     * Do NOT release the shared desktop root visual.
     */
    if (pData->ClientNode &&
        DwmDesktopInstance->GlobalChannel &&
        pData->ClientNode != (UINT32)DwmDesktopInstance->hRootNode)
    {
        (void)MilResource_ReleaseOnChannel(DwmDesktopInstance->GlobalChannel,
                                           (HMIL_RESOURCE)pData->ClientNode,
                                           NULL);
        pData->ClientNode = 0;
    }

    DwmWindowInterface->SetClientData(NULL);
    HeapFree(GetProcessHeap(), 0, pData);

    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmCreateSprite(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmCreateSprite Called\n");
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        pData->hSprite = DwmWindowInterface->GetSpriteHandle();
        (void)uDwmEnsureWindowVisual(pData);
        (void)uDwmWindowNode_UpdateSpriteHandle(pData, pData->hSprite);
    }
    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmDestroySprite(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmDestroySprite Called\n");
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        pData->hSprite = 0;
        (void)uDwmWindowNode_UpdateSpriteHandle(pData, 0);
    }
    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmShowHide(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmShowHide Called\n");
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        uDwmLogWindowBrief("showhide", pData->hWnd);
        /*
         * Reference behavior: ShowHide reflects actual HWND visibility.
         * Our CompositedWindow::IsVisible() implementation may be stubbed early-on,
         * which would incorrectly hide everything (classic "shows briefly then disappears").
         *
         * Prefer user32's IsWindowVisible on the HWND when available.
         */
        BOOLEAN visibleIface = DwmWindowInterface->IsVisible();
        BOOLEAN visibleHwnd = visibleIface;
        if (pData->hWnd)
            visibleHwnd = IsWindowVisible(pData->hWnd) ? TRUE : FALSE;
        const BOOLEAN visibleStyle = uDwmComputeVisibleFromHwnd(pData->hWnd);

        static ULONG s_visLog = 0;
        if ((++s_visLog % 120) == 1 && pData->hWnd)
        {
            DPRINT1("uDwmShowHide: hwnd=0x%p iface=%u win32=%u style=%u\n",
                    pData->hWnd, (ULONG)visibleIface, (ULONG)visibleHwnd, (ULONG)visibleStyle);
        }

        const BOOLEAN visible = visibleStyle;
        uDwmSetDesiredVisible(pData, visible);
        (void)uDwmEnsureWindowVisual(pData);
        uDwmMarkRebuildNeeded();
    }
    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmMoveSize(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmMoveSize Called\n");
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        (void)uDwmEnsureWindowVisual(pData);
        (void)uDwmWindowNode_SetBounds(pData);
        (void)uDwmTryAttachClientSurface(pData); /* keep surface margins in sync */
        if (uDwmNeedsExampleTitlebar(pData))
            (void)uDwmUpdateNcGlassRenderData(pData);
    }
    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmZOrder(CompositedWindow* DwmWindowInterface, CompositedWindow* DwmWindowToInsertAfter)
{
    DPRINT1("uDwmZOrder Called\n");
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        /* Reorder our internal list to reflect the requested z-order. */
        RemoveEntryList(&pData->ListEntry);

        if (DwmWindowToInsertAfter)
        {
            PRWM_WINDOWDATA_VISTA_SP1 pAfter = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowToInsertAfter->GetClientData();
            if (pAfter && pAfter->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
            {
                InsertHeadList(&pAfter->ListEntry, &pData->ListEntry);
            }
            else
            {
                InsertTailList(&g_RwmWindowListHead, &pData->ListEntry);
            }
        }
        else
        {
            InsertTailList(&g_RwmWindowListHead, &pData->ListEntry);
        }
        uDwmMarkRebuildNeeded();
    }
    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmStyleChange(CompositedWindow* DwmWindowInterface)
{
    return 0;
}

HRESULT WINAPI uDwmOwnerChange(CompositedWindow* DwmWindowInterface, CompositedWindow* NewDwmWindowInterfac)
{
    return 0;
}

HRESULT WINAPI uDwmClientMarginsChange(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmClientMarginsChange Called\n");
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        (void)uDwmEnsureWindowVisual(pData);
        (void)uDwmWindowNode_SetAlphaMargins(pData);
        (void)uDwmWindowNode_SetBounds(pData);
        (void)uDwmTryAttachClientSurface(pData); /* keep surface margins in sync */
        if (uDwmNeedsExampleTitlebar(pData))
            (void)uDwmUpdateNcGlassRenderData(pData);
    }
    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmClientGlassChange(CompositedWindow* DwmWindowInterface)
{
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        (void)uDwmEnsureWindowVisual(pData);
        if (uDwmNeedsExampleTitlebar(pData))
            (void)uDwmUpdateNcGlassRenderData(pData);
    }
    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmActivationChange(CompositedWindow* DwmWindowInterface)
{
    return 0;
}

HRESULT WINAPI uDwmAlphaChange(CompositedWindow* DwmWindowInterface)
{
    return 0;
}

HRESULT WINAPI uDwmBlurBehindChange(CompositedWindow* DwmWindowInterface, DWM_BLURBEHIND * BlurBehind)
{
    UNREFERENCED_PARAMETER(BlurBehind);
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        (void)uDwmEnsureWindowVisual(pData);
        if (uDwmNeedsExampleTitlebar(pData))
            (void)uDwmUpdateNcGlassRenderData(pData);
    }
    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmClipChange(CompositedWindow* DwmWindowInterface)
{
    return 0;
}

HRESULT WINAPI uDwmDXContentChange(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmDXContentChange Called\n");
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        (void)uDwmEnsureWindowVisual(pData);
        (void)uDwmTryAttachClientSurface(pData);

        /*
         * Ensure the command stream makes forward progress immediately.
         * Some apps (notably GDI/DX hybrid surfaces) rely on surface-change
         * notifications driving a commit even when UpdateScene is not called.
         */
        if (DwmDesktopInstance->GlobalChannel)
        {
            MILCMD_TRANSPORT_SYNCFLUSH tf = {};
            tf.Type = (MILCMD)RWM_MILCMD_VSP1_TRANSPORT_SYNCFLUSH;
            (void)MilResource_SendCommand(&tf, sizeof(tf), DwmDesktopInstance->GlobalChannel);
            HRESULT hrCommit = MilChannel_CommitChannel(DwmDesktopInstance->GlobalChannel);
            if (FAILED(hrCommit))
                DPRINT1("uDwmGDISurfaceChange: Commit failed hr=0x%08lx hwnd=0x%p\n", hrCommit, pData->hWnd);
        }
    }
    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmGDISurfaceChange(CompositedWindow* DwmWindowInterface)
{
    DPRINT1("uDwmGDISurfaceChange Called\n");
    if (!DwmWindowInterface || !DwmDesktopInstance)
        return E_INVALIDARG;

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    PRWM_WINDOWDATA_VISTA_SP1 pData = (PRWM_WINDOWDATA_VISTA_SP1)DwmWindowInterface->GetClientData();
    if (pData && pData->Signature == RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
    {
        (void)uDwmEnsureWindowVisual(pData);

        /*
         * GDI surfaces can change contents without changing the surface handle.
         * Force a best-effort re-bind to act as a "dirty" hint.
         */
        pData->LastSpriteImageSurface = 0;
        (void)uDwmTryAttachClientSurface(pData);
        (void)uDwmUpdateWindowSpriteClip(pData);

        /*
         * Fix "needs minimize/restore to update":
         * this callback may arrive even when uDwmUpdateScene is not being called
         * frequently. We must commit the MIL channel here so the render thread
         * sees the updated surface.
         */
        if (DwmDesktopInstance->GlobalChannel)
        {
            MILCMD_TRANSPORT_SYNCFLUSH tf = {};
            tf.Type = (MILCMD)RWM_MILCMD_VSP1_TRANSPORT_SYNCFLUSH;
            (void)MilResource_SendCommand(&tf, sizeof(tf), DwmDesktopInstance->GlobalChannel);
            (void)MilChannel_CommitChannel(DwmDesktopInstance->GlobalChannel);
        }
    }
    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return S_OK;
}

HRESULT WINAPI uDwmGhostChange(CompositedWindow* DwmWindowInterface)
{
    return 0;
}

HRESULT WINAPI uDwmForceIconicRepresentationChange(CompositedWindow* DwmWindowInterface)
{
    return 0;
}

HRESULT WINAPI uDwmFlip3DWindowPolicyChange(CompositedWindow* DwmWindowInterface)
{
    return 0;
}

HRESULT WINAPI uDwmForceDisconnectClientNode(CompositedWindow* DwmWindowInterface)
{
    return 0;
}

HRESULT WINAPI uDwmGetWindowBounds(HWND hWnd, RECT* rect)
{
    return 0;
}

HRESULT
WINAPI
uDwmProcessAsyncDwmMessage(RWM_COMMANDS Command, PVOID CommandData, UINT32 CommandDataSize, BOOLEAN IsKernelMessage)
{
    DPRINT1("uDwmProcessAsyncDwmMessage Called cmd=0x%X size=%u kernel=%u\n",
            Command, CommandDataSize, IsKernelMessage);
    switch(Command)
    {
        case RWMCMD_REDIR_STARTUPBEGIN:
        {
            /* Vista: StartupBegin just flips internal state under a critical section. */
            DPRINT1("RWMCMD_REDIR_STARTUPBEGIN\n");
            break;
        }
        case RWMCMD_REDIR_STARTUP:
        {
            DPRINT1("RWMCMD_REDIR_STARTUP\n");
            /*
             * Vista: StartupEnd enables the desktop render target.
             * Our equivalent: ensure the desktop target/root are created using
             * the uDWM (desktop target) path, and commit.
             */
            if (DwmDesktopInstance)
            {
                (void)DwmDesktopInstance->EnsureDesktopTargetAndRoot();
                if (DwmDesktopInstance->GlobalChannel)
                    (void)MilChannel_CommitChannel(DwmDesktopInstance->GlobalChannel);
            }
            break;
        }
        
        case RWMCMD_REDIR_CHANGESETTINGS:
        {
            DPRINT1("RWMCMD_REDIR_CHANGESETTINGS\n");
            break;
        }
        default:
        {
            break;
        }
    }
    return 0;
}

HRESULT WINAPI uDwmProcessSyncDwmMessage(RWM_COMMANDS Command, PVOID CommandData, UINT32 CommandDataSize, BOOLEAN IsKernelMessage, UINT32 ProcessId, REMOTE_PORT_VIEW* RemotePortView, HRESULT* ReplyHr, UINT32* ReplySize)
{
    return 0;
}

HRESULT WINAPI uDwmProcessBackChannelMessage(MIL_MESSAGE* Message)
{
    return 0;
}

HRESULT WINAPI uDwmUpdateScene()
{
    static ULONG s_scenePrint = 0;
    if ((++s_scenePrint % 60) == 1)
        DPRINT1("uDwmUpdateScene Called (%lu)\n", s_scenePrint);

    g_uDwmUpdateSceneCount++;
    if (!DwmDesktopInstance || !DwmDesktopInstance->GlobalChannel)
        return E_UNEXPECTED;

    /* Minimal breadcrumb: verify DWM is driving us. */
    if (g_uDwmUpdateSceneCount == 1)
        DPRINT1("uDwmUpdateScene: first call, target=0x%lx root=0x%lx\n",
                (ULONG)DwmDesktopInstance->hDesktopTarget,
                (ULONG)DwmDesktopInstance->hRootNode);

    EnterCriticalSection(&DwmDesktopInstance->CsDwmInstance);

    /*
     * Vista routes UpdateScene through the desktop manager which renders the
     * root visual tree, destroys pending sprites, then commits and sync-flushes
     * the MIL channel. We don't have the full scene graph yet; still, we keep
     * the same commit/flush semantics so the redirection pipeline makes forward
     * progress.
     */
    /* Minimal Vista-ish walk: ensure nodes exist and bounds/transform are up to date. */
    if (g_RwmWindowListInitialized)
    {
        for (PLIST_ENTRY e = g_RwmWindowListHead.Flink; e != &g_RwmWindowListHead; e = e->Flink)
        {
            PRWM_WINDOWDATA_VISTA_SP1 pData = CONTAINING_RECORD(e, RWM_WINDOWDATA_VISTA_SP1, ListEntry);
            if (!pData || pData->Signature != RWM_WINDOWDATA_VISTA_SP1_SIGNATURE)
                continue;

            (void)uDwmEnsureWindowVisual(pData);
        }
    }

    /* Apply visibility + z-order changes in one deterministic rebuild pass. */
    uDwmRebuildRootChildren();

    /*
     * Do not emit arbitrary test commands here.
     * In particular, DwmVisual::DrawBullshit was sending a ColorResource update
     * against a Visual handle, which can poison a Vista milcore command stream.
     */

    /*
     * IMPORTANT:
     * Do NOT send TargetInvalidate with a guessed command ID. On Vista SP1 this
     * can crash the milcore render thread (batch processing error 0x8000ffff),
     * killing all rendering.
     *
     * For now, we only commit + syncflush; once we confirm the correct Vista
     * numeric ID and packet shape for TargetInvalidate, we can reintroduce it.
     */
    /*
     * IMPORTANT (Vista behavior):
     * uDWM does NOT call MilComposition_SyncFlush. It sends a transport command
     * MILCMD_TRANSPORT_SYNCFLUSH (Type=3, sizeof=4) and then commits.
     *
     * Calling MilComposition_SyncFlush against Vista milcore has been observed
     * to crash the render thread with 0x8000ffff (batch processing error).
     */
    MILCMD_TRANSPORT_SYNCFLUSH tf = {};
    tf.Type = (MILCMD)RWM_MILCMD_VSP1_TRANSPORT_SYNCFLUSH;
    (void)MilResource_SendCommand(&tf, sizeof(tf), DwmDesktopInstance->GlobalChannel);

    HRESULT hr = MilChannel_CommitChannel(DwmDesktopInstance->GlobalChannel);
    if (((s_scenePrint % 60) == 1) && FAILED(hr))
        DPRINT1("uDwmUpdateScene: Commit failed hr=0x%08lx\n", hr);

    LeaveCriticalSection(&DwmDesktopInstance->CsDwmInstance);
    return hr;
}


EXTERN_C
VOID
WINAPI 
UpdateWindowList( IDwmWindowList* WindowListInstance)
{
    WindowListInstance->lpVtbl->WindowListCreateWindow = uDwmCreateWindow;
    WindowListInstance->lpVtbl->WindowListDestroyWindow = uDwmDestroyWindow;
    WindowListInstance->lpVtbl->WindowListCreateSprite = uDwmCreateSprite;
    WindowListInstance->lpVtbl->WindowListDestroySprite = uDwmDestroySprite;
    WindowListInstance->lpVtbl->WindowListShowHide = uDwmShowHide;
    WindowListInstance->lpVtbl->WindowListMoveSize = uDwmMoveSize;
    WindowListInstance->lpVtbl->WindowListZOrder = uDwmZOrder;
    WindowListInstance->lpVtbl->WindowListStyleChange = uDwmStyleChange;
    WindowListInstance->lpVtbl->WindowListOwnerChange = uDwmOwnerChange;
    WindowListInstance->lpVtbl->WindowListClientMarginsChange = uDwmClientMarginsChange;
    WindowListInstance->lpVtbl->WindowListClientGlassChange = uDwmClientGlassChange;
    WindowListInstance->lpVtbl->WindowListActivationChange = uDwmActivationChange;
    WindowListInstance->lpVtbl->WindowListAlphaChange = uDwmAlphaChange;
    WindowListInstance->lpVtbl->WindowListBlurBehindChange = uDwmBlurBehindChange;
    WindowListInstance->lpVtbl->WindowListClipChange = uDwmClipChange;
    WindowListInstance->lpVtbl->WindowListDXContentChange = uDwmDXContentChange;
    WindowListInstance->lpVtbl->WindowListGDISurfaceChange = uDwmGDISurfaceChange;
    WindowListInstance->lpVtbl->WindowListGhostChange = uDwmGhostChange;
    WindowListInstance->lpVtbl->WindowListForceIconicRepresentationChange = uDwmForceIconicRepresentationChange;
    WindowListInstance->lpVtbl->WindowListFlip3DWindowPolicyChange = uDwmFlip3DWindowPolicyChange;
    WindowListInstance->lpVtbl->WindowListForceDisconnectClientNode = uDwmForceDisconnectClientNode;
    WindowListInstance->lpVtbl->WindowListGetWindowBounds = uDwmGetWindowBounds;
    WindowListInstance->lpVtbl->WindowListProcessAsyncDwmMessage = uDwmProcessAsyncDwmMessage;
    WindowListInstance->lpVtbl->WindowListProcessSyncDwmMessage = uDwmProcessSyncDwmMessage;
    WindowListInstance->lpVtbl->WindowListProcessBackChannelMessage = uDwmProcessBackChannelMessage;
    WindowListInstance->lpVtbl->WindowListUpdateScene = uDwmUpdateScene;
}
