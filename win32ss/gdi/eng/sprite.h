/*
 * PROJECT:     ReactOS win32k
 * PURPOSE:     Longhorn-style sprite list (SPRITESTATE / SPRITE / pSp*) — LH5048 reference layout
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 */

#pragma once

struct _SPRITE;
typedef struct _SPRITE *PSPRITE;

/*
 * LH5048 x86: z-head +8, y-head +12, sentinel ptr +384 (dword 96).
 */
typedef struct _SPRITESTATE
{
    ULONG ulReserved0;
    ULONG ulReserved1;
    PSPRITE pspriteZHead;
    PSPRITE pspriteYHead;
    UCHAR _padToSentinel[384 - 12 - sizeof(PSPRITE)];
    PSPRITE pspriteSentinel;
} SPRITESTATE;

typedef struct _SPRITESTATE *PSPRITESTATE;

C_ASSERT(FIELD_OFFSET(SPRITESTATE, pspriteZHead) == 8);
C_ASSERT(FIELD_OFFSET(SPRITESTATE, pspriteYHead) == 12);
C_ASSERT(FIELD_OFFSET(SPRITESTATE, pspriteSentinel) == 384);

/*
 * dword 21 @84: either auxiliary HSURF or meta pointer where PSURFACE == (PUCHAR)ptr - 16 (LH pSpGetShape).
 * dword 11 @44 (lUnk44): Y-order key used by vSpOrderInY (we keep it in sync with screen y).
 */
typedef union _SPRITE_SHAPE_D21
{
    HSURF hsurfAux;
    PVOID pvMinus16ToSurf;
} SPRITE_SHAPE_D21;

#if defined(_M_IX86)
typedef struct _SPRITE
{
    ULONG fl;
    ULONG ulSpFlags04;
    ULONG ulUnk08;
    PSPRITESTATE pSpriteState;
    PSPRITE pspriteNextZ;
    PSPRITE pspriteNextY;
    PSPRITE pspritePrevY;
    ULONG ulUnk28;
    ULONG zOrder;
    HWND hwnd;
    LONG lUnk40;
    LONG lUnk44; /* dword 11 — Y sort key for vSpOrderInY */
    LONG lUnk48;
    LONG lUnk52;
    UCHAR _pad56to72[72 - 56];
    LONG x;
    LONG y;
    ULONG ulSpDword20;
    SPRITE_SHAPE_D21 shapeD21;
    UCHAR _pad88to108[108 - 88];
    HSURF hsurfShape;
    ULONG ulUnk112;
    ULONG ulUnk116;
    ULONG cx;
    ULONG cy;
    ULONG ulSpPad32to38[7];
    ULONG ulSpriteAttr39;
    BLENDFUNCTION BlendSprite;
    ULONG ulSpriteAttr41;
    UCHAR _pad168to172[4];
} SPRITE;
C_ASSERT(sizeof(SPRITE) == 0xAC);
C_ASSERT(FIELD_OFFSET(SPRITE, lUnk44) == 44);
C_ASSERT(FIELD_OFFSET(SPRITE, shapeD21) == 84);
C_ASSERT(FIELD_OFFSET(SPRITE, ulSpriteAttr39) == 156);
C_ASSERT(FIELD_OFFSET(SPRITE, BlendSprite) == 160);
C_ASSERT(FIELD_OFFSET(SPRITE, ulSpriteAttr41) == 164);
#else
typedef struct _SPRITE
{
    ULONG fl;
    ULONG ulSpFlags04;
    ULONG ulUnk08;
    PSPRITESTATE pSpriteState;
    PSPRITE pspriteNextZ;
    PSPRITE pspriteNextY;
    PSPRITE pspritePrevY;
    ULONG ulUnk28;
    ULONG zOrder;
    HWND hwnd;
    LONG lUnk40;
    LONG lUnk44;
    LONG lUnk48;
    LONG lUnk52;
    LONG x;
    LONG y;
    ULONG ulSpDword20;
    SPRITE_SHAPE_D21 shapeD21;
    HSURF hsurfShape;
    ULONG ulUnk112;
    ULONG ulUnk116;
    ULONG cx;
    ULONG cy;
    ULONG ulSpriteAttr39;
    BLENDFUNCTION BlendSprite;
    ULONG ulSpriteAttr41;
} SPRITE;
#endif

#define SPRITE_FL_SENTINEL           0x80000000ul
#define SPRITE_FL_SKIP_YORDER        0x40
#define SPRITE_FL_SHAPE_D21_IS_THUNK 0x00008000ul /* shapeD21 is pvMinus16ToSurf, not hsurfAux */

typedef struct _SPRITESTATE_BLOCK
{
    SPRITESTATE st;
    SPRITE Sentinel;
    PPDEVOBJ OwningPdev;
} SPRITESTATE_BLOCK, *PSPRITESTATE_BLOCK;

NTSTATUS NTAPI EngpSpriteStateCreate(_In_ PPDEVOBJ ppdev);
VOID NTAPI EngpSpriteStateDestroy(_In_ PPDEVOBJ ppdev);

PSPRITE NTAPI EngpSpriteCreate(
    _In_ HDEV hdev,
    _In_opt_ PRECTL prcl,
    _In_ HWND hwnd,
    _In_opt_ PPOINTL pptlOffset);

PSPRITE NTAPI EngpSpriteGet(
    _In_ PSPRITESTATE pss,
    _In_opt_ HWND hwnd,
    _In_opt_ PVOID pShapeHint);

BOOL NTAPI EngpGdiDeleteSprite(
    _In_ PPDEVOBJ ppdev,
    _In_opt_ HWND hwnd,
    _In_opt_ PVOID pShapeHint);

VOID FASTCALL IntEngSpriteOnWindowCreated(_In_ PWND Wnd);
VOID FASTCALL IntEngSpriteOnWindowDestroyed(_In_ PWND Wnd);
VOID FASTCALL IntEngSpriteOnWindowPosChanged(_In_ PWND Wnd);
VOID FASTCALL IntEngSpriteOnZorderChanged(_In_ PWND Wnd, _In_ HWND hwndInsertAfter);

VOID NTAPI EngpSpriteUpdateLayout(
    _In_ PPDEVOBJ ppdev,
    _In_ HWND hwnd,
    _In_ PRECTL prcl);

NTSTATUS NTAPI EngpTransferSpriteStateToVisualState(_In_ PPDEVOBJ ppdev);

PSURFACE NTAPI EngpSpGetShapeSurface(_In_ PPDEVOBJ ppdev, _In_ HWND hwnd);

BOOL NTAPI EngpGdiGetSpriteAttributes(
    _In_ PPDEVOBJ ppdev,
    _In_ HWND hwnd,
    _In_opt_ PVOID pShapeHint,
    _Out_opt_ PULONG pulAttrA4,
    _Out_opt_ PBLENDFUNCTION pBlend,
    _Out_opt_ PULONG pulAttrA6);

BOOL NTAPI EngpSpriteTryGetExtents(
    _In_ PPDEVOBJ ppdev,
    _In_ HWND hwnd,
    _Out_ PULONG pcx,
    _Out_ PULONG pcy);

ULONG_PTR NTAPI EngpDwmBlendStateFromSpriteAttrs(
    _In_opt_ const BLENDFUNCTION *bf,
    _In_ ULONG ulAttr39,
    _In_ ULONG ulAttr41);
