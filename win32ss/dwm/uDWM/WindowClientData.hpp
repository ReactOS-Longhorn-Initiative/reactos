#pragma once

#include <windef.h>
#include <winnt.h>

class CompositedWindow;

/*
 * Vista SP1 uDWM allocates a large per-window "client data" block and attaches it
 * to the IDwmWindow object (our `CompositedWindow`) via SetClientData().
 *
 * ReactOS does not yet implement the full private CWindowData contract, but we
 * keep a similarly-sized opaque block on 32-bit builds so other modules can
 * evolve around a stable ABI-sized payload.
 */
#pragma pack(push, 4)
typedef struct _RWM_WINDOWDATA_VISTA_SP1
{
    UINT32 cbSize;                 /* sizeof(this) */
    UINT32 Signature;              /* '1WDW' (little-endian) */
    CompositedWindow* Window;      /* IDwmWindow / CompositedWindow back-pointer */
    HWND hWnd;                     /* HWND for quick lookup */
    HSPRITE hSprite;               /* Sprite handle (if any) */
    UINT32 ClientNode;             /* MIL client node on our channel */
    union
    {
        UINT32 ClientNodeClone;        /* Optional clone (if we ever start using it) */
        UINT32 LastSpriteImageSurface; /* Cache: last surface set via SetSpriteImage */
    };
    RECT WindowRect;               /* Cached WINDOW rect in SCREEN coords (best-effort) */
    RECT ClientMarginsRect;        /* Cached CLIENT rect in SCREEN coords (best-effort) */
    RECT ContentRectLocal;         /* Cached CONTENT rect in SCREEN coords (best-effort; name kept for ABI) */
    LIST_ENTRY ListEntry;          /* uDWM internal tracking */
    HMIL_RESOURCE hWindowVisual;   /* Per-window TYPE_VISUAL resource */
    HMIL_RESOURCE hWindowTransform;/* Per-window TYPE_TRANSLATETRANSFORM */
    HMIL_RESOURCE hClipGeometry;   /* Per-window TYPE_PATHGEOMETRY for sprite clipping */
    HMIL_RESOURCE hNcVisual;       /* Per-window NC root visual (child of hWindowVisual) */
    HMIL_RESOURCE hNcRenderData;   /* RenderData bound to hNcVisual */
    HMIL_RESOURCE hNcCaptionBrush; /* SolidColorBrush for caption area */
    HMIL_RESOURCE hNcBorderBrush;  /* SolidColorBrush for border */
    HMIL_RESOURCE hNcGlassGeomTop;    /* RectangleGeometry: top non-client glass band */
    HMIL_RESOURCE hNcGlassGeomLeft;   /* RectangleGeometry: left frame band */
    HMIL_RESOURCE hNcGlassGeomRight;  /* RectangleGeometry: right frame band */
    HMIL_RESOURCE hNcGlassGeomBottom; /* RectangleGeometry: bottom frame band */
    HMIL_RESOURCE hNcColorization;    /* ColorResource for glass tint */
    DOUBLE OffsetX;                /* Cached translation X */
    DOUBLE OffsetY;                /* Cached translation Y */
    UINT32 Flags;                  /* RWM_WD_* */

#if !defined(_WIN64)
    BYTE Reserved[0xB3C - (4 + 4 + 4 + 4 + 4 + 4 + 4 + 16 + 16 + 8 +
                           16 + /* ContentRectLocal */
                           4 + 4 + 4 + 4 + 4 + 4 + 4 + /* handles: WindowVisual..NcBorderBrush */
                           4 + 4 + 4 + 4 + 4 +         /* handles: glass geoms + colorization */
                           8 + 8 + 4)];
#endif
} RWM_WINDOWDATA_VISTA_SP1, *PRWM_WINDOWDATA_VISTA_SP1;

static_assert(sizeof(RWM_WINDOWDATA_VISTA_SP1) <= 0xB3C, "RWM_WINDOWDATA_VISTA_SP1 grew beyond Vista-sized payload");
#pragma pack(pop)

#define RWM_WINDOWDATA_VISTA_SP1_SIGNATURE (0x31574457u) /* '1WDW' */
#define RWM_WD_VISUAL_INSERTED             (0x00000001u)
#define RWM_WD_DESIRED_VISIBLE             (0x00000002u)
#define RWM_WD_HAS_DESIRED_VISIBLE         (0x00000004u)
#define RWM_WD_LOGGED_CLIENTNODE           (0x00000008u)
#define RWM_WD_FRAME_CHILD_INSERTED        (0x00000010u)
#define RWM_WD_NC_VISUAL_INSERTED          (0x00000020u)

class WindowClientData
{
public:
    WindowClientData(); 
    ~WindowClientData();
    wchar_t *pszTitle;
    CompositedWindow *CompositedWindow;
    HWND hWnd;
    HSPRITE SpriteHandle;
};
