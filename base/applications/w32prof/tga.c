#include "tga.h"

#include <string.h>

#include "resource.h"

#pragma pack(push, 1)
typedef struct _TGA_HEADER
{
    BYTE  IdLength;
    BYTE  ColorMapType;
    BYTE  ImageType;
    WORD  ColorMapFirstIndex;
    WORD  ColorMapLength;
    BYTE  ColorMapEntrySize;
    WORD  XOrigin;
    WORD  YOrigin;
    WORD  Width;
    WORD  Height;
    BYTE  PixelDepth;
    BYTE  ImageDescriptor;
} TGA_HEADER;
#pragma pack(pop)

static BOOL
SafeAddSizeT(SIZE_T a, SIZE_T b, SIZE_T* out)
{
    if (!out) return FALSE;
    if (a > ((SIZE_T)-1) - b) return FALSE;
    *out = a + b;
    return TRUE;
}

static BOOL
AllocImage(UINT w, UINT h, W32PROF_IMAGE_RGBA* out)
{
    SIZE_T stride;
    SIZE_T total;
    BYTE* buf;

    if (!out || w == 0 || h == 0)
        return FALSE;

    stride = (SIZE_T)w * 4;
    if (!SafeAddSizeT(stride, 0, &stride))
        return FALSE;

    if (!SafeAddSizeT(stride * (SIZE_T)h, 0, &total))
        return FALSE;

    buf = (BYTE*)HeapAlloc(GetProcessHeap(), 0, total);
    if (!buf)
        return FALSE;

    out->Width = w;
    out->Height = h;
    out->StrideBytes = (UINT)stride;
    out->Pixels = buf;
    return TRUE;
}

void
W32Prof_ImageFree(W32PROF_IMAGE_RGBA* image)
{
    if (!image)
        return;

    if (image->Pixels)
        HeapFree(GetProcessHeap(), 0, image->Pixels);

    image->Width = 0;
    image->Height = 0;
    image->StrideBytes = 0;
    image->Pixels = NULL;
}

static void
WritePixelRGBA(BYTE* dst, BYTE b, BYTE g, BYTE r, BYTE a)
{
    dst[0] = r;
    dst[1] = g;
    dst[2] = b;
    dst[3] = a;
}

BOOL
W32Prof_TgaDecodeToRgba(const void* data, SIZE_T size, W32PROF_IMAGE_RGBA* outImage)
{
    const BYTE* p;
    const BYTE* end;
    const TGA_HEADER* hdr;
    UINT w, h;
    UINT bytesPerPixel;
    BOOL originTop;
    UINT x, y;

    if (!data || size < sizeof(TGA_HEADER) || !outImage)
        return FALSE;

    ZeroMemory(outImage, sizeof(*outImage));

    p = (const BYTE*)data;
    end = p + size;
    hdr = (const TGA_HEADER*)p;

    /* Only support true-color, no colormap. */
    if (hdr->ColorMapType != 0)
        return FALSE;

    if (!(hdr->ImageType == 2 || hdr->ImageType == 10))
        return FALSE;

    if (!(hdr->PixelDepth == 24 || hdr->PixelDepth == 32))
        return FALSE;

    w = (UINT)hdr->Width;
    h = (UINT)hdr->Height;
    if (w == 0 || h == 0)
        return FALSE;

    bytesPerPixel = (hdr->PixelDepth == 32) ? 4 : 3;

    /* Bit 5 of ImageDescriptor: 1 = top-left origin */
    originTop = ((hdr->ImageDescriptor & 0x20) != 0);

    p += sizeof(TGA_HEADER);

    /* Skip ID field */
    if (hdr->IdLength)
    {
        if (p + hdr->IdLength > end)
            return FALSE;
        p += hdr->IdLength;
    }

    /* Skip color map if present (shouldn't happen due to ColorMapType==0) */

    if (!AllocImage(w, h, outImage))
        return FALSE;

    if (hdr->ImageType == 2)
    {
        /* Uncompressed */
        SIZE_T need = (SIZE_T)w * (SIZE_T)h * (SIZE_T)bytesPerPixel;
        if (p + need > end)
        {
            W32Prof_ImageFree(outImage);
            return FALSE;
        }

        for (y = 0; y < h; y++)
        {
            UINT sy = originTop ? y : (h - 1 - y);
            BYTE* row = outImage->Pixels + (SIZE_T)sy * outImage->StrideBytes;
            for (x = 0; x < w; x++)
            {
                BYTE b = p[0];
                BYTE g = p[1];
                BYTE r = p[2];
                BYTE a = (bytesPerPixel == 4) ? p[3] : 0xFF;
                WritePixelRGBA(row + (SIZE_T)x * 4, b, g, r, a);
                p += bytesPerPixel;
            }
        }

        return TRUE;
    }

    /* RLE true-color (type 10) */
    x = 0;
    y = 0;
    while (y < h)
    {
        BYTE packet;
        UINT count;
        BOOL rle;

        if (p >= end)
        {
            W32Prof_ImageFree(outImage);
            return FALSE;
        }

        packet = *p++;
        rle = ((packet & 0x80) != 0);
        count = (packet & 0x7F) + 1;

        if (rle)
        {
            BYTE b, g, r, a;
            if (p + bytesPerPixel > end)
            {
                W32Prof_ImageFree(outImage);
                return FALSE;
            }
            b = p[0]; g = p[1]; r = p[2];
            a = (bytesPerPixel == 4) ? p[3] : 0xFF;
            p += bytesPerPixel;

            while (count--)
            {
                UINT sy = originTop ? y : (h - 1 - y);
                BYTE* row = outImage->Pixels + (SIZE_T)sy * outImage->StrideBytes;
                WritePixelRGBA(row + (SIZE_T)x * 4, b, g, r, a);

                x++;
                if (x >= w)
                {
                    x = 0;
                    y++;
                    if (y >= h)
                        break;
                }
            }
        }
        else
        {
            while (count--)
            {
                BYTE b, g, r, a;
                if (p + bytesPerPixel > end)
                {
                    W32Prof_ImageFree(outImage);
                    return FALSE;
                }
                b = p[0]; g = p[1]; r = p[2];
                a = (bytesPerPixel == 4) ? p[3] : 0xFF;
                p += bytesPerPixel;

                {
                    UINT sy = originTop ? y : (h - 1 - y);
                    BYTE* row = outImage->Pixels + (SIZE_T)sy * outImage->StrideBytes;
                    WritePixelRGBA(row + (SIZE_T)x * 4, b, g, r, a);
                }

                x++;
                if (x >= w)
                {
                    x = 0;
                    y++;
                    if (y >= h)
                        break;
                }
            }
        }
    }

    return TRUE;
}

BOOL
W32Prof_LoadLogoTestTgaFromResource(HINSTANCE hInstance, W32PROF_IMAGE_RGBA* outImage)
{
    HRSRC hrsrc;
    HGLOBAL hglob;
    DWORD sz;
    const void* ptr;

    if (!outImage)
        return FALSE;

    if (!hInstance)
        hInstance = GetModuleHandle(NULL);

    hrsrc = FindResource(hInstance, MAKEINTRESOURCE(IDR_LOGO_TEST_TGA), RT_RCDATA);
    if (!hrsrc)
        return FALSE;

    sz = SizeofResource(hInstance, hrsrc);
    if (sz == 0)
        return FALSE;

    hglob = LoadResource(hInstance, hrsrc);
    if (!hglob)
        return FALSE;

    ptr = LockResource(hglob);
    if (!ptr)
        return FALSE;

    return W32Prof_TgaDecodeToRgba(ptr, (SIZE_T)sz, outImage);
}
