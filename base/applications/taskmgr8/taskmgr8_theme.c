/*
 * Theme registry: HKCU\Software\ReactOS\TaskMgr8\DarkMode (REG_DWORD, 1 = dark).
 */

#include "taskmgr8_theme.h"

static BOOL s_dark;
static HBRUSH s_brPanel;

#define TM8_REG_SUBKEY L"Software\\ReactOS\\TaskMgr8"

static void
Tm8ThemeFill(TM8_THEME *t, BOOL dark)
{
    if (!dark)
    {
        t->navBg = RGB(243, 243, 243);
        t->navSel = RGB(0, 120, 215);
        t->navText = RGB(32, 32, 32);
        t->navTileBorder = RGB(218, 218, 218);
        t->navTileBorderSel = RGB(210, 214, 220);
        t->navTileBg = RGB(255, 255, 255);
        t->navTileSelBg = RGB(248, 249, 251);
        t->graphBg = RGB(252, 252, 252);
        t->graphGrid = RGB(238, 240, 245);
        t->graphFill = RGB(228, 238, 252);
        t->graphLine = RGB(88, 152, 218);
        t->barGreen = RGB(77, 181, 89);
        t->barTrack = RGB(237, 237, 237);
        t->memGraphFill = RGB(228, 236, 252);
        t->memGraphLine = RGB(56, 112, 188);
        t->memGraphGrid = RGB(244, 246, 250);
        t->memGraphEdge = RGB(120, 170, 220);
        t->panelBg = RGB(255, 255, 255);
        t->textPrimary = RGB(32, 32, 32);
        t->textMuted = RGB(96, 96, 96);
        t->textTertiary = RGB(64, 64, 64);
        t->graphCaption = RGB(105, 105, 105);
        t->compUse = RGB(138, 194, 244);
        t->compAvail = RGB(255, 255, 255);
        t->compFrame = RGB(0, 120, 215);
        t->compInnerSep = RGB(200, 220, 245);
        t->graphCellFrame = RGB(235, 235, 235);
        t->miniSparkBg = RGB(255, 255, 255);
        t->miniSparkBorder = RGB(200, 200, 200);
        t->listText = RGB(32, 32, 32);
    }
    else
    {
        t->navBg = RGB(42, 42, 45);
        t->navSel = RGB(0, 120, 215);
        t->navText = RGB(230, 230, 235);
        t->navTileBorder = RGB(70, 72, 78);
        t->navTileBorderSel = RGB(90, 95, 105);
        t->navTileBg = RGB(48, 48, 52);
        t->navTileSelBg = RGB(58, 62, 72);
        t->graphBg = RGB(32, 33, 38);
        t->graphGrid = RGB(55, 58, 65);
        t->graphFill = RGB(45, 55, 75);
        t->graphLine = RGB(100, 165, 230);
        t->barGreen = RGB(83, 181, 89);
        t->barTrack = RGB(52, 52, 56);
        t->memGraphFill = RGB(45, 52, 68);
        t->memGraphLine = RGB(90, 140, 200);
        t->memGraphGrid = RGB(50, 54, 62);
        t->memGraphEdge = RGB(100, 140, 190);
        t->panelBg = RGB(32, 33, 38);
        t->textPrimary = RGB(235, 235, 238);
        t->textMuted = RGB(160, 162, 170);
        t->textTertiary = RGB(190, 192, 200);
        t->graphCaption = RGB(150, 152, 160);
        t->compUse = RGB(80, 130, 200);
        t->compAvail = RGB(48, 50, 56);
        t->compFrame = RGB(0, 120, 215);
        t->compInnerSep = RGB(70, 85, 110);
        t->graphCellFrame = RGB(60, 62, 70);
        t->miniSparkBg = RGB(40, 42, 48);
        t->miniSparkBorder = RGB(75, 78, 88);
        t->listText = RGB(235, 235, 238);
    }
}

static void
Tm8ThemeRefreshPanelBrush(void)
{
    if (s_brPanel)
    {
        DeleteObject(s_brPanel);
        s_brPanel = NULL;
    }
    s_brPanel = CreateSolidBrush(g_Tm8Theme.panelBg);
}

TM8_THEME g_Tm8Theme;

void
Tm8ThemeSetDark(BOOL dark)
{
    s_dark = dark ? TRUE : FALSE;
    Tm8ThemeFill(&g_Tm8Theme, s_dark);
    Tm8ThemeRefreshPanelBrush();
}

void
Tm8ThemeLoad(void)
{
    HKEY hk;
    DWORD val = 0, cb = sizeof(val), typ = 0;
    BOOL dark = FALSE;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, TM8_REG_SUBKEY, 0, KEY_READ, &hk) == ERROR_SUCCESS)
    {
        if (RegQueryValueExW(hk, L"DarkMode", NULL, &typ, (BYTE *)&val, &cb) == ERROR_SUCCESS &&
            typ == REG_DWORD && val != 0)
        {
            dark = TRUE;
        }
        RegCloseKey(hk);
    }
    Tm8ThemeSetDark(dark);
}

void
Tm8ThemeSave(void)
{
    HKEY hk;
    DWORD val = s_dark ? 1u : 0u;

    if (RegCreateKeyExW(HKEY_CURRENT_USER, TM8_REG_SUBKEY, 0, NULL, 0, KEY_WRITE, NULL, &hk, NULL) ==
        ERROR_SUCCESS)
    {
        RegSetValueExW(hk, L"DarkMode", 0, REG_DWORD, (const BYTE *)&val, sizeof(val));
        RegCloseKey(hk);
    }
}

BOOL
Tm8ThemeIsDark(void)
{
    return s_dark;
}

HBRUSH
Tm8ThemePanelBrush(void)
{
    return s_brPanel;
}

void
Tm8ThemeShutdown(void)
{
    if (s_brPanel)
    {
        DeleteObject(s_brPanel);
        s_brPanel = NULL;
    }
}
