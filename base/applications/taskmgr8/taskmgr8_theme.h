/*
 * Light / dark palette for Task Manager 8 (View → Dark mode).
 */

#pragma once

#include <windows.h>

typedef struct _TM8_THEME
{
    COLORREF navBg;
    COLORREF navSel;
    COLORREF navText;
    COLORREF navTileBorder;
    COLORREF navTileBorderSel;
    COLORREF navTileBg;
    COLORREF navTileSelBg;
    COLORREF graphBg;
    COLORREF graphGrid;
    COLORREF graphFill;
    COLORREF graphLine;
    COLORREF barGreen;
    COLORREF barTrack;
    COLORREF memGraphFill;
    COLORREF memGraphLine;
    COLORREF memGraphGrid;
    COLORREF memGraphEdge;
    COLORREF panelBg;
    COLORREF textPrimary;
    COLORREF textMuted;
    COLORREF textTertiary;
    COLORREF graphCaption;
    COLORREF compUse;
    COLORREF compAvail;
    COLORREF compFrame;
    COLORREF compInnerSep;
    COLORREF graphCellFrame;
    COLORREF miniSparkBg;
    COLORREF miniSparkBorder;
    COLORREF listText;
} TM8_THEME;

extern TM8_THEME g_Tm8Theme;

void Tm8ThemeLoad(void);
void Tm8ThemeSave(void);
void Tm8ThemeSetDark(BOOL dark);
BOOL Tm8ThemeIsDark(void);
HBRUSH Tm8ThemePanelBrush(void);
void Tm8ThemeShutdown(void);
