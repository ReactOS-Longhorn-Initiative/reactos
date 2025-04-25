#include <uxthemep.h>
#include <debug.h>

EXTERN_C
HRESULT 
WINAPI
QueryThemeServices(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
OpenThemeFile(LPCWSTR pszThemeFileName,
              LPCWSTR pszColorName,
              LPCWSTR pszSizeName,
              HTHEMEFILE *hThemeFile,
              DWORD unknown)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
CloseThemeFile(HTHEMEFILE hThemeFile)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
ApplyTheme(HTHEMEFILE hThemeFile,
           char *unknown,
           HWND hWnd)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HANIMATIONBUFFER
WINAPI
BeginBufferedAnimation(HWND hwnd, HDC hdcTarget, const RECT *rcTarget,
    BP_BUFFERFORMAT dwFormat, BP_PAINTPARAMS *pPaintParams,
    BP_ANIMATIONPARAMS *pAnimationParams, HDC *phdcFrom,
    HDC *phdcTo)
{
    UNIMPLEMENTED;
    return NULL;
}

EXTERN_C
HPAINTBUFFER
WINAPI
BeginBufferedPaint(HDC hdcTarget,
                   const RECT * prcTarget,
                   BP_BUFFERFORMAT dwFormat,
                   BP_PAINTPARAMS *pPaintParams,
                   HDC *phdc)
{
    UNIMPLEMENTED;
    return NULL;
}

EXTERN_C
HRESULT 
WINAPI
GetThemeDefaults(LPCWSTR pszThemeFileName, LPWSTR pszColorName,
                 DWORD dwColorNameLen, LPWSTR pszSizeName,
                 DWORD dwSizeNameLen)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
EnumThemes(LPCWSTR pszThemePath, ENUMTHEMEPROC callback,
           LPVOID lpData)
{
    UNIMPLEMENTED;
    return 0;
} 

EXTERN_C
HRESULT 
WINAPI
EnumThemeColors(LPWSTR pszThemeFileName, LPWSTR pszSizeName,
                DWORD dwColorNum, PTHEMENAMES pszColorNames)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
EnumThemeSizes(LPWSTR pszThemeFileName, LPWSTR pszColorName,
               DWORD dwSizeNum, PTHEMENAMES pszSizeNames)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
ParseThemeIniFile(LPCWSTR pszIniFileName, LPWSTR pszUnknown,
                  PARSETHEMEINIFILEPROC callback, LPVOID lpData)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
BufferedPaintClear(HPAINTBUFFER hBufferedPaint, const RECT *prc)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
DrawNCPreview(HDC hDC,
              DWORD DNCP_Flag,
              LPRECT prcPreview,
              _In_ LPCWSTR pszThemeFileName,
              _In_ LPCWSTR pszColorName,
              _In_ LPCWSTR pszSizeName,
              PNONCLIENTMETRICSW pncMetrics,
              COLORREF* lpaRgbValues)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
RegisterDefaultTheme(_In_ LPCWSTR lpString, _In_ BOOLEAN DoForce)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
DumpLoadedThemeToTextFile(_In_  HTHEMEFILE hThemeFile,
                          _Out_ LPCWSTR pszTextFile,
                          _In_  BOOLEAN IsPacked,
                          _In_  BOOLEAN IsFullInfo)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
OpenThemeDataFromFile(_In_ HTHEMEFILE hLoadedThemeFile,
                      _In_ HWND hwnd,
                      _In_ LPCWSTR pszClassList,
                      _In_ BOOLEAN IsClient)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
OpenThemeFileFromData(_In_  HTHEMEFILE hTheme,
                      _Out_ HTHEMEFILE *phThemeFile)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
SessionAllocate()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
SessionFree()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
ThemeHooksOn()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
ThemeHooksOff()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
AreThemeHooksActive()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
GetCurrentChangeNumber()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
GetNewChangeNumber()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
SetGlobalTheme()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
GetGlobalTheme()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
CheckThemeSignature(wstr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
LoadTheme()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
InitUserTheme()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
InitUserRegistry()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
ReestablishServerConnection()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
ThemeHooksInstall()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
DWORD 
WINAPI
ThemeHooksRemove()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
RefreshThemeForTS()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
BufferedPaintInit()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
BufferedPaintRenderAnimation(ptr ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
BufferedPaintSetAlpha(ptr ptr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
BufferedPaintStopAllAnimations(ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
BufferedPaintUnInit()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
CloseThemeData(ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
ClassicGetSystemMetrics(long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
ClassicSystemParametersInfoA(long long ptr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
ClassicSystemParametersInfoW(long long ptr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
ClassicAdjustWindowRectEx(ptr long long long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
DrawThemeBackgroundEx(ptr ptr long long ptr ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
GetThemeParseErrorInfo()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
OpenNcThemeData()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
IsThemeClassDefined(ptr ptr long long ptr ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
DrawThemeBackground(ptr ptr long long ptr ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
DrawThemeEdge(ptr ptr long long ptr long long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
DrawThemeIcon(ptr ptr long long ptr ptr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
DrawThemeParentBackground(ptr ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
DrawThemeParentBackgroundEx()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
DrawThemeText(ptr ptr long long wstr long long long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
DrawThemeTextEx(ptr ptr long long wstr long long ptr ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
EnableThemeDialogTexture(ptr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
EnableTheming(long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
CreateThemeDataFromObjects()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
OpenThemeDataEx(ptr wstr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
ServerClearStockObjects()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
MarkSelection()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
ProcessLoadTheme_RunDLLW()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
SetSystemVisualStyle()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
ServiceClearStockObjects()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
AddThemeAppCompatFlag(ptr long long long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
ResetThemeAppCompatFlags(ptr ptr long long long ptr ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
EnumThemeProperties(ptr ptr long long long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
EndBufferedAnimation(ptr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
EndBufferedPaint(ptr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
DrawThemeIconEx(ptr long long long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
IsThemeActiveByPolicy()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
GetThemeClass(ptr long long long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
ThemeForwardCapturedMouseMessage(ptr long long long wstr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
EnableServiceConnection(ptr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT 
WINAPI
Remote_LoadTheme(ptr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetBufferedPaintBits(ptr ptr ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetBufferedPaintDC(ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetBufferedPaintTargetDC(ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetBufferedPaintTargetRect(ptr ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetCurrentThemeName(wstr long wstr long wstr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeAppProperties()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeBackgroundContentRect(ptr ptr long long ptr ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeBackgroundExtent(ptr ptr long long ptr ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeBackgroundRegion(ptr ptr long long ptr ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeBitMap()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeBool(ptr long long long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeColor(ptr long long long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeDocumentationProperty(wstr wstr wstr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeEnumValue(ptr long long long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeFilename(ptr long long long wstr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeFont(ptr ptr long long long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeInt(ptr long long long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeIntList(ptr long long long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeMargins(ptr ptr long long long ptr ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeMetric(ptr ptr long long long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemePartSize(ptr ptr long long ptr long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemePosition(ptr long long long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemePropertyOrigin(ptr long long long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeRect(ptr long long long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeStream()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeString(ptr long long long wstr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeSysBool(ptr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeSysColor(ptr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeSysColorBrush(ptr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeSysFont(ptr long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeSysInt(ptr long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeSysSize(ptr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeSysString(ptr long wstr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeTextExtent(ptr ptr long long wstr long long ptr ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeTextMetrics(ptr ptr long long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetThemeTransitionDuration(ptr long long long long ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
GetWindowTheme(ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
HitTestThemeBackground(ptr long long long long ptr long double ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
IsAppThemed()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
IsCompositionActive()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
IsThemeActive()
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
IsThemeBackgroundPartiallyTransparent(ptr long long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
IsThemeDialogTextureEnabled(ptr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
IsThemePartDefined(ptr long long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
OpenThemeData(ptr wstr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
SetThemeAppProperties(long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
SetWindowTheme(ptr wstr wstr)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
SetWindowThemeAttribute(ptr long ptr long)
{
    UNIMPLEMENTED;
    return 0;
}

EXTERN_C
HRESULT
WINAPI
ThemeInitApiHook(long ptr)
{
    UNIMPLEMENTED;
    return 0;
}