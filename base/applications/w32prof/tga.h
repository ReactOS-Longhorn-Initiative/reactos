#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _W32PROF_IMAGE_RGBA
{
    UINT Width;
    UINT Height;
    UINT StrideBytes; /* bytes per row */
    BYTE* Pixels;     /* RGBA8, top-left origin */
} W32PROF_IMAGE_RGBA;

BOOL W32Prof_TgaDecodeToRgba(const void* data, SIZE_T size, W32PROF_IMAGE_RGBA* outImage);
void W32Prof_ImageFree(W32PROF_IMAGE_RGBA* image);

BOOL W32Prof_LoadLogoTestTgaFromResource(HINSTANCE hInstance, W32PROF_IMAGE_RGBA* outImage);

#ifdef __cplusplus
}
#endif
