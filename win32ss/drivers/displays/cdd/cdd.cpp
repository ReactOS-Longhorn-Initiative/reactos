
#include "cdd.h"

#include <debug.h>

static LOGFONTW SystemFont = { 16, 7, 0, 0, 700, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH | FF_DONTCARE, L"System" };
static LOGFONTW AnsiVariableFont = { 12, 9, 0, 0, 400, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_STROKE_PRECIS, PROOF_QUALITY, VARIABLE_PITCH | FF_DONTCARE, L"MS Sans Serif" };
static LOGFONTW AnsiFixedFont = { 12, 9, 0, 0, 400, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_STROKE_PRECIS, PROOF_QUALITY, FIXED_PITCH | FF_DONTCARE, L"Courier" };
ULONG_PTR FramebufferMapped;
const PALETTEENTRY BASEPALETTE[20] =
{
   { 0x00, 0x00, 0x00, 0x00 },
   { 0x80, 0x00, 0x00, 0x00 },
   { 0x00, 0x80, 0x00, 0x00 },
   { 0x80, 0x80, 0x00, 0x00 },
   { 0x00, 0x00, 0x80, 0x00 },
   { 0x80, 0x00, 0x80, 0x00 },
   { 0x00, 0x80, 0x80, 0x00 },
   { 0xC0, 0xC0, 0xC0, 0x00 },
   { 0xC0, 0xDC, 0xC0, 0x00 },
   { 0xD4, 0xD0, 0xC8, 0x00 },
   { 0xFF, 0xFB, 0xF0, 0x00 },
   { 0x3A, 0x6E, 0xA5, 0x00 },
   { 0x80, 0x80, 0x80, 0x00 },
   { 0xFF, 0x00, 0x00, 0x00 },
   { 0x00, 0xFF, 0x00, 0x00 },
   { 0xFF, 0xFF, 0x00, 0x00 },
   { 0x00, 0x00, 0xFF, 0x00 },
   { 0xFF, 0x00, 0xFF, 0x00 },
   { 0x00, 0xFF, 0xFF, 0x00 },
   { 0xFF, 0xFF, 0xFF, 0x00 },
};

DWORD
GetAvailableModes(
   HANDLE hDriver,
   PVIDEO_MODE_INFORMATION *ModeInfo,
   DWORD *ModeInfoSize)
{
   UNIMPLEMENTED;
   ULONG ulTemp;
   VIDEO_NUM_MODES Modes;
   PVIDEO_MODE_INFORMATION ModeInfoPtr;

   /*
    * Get the number of modes supported by the mini-port
    */

   if (EngDeviceIoControl(hDriver, IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES, NULL,
                          0, &Modes, sizeof(VIDEO_NUM_MODES), &ulTemp))
   {
      return 0;
   }

   if (Modes.NumModes == 0)
   {
      return 0;
   }

   *ModeInfoSize = Modes.ModeInformationLength;

   /*
    * Allocate the buffer for the miniport to write the modes in.
    */

   *ModeInfo = (PVIDEO_MODE_INFORMATION)EngAllocMem(0, Modes.NumModes *
      Modes.ModeInformationLength, ALLOC_TAG);

   if (*ModeInfo == NULL)
   {
      return 0;
   }
#if 0
   /*
    * Ask the miniport to fill in the available modes.
    */

   if (EngDeviceIoControl(hDriver, IOCTL_VIDEO_QUERY_AVAIL_MODES, NULL, 0,
                          *ModeInfo, Modes.NumModes * Modes.ModeInformationLength,
                          &ulTemp))
   {
      EngFreeMem(*ModeInfo);
      *ModeInfo = NULL;
      return 0;
   }
#endif
   EngFreeMem(*ModeInfo);
    *ModeInfo = NULL;
    return 0;
   /*
    * Now see which of these modes are supported by the display driver.
    * As an internal mechanism, set the length to 0 for the modes we
    * DO NOT support.
    */

   ulTemp = Modes.NumModes;
   ModeInfoPtr = *ModeInfo;

   /*
    * Mode is rejected if it is not one plane, or not graphics, or is not
    * one of 8, 16 or 32 bits per pel.
    */

   while (ulTemp--)
   {
      if ((ModeInfoPtr->NumberOfPlanes != 1) ||
          !(ModeInfoPtr->AttributeFlags & VIDEO_MODE_GRAPHICS) ||
          ((ModeInfoPtr->BitsPerPlane != 8) &&
           (ModeInfoPtr->BitsPerPlane != 16) &&
           (ModeInfoPtr->BitsPerPlane != 24) &&
           (ModeInfoPtr->BitsPerPlane != 32)))
      {
         ModeInfoPtr->Length = 0;
      }

      ModeInfoPtr = (PVIDEO_MODE_INFORMATION)
         (((PUCHAR)ModeInfoPtr) + Modes.ModeInformationLength);
   }

   return Modes.NumModes;
}

BOOL
IntInitScreenInfo(
   PCDDPDEV ppdev,
   LPDEVMODEW pDevMode,
   PGDIINFO pGdiInfo,
   PDEVINFO pDevInfo)
{
   ULONG ModeCount;
   ULONG ModeInfoSize;
   PVIDEO_MODE_INFORMATION ModeInfo, ModeInfoPtr, SelectedMode = NULL;
   //VIDEO_COLOR_CAPABILITIES ColorCapabilities;
  // ULONG Temp;
    ULONG BytesPerPixel = 4;

    VIDEO_MODE_INFORMATION ModeInfoList[1];
   ModeInfoList[0].ModeIndex = 0;
   ModeInfoList[0].VisScreenWidth = 800;
   ModeInfoList[0].VisScreenHeight = 600;
   ModeInfoList[0].Length = 0x1D4C00;
    ModeInfoList[0].ScreenStride = 800 * BytesPerPixel;
    ModeInfoList[0].NumberOfPlanes = 1;
    ModeInfoList[0].BitsPerPlane = BytesPerPixel * 8;
    ModeInfoList[0].Frequency = 60;
   ModeInfoList[0].XMillimeter = 0; /* FIXME */
   ModeInfoList[0].YMillimeter = 0; /* FIXME */
    if (BytesPerPixel >= 3)
    {
        ModeInfoList[0].NumberRedBits = 8;
        ModeInfoList[0].NumberGreenBits = 8;
        ModeInfoList[0].NumberBlueBits = 8;
        ModeInfoList[0].RedMask = 0xFF0000;
        ModeInfoList[0].GreenMask = 0x00FF00;
        ModeInfoList[0].BlueMask = 0x0000FF;
    }
    else
    {
        /* FIXME: not implemented */
       // WARN_(IHVVIDEO, "BytesPerPixel %d - not implemented\n", BytesPerPixel);
    }
    ModeInfoList[0].VideoMemoryBitmapWidth = ModeInfoList[0].VisScreenWidth;
    ModeInfoList[0].VideoMemoryBitmapHeight = ModeInfoList[0].VisScreenHeight;
    ModeInfoList[0].AttributeFlags = VIDEO_MODE_GRAPHICS | VIDEO_MODE_COLOR |
        VIDEO_MODE_NO_OFF_SCREEN;
   ModeInfoList[0].DriverSpecificAttributeFlags = 0;

   ModeInfoSize = sizeof(VIDEO_MODE_INFORMATION) * 2;
   ModeInfo = ModeInfoList;
   /*
    * Call miniport to get information about video modes.
    */

   ModeCount = 1;//GetAvailableModes(ppdev->hDriver, &ModeInfo, &ModeInfoSize);
   if (ModeCount == 0)
   {
      return FALSE;
   }

   /*
    * Select the video mode depending on the info passed in pDevMode.
    */

   if (pDevMode->dmPelsWidth == 0 && pDevMode->dmPelsHeight == 0 &&
       pDevMode->dmBitsPerPel == 0 && pDevMode->dmDisplayFrequency == 0)
   {
      ModeInfoPtr = ModeInfo;
      while (ModeCount-- > 0)
      {
         if (ModeInfoPtr->Length == 0)
         {
            ModeInfoPtr = (PVIDEO_MODE_INFORMATION)
               (((PUCHAR)ModeInfoPtr) + ModeInfoSize);
            continue;
         }
         SelectedMode = ModeInfoPtr;
         break;
      }
   }
   else
   {
      ModeInfoPtr = ModeInfo;
      while (ModeCount-- > 0)
      {
         if (ModeInfoPtr->Length > 0 &&
             pDevMode->dmPelsWidth == ModeInfoPtr->VisScreenWidth &&
             pDevMode->dmPelsHeight == ModeInfoPtr->VisScreenHeight &&
             pDevMode->dmBitsPerPel == (ModeInfoPtr->BitsPerPlane *
                                        ModeInfoPtr->NumberOfPlanes) &&
             pDevMode->dmDisplayFrequency == ModeInfoPtr->Frequency)
         {
            SelectedMode = ModeInfoPtr;
            break;
         }

         ModeInfoPtr = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)ModeInfoPtr) + ModeInfoSize);
      }
   }

   if (SelectedMode == NULL)
   {

      //EngFreeMem(ModeInfo);
      //return FALSE;
   }

   /*
    * Fill in the GDIINFO data structure with the information returned from
    * the kernel driver.
    */

   ppdev->ModeIndex = SelectedMode->ModeIndex;
   ppdev->ScreenWidth = SelectedMode->VisScreenWidth;
   ppdev->ScreenHeight = SelectedMode->VisScreenHeight;
   ppdev->ScreenDelta = SelectedMode->ScreenStride;
   ppdev->BitsPerPixel = (UCHAR)(SelectedMode->BitsPerPlane * SelectedMode->NumberOfPlanes);

   ppdev->MemWidth = SelectedMode->VideoMemoryBitmapWidth;
   ppdev->MemHeight = SelectedMode->VideoMemoryBitmapHeight;

   ppdev->RedMask = SelectedMode->RedMask;
   ppdev->GreenMask = SelectedMode->GreenMask;
   ppdev->BlueMask = SelectedMode->BlueMask;

   pGdiInfo->ulVersion = GDI_DRIVER_VERSION;
   pGdiInfo->ulTechnology = DT_RASDISPLAY;
   pGdiInfo->ulHorzSize = SelectedMode->XMillimeter;
   pGdiInfo->ulVertSize = SelectedMode->YMillimeter;
   pGdiInfo->ulHorzRes = SelectedMode->VisScreenWidth;
   pGdiInfo->ulVertRes = SelectedMode->VisScreenHeight;
   pGdiInfo->ulPanningHorzRes = SelectedMode->VisScreenWidth;
   pGdiInfo->ulPanningVertRes = SelectedMode->VisScreenHeight;
   pGdiInfo->cBitsPixel = SelectedMode->BitsPerPlane;
   pGdiInfo->cPlanes = SelectedMode->NumberOfPlanes;
   pGdiInfo->ulVRefresh = SelectedMode->Frequency;
   pGdiInfo->ulBltAlignment = 1;
   pGdiInfo->ulLogPixelsX = pDevMode->dmLogPixels;
   pGdiInfo->ulLogPixelsY = pDevMode->dmLogPixels;
   pGdiInfo->flTextCaps = TC_RA_ABLE;
   pGdiInfo->flRaster = 0;
   pGdiInfo->ulDACRed = SelectedMode->NumberRedBits;
   pGdiInfo->ulDACGreen = SelectedMode->NumberGreenBits;
   pGdiInfo->ulDACBlue = SelectedMode->NumberBlueBits;
   pGdiInfo->ulAspectX = 0x24;
   pGdiInfo->ulAspectY = 0x24;
   pGdiInfo->ulAspectXY = 0x33;
   pGdiInfo->xStyleStep = 1;
   pGdiInfo->yStyleStep = 1;
   pGdiInfo->denStyleStep = 3;
   pGdiInfo->ptlPhysOffset.x = 0;
   pGdiInfo->ptlPhysOffset.y = 0;
   pGdiInfo->szlPhysSize.cx = 0;
   pGdiInfo->szlPhysSize.cy = 0;



   pGdiInfo->ciDevice.Red.Y = 0;
   pGdiInfo->ciDevice.Green.Y = 0;
   pGdiInfo->ciDevice.Blue.Y = 0;
   pGdiInfo->ciDevice.Cyan.x = 0;
   pGdiInfo->ciDevice.Cyan.y = 0;
   pGdiInfo->ciDevice.Cyan.Y = 0;
   pGdiInfo->ciDevice.Magenta.x = 0;
   pGdiInfo->ciDevice.Magenta.y = 0;
   pGdiInfo->ciDevice.Magenta.Y = 0;
   pGdiInfo->ciDevice.Yellow.x = 0;
   pGdiInfo->ciDevice.Yellow.y = 0;
   pGdiInfo->ciDevice.Yellow.Y = 0;
   pGdiInfo->ciDevice.MagentaInCyanDye = 0;
   pGdiInfo->ciDevice.YellowInCyanDye = 0;
   pGdiInfo->ciDevice.CyanInMagentaDye = 0;
   pGdiInfo->ciDevice.YellowInMagentaDye = 0;
   pGdiInfo->ciDevice.CyanInYellowDye = 0;
   pGdiInfo->ciDevice.MagentaInYellowDye = 0;
   pGdiInfo->ulDevicePelsDPI = 0;
   pGdiInfo->ulPrimaryOrder = PRIMARY_ORDER_CBA;
   pGdiInfo->ulHTPatternSize = HT_PATSIZE_4x4_M;
   pGdiInfo->flHTFlags = HT_FLAG_ADDITIVE_PRIMS;

   pDevInfo->flGraphicsCaps = 0;
   pDevInfo->lfDefaultFont = SystemFont;
   pDevInfo->lfAnsiVarFont = AnsiVariableFont;
   pDevInfo->lfAnsiFixFont = AnsiFixedFont;
   pDevInfo->cFonts = 0;
   pDevInfo->cxDither = 0;
   pDevInfo->cyDither = 0;
   pDevInfo->hpalDefault = 0;
   pDevInfo->flGraphicsCaps2 = 0;

      pGdiInfo->ulNumColors = (ULONG)(-1);
      pGdiInfo->ulNumPalReg = 0;

            pGdiInfo->ulHTOutputFormat = HT_FORMAT_32BPP;
            pDevInfo->iDitherFormat = BMF_32BPP;



 //  EngFreeMem(ModeInfo);
   return TRUE;
}

BOOL
IntInitDefaultPalette(
   PCDDPDEV ppdev,
   PDEVINFO pDevInfo)
{
   ULONG ColorLoop;
   PPALETTEENTRY PaletteEntryPtr;

   if (ppdev->BitsPerPixel > 8)
   {
      ppdev->DefaultPalette = pDevInfo->hpalDefault =
         EngCreatePalette(PAL_BITFIELDS, 0, NULL,
            ppdev->RedMask, ppdev->GreenMask, ppdev->BlueMask);
   }
   else
   {
      ppdev->PaletteEntries = (PALETTEENTRY*)EngAllocMem(0, sizeof(PALETTEENTRY) << 8, ALLOC_TAG);
      if (ppdev->PaletteEntries == NULL)
      {
         return FALSE;
      }

      for (ColorLoop = 256, PaletteEntryPtr = ppdev->PaletteEntries;
           ColorLoop != 0;
           ColorLoop--, PaletteEntryPtr++)
      {
         PaletteEntryPtr->peRed = ((ColorLoop >> 5) & 7) * 255 / 7;
         PaletteEntryPtr->peGreen = ((ColorLoop >> 3) & 3) * 255 / 3;
         PaletteEntryPtr->peBlue = (ColorLoop & 7) * 255 / 7;
         PaletteEntryPtr->peFlags = 0;
      }

      memcpy(ppdev->PaletteEntries, BASEPALETTE, 10 * sizeof(PALETTEENTRY));
      memcpy(ppdev->PaletteEntries + 246, BASEPALETTE + 10, 10 * sizeof(PALETTEENTRY));

      ppdev->DefaultPalette = pDevInfo->hpalDefault =
         EngCreatePalette(PAL_INDEXED, 256, (PULONG)ppdev->PaletteEntries, 0, 0, 0);
    }

    return ppdev->DefaultPalette != NULL;
}

BOOL APIENTRY
IntSetPalette(
   IN DHPDEV dhpdev,
   IN PPALETTEENTRY ppalent,
   IN ULONG iStart,
   IN ULONG cColors)
{
   UNIMPLEMENTED;
 //  __debugbreak();
   return TRUE;
}

/* *******************************************************************/
VOID APIENTRY
DrvDisableSurface(
   IN DHPDEV dhpdev)
{
   PCDDPDEV ppdev = (PCDDPDEV)dhpdev;

   EngDeleteSurface(ppdev->hSurfEng);
   ppdev->hSurfEng = NULL;
}

HSURF APIENTRY
DrvEnableSurface(
   IN DHPDEV dhpdev)
{
   PCDDPDEV ppdev = (PCDDPDEV)dhpdev;
   HSURF hSurface;
   ULONG BitmapType;
   SIZEL ScreenSize;
   VIDEO_MEMORY_INFORMATION VideoMemoryInfo;
  // ULONG ulTemp;

   DPRINT1("DrvEnableSurface: Entry\n");
  // __debugbreak();

   /*
    * Set the current mode
    */

   /*
    * Map the framebuffer into our memory.
    */
   PHYSICAL_ADDRESS Addr;
   Addr.QuadPart = 0x80000000;
   FramebufferMapped = (ULONG_PTR)MmMapIoSpace(Addr, 0x1D4C00, MmNonCached);
   RtlZeroMemory((PVOID)FramebufferMapped,0x1D4C00);
   //TODO: This is just a note, this isn't suppose to be called yet
   VideoMemoryInfo.FrameBufferBase = (PVOID)FramebufferMapped;
#if 0
   if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                          &VideoMemory, sizeof(VIDEO_MEMORY),
                          &VideoMemoryInfo, sizeof(VIDEO_MEMORY_INFORMATION),
                          &ulTemp))
   {
      return NULL;
   }
#endif
   ppdev->ScreenPtr = VideoMemoryInfo.FrameBufferBase;

   switch (ppdev->BitsPerPixel)
   {
      case 8:
       //  IntSetPalette(dhpdev, ppdev->PaletteEntries, 0, 256);
         BitmapType = BMF_8BPP;
         break;

      case 16:
         BitmapType = BMF_16BPP;
         break;

      case 24:
         BitmapType = BMF_24BPP;
         break;

      case 32:
         BitmapType = BMF_32BPP;
         break;

      default:
         return NULL;
   }

   ppdev->iDitherFormat = BitmapType;

   ScreenSize.cx = ppdev->ScreenWidth;
   ScreenSize.cy = ppdev->ScreenHeight;

   hSurface = (HSURF)EngCreateBitmap(ScreenSize, ppdev->ScreenDelta, BitmapType,
                                     (ppdev->ScreenDelta > 0) ? BMF_TOPDOWN : 0,
                                     ppdev->ScreenPtr);
   if (hSurface == NULL)
   {
      return NULL;
   }

   /*
    * Associate the surface with our device.
    */

   if (!EngAssociateSurface(hSurface, ppdev->hDevEng, 0))
   {
      EngDeleteSurface(hSurface);
      return NULL;
   }

   ppdev->hSurfEng = hSurface;

   return hSurface;
}

VOID APIENTRY
DrvMovePointer(
   IN SURFOBJ *pso,
   IN LONG x,
   IN LONG y,
   IN RECTL *prcl)
{
   EngMovePointer(pso, x, y, prcl);
}

BOOL
APIENTRY
DrvIcmSetDeviceGammaRamp(DHPDEV  dhpdev,
                        ULONG   iFormat,
                        LPVOID  lpRamp)
{
   UNIMPLEMENTED;
   return 0;
}

ULONG APIENTRY
DrvSetPointerShape(
   IN SURFOBJ *pso,
   IN SURFOBJ *psoMask,
   IN SURFOBJ *psoColor,
   IN XLATEOBJ *pxlo,
   IN LONG xHot,
   IN LONG yHot,
   IN LONG x,
   IN LONG y,
   IN RECTL *prcl,
   IN FLONG fl)
{
   return EngSetPointerShape(pso, psoMask, psoColor, pxlo, xHot, yHot, x, y, prcl, fl);
}

ULONG
APIENTRY
DrvGetModes(_In_ HANDLE hDriver,
            _In_ ULONG cjSize,
            _Out_ DEVMODEW *pdm)
{
   NTSTATUS Status = STATUS_PROCEDURE_NOT_FOUND;
   /*
    * X 1) Call EngQueryW32kCddInterface here for some reason TODO: why the fuck here?
    * 2) There's some functiom callback happening in the stack trace i dont udnerstand here
    * X 3) Load the Dxgkrnl DeviceObject so we can make IOCTRL calls
    * X 4) Build IOCTRL request seems like IOCTRL code is : 0x23E05B - Not sure what the name would be
    * ill have to make oen up but i know this is where the CDD Interface gets passed.. aka
    * stuff for making some of these requests :)
    * 5) Get the dipslay mode list. from here we parse ths auto translating from D3DFORMAT to
    * what GDI will find quite helpful.
    * 6)
    *
    */
   PFILE_OBJECT RDDM_FileObject;
   PDEVICE_OBJECT RDDM_DeviceObject;
   WIN32CDD_INTERFACE W32kCddInterface;

   PIRP Irp;
   KEVENT Event;

   IO_STATUS_BLOCK IoStatusBlock;
   UNICODE_STRING DestinationString;
  //  D3DKMT_GETDISPLAYMODELIST *GetDisplayModeList;
    W32kCddInterface.Version = RTM_VISTA;
   RtlInitUnicodeString(&DestinationString, L"\\Device\\DxgKrnl");
   Status = IoGetDeviceObjectPointer(&DestinationString, FILE_ALL_ACCESS, &RDDM_FileObject, &RDDM_DeviceObject);
   if(Status != STATUS_SUCCESS)
   {
       DPRINT1("DrvGetModes: Setting up DxgKrnl Failed\n");
   }
   DXGKCDD_INTERFACE Interfaces;
#if 0
   Status = EngQueryW32kCddInterface(hDriver, 0, (PVOID)&W32kCddInterface, (PVOID)DxgAdapter, (PVOID)&OkayLol, (PVOID)&ProcessLocal);
   if (Status != STATUS_SUCCESS)
   {
      DPRINT1("DrvGetModes: EngQueryW32kCddInterface Failed with Status %d\n", Status);
   }
#endif
   /* Build event and create IRP */
   DPRINT1("DrvGetModes: Building IOCTRL with DxgKrnl\n");
   KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
   Irp = IoBuildDeviceIoControlRequest(0x23E05B, /* TODO: decide name*/
                                         RDDM_DeviceObject,
                                         &Interfaces,
                                         sizeof(DXGKCDD_INTERFACE),
                                         &Interfaces,
                                         sizeof(DXGKCDD_INTERFACE),
                                         TRUE,
                                         &Event,
                                         &IoStatusBlock);

   Status = IofCallDriver(RDDM_DeviceObject, Irp);
   KeWaitForSingleObject(&Event, Executive, 0, 0, 0);
   Status = IoStatusBlock.Status;

  // Status = Interfaces.DxgkCddGetDisplayModeList(NULL,&GetDisplayModeList);
   DPRINT1("DxgkCddGetDisplayModeList: Status %d\n", Status);
  // DPRINT1("DxgkCddGetDisplayModeList: Screen Height %d\n", GetDisplayModeList->pModeList->Height);

#if 0
typedef struct _VIDEO_MODE_INFORMATION {
    ULONG Length;
    ULONG ModeIndex;
    ULONG VisScreenWidth;
    ULONG VisScreenHeight;
    ULONG ScreenStride;
    ULONG NumberOfPlanes;
    ULONG BitsPerPlane;
    ULONG Frequency;
    ULONG XMillimeter;
    ULONG YMillimeter;
    ULONG NumberRedBits;
    ULONG NumberGreenBits;
    ULONG NumberBlueBits;
    ULONG RedMask;
    ULONG GreenMask;
    ULONG BlueMask;
    ULONG AttributeFlags;
    ULONG VideoMemoryBitmapWidth;
    ULONG VideoMemoryBitmapHeight;
    ULONG DriverSpecificAttributeFlags;
} VIDEO_MODE_INFORMATION, *PVIDEO_MODE_INFORMATION;
#endif
       ULONG BytesPerPixel = 4;
   VIDEO_MODE_INFORMATION ModeInfoList[1];
   ModeInfoList[0].ModeIndex = 0;
   ModeInfoList[0].VisScreenWidth = 800;
   ModeInfoList[0].VisScreenHeight = 600;
   ModeInfoList[0].Length = 0x1D4C00;
    ModeInfoList[0].ScreenStride = 800 * BytesPerPixel;

    ModeInfoList[0].NumberOfPlanes = 1;
    ModeInfoList[0].BitsPerPlane = BytesPerPixel * 8;
    ModeInfoList[0].Frequency = 60;
   ModeInfoList[0].XMillimeter = 0; /* FIXME */
   ModeInfoList[0].YMillimeter = 0; /* FIXME */
    if (BytesPerPixel >= 3)
    {
        ModeInfoList[0].NumberRedBits = 8;
        ModeInfoList[0].NumberGreenBits = 8;
        ModeInfoList[0].NumberBlueBits = 8;
        ModeInfoList[0].RedMask = 0xFF0000;
        ModeInfoList[0].GreenMask = 0x00FF00;
        ModeInfoList[0].BlueMask = 0x0000FF;
    }
    else
    {
        /* FIXME: not implemented */
       // WARN_(IHVVIDEO, "BytesPerPixel %d - not implemented\n", BytesPerPixel);
    }
    ModeInfoList[0].VideoMemoryBitmapWidth = ModeInfoList[0].VisScreenWidth;
    ModeInfoList[0].VideoMemoryBitmapHeight = ModeInfoList[0].VisScreenHeight;
    ModeInfoList[0].AttributeFlags = VIDEO_MODE_GRAPHICS | VIDEO_MODE_COLOR |
        VIDEO_MODE_NO_OFF_SCREEN;
   ModeInfoList[0].DriverSpecificAttributeFlags = 0;

   ULONG ModeCount;
   ULONG ModeInfoSize = sizeof(VIDEO_MODE_INFORMATION) * 2;
   PVIDEO_MODE_INFORMATION ModeInfo, ModeInfoPtr;
   ULONG OutputSize;

   ModeInfo = ModeInfoList;
   ModeCount = 1;
   if (ModeCount == 0)
   {
      return 0;
   }

   if (pdm == NULL)
   {
      //EngFreeMem(ModeInfo);
      return 1 * sizeof(DEVMODEW);
   }

   /*
    * Copy the information about supported modes into the output buffer.
    */

   OutputSize = 0;
   ModeInfoPtr = ModeInfo;

   while (ModeCount-- > 0)
   {

      memset(pdm, 0, sizeof(DEVMODEW));
      memcpy(pdm->dmDeviceName, DEVICE_NAME, sizeof(DEVICE_NAME));
      pdm->dmSpecVersion =
      pdm->dmDriverVersion = DM_SPECVERSION;
      pdm->dmSize = sizeof(DEVMODEW);
      pdm->dmDriverExtra = 0;
      pdm->dmBitsPerPel = ModeInfoPtr->NumberOfPlanes * ModeInfoPtr->BitsPerPlane;
      pdm->dmPelsWidth = ModeInfoPtr->VisScreenWidth;
      pdm->dmPelsHeight = ModeInfoPtr->VisScreenHeight;
      pdm->dmDisplayFrequency = ModeInfoPtr->Frequency;
      pdm->dmDisplayFlags = 0;
      pdm->dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
                      DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS;

      ModeInfoPtr = (PVIDEO_MODE_INFORMATION)(((ULONG_PTR)ModeInfoPtr) + ModeInfoSize);
      pdm = (LPDEVMODEW)(((ULONG_PTR)pdm) + sizeof(DEVMODEW));
      OutputSize += sizeof(DEVMODEW);
   }

   return OutputSize;
}


BOOL
APIENTRY
DrvSetPalette(
   IN DHPDEV dhpdev,
   IN PALOBJ *ppalo,
   IN FLONG fl,
   IN ULONG iStart,
   IN ULONG cColors)
{
   PPALETTEENTRY PaletteEntries;
   BOOL bRet;

   if (cColors == 0)
       return FALSE;

   PaletteEntries = (PALETTEENTRY*)EngAllocMem(0, cColors * sizeof(ULONG), ALLOC_TAG);
   if (PaletteEntries == NULL)
   {
      return FALSE;
   }

   if (PALOBJ_cGetColors(ppalo, iStart, cColors, (PULONG)PaletteEntries) !=
       cColors)
   {
      EngFreeMem(PaletteEntries);
      return FALSE;
   }

   bRet = FALSE;//IntSetPalette(dhpdev, PaletteEntries, iStart, cColors);
   EngFreeMem(PaletteEntries);
   return bRet;
}

DHPDEV
APIENTRY
DrvEnablePDEV(
   IN DEVMODEW *pdm,
   IN LPWSTR pwszLogAddress,
   IN ULONG cPat,
   OUT HSURF *phsurfPatterns,
   IN ULONG cjCaps,
   OUT ULONG *pdevcaps,
   IN ULONG cjDevInfo,
   OUT DEVINFO *pdi,
   IN HDEV hdev,
   IN LPWSTR pwszDeviceName,
   IN HANDLE hDriver)
{
   PCDDPDEV ppdev;
   GDIINFO GdiInfo;
   DEVINFO DevInfo;

   ppdev = (PCDDPDEV)EngAllocMem(FL_ZERO_MEMORY, sizeof(CDDPDEV), ALLOC_TAG);
   if (ppdev == NULL)
   {
      return NULL;
   }

   ppdev->hDriver = hDriver;

   if (!IntInitScreenInfo(ppdev, pdm, &GdiInfo, &DevInfo))
   {
      //EngFreeMem(ppdev);
     // return NULL;
   }

   if (!IntInitDefaultPalette(ppdev, &DevInfo))
   {
      EngFreeMem(ppdev);
      return NULL;
   }

   memcpy(pdi, &DevInfo, min(sizeof(DEVINFO), cjDevInfo));
   memcpy(pdevcaps, &GdiInfo, min(sizeof(GDIINFO), cjCaps));

   return (DHPDEV)ppdev;
}


/*
 * @implemented
 */
VOID APIENTRY
DrvCompletePDEV(
   IN DHPDEV dhpdev,
   IN HDEV hdev)
{
   ((PCDDPDEV)dhpdev)->hDevEng = hdev;
}

/*
 * @implemented
 */
VOID APIENTRY
DrvDisablePDEV(
   IN DHPDEV dhpdev)
{
   if (((PCDDPDEV)dhpdev)->DefaultPalette)
   {
      EngDeletePalette(((PCDDPDEV)dhpdev)->DefaultPalette);
   }

   if (((PCDDPDEV)dhpdev)->PaletteEntries != NULL)
   {
      EngFreeMem(((PCDDPDEV)dhpdev)->PaletteEntries);
   }

   EngFreeMem(dhpdev);
}