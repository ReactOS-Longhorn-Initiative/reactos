

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "commctrl.h"
#include "objbase.h"
#include "winerror.h"

#include "comctl32.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(commctrl);

#ifdef __REACTOS__
typedef int (CALLBACK *PFNDACOMPARE)(_In_opt_ void *p1, _In_opt_ void *p2, _In_ LPARAM lParam);
BOOL
WINAPI
DSA_Sort(HDSA Dsa, PFNDACOMPARE Compare, LPARAM lParam)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 0;
}

UINT32 
WINAPI
QuerySystemGestureStatus(HWND hwnd,
                         WPARAM wParam,
                         LPARAM lParam,
                         UINT32 nHtCode)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 0;
}

/**************************************************************************
 * GetTitleFromPropSheetHeader [COMCTL32.174]
 * 
 * TODO:
 */
HRESULT 
WINAPI
CreateDUIPropertySheetPage(_In_ PROPSHEETPAGEW *psp,
                           _Out_ HPROPSHEETPAGE **phpsp)
{
    UNIMPLEMENTED;
    __debugbreak();
    return S_FALSE;
}

/**************************************************************************
 * GetTitleFromPropSheetHeader [COMCTL32.391]
 * 
 * TODO:
 */

HRESULT
WINAPI
GetTitleFromPropSheetHeader(_In_  PROPSHEETHEADERW *ppsh, // TODO: could be wrong, seems like there's new varients in winsdk
                            _In_  UINT32 nIndex,
                            _Out_ LPWSTR pszTitle,
                            _In_  UINT32 cch)
{
    UNIMPLEMENTED;
    __debugbreak();
    return S_FALSE;
}

/**************************************************************************
 * DestroyPropsheetPageArray [COMCTL32.165]
 * 
 * 
 * TODO: 
 */

HRESULT
WINAPI
DestroyPropsheetPageArray(_In_  PROPSHEETHEADERW *ppsh)
{
    UNIMPLEMENTED;
    __debugbreak();
    return S_FALSE;
}

/**************************************************************************
 * Markup_Create [COMCTL32.395]
 * 
 * Based on Geoff's site
 * 
 * Seems to return an interface that needs to be reversed in of itself.
 */
HRESULT
WINAPI
Markup_Create(_In_ IUnknown *pMarkupCallback,
              _In_ UINT32 dwId,
              _In_ HFONT *hf,
              _In_ HFONT *hfUnderline,
              _In_ GUID *riid,
              _Out_ PVOID *ppv)
{
    UNIMPLEMENTED;
    __debugbreak();
    return S_OK;
}

/*
 *  The behavior of this ordinal changes between vista, 7, 8, 19
 * Hppefully it's not useful...
 */
HRESULT
WINAPI
UnStableAbiFunction(PVOID Unused)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * Seems to be required for some win10 apps.
 */
HRESULT
WINAPI
ThisFunctionDoesNothing(PVOID Unused)
{
    return 1;
}

/**************************************************************************
 * PremultiplyAlphaChannel [COMCTL32.392]
 */
HBITMAP WINAPI PremultiplyAlphaChannel(HBITMAP hBmpIn, int unknown)
{
    UNREFERENCED_PARAMETER(unknown);
    TRACE("PremultiplyAlphaChannel(%p,%i)\n", hBmpIn, unknown);

    if (!hBmpIn)
        return NULL;

    return CopyImage(
        hBmpIn
        , IMAGE_BITMAP
        , 0, 0
        , LR_CREATEDIBSECTION
    );
}
#endif
