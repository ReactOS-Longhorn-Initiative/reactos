/*
 * ReactOS Generic Framebuffer display driver directdraw interface
 *
 * Copyright (C) 2006 Magnus Olsen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* Here we put in all 2d api for directdraw and redirect some of them to GDI api */

#include "framebuf.h"
#include <debug.h>
static ULONG
DdGetBytesPerPixel(PDD_SURFACE_LOCAL pSurface, PPDEV ppdev)
{
    ULONG bpp = 0;

    if (pSurface && pSurface->lpGbl)
        bpp = pSurface->lpGbl->ddpfSurface.dwRGBBitCount;

    if (bpp == 0 && ppdev)
        bpp = ppdev->BitsPerPixel;

    return bpp / 8;
}

static PBYTE
DdGetSurfaceBase(PPDEV ppdev, PDD_SURFACE_LOCAL pSurface)
{
    FLATPTR offset;

    if (!ppdev || !pSurface || !pSurface->lpGbl)
        return NULL;

    offset = pSurface->lpGbl->fpVidMem;
    if ((pSurface->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) && offset == 0)
        return (PBYTE)ppdev->ScreenPtr;

    return (PBYTE)ppdev->ScreenPtr + offset;
}

static void
DdFillLine(PBYTE pLine, ULONG width, ULONG bytesPerPixel, DWORD color)
{
    ULONG x;

    switch (bytesPerPixel)
    {
        case 1:
            memset(pLine, (BYTE)color, width);
            break;
        case 2:
        {
            USHORT value = (USHORT)color;
            USHORT *dst = (USHORT *)pLine;
            for (x = 0; x < width; x++)
                dst[x] = value;
            break;
        }
        case 3:
        {
            BYTE r = (BYTE)(color & 0xFF);
            BYTE g = (BYTE)((color >> 8) & 0xFF);
            BYTE b = (BYTE)((color >> 16) & 0xFF);
            for (x = 0; x < width; x++)
            {
                pLine[x * 3 + 0] = r;
                pLine[x * 3 + 1] = g;
                pLine[x * 3 + 2] = b;
            }
            break;
        }
        case 4:
        {
            ULONG *dst = (ULONG *)pLine;
            for (x = 0; x < width; x++)
                dst[x] = color;
            break;
        }
        default:
            break;
    }
}

DWORD CALLBACK
DdCanCreateSurface(PDD_CANCREATESURFACEDATA pccsd)
{

	 /* We do not support 3d buffer so we fail here */
	 if ((pccsd->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_ZBUFFER) &&
		(pccsd->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
	 {
		pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
        return DDHAL_DRIVER_HANDLED;
	 }


	 /* Check if another pixel format or not, we fail for now */
	 if (pccsd->bIsDifferentPixelFormat)
     {
		/* check the fourcc diffent FOURCC, but we only support BMP for now */
		//if(pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_FOURCC)
        //{
		//	/* We do not support other pixel format */
		//	switch (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC)
		//	{
		//		default:
		//			pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
		//			return DDHAL_DRIVER_HANDLED;
		//	}
		//}
		// /* check the texture support, we do not support testure for now */
		//else if((pccsd->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_TEXTURE))
		//{
		//	/* We do not support texture surface */
		//	pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
		//	return DDHAL_DRIVER_HANDLED;
		//}

		/* Fail */
		pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
		return DDHAL_DRIVER_HANDLED;
    }

	 pccsd->ddRVal = DD_OK;
	 return DDHAL_DRIVER_HANDLED;
}

DWORD CALLBACK
DdCreateSurface(PDD_CREATESURFACEDATA pcsd)
{
	int i;

	if (pcsd->dwSCnt < 1)
	{
		pcsd->ddRVal = DDERR_GENERIC;
        return DDHAL_DRIVER_NOTHANDLED;
	}


	for (i=0; i<(int)pcsd->dwSCnt; i++)
    {
		pcsd->lplpSList[i]->lpGbl->lPitch = (DWORD)(pcsd->lplpSList[i]->lpGbl->wWidth *
			                                (pcsd->lplpSList[i]->lpGbl->ddpfSurface.dwRGBBitCount / 8));

		pcsd->lplpSList[i]->lpGbl->dwBlockSizeX = pcsd->lplpSList[i]->lpGbl->lPitch *
			                                      (DWORD)(pcsd->lplpSList[i]->lpGbl->wHeight);

        pcsd->lplpSList[i]->lpGbl->dwBlockSizeY = 1;

        if ( pcsd->lplpSList[i] ->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
        {
			/* We maybe should alloc it with EngAlloc
			   for now we trusting ddraw alloc it        */
            pcsd->lplpSList[i]->lpGbl->fpVidMem = 0;
        }
        else
        {

			/* We maybe should alloc it with EngAlloc
			   for now we trusting ddraw alloc it        */
            pcsd->lplpSList[i]->lpGbl->fpVidMem = DDHAL_PLEASEALLOC_BLOCKSIZE;
        }

        pcsd->lpDDSurfaceDesc->lPitch = pcsd->lplpSList[i]->lpGbl->lPitch;
        pcsd->lpDDSurfaceDesc->dwFlags |= DDSD_PITCH;

    } // for i



	pcsd->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
}

DWORD CALLBACK
DdLock(PDD_LOCKDATA pld)
{
    PPDEV ppdev;
    PBYTE base;
    ULONG bytesPerPixel;
    LONG pitch;
    LONG left = 0;
    LONG top = 0;

    if (!pld || !pld->lpDD || !pld->lpDDSurface || !pld->lpDDSurface->lpGbl)
    {
        if (pld)
            pld->ddRVal = DDERR_INVALIDPARAMS;
        return DDHAL_DRIVER_HANDLED;
    }

    ppdev = (PPDEV)pld->lpDD->dhpdev;
    base = DdGetSurfaceBase(ppdev, pld->lpDDSurface);
    if (!base)
    {
        pld->ddRVal = DDERR_GENERIC;
        return DDHAL_DRIVER_HANDLED;
    }

    pitch = pld->lpDDSurface->lpGbl->lPitch;
    bytesPerPixel = DdGetBytesPerPixel(pld->lpDDSurface, ppdev);
    if (bytesPerPixel == 0)
    {
        pld->ddRVal = DDERR_INVALIDPIXELFORMAT;
        return DDHAL_DRIVER_HANDLED;
    }

    if (pld->bHasRect)
    {
        left = pld->rArea.left;
        top = pld->rArea.top;
    }

    pld->lpSurfData = base + (top * pitch) + (left * bytesPerPixel);
    pld->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
}

DWORD CALLBACK
DdUnlock(PDD_UNLOCKDATA pud)
{
    if (!pud)
        return DDHAL_DRIVER_NOTHANDLED;

    pud->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
}

DWORD CALLBACK
DdSetColorKey(PDD_SETCOLORKEYDATA psck)
{
    if (!psck)
        return DDHAL_DRIVER_NOTHANDLED;

    psck->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
}

DWORD CALLBACK
DdDestroySurface(PDD_DESTROYSURFACEDATA pdsd)
{
    if (!pdsd)
        return DDHAL_DRIVER_NOTHANDLED;

    pdsd->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
}

DWORD CALLBACK
DdBlt(PDD_BLTDATA pbd)
{
    PPDEV ppdev;
    PBYTE destBase;
    PBYTE srcBase;
    ULONG bytesPerPixel;
    LONG destPitch;
    LONG srcPitch;
    LONG width;
    LONG height;
    DWORD flags;

    if (!pbd || !pbd->lpDDDestSurface || !pbd->lpDDDestSurface->lpGbl)
        return DDHAL_DRIVER_NOTHANDLED;

    ppdev = (PPDEV)pbd->lpDD->dhpdev;
    destBase = DdGetSurfaceBase(ppdev, pbd->lpDDDestSurface);
    if (!destBase)
    {
        pbd->ddRVal = DDERR_GENERIC;
        return DDHAL_DRIVER_HANDLED;
    }

    bytesPerPixel = DdGetBytesPerPixel(pbd->lpDDDestSurface, ppdev);
    destPitch = pbd->lpDDDestSurface->lpGbl->lPitch;
    width = pbd->rDest.right - pbd->rDest.left;
    height = pbd->rDest.bottom - pbd->rDest.top;

    if (width <= 0 || height <= 0 || bytesPerPixel == 0)
    {
        pbd->ddRVal = DDERR_INVALIDPARAMS;
        return DDHAL_DRIVER_HANDLED;
    }

    flags = pbd->dwFlags & ~DDBLT_WAIT;
    if (flags == DDBLT_COLORFILL)
    {
        LONG y;
        PBYTE line = destBase + (pbd->rDest.top * destPitch) +
                     (pbd->rDest.left * bytesPerPixel);

        for (y = 0; y < height; y++)
        {
            DdFillLine(line, width, bytesPerPixel, pbd->bltFX.dwFillColor);
            line += destPitch;
        }

        pbd->ddRVal = DD_OK;
        return DDHAL_DRIVER_HANDLED;
    }

    if (flags != 0 || !pbd->lpDDSrcSurface || !pbd->lpDDSrcSurface->lpGbl)
        return DDHAL_DRIVER_NOTHANDLED;

    srcBase = DdGetSurfaceBase(ppdev, pbd->lpDDSrcSurface);
    if (!srcBase)
    {
        pbd->ddRVal = DDERR_GENERIC;
        return DDHAL_DRIVER_HANDLED;
    }

    srcPitch = pbd->lpDDSrcSurface->lpGbl->lPitch;
    if (width != (pbd->rSrc.right - pbd->rSrc.left) ||
        height != (pbd->rSrc.bottom - pbd->rSrc.top))
    {
        return DDHAL_DRIVER_NOTHANDLED;
    }

    {
        LONG y;
        PBYTE dstLine = destBase + (pbd->rDest.top * destPitch) +
                        (pbd->rDest.left * bytesPerPixel);
        PBYTE srcLine = srcBase + (pbd->rSrc.top * srcPitch) +
                        (pbd->rSrc.left * bytesPerPixel);
        ULONG rowBytes = (ULONG)width * bytesPerPixel;

        for (y = 0; y < height; y++)
        {
            memmove(dstLine, srcLine, rowBytes);
            dstLine += destPitch;
            srcLine += srcPitch;
        }
    }

    pbd->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
}

DWORD CALLBACK
DdFlip(PDD_FLIPDATA pfd)
{
    if (!pfd || !pfd->lpSurfCurr || !pfd->lpSurfTarg)
        return DDHAL_DRIVER_NOTHANDLED;

    pfd->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
}
