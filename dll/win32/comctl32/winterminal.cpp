
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "uxtheme.h"
#include "vssym32.h"
#include "wine/heap.h"
#include "wine/debug.h"
#include "commoncontrols.h"

WINE_DEFAULT_DEBUG_CHANNEL(comctl32);

/**************************************************************************
 * CreateSmallerIcon [COMCTL32.393]
 * 
 * Reimplemented From Windows Terminal.
 */
EXTERN_C
HRESULT CreateSmallerIcon(HICON hicon, UINT cx, UINT cy, HICON* phico)
{
    TRACE("CreateSmallerIcon: STUB! Doesn't try to fallback to smaller icon\n");
    *phico = nullptr;
    HRESULT hr = S_OK;
    if (!*phico)
    {
        // For whatever reason, we still don't have an icon. Maybe we have a cursor. At any rate,
        // we'll use CopyImage as a last-ditch effort.
        *phico = (HICON)CopyImage(hicon, IMAGE_ICON, cx, cy, LR_COPYFROMRESOURCE);
        hr = *phico ? S_OK : E_FAIL;
    }

    return hr;
}