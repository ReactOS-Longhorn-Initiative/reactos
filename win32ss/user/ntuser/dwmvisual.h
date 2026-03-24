#pragma once

/*
 * Longhorn 5112-style DWM "visual" list (gDceState / pFindVisual analogue).
 * Nodes are ~0x2C bytes like LH DceState; filled from USER DCEs + redirect surfaces
 * and/or EngpTransferSpriteStateToVisualState (sprite z-list migration at DWM startup).
 */

struct _SURFOBJ;
typedef struct _SURFOBJ SURFOBJ;

typedef struct _ROS_DWM_VISUAL
{
    struct _ROS_DWM_VISUAL *pNext;
    ULONG Flags;
    HWND hwnd;
    RECTL rcScreen;
    SURFOBJ *pso; /* EngLockSurface; EngUnlockSurface on free */
    ULONG ulSpriteAttr8;
    BLENDFUNCTION Blend;
    ULONG ulSpriteAttr10;
    HSURF hsurfEngAlloc; /* EngCreateBitmap copy from transfer — GreDeleteObject after unlock */
} ROS_DWM_VISUAL, *PROS_DWM_VISUAL;

#define ROS_DWM_VISUAL_FLAG_VALID 0x00000001u

#define TAG_ROS_DWM_VISUAL 'lmVD'

VOID FASTCALL IntRosDwmFreeAllVisuals(VOID);
VOID FASTCALL IntRosDwmRebuildVisualList(_In_ HDEV hdev);
PROS_DWM_VISUAL FASTCALL IntRosDwmFindVisual(_In_opt_ HWND hwnd);

VOID FASTCALL IntRosDwmUpsertForPwnd(_In_ PWND pwnd);
VOID FASTCALL IntRosDwmRemoveVisualForPwnd(_In_ PWND pwnd);

/* Frees the current list and installs NewHead (used after TransferSpriteStateToVisualState). */
VOID FASTCALL IntRosDwmReplaceVisualListHead(_In_opt_ PROS_DWM_VISUAL NewHead);
