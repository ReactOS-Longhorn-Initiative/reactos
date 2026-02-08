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
    UINT32 ClientNodeClone;        /* Optional clone */
    RECT WindowRect;               /* Cached window rect (best-effort) */
    RECT ClientMarginsRect;        /* Cached client margins (best-effort) */
    LIST_ENTRY ListEntry;          /* uDWM internal tracking */
    HMIL_RESOURCE hWindowVisual;   /* Per-window TYPE_VISUAL resource */
    HMIL_RESOURCE hWindowTransform;/* Per-window TYPE_TRANSLATETRANSFORM */
    DOUBLE OffsetX;                /* Cached translation X */
    DOUBLE OffsetY;                /* Cached translation Y */
    UINT32 Flags;                  /* RWM_WD_* */

#if !defined(_WIN64)
    BYTE Reserved[0xB3C - (4 + 4 + 4 + 4 + 4 + 4 + 4 + 16 + 16 + 8 + 4 + 8 + 8 + 4)];
#endif
} RWM_WINDOWDATA_VISTA_SP1, *PRWM_WINDOWDATA_VISTA_SP1;
#pragma pack(pop)

#define RWM_WINDOWDATA_VISTA_SP1_SIGNATURE (0x31574457u) /* '1WDW' */
#define RWM_WD_VISUAL_INSERTED             (0x00000001u)
#define RWM_WD_DESIRED_VISIBLE             (0x00000002u)
#define RWM_WD_HAS_DESIRED_VISIBLE         (0x00000004u)
#define RWM_WD_LOGGED_CLIENTNODE           (0x00000008u)

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
