/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
 * Copyright 2018 Ged Murphy <gedmurphy@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"
#include <commoncontrols.h>

static const WCHAR szTrayNotifyWndClass[] = L"TrayNotifyWnd";

#define TRAY_NOTIFY_WND_SPACING_X   1
#define TRAY_NOTIFY_WND_SPACING_Y   1
#define CLOCK_TEXT_HACK             4

/*
 * Tray overflow toggle button
 */

class CTrayToggleButton
    : public CWindowImpl<CTrayToggleButton>
{
    SIZE m_Size;
    BOOL isExpanded;
    BOOL isHorizontal;
    BOOL m_bHovering;
    BOOL m_bPressed;
    CNotifyToolbar* toolbar;
    HICON m_expandIcon;
    HICON m_collapseIcon;
    HTHEME m_hTheme;

public:
    CTrayToggleButton()
        : m_Size({ 19, 20 })
        , isExpanded(FALSE)
        , isHorizontal(TRUE)
        , m_bHovering(FALSE)
        , m_bPressed(FALSE)
        , toolbar(NULL)
        , m_expandIcon(NULL)
        , m_collapseIcon(NULL)
        , m_hTheme(NULL)
    {
        //m_Size.cx = 19;
        //m_Size.cy = 20;
        //isExpanded = FALSE;
        //isHorizontal = TRUE;
    }

    virtual ~CTrayToggleButton()
    {

    }

    VOID Toggle()
    {
        isExpanded = !isExpanded;
        if (toolbar)
            toolbar->SetIsExpanded(isExpanded);
    }

    SIZE GetSize()
    {
        return m_Size;
    }

    VOID UpdateSize()
    {
       /* SIZE Size = { 0, 0 };

        if (m_ImageList == NULL ||
            !SendMessageW(BCM_GETIDEALSIZE, 0, (LPARAM) &Size))
        {
            Size.cx = 2 * GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CYCAPTION) * 3;
        }

        Size.cy = max(Size.cy, GetSystemMetrics(SM_CYCAPTION));

        m_Size = Size;*/
    }

    VOID UpdateFont()
    {
        /* Get the system fonts, we use the caption font, always bold, though. 
        NONCLIENTMETRICS ncm = {sizeof(ncm)};
        if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE))
            return;

        if (m_Font)
            DeleteObject(m_Font);

        ncm.lfCaptionFont.lfWeight = FW_BOLD;
        m_Font = CreateFontIndirect(&ncm.lfCaptionFont);

        SetFont(m_Font, FALSE);*/
    }

    VOID Initialize()
    {
        SubclassWindow(m_hWnd);
        SetWindowTheme(m_hWnd, L"TrayNotifyHoriz", NULL);

        UpdateSize();
    }

    HWND Create(HWND hwndParent, IUnknown* pager)
    {
        DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_OVERLAPPED | BS_PUSHBUTTON | BS_TEXT;

        m_hWnd = CreateWindowEx(
            0,
            WC_BUTTON,
            NULL,
            dwStyle,
            0, 6, 19, 20,
            hwndParent,
            NULL,
            hExplorerInstance,
            NULL);

        if (m_hWnd)
            Initialize();

        toolbar = CSysPagerWnd_GetTrayToolbar(pager);

        ExtractIconExW(L"explorer.exe", -IDI_ARROWLEFT, NULL, &m_expandIcon, 1);
        ExtractIconExW(L"explorer.exe", -IDI_ARROWRIGHT, NULL, &m_collapseIcon, 1);
        
        return m_hWnd;
    }

	VOID UpdateTheme()
    {
        SetWindowTheme(m_hWnd, isExpanded 
                ? (isHorizontal ? L"TrayNotifyHorizOpen" : L"TrayNotifyVertOpen")
                : (isHorizontal ? L"TrayNotifyHoriz" : L"TrayNotifyVert")
            , NULL);
        //m_hTheme = OpenThemeData(m_hWnd, L"Button");
    }

    LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT ret = DefWindowProc(uMsg, wParam, lParam);
        m_bPressed = true;
        return ret;
    }
    
    
    LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT ret = DefWindowProc(uMsg, wParam, lParam);
        if (m_bPressed)
        {
            Toggle();
            m_bPressed = false;
        }
        return ret;
    }

    //LRESULT OnThemeChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    LRESULT OnSettingChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        UpdateTheme();
        return 0;
    }

    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        //LRESULT ret = TRUE;
        if (IsThemeActive())
            return DefWindowProc(uMsg, wParam, lParam);
        return TRUE;
    }

#ifdef NO
    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (IsThemeActive())
            return DefWindowProc(uMsg, wParam, lParam);
        //LRESULT ret = DefWindowProc(uMsg, wParam, lParam);
        
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(&ps);
        /*
        if (IsThemeActive() && m_hTheme)
        {
            int part = TP_BUTTON;
            int state = PBS_NORMAL;
            if (m_bPressed)

            ::DrawThemeBackground(theme, hdc, part, state, lpRc, lpRc);
        }
        */
        DrawIconEx(hdc, 0, 0,
                    isExpanded ? m_collapseIcon : m_expandIcon
                    , 0, 0, 0, NULL, DI_NORMAL);
        
        EndPaint(&ps);

        return 0;
    }
#else
    VOID OnClassicDraw(HDC hdc, LPRECT prc)
    {
        RECT rc = { prc->left, prc->top, prc->right, prc->bottom };
        LPRECT lpRc = &rc;
        HBRUSH hbrBackground = NULL;

        
        hbrBackground = ::GetSysColorBrush(COLOR_3DFACE);
        ::FillRect(hdc, lpRc, hbrBackground);

        if (m_bPressed || m_bHovering)
        {
            UINT edge = m_bPressed ? BDR_SUNKENOUTER : BDR_RAISEDINNER;
            DrawEdge(hdc, lpRc, edge, BF_RECT);
        }

        
        /* Prepare to draw icon */

        // Determine X-position of icon's top-left corner
        int iconX = rc.left;
        iconX += (rc.right - iconX) / 2;
        iconX -= 8; //m_szIcon.cx / 2;

        // Determine Y-position of icon's top-left corner
        int iconY = rc.top;
        iconY += (rc.bottom - iconY) / 2;
        iconY -= 8; //m_szIcon.cy / 2;

        // Ok now actually draw the icon itself
        HICON icon = isExpanded ? m_collapseIcon : m_expandIcon;
        if (icon)
        {
            DrawIconEx(hdc, iconX, iconY,
                    icon, 0, 0,
                    0, hbrBackground, DI_NORMAL);
        }
    }

    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (IsThemeActive())
            return DefWindowProc(uMsg, wParam, lParam);
        RECT rc;
        GetClientRect(&rc);

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(&ps);
        OnClassicDraw(hdc, &rc);
        EndPaint(&ps);

        return 0;
    }

    LRESULT OnPrintClient(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (IsThemeActive())
            return DefWindowProc(uMsg, wParam, lParam);
        if ((lParam & PRF_CHECKVISIBLE) && !IsWindowVisible())
            return 0;

        RECT rc;
        GetClientRect(&rc);

        HDC hdc = (HDC)wParam;
        OnClassicDraw(hdc, &rc);

        return 0;
    }

#endif
    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LPNMHDR header = (LPNMHDR)lParam;
        auto code = header->code;
        /*if (code == NM_HOVER)
            m_bHovering = true; */
        if (code == BCN_HOTITEMCHANGE)
        {
            NMBCHOTITEM* hoveredItem = reinterpret_cast<NMBCHOTITEM*>(lParam);
            if (header->hwndFrom == m_hWnd)
                m_bHovering = hoveredItem->dwFlags & HICF_ENTERING;
        }
        return DefWindowProc(uMsg, wParam, lParam);
    }

#define TRAY_TOGGLE_BUTTON_TIMER_ID 998
#define TRAY_TOGGLE_BUTTON_TIMER_INTERVAL 200
    LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (m_bHovering)
            return 0;

        m_bHovering = TRUE;
        Invalidate(TRUE);

        SetTimer(TRAY_TOGGLE_BUTTON_TIMER_ID, TRAY_TOGGLE_BUTTON_TIMER_INTERVAL, NULL);

        return 0;
    }
    
    BOOL PtInButton(LPPOINT ppt) const
    {
        if (!ppt || !IsWindow())
            return FALSE;

        RECT rc;
        GetWindowRect(&rc);
        INT cxEdge = ::GetSystemMetrics(SM_CXEDGE), cyEdge = ::GetSystemMetrics(SM_CYEDGE);
        ::InflateRect(&rc, max(cxEdge, 1), max(cyEdge, 1));

        return isHorizontal
            ? (ppt->x > rc.left)
            : (ppt->y > rc.top);
    }

    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (wParam != TRAY_TOGGLE_BUTTON_TIMER_ID || !m_bHovering)
            return 0;

        POINT pt;
        ::GetCursorPos(&pt);
        if (!PtInButton(&pt)) // The end of hovering?
        {
            m_bHovering = FALSE;
            KillTimer(TRAY_TOGGLE_BUTTON_TIMER_ID);
            Invalidate(TRUE);

            //::PostMessage(m_hWndTaskbar, WM_NCPAINT, 0, 0);
        }

        return 0;
    }
    
    
    VOID SetHorizontal(BOOL horizontal)
    {
        isHorizontal = horizontal;
        
        UpdateTheme();
    }

    BEGIN_MSG_MAP(CTrayToggleButton)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
        //MESSAGE_HANDLER(WM_THEMECHANGED, OnThemeChanged)
        MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChanged)
        //MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
    END_MSG_MAP()

};

/*
 * TrayNotifyWnd
 */

class CTrayNotifyWnd :
    public CComCoClass<CTrayNotifyWnd>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl < CTrayNotifyWnd, CWindow, CControlWinTraits >,
    public IOleWindow
{
    CComPtr<IUnknown> m_clock;
    CTrayShowDesktopButton m_ShowDesktopButton;
    CComPtr<IUnknown> m_pager;
	CTrayToggleButton m_ToggleButton;

    HWND m_hwndClock;
    HWND m_hwndShowDesktop;
    HWND m_hwndPager;

    HTHEME TrayTheme;
    SIZE trayClockMinSize;
    SIZE trayShowDesktopSize;
    SIZE trayNotifySize;
    MARGINS ContentMargin;
    BOOL IsHorizontal;

public:
    CTrayNotifyWnd() :
	    m_ToggleButton(),
        m_hwndClock(NULL),
        m_hwndPager(NULL),
        TrayTheme(NULL),
        IsHorizontal(FALSE)
    {
        ZeroMemory(&trayClockMinSize, sizeof(trayClockMinSize));
        ZeroMemory(&trayShowDesktopSize, sizeof(trayShowDesktopSize));
        ZeroMemory(&trayNotifySize, sizeof(trayNotifySize));
        ZeroMemory(&ContentMargin, sizeof(ContentMargin));
    }
    ~CTrayNotifyWnd() { }

    LRESULT OnThemeChanged()
    {
        if (TrayTheme)
            CloseThemeData(TrayTheme);

        if (IsThemeActive())
            TrayTheme = OpenThemeData(m_hWnd, L"TrayNotify");
        else
            TrayTheme = NULL;

        if (TrayTheme)
        {
            SetWindowExStyle(m_hWnd, WS_EX_STATICEDGE, 0);

            GetThemeMargins(TrayTheme,
                NULL,
                TNP_BACKGROUND,
                0,
                TMT_CONTENTMARGINS,
                NULL,
                &ContentMargin);
        }
        else
        {
            SetWindowExStyle(m_hWnd, WS_EX_STATICEDGE, WS_EX_STATICEDGE);

            bool useClassicThemeToggleButtonMargins = (!IsThemeActive()) && m_ToggleButton && m_ToggleButton.IsWindowVisible();
            ContentMargin.cxLeftWidth = (useClassicThemeToggleButtonMargins && IsHorizontal) ? 21 : 2;
            ContentMargin.cxRightWidth = 2;
            ContentMargin.cyTopHeight = (useClassicThemeToggleButtonMargins && !IsHorizontal) ? 22 : 2;
            ContentMargin.cyBottomHeight = 2;
        }

        return TRUE;
    }

    LRESULT OnThemeChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return OnThemeChanged();
    }

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        HRESULT hr;

        hr = CTrayClockWnd_CreateInstance(m_hWnd, IID_PPV_ARG(IUnknown, &m_clock));
        if (FAILED_UNEXPECTEDLY(hr))
            return FALSE;

        hr = IUnknown_GetWindow(m_clock, &m_hwndClock);
        if (FAILED_UNEXPECTEDLY(hr))
            return FALSE;

        hr = CSysPagerWnd_CreateInstance(m_hWnd, IID_PPV_ARG(IUnknown, &m_pager));
        if (FAILED_UNEXPECTEDLY(hr))
            return FALSE;
        
        /* Create the Tray Toggle button */
        m_ToggleButton.Create(m_hWnd, m_pager);

        hr = IUnknown_GetWindow(m_pager, &m_hwndPager);
        if (FAILED_UNEXPECTEDLY(hr))
            return FALSE;

        /* Create the 'Show Desktop' button */
        m_ShowDesktopButton.DoCreate(m_hWnd);
        m_hwndShowDesktop = m_ShowDesktopButton.m_hWnd;

        return TRUE;
    }

    BOOL GetMinimumSize(IN OUT PSIZE pSize)
    {
        SIZE clockSize = { 0, 0 };
        SIZE traySize = { 0, 0 };
        SIZE showDesktopSize = { 0, 0 };

        if (!g_TaskbarSettings.sr.HideClock)
        {
            if (IsHorizontal)
            {
                clockSize.cy = pSize->cy;
                if (clockSize.cy <= 0)
                    goto NoClock;
            }
            else
            {
                clockSize.cx = pSize->cx;
                if (clockSize.cx <= 0)
                    goto NoClock;
            }

            ::SendMessage(m_hwndClock, TNWM_GETMINIMUMSIZE, (WPARAM) IsHorizontal, (LPARAM) &clockSize);

            trayClockMinSize = clockSize;
        }
        else
        NoClock:
        trayClockMinSize = clockSize;

        if (IsHorizontal)
        {
            traySize.cy = pSize->cy - 2 * TRAY_NOTIFY_WND_SPACING_Y;
        }
        else
        {
            traySize.cx = pSize->cx - 2 * TRAY_NOTIFY_WND_SPACING_X;
        }

        ::SendMessage(m_hwndPager, TNWM_GETMINIMUMSIZE, (WPARAM) IsHorizontal, (LPARAM) &traySize);

        trayNotifySize = traySize;

        INT showDesktopButtonExtent = 0;
        if (g_TaskbarSettings.bShowDesktopButton)
        {
            showDesktopButtonExtent = m_ShowDesktopButton.WidthOrHeight();
            if (IsHorizontal)
            {
                showDesktopSize.cx = showDesktopButtonExtent;
                showDesktopSize.cy = pSize->cy;
            }
            else
            {
                showDesktopSize.cx = pSize->cx;
                showDesktopSize.cy = showDesktopButtonExtent;
            }
        }
        trayShowDesktopSize = showDesktopSize;

        if (IsHorizontal)
        {
            pSize->cx = 2 * TRAY_NOTIFY_WND_SPACING_X;

            if (!g_TaskbarSettings.sr.HideClock)
                pSize->cx += TRAY_NOTIFY_WND_SPACING_X + trayClockMinSize.cx;

            if (g_TaskbarSettings.bShowDesktopButton)
                pSize->cx += showDesktopButtonExtent;

            pSize->cx += traySize.cx;
            pSize->cx += ContentMargin.cxLeftWidth + ContentMargin.cxRightWidth;
        }
        else
        {
            pSize->cy = 2 * TRAY_NOTIFY_WND_SPACING_Y;

            if (!g_TaskbarSettings.sr.HideClock)
                pSize->cy += TRAY_NOTIFY_WND_SPACING_Y + trayClockMinSize.cy;

            if (g_TaskbarSettings.bShowDesktopButton)
                pSize->cy += showDesktopButtonExtent;

            pSize->cy += traySize.cy;
            pSize->cy += ContentMargin.cyTopHeight + ContentMargin.cyBottomHeight;
        }

        return TRUE;
    }

    VOID Size(IN OUT SIZE *pszClient)
    {
        RECT rcClient = {0, 0, pszClient->cx, pszClient->cy};
        AlignControls(&rcClient);
        pszClient->cx = rcClient.right - rcClient.left;
        pszClient->cy = rcClient.bottom - rcClient.top;
    }

    VOID AlignControls(IN CONST PRECT prcClient OPTIONAL)
    {
        RECT rcClient;
        if (prcClient != NULL)
            rcClient = *prcClient;
        else
            GetClientRect(&rcClient);

        rcClient.left += ContentMargin.cxLeftWidth;
        rcClient.top += ContentMargin.cyTopHeight;
        rcClient.right -= ContentMargin.cxRightWidth;
        rcClient.bottom -= ContentMargin.cyBottomHeight;

        CONST UINT swpFlags = SWP_DRAWFRAME | SWP_NOCOPYBITS | SWP_NOZORDER;

        if (g_TaskbarSettings.bShowDesktopButton)
        {
            POINT ptShowDesktop =
            {
                rcClient.left,
                rcClient.top
            };
            SIZE showDesktopSize =
            {
                rcClient.right - rcClient.left,
                rcClient.bottom - rcClient.top
            };

            INT cxyShowDesktop = m_ShowDesktopButton.WidthOrHeight();
            if (IsHorizontal)
            {
                if (!TrayTheme)
                {
                    ptShowDesktop.y -= ContentMargin.cyTopHeight;
                    showDesktopSize.cy += ContentMargin.cyTopHeight + ContentMargin.cyBottomHeight;
                }

                rcClient.right -= (cxyShowDesktop - ContentMargin.cxRightWidth);

                ptShowDesktop.x = rcClient.right;
                showDesktopSize.cx = cxyShowDesktop;

                // HACK: Clock has layout problems - remove this once addressed.
                rcClient.right -= CLOCK_TEXT_HACK;
            }
            else
            {
                if (!TrayTheme)
                {
                    ptShowDesktop.x -= ContentMargin.cxLeftWidth;
                    showDesktopSize.cx += ContentMargin.cxLeftWidth + ContentMargin.cxRightWidth;
                }

                rcClient.bottom -= (cxyShowDesktop - ContentMargin.cyBottomHeight);

                ptShowDesktop.y = rcClient.bottom;
                showDesktopSize.cy = cxyShowDesktop;

                // HACK: Clock has layout problems - remove this once addressed.
                rcClient.bottom -= CLOCK_TEXT_HACK;
            }

            /* Resize and reposition the button */
            ::SetWindowPos(m_hwndShowDesktop,
                NULL,
                ptShowDesktop.x,
                ptShowDesktop.y,
                showDesktopSize.cx,
                showDesktopSize.cy,
                swpFlags);
        }

        if (!g_TaskbarSettings.sr.HideClock)
        {
            POINT ptClock = { rcClient.left, rcClient.top };
            SIZE clockSize = { rcClient.right - rcClient.left, rcClient.bottom - rcClient.top };

            if (IsHorizontal)
            {
                rcClient.right -= trayClockMinSize.cx;

                ptClock.x = rcClient.right;
                clockSize.cx = trayClockMinSize.cx;
            }
            else
            {
                rcClient.bottom -= trayClockMinSize.cy;

                ptClock.y = rcClient.bottom;
                clockSize.cy = trayClockMinSize.cy;
            }

            ::SetWindowPos(m_hwndClock,
                NULL,
                ptClock.x,
                ptClock.y,
                clockSize.cx,
                clockSize.cy,
                swpFlags);
        }

        POINT ptPager;
        if (IsHorizontal)
        {
            ptPager.x = ContentMargin.cxLeftWidth;
            ptPager.y = ((rcClient.bottom - rcClient.top) - trayNotifySize.cy) / 2;
            if (g_TaskbarSettings.UseCompactTrayIcons())
                ptPager.y += ContentMargin.cyTopHeight;
        }
        else
        {
            ptPager.x = ((rcClient.right - rcClient.left) - trayNotifySize.cx) / 2;
            if (g_TaskbarSettings.UseCompactTrayIcons())
                ptPager.x += ContentMargin.cxLeftWidth;
            ptPager.y = ContentMargin.cyTopHeight;
        }

        ::SetWindowPos(m_hwndPager,
            NULL,
            ptPager.x,
            ptPager.y,
            trayNotifySize.cx,
            trayNotifySize.cy,
            swpFlags);

        if (prcClient != NULL)
        {
            prcClient->left = rcClient.left - ContentMargin.cxLeftWidth;
            prcClient->top = rcClient.top - ContentMargin.cyTopHeight;
            prcClient->right = rcClient.right + ContentMargin.cxRightWidth;
            prcClient->bottom = rcClient.bottom + ContentMargin.cyBottomHeight;
        }
    }

    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        HDC hdc = (HDC) wParam;

        if (!TrayTheme)
        {
            bHandled = FALSE;
            return 0;
        }

        RECT rect;
        GetClientRect(&rect);
        if (IsThemeBackgroundPartiallyTransparent(TrayTheme, TNP_BACKGROUND, 0))
            DrawThemeParentBackground(m_hWnd, hdc, &rect);

        DrawThemeBackground(TrayTheme, hdc, TNP_BACKGROUND, 0, &rect, 0);

        return TRUE;
    }

    LRESULT OnGetMinimumSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        BOOL Horizontal = (BOOL) wParam;

        if (Horizontal != IsHorizontal)
            IsHorizontal = Horizontal;

        SetWindowTheme(m_hWnd,
                       IsHorizontal ? L"TrayNotifyHoriz" : L"TrayNotifyVert",
                       NULL);
        m_ShowDesktopButton.m_bHorizontal = Horizontal;
        m_ToggleButton.SetHorizontal(IsHorizontal);
        return (LRESULT)GetMinimumSize((PSIZE)lParam);
    }

    LRESULT OnGetShowDesktopButton(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (wParam == NULL)
            return 0;

        CTrayShowDesktopButton** ptr = (CTrayShowDesktopButton**)wParam;
        if (!m_ShowDesktopButton)
        {
            *ptr = NULL;
            return 0;
        }

        m_ToggleButton.SetHorizontal(IsHorizontal);
        *ptr = &m_ShowDesktopButton;
        bHandled = TRUE;
        return 0;
    }

    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SIZE clientSize;

        clientSize.cx = LOWORD(lParam);
        clientSize.cy = HIWORD(lParam);

        Size(&clientSize);

        return TRUE;
    }

    LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);

        if (m_ShowDesktopButton && m_ShowDesktopButton.PtInButton(&pt))
            return HTCLIENT;

        return HTTRANSPARENT;
    }

    LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        POINT pt;
        ::GetCursorPos(&pt);

        if (m_ShowDesktopButton && m_ShowDesktopButton.PtInButton(&pt))
            m_ShowDesktopButton.StartHovering();

        return TRUE;
    }

    LRESULT OnCtxMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        bHandled = TRUE;

        if (reinterpret_cast<HWND>(wParam) == m_hwndClock)
            return GetParent().SendMessage(uMsg, wParam, lParam);
        else
            return 0;
    }

    LRESULT OnClockMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return SendMessageW(m_hwndClock, uMsg, wParam, lParam);
    }

    LRESULT OnTaskbarSettingsChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        TaskbarSettings* newSettings = (TaskbarSettings*)lParam;

        /* Toggle show desktop button */
        if (newSettings->bShowDesktopButton != g_TaskbarSettings.bShowDesktopButton)
        {
            g_TaskbarSettings.bShowDesktopButton = newSettings->bShowDesktopButton;
            ::ShowWindow(m_hwndShowDesktop, g_TaskbarSettings.bShowDesktopButton ? SW_SHOW : SW_HIDE);

            /* Ask the parent to resize */
            NMHDR nmh = {m_hWnd, 0, NTNWM_REALIGN};
            SendMessage(WM_NOTIFY, 0, (LPARAM) &nmh);
        }

        return OnClockMessage(uMsg, wParam, lParam, bHandled);
    }

    LRESULT OnPagerMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return SendMessageW(m_hwndPager, uMsg, wParam, lParam);
    }

    LRESULT OnRealign(INT uCode, LPNMHDR hdr, BOOL& bHandled)
    {
        hdr->hwndFrom = m_hWnd;
        return GetParent().SendMessage(WM_NOTIFY, 0, (LPARAM)hdr);
    }

    HRESULT WINAPI GetWindow(HWND* phwnd)
    {
        if (!phwnd)
            return E_INVALIDARG;
        *phwnd = m_hWnd;
        return S_OK;
    }

    HRESULT WINAPI ContextSensitiveHelp(BOOL fEnterMode)
    {
        return E_NOTIMPL;
    }

    HRESULT Initialize(IN HWND hwndParent)
    {
        const DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        Create(hwndParent, 0, NULL, dwStyle, WS_EX_STATICEDGE);
        return m_hWnd ? S_OK : E_FAIL;
    }
    IUnknown* GetPager()
    {
        return m_pager;
    }

    DECLARE_NOT_AGGREGATABLE(CTrayNotifyWnd)

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CTrayNotifyWnd)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
    END_COM_MAP()

    DECLARE_WND_CLASS_EX(szTrayNotifyWndClass, CS_DBLCLKS, COLOR_3DFACE)

    BEGIN_MSG_MAP(CTrayNotifyWnd)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_THEMECHANGED, OnThemeChanged)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_NCMOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnCtxMenu)
        MESSAGE_HANDLER(WM_NCLBUTTONDBLCLK, OnClockMessage)
        MESSAGE_HANDLER(WM_SETFONT, OnClockMessage)
        MESSAGE_HANDLER(WM_SETTINGCHANGE, OnPagerMessage)
        MESSAGE_HANDLER(WM_COPYDATA, OnPagerMessage)
        MESSAGE_HANDLER(TWM_SETTINGSCHANGED, OnTaskbarSettingsChanged)
        NOTIFY_CODE_HANDLER(NTNWM_REALIGN, OnRealign)
        MESSAGE_HANDLER(TNWM_GETMINIMUMSIZE, OnGetMinimumSize)
        MESSAGE_HANDLER(TNWM_GETSHOWDESKTOPBUTTON, OnGetShowDesktopButton)
    END_MSG_MAP()
};

HRESULT CTrayNotifyWnd_CreateInstance(HWND hwndParent, REFIID riid, void **ppv)
{
    return ShellObjectCreatorInit<CTrayNotifyWnd>(hwndParent, riid, ppv);
}

CNotifyToolbar* CTrayNotifyWnd_GetTrayToolbar(IUnknown* pTray)
{
    CTrayNotifyWnd *tray = static_cast<CTrayNotifyWnd*>(pTray);

    if (tray != NULL)
    {
        return CSysPagerWnd_GetTrayToolbar(tray->GetPager());
    }

    return NULL;
}