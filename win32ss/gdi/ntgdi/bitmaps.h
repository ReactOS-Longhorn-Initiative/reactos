#pragma once

/* ---- DWM content redirection: gdi/ntgdi/redirdc.c ---- */

HBITMAP NTAPI GreSelectRedirectionBitmap(_In_ HDC hdc, _In_ HBITMAP hbm);
BOOL    NTAPI GreConvertMemToRedirectionDC(_In_ HDC hdc, _In_ BOOL bRedirect);
BOOL    NTAPI GreConvertRedirectionToMemDC(_In_ HDC hdc, _In_ BOOL bUnredirect);
BOOL    NTAPI GreIsRedirectionDC(_In_ HDC hdc);
BOOL    NTAPI GreGetBitmapPixelSize(_In_ HBITMAP hbm, _Out_ PSIZEL psizl);

INT     APIENTRY  BITMAP_GetObject(SURFACE * bmp, INT count, LPVOID buffer);
HBITMAP FASTCALL BITMAP_CopyBitmap (HBITMAP  hBitmap);

BOOL
NTAPI
GreSetBitmapOwner(
    _In_ HBITMAP hbmp,
    _In_ ULONG ulOwner);

HBITMAP
NTAPI
GreCreateBitmap(
    _In_ ULONG nWidth,
    _In_ ULONG nHeight,
    _In_ ULONG cPlanes,
    _In_ ULONG cBitsPixel,
    _In_opt_ PVOID pvBits);

HBITMAP
NTAPI
GreCreateBitmapEx(
    _In_ ULONG nWidth,
    _In_ ULONG nHeight,
    _In_ ULONG cjWidthBytes,
    _In_ ULONG iFormat,
    _In_ USHORT fjBitmap,
    _In_ ULONG cjSizeImage,
    _In_opt_ PVOID pvBits,
    _In_ FLONG flags);

HBITMAP
NTAPI
GreCreateDIBitmapInternal(
    IN HDC hDc,
    IN INT cx,
    IN INT cy,
    IN DWORD fInit,
    IN OPTIONAL LPBYTE pjInit,
    IN OPTIONAL PBITMAPINFO pbmi,
    IN DWORD iUsage,
    IN FLONG fl,
    IN UINT cjMaxBits,
    IN HANDLE hcmXform);

BOOL
NTAPI
GreGetBitmapDimension(
    _In_ HBITMAP hBitmap,
    _Out_ LPSIZE psizDim);

