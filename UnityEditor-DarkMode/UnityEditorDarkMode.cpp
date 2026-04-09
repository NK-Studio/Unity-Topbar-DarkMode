// Including SDKDDKVer.h defines the highest available Windows platform.
#include <SDKDDKVer.h>

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN

// Windows header files
#include <cstdio>
#include <windows.h>
#include <windowsx.h>
#include <tlhelp32.h>
#include <atlstr.h>

// COM header files
#include <ole2.h>

// Generic C++ stuff
#include <filesystem>
#include <fstream>
#include <vector>
#include <unordered_map>

// for subclassing
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

#include <Uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

#include <vsstyle.h>

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#pragma comment(lib, "msimg32.lib") // GradientFill

// inipp from https://github.com/mcmtroffaes/inipp
#include "inipp.h"

// window messages related to menu bar drawing
enum
{
    WM_UAHDESTROYWINDOW = 0x0090,
    WM_UAHDRAWMENU = 0x0091,
    WM_UAHDRAWMENUITEM = 0x0092,
    WM_UAHINITMENU = 0x0093,
    WM_UAHMEASUREMENUITEM = 0x0094,
    WM_UAHNCPAINTMENUPOPUP = 0x0095
};

// undocumented app mode enum for the private SetPreferredAppMode API
enum class PreferredAppMode
{
    Default,
    AllowDark,
    ForceDark,
    ForceLight,
    Max
};

// describes the sizes of the menu bar or menu item
typedef union tagUAHMENUITEMMETRICS
{
    // cx appears to be 14 / 0xE less than rcItem's width!
    // cy 0x14 seems stable, i wonder if it is 4 less than rcItem's height which is always 24 atm
    struct {
        DWORD cx;
        DWORD cy;
    } rgsizeBar[2];
    struct {
        DWORD cx;
        DWORD cy;
    } rgsizePopup[4];
} UAHMENUITEMMETRICS;

// not really used in our case but part of the other structures
typedef struct tagUAHMENUPOPUPMETRICS
{
    DWORD rgcx[4];
    DWORD fUpdateMaxWidths : 2; // from kernel symbols, padded to full dword
} UAHMENUPOPUPMETRICS;

// hmenu is the main window menu; hdc is the context to draw in
typedef struct tagUAHMENU
{
    HMENU hmenu;
    HDC hdc;
    DWORD dwFlags; // no idea what these mean, in my testing it's either 0x00000a00 or sometimes 0x00000a10
} UAHMENU;

// menu items are always referred to by iPosition here
typedef struct tagUAHMENUITEM
{
    int iPosition; // 0-based position of menu item in menubar
    UAHMENUITEMMETRICS umim;
    UAHMENUPOPUPMETRICS umpm;
} UAHMENUITEM;

// the DRAWITEMSTRUCT contains the states of the menu items, as well as
// the position index of the item in the menu, which is duplicated in
// the UAHMENUITEM's iPosition as well
typedef struct UAHDRAWMENUITEM
{
    DRAWITEMSTRUCT dis; // itemID looks uninitialized
    UAHMENU um;
    UAHMENUITEM umi;
} UAHDRAWMENUITEM;

// the MEASUREITEMSTRUCT is intended to be filled with the size of the item
// height appears to be ignored, but width can be modified
typedef struct tagUAHMEASUREMENUITEM
{
    MEASUREITEMSTRUCT mis;
    UAHMENU um;
    UAHMENUITEM umi;
} UAHMEASUREMENUITEM;

// theme config struct
typedef struct {
    COLORREF menubar_textcolor;
    COLORREF menubar_textcolor_disabled;
    COLORREF menubar_bgcolor;
    COLORREF menubaritem_bgcolor;
    COLORREF menubaritem_bgcolor_hot;
    COLORREF menubaritem_bgcolor_selected;

    HBRUSH menubar_bgbrush;
    HBRUSH menubaritem_bgbrush;
    HBRUSH menubaritem_bgbrush_hot;
    HBRUSH menubaritem_bgbrush_selected;

    // 다이얼로그 전용 색상
    COLORREF dialog_bgcolor;
    HBRUSH   dialog_bgbrush;
    COLORREF dialog_titlebar_color;

    // 버튼 전용 색상
    COLORREF button_textcolor;
    COLORREF button_bgcolor;
    COLORREF button_bgcolor_hot;
    COLORREF button_bgcolor_pressed;
    COLORREF button_bordercolor;
    COLORREF button_inner_bordercolor;

} theme_cfg;

// global variables
static HTHEME g_menuTheme = nullptr;
HHOOK g_hook = nullptr;
static bool g_isDarkMode = true; // Unity 테마 상태 (기본값: 다크)





bool IsWndClass(HWND hWnd, const TCHAR* classname) {
    TCHAR buf[512];
    GetClassName(hWnd, buf, 512);
    return _wcsicmp(classname, buf) == 0;
}

bool IsUnityWndClass(HWND hWnd) {
    return IsWndClass(hWnd, L"UnityContainerWndClass");
}

// 버튼 하나에 현재 g_isDarkMode 테마를 적용
void ApplyButtonTheme(HWND hWnd) {
    if (!IsWndClass(hWnd, L"Button")) return;
    DWORD style = GetWindowLongPtr(hWnd, GWL_STYLE);

    DWORD btnType = style & 0xF;

    switch (btnType) {
        case BS_AUTOCHECKBOX:    // 0x3
        case BS_AUTORADIOBUTTON: // 0x9
        case BS_CHECKBOX:        // 0x2
        case BS_GROUPBOX:        // 0x7
        case BS_RADIOBUTTON:     // 0x4
            SetWindowTheme(hWnd, g_isDarkMode ? L"Explorer" : nullptr, nullptr);
            break;
        case BS_DEFPUSHBUTTON:   // 0x1
        case BS_PUSHBUTTON:      // 0x0
            if (g_isDarkMode) {
                // 테마 제거 후 Owner Draw로 전환 → 코너 아티팩트 완전 제거
                SetWindowTheme(hWnd, L"", L"");
                SetWindowLong(hWnd, GWL_STYLE, (style & ~0xF) | BS_OWNERDRAW);
            } else {
                SetWindowLong(hWnd, GWL_STYLE, (style & ~0xF) | BS_PUSHBUTTON);
                SetWindowTheme(hWnd, nullptr, nullptr);
            }
            break;
        default:
            SetWindowTheme(hWnd, g_isDarkMode ? L"Explorer" : nullptr, nullptr);
            break;
    }
}

// 창과 모든 자식 창에 버튼 테마를 재귀 적용
void ApplyButtonThemeToChildren(HWND hWnd) {
    ApplyButtonTheme(hWnd);
    HWND child = GetWindow(hWnd, GW_CHILD);
    while (child) {
        ApplyButtonThemeToChildren(child);
        child = GetWindow(child, GW_HWNDNEXT);
    }
}

void GetAllWindowsByProcessID(DWORD dwProcessID, std::vector<HWND>& vhWnds) {
    HWND hCurWnd = nullptr;
    do
    {
        hCurWnd = FindWindowEx(nullptr, hCurWnd, nullptr, nullptr);
        if (hCurWnd != nullptr)
        {
            DWORD processID = 0;
            GetWindowThreadProcessId(hCurWnd, &processID);
            if (processID == dwProcessID)
            {
                vhWnds.push_back(hCurWnd);
            }
        }
    } while (hCurWnd != nullptr);
}

void FillGradientRect(HDC hdc, const RECT* prc, COLORREF colorStart, COLORREF colorEnd, bool vertical = false, float stopRatio = 1.0f) {
    if (colorStart == colorEnd) {
        HBRUSH bgBrush = CreateSolidBrush(colorStart);
        FillRect(hdc, prc, bgBrush);
        DeleteObject(bgBrush);
        return;
    }
    
    if (stopRatio >= 1.0f || stopRatio <= 0.0f) {
        TRIVERTEX vertex[2];
        vertex[0].x     = prc->left;
        vertex[0].y     = prc->top;
        vertex[0].Red   = GetRValue(colorStart) << 8;
        vertex[0].Green = GetGValue(colorStart) << 8;
        vertex[0].Blue  = GetBValue(colorStart) << 8;
        vertex[0].Alpha = 0x0000;

        vertex[1].x     = prc->right;
        vertex[1].y     = prc->bottom;
        vertex[1].Red   = GetRValue(colorEnd) << 8;
        vertex[1].Green = GetGValue(colorEnd) << 8;
        vertex[1].Blue  = GetBValue(colorEnd) << 8;
        vertex[1].Alpha = 0x0000;

        GRADIENT_RECT gRect;
        gRect.UpperLeft  = 0;
        gRect.LowerRight = 1;

        GradientFill(hdc, vertex, 2, &gRect, 1, vertical ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H);
    } else {
        TRIVERTEX vertex[6];
        // 앞쪽의 어두운 구간을 더 짧고 강렬하게 빼기 위해, 
        // 멈춤 지점(stopRatio)의 중앙이 아닌 4분의 1(25%) 앞쪽 지점으로 가장 빛나는 피크(Peak)를 당겨줍니다.
        long peakOffset = (long)((vertical ? (prc->bottom - prc->top) : (prc->right - prc->left)) * (stopRatio * 0.25f));
        long stopOffset = (long)((vertical ? (prc->bottom - prc->top) : (prc->right - prc->left)) * stopRatio);

        long peakX = vertical ? prc->right : prc->left + peakOffset;
        long peakY = vertical ? prc->top + peakOffset : prc->bottom;

        long stopX = vertical ? prc->right : prc->left + stopOffset;
        long stopY = vertical ? prc->top + stopOffset : prc->bottom;

        // Rect 1 (0 to Peak): colorEnd -> colorStart
        vertex[0].x = prc->left;
        vertex[0].y = prc->top;
        vertex[0].Red   = GetRValue(colorEnd) << 8;
        vertex[0].Green = GetGValue(colorEnd) << 8;
        vertex[0].Blue  = GetBValue(colorEnd) << 8;
        vertex[0].Alpha = 0x0000;

        vertex[1].x = peakX;
        vertex[1].y = peakY;
        vertex[1].Red   = GetRValue(colorStart) << 8;
        vertex[1].Green = GetGValue(colorStart) << 8;
        vertex[1].Blue  = GetBValue(colorStart) << 8;
        vertex[1].Alpha = 0x0000;

        // Rect 2 (Peak to Stop): colorStart -> colorEnd
        vertex[2].x = vertical ? prc->left : peakX;
        vertex[2].y = vertical ? peakY : prc->top;
        vertex[2].Red   = GetRValue(colorStart) << 8;
        vertex[2].Green = GetGValue(colorStart) << 8;
        vertex[2].Blue  = GetBValue(colorStart) << 8;
        vertex[2].Alpha = 0x0000;

        vertex[3].x = stopX;
        vertex[3].y = stopY;
        vertex[3].Red   = GetRValue(colorEnd) << 8;
        vertex[3].Green = GetGValue(colorEnd) << 8;
        vertex[3].Blue  = GetBValue(colorEnd) << 8;
        vertex[3].Alpha = 0x0000;

        // Rect 3 (Stop to 100%): colorEnd -> colorEnd (Solid)
        vertex[4].x = vertical ? prc->left : stopX;
        vertex[4].y = vertical ? stopY : prc->top;
        vertex[4].Red   = GetRValue(colorEnd) << 8;
        vertex[4].Green = GetGValue(colorEnd) << 8;
        vertex[4].Blue  = GetBValue(colorEnd) << 8;
        vertex[4].Alpha = 0x0000;

        vertex[5].x = prc->right;
        vertex[5].y = prc->bottom;
        vertex[5].Red   = GetRValue(colorEnd) << 8;
        vertex[5].Green = GetGValue(colorEnd) << 8;
        vertex[5].Blue  = GetBValue(colorEnd) << 8;
        vertex[5].Alpha = 0x0000;

        GRADIENT_RECT gRect[3];
        gRect[0].UpperLeft  = 0;
        gRect[0].LowerRight = 1;
        gRect[1].UpperLeft  = 2;
        gRect[1].LowerRight = 3;
        gRect[2].UpperLeft  = 4;
        gRect[2].LowerRight = 5;

        GradientFill(hdc, vertex, 6, gRect, 3, vertical ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H);
    }
}

static void FreeCfgBrushes(theme_cfg& cfg) {
    if (cfg.menubar_bgbrush)            { DeleteObject(cfg.menubar_bgbrush);            cfg.menubar_bgbrush = nullptr; }
    if (cfg.menubaritem_bgbrush)        { DeleteObject(cfg.menubaritem_bgbrush);        cfg.menubaritem_bgbrush = nullptr; }
    if (cfg.menubaritem_bgbrush_hot)    { DeleteObject(cfg.menubaritem_bgbrush_hot);    cfg.menubaritem_bgbrush_hot = nullptr; }
    if (cfg.menubaritem_bgbrush_selected){ DeleteObject(cfg.menubaritem_bgbrush_selected); cfg.menubaritem_bgbrush_selected = nullptr; }
    if (cfg.dialog_bgbrush)              { DeleteObject(cfg.dialog_bgbrush);              cfg.dialog_bgbrush = nullptr; }
}

const theme_cfg* LoadThemeConfig() {
    static theme_cfg _cfg = {};
    static bool last_loaded_dark = !true; // 강제 첫 로드를 위해 반대값으로 초기화
    static bool first_load = true;

    // 모드가 바뀌지 않았으면 캐시 반환
    if (!first_load && last_loaded_dark == g_isDarkMode) return &_cfg;
    first_load = false;
    last_loaded_dark = g_isDarkMode;

    // 이전 브러시 해제
    FreeCfgBrushes(_cfg);

    HMODULE hm = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)LoadThemeConfig, &hm);
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(hm, path, MAX_PATH);
    CStringW inifn(path);
    inifn.Append(L".ini");

    // ini 파일이 없으면 기본값으로 생성 ([dark] / [light] 두 섹션)
    if (!std::filesystem::exists(inifn.GetString())) {
        std::ofstream configFile;
        configFile.open(inifn.GetString());
        configFile << "[dark]" << std::endl;
        configFile << "menubar_textcolor = 200,200,200" << std::endl;
        configFile << "menubar_textcolor_disabled = 160,160,160" << std::endl;
        configFile << "menubar_bgcolor = 20,20,20" << std::endl;
        configFile << "menubaritem_bgcolor = 20,20,20" << std::endl;
        configFile << "menubaritem_bgcolor_hot = 74,74,74" << std::endl;
        configFile << "menubaritem_bgcolor_selected = 74,74,74" << std::endl;
        configFile << "dialog_bgcolor = 56,56,56" << std::endl;
        configFile << "dialog_titlebar_color = 44,44,44" << std::endl;
        configFile << "button_bgcolor = 56,56,56" << std::endl;
        configFile << "button_bgcolor_hot = 70,70,70" << std::endl;
        configFile << "button_bgcolor_pressed = 44,44,44" << std::endl;
        configFile << "button_bordercolor = 80,80,80" << std::endl;
        configFile << "button_inner_bordercolor = 80,80,80" << std::endl;
        configFile << std::endl;
        configFile << "[light]" << std::endl;
        configFile << "menubar_textcolor = 30,30,30" << std::endl;
        configFile << "menubar_textcolor_disabled = 130,130,130" << std::endl;
        configFile << "menubar_bgcolor = 220,220,220" << std::endl;
        configFile << "menubaritem_bgcolor = 220,220,220" << std::endl;
        configFile << "menubaritem_bgcolor_hot = 200,200,200" << std::endl;
        configFile << "menubaritem_bgcolor_selected = 200,200,200" << std::endl;
        configFile << "dialog_bgcolor = 240,240,240" << std::endl;
        configFile << "dialog_titlebar_color = 0,0,0" << std::endl;
        configFile << "button_bgcolor = 240,240,240" << std::endl;
        configFile << "button_bgcolor_hot = 220,220,220" << std::endl;
        configFile << "button_bgcolor_pressed = 200,200,200" << std::endl;
        configFile << "button_bordercolor = 180,180,180" << std::endl;
        configFile << "button_inner_bordercolor = 180,180,180" << std::endl;
        configFile.close();
    }

    inipp::Ini<char> ini;
    std::ifstream is(inifn.GetString());
    ini.parse(is);
    is.close();

    // 현재 모드에 맞는 섹션 선택
    const std::string section = g_isDarkMode ? "dark" : "light";

#define _PARSE_COLOR(x) \
    { \
        auto static_it = ini.sections[section].find(#x); \
        if (static_it != ini.sections[section].end()) { \
            int r = 0, g = 0, b = 0; \
            char comma; \
            std::stringstream ss(static_it->second); \
            ss >> r >> comma >> g >> comma >> b; \
            _cfg.x = RGB(r, g, b); \
        } \
    }

    _PARSE_COLOR(menubar_textcolor)
    _PARSE_COLOR(menubar_textcolor_disabled)
    _PARSE_COLOR(menubar_bgcolor)
    _PARSE_COLOR(menubaritem_bgcolor)
    _PARSE_COLOR(menubaritem_bgcolor_hot)
    _PARSE_COLOR(menubaritem_bgcolor_selected)

    _cfg.dialog_bgcolor = _cfg.menubar_bgcolor; // 기본값: 메뉴바 색과 동일
    _PARSE_COLOR(dialog_bgcolor)

    _cfg.dialog_titlebar_color = _cfg.dialog_bgcolor; // 기본값: 다이얼로그 배경색과 동일
    _PARSE_COLOR(dialog_titlebar_color)

    _cfg.button_bgcolor         = _cfg.dialog_bgcolor;          // 기본값: 다이얼로그 배경과 동일
    _PARSE_COLOR(button_bgcolor)
    _cfg.button_bgcolor_hot     = RGB(70, 70, 70);              // 기본값: hover 시 밝게
    _PARSE_COLOR(button_bgcolor_hot)
    _cfg.button_bgcolor_pressed = RGB(44, 44, 44);              // 기본값: 클릭 시 어둡게
    _PARSE_COLOR(button_bgcolor_pressed)

    _cfg.button_bordercolor = RGB(80, 80, 80);
    _PARSE_COLOR(button_bordercolor)

    _cfg.button_inner_bordercolor = _cfg.button_bordercolor;
    _PARSE_COLOR(button_inner_bordercolor)

    _cfg.menubar_bgbrush            = CreateSolidBrush(_cfg.menubar_bgcolor);
    _cfg.menubaritem_bgbrush        = CreateSolidBrush(_cfg.menubaritem_bgcolor);
    _cfg.menubaritem_bgbrush_hot    = CreateSolidBrush(_cfg.menubaritem_bgcolor_hot);
    _cfg.menubaritem_bgbrush_selected = CreateSolidBrush(_cfg.menubaritem_bgcolor_selected);
    _cfg.dialog_bgbrush               = CreateSolidBrush(_cfg.dialog_bgcolor);

    return &_cfg;
}

// https://stackoverflow.com/questions/39261826/change-the-color-of-the-title-bar-caption-of-a-win32-application
// https://gist.github.com/rounk-ctrl/b04e5622e30e0d62956870d5c22b7017
// https://github.com/microsoft/WindowsAppSDK/issues/41
// https://gist.github.com/ericoporto/1745f4b912e22f9eabfce2c7166d979b
void EnableDarkMode(HWND hWnd) {

    // apply dark/light mode to the window title bar
    if (hWnd) {
        const BOOL USE_DARK_MODE = g_isDarkMode ? TRUE : FALSE;
        DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &USE_DARK_MODE, sizeof(USE_DARK_MODE));

        #ifndef DWMWA_CAPTION_COLOR
        #define DWMWA_CAPTION_COLOR 35
        #endif
        #ifndef DWMWA_COLOR_DEFAULT
        #define DWMWA_COLOR_DEFAULT 0xFFFFFFFFu
        #endif

        // 다이얼로그 창 타이틀바만 단색으로 지정 (메인 창은 Mica 유지)
        if (IsWndClass(hWnd, L"#32770")) {
            COLORREF captionColor = g_isDarkMode
                ? LoadThemeConfig()->dialog_titlebar_color
                : DWMWA_COLOR_DEFAULT;
            DwmSetWindowAttribute(hWnd, DWMWA_CAPTION_COLOR, &captionColor, sizeof(captionColor));
        }
    }

    // apply dark/light mode to context menus
    {
        using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode appMode);
        fnSetPreferredAppMode SetPreferredAppMode;

        static HMODULE hUxtheme = nullptr;

        if (!hUxtheme) {
            hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        }

        if (hUxtheme) {
            // #135 is the ordinal for SetPreferredAppMode private API
            // which is available in uxtheme.dll since Windows 10 1903+
            SetPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
            SetPreferredAppMode(g_isDarkMode ? PreferredAppMode::ForceDark : PreferredAppMode::ForceLight);
        }
    }
}

LRESULT CALLBACK CallWndSubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam) {
    switch (nCode) {
        case HCBT_CREATEWND:
        {
            HWND hWnd = (HWND)wParam;
            if (IsUnityWndClass(hWnd) ||
                IsWndClass(hWnd, L"#32768") ||   // 팝업/컨텍스트 메뉴
                IsWndClass(hWnd, L"#32770") ||
                IsWndClass(hWnd, L"Button") ||
                IsWndClass(hWnd, L"tooltips_class32") ||
                IsWndClass(hWnd, L"ComboBox") ||
                IsWndClass(hWnd, L"SysListView32") ||
                IsWndClass(hWnd, L"SysTreeView32")) {

                EnableDarkMode(hWnd);
                SetWindowSubclass(hWnd, CallWndSubClassProc, 0, 0);
            }
            break;
        }
        case HCBT_DESTROYWND:
        {
            HWND hWnd = (HWND)wParam;
            if (IsUnityWndClass(hWnd) ||
                IsWndClass(hWnd, L"#32768") ||   // 팝업/컨텍스트 메뉴
                IsWndClass(hWnd, L"#32770") ||
                IsWndClass(hWnd, L"Button") ||
                IsWndClass(hWnd, L"tooltips_class32") ||
                IsWndClass(hWnd, L"ComboBox") ||
                IsWndClass(hWnd, L"SysListView32") ||
                IsWndClass(hWnd, L"SysTreeView32")) {

                RemoveWindowSubclass(hWnd, CallWndSubClassProc, 0);
            }
            break;
        }
        default:
            break;
    }
    return 0;
}





void UAHDrawMenuNCBottomLine(HWND hWnd) {
    MENUBARINFO mbi = { sizeof(mbi) };

    if (!GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi))
    {
        return;
    }

    RECT rcClient = { 0 };
    GetClientRect(hWnd, &rcClient);
    MapWindowPoints(hWnd, nullptr, (POINT*)&rcClient, 2);

    RECT rcWindow = { 0 };
    GetWindowRect(hWnd, &rcWindow);
    OffsetRect(&rcClient, -rcWindow.left, -rcWindow.top);
    // the rcBar is offset by the window rect
    RECT rcAnnoyingLine = rcClient;
    rcAnnoyingLine.bottom = rcAnnoyingLine.top;
    rcAnnoyingLine.top--;

    HDC hdc = GetWindowDC(hWnd);
    FillRect(hdc, &rcAnnoyingLine, LoadThemeConfig()->menubar_bgbrush);
    ReleaseDC(hWnd, hdc);
}


LRESULT CALLBACK CallWndSubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (uMsg) {
        case WM_CTLCOLORDLG:
        {
            // 다이얼로그 배경: dialog_bgcolor 사용
            HDC hdcDlg = reinterpret_cast<HDC>(wParam);
            SetBkColor(hdcDlg, LoadThemeConfig()->dialog_bgcolor);
            return (INT_PTR)LoadThemeConfig()->dialog_bgbrush;
        }
        case WM_CTLCOLOREDIT:
        {
            HDC hdcStatic = (HDC)wParam;
            auto cfg = LoadThemeConfig();
            bool isDialog = IsWndClass(hWnd, L"#32770");
            SetTextColor(hdcStatic, cfg->menubar_textcolor);
            SetBkColor(hdcStatic, isDialog ? cfg->dialog_bgcolor : cfg->menubar_bgcolor);
            return (INT_PTR)(isDialog ? cfg->dialog_bgbrush : cfg->menubar_bgbrush);
        }
        case WM_CTLCOLORLISTBOX:
        {
            if (IsWndClass(hWnd, L"ComboBox")) {
                COMBOBOXINFO info;
                info.cbSize = sizeof(info);
                SendMessage(hWnd, CB_GETCOMBOBOXINFO, 0, (LPARAM)&info);

                if ((HWND)lParam == info.hwndList)
                {
                    HDC dc = (HDC)wParam;
                    SetBkMode(dc, OPAQUE);
                    SetTextColor(dc, LoadThemeConfig()->menubar_textcolor);
                    SetBkColor(dc, LoadThemeConfig()->menubar_bgcolor);
                    return (LRESULT)LoadThemeConfig()->menubar_bgbrush;
                }
            }
            break;
        }
        case WM_CTLCOLORSCROLLBAR:
        {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            auto cfg = LoadThemeConfig();
            bool isDialog = IsWndClass(hWnd, L"#32770");
            SetTextColor(hdc, cfg->menubar_textcolor);
            SetBkColor(hdc, isDialog ? cfg->dialog_bgcolor : cfg->menubar_bgcolor);
            return reinterpret_cast<LRESULT>(isDialog ? cfg->dialog_bgbrush : cfg->menubar_bgbrush);
        }
        case WM_CTLCOLORSTATIC:
        {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            auto cfg = LoadThemeConfig();
            bool isDialog = IsWndClass(hWnd, L"#32770");
            SetTextColor(hdc, cfg->menubar_textcolor);
            SetBkColor(hdc, isDialog ? cfg->dialog_bgcolor : cfg->menubar_bgcolor);
            return reinterpret_cast<LRESULT>(isDialog ? cfg->dialog_bgbrush : cfg->menubar_bgbrush);
        }
        case WM_ERASEBKGND:
        {
            // 버튼 창은 타입에 따라 분기
            if (IsWndClass(hWnd, L"Button")) {
                DWORD style = GetWindowLongPtr(hWnd, GWL_STYLE);
                if ((style & 0xF) == BS_GROUPBOX) break; // GroupBox는 시스템에 맡김
            }
            // 다이얼로그는 dialog_bgcolor, 나머지는 menubar_bgcolor
            RECT rc;
            GetClientRect(hWnd, &rc);
            auto cfg = LoadThemeConfig();
            HBRUSH fillBrush = IsWndClass(hWnd, L"#32770") ? cfg->dialog_bgbrush : cfg->menubar_bgbrush;
            FillRect(reinterpret_cast<HDC>(wParam), &rc, fillBrush);
            return TRUE;
        }
        case WM_NCACTIVATE:
        {
            if (IsUnityWndClass(hWnd) || IsWndClass(hWnd, L"#32770")) {
                LRESULT lr = DefSubclassProc(hWnd, uMsg, wParam, lParam);
                UAHDrawMenuNCBottomLine(hWnd);
                return lr;
            }
            break;
        }
        case WM_NCCREATE:
        {
            EnableDarkMode(hWnd);
            
            if (IsWndClass(hWnd, L"#32768")) {
                // 윈도우 11 지정 API: 권한 상실 없이 네이티브 컨텍스트 메뉴 보더 컬러 입히기
                #ifndef DWMWA_BORDER_COLOR
                #define DWMWA_BORDER_COLOR 34
                #endif
                COLORREF borderColor = LoadThemeConfig()->menubar_bgcolor;
                DwmSetWindowAttribute(hWnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
            }
            
            if (IsWndClass(hWnd, L"tooltips_class32")) {
                SetWindowTheme(hWnd, L"wstr", L"wstr");
            }
            else if (IsWndClass(hWnd, L"ComboBox")) {
                SetWindowTheme(hWnd, L"wstr", L"wstr");
            }
            else if (IsWndClass(hWnd, L"Button")) {
                ApplyButtonTheme(hWnd);
            }
            break;
        }
        case WM_NCPAINT:
        {
            if (IsUnityWndClass(hWnd) || IsWndClass(hWnd, L"#32770")) {
                LRESULT lr = DefSubclassProc(hWnd, uMsg, wParam, lParam);
                UAHDrawMenuNCBottomLine(hWnd);
                return lr;
            }
            break;
        }

        case WM_DRAWITEM:
        {
            // Owner Draw 버튼 직접 렌더링 (코너 아티팩트 없음)
            if (!IsWndClass(hWnd, L"#32770")) break;
            DRAWITEMSTRUCT* pdis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            if (pdis->CtlType != ODT_BUTTON) break;

            const auto* cfg = LoadThemeConfig();
            HDC hdc = pdis->hDC;
            RECT rc = pdis->rcItem;

            bool isPressed = (pdis->itemState & ODS_SELECTED) != 0;
            bool isHovered = GetProp(pdis->hwndItem, L"Hovering") != nullptr;

            // 배경: 상태별 색상
            COLORREF bgColor = isPressed ? cfg->button_bgcolor_pressed
                             : isHovered ? cfg->button_bgcolor_hot
                             :             cfg->button_bgcolor;
            HBRUSH hBgBrush = CreateSolidBrush(bgColor);
            FillRect(hdc, &rc, hBgBrush);
            DeleteObject(hBgBrush);

            // 이너 보더
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            HPEN hInnerPen = CreatePen(PS_SOLID, 1, cfg->button_inner_bordercolor);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hInnerPen);
            Rectangle(hdc, rc.left + 1, rc.top + 1, rc.right - 1, rc.bottom - 1);
            SelectObject(hdc, hOldPen);
            DeleteObject(hInnerPen);

            // 외곽 보더
            HPEN hOuterPen = CreatePen(PS_SOLID, 1, cfg->button_bordercolor);
            SelectObject(hdc, hOuterPen);
            Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
            SelectObject(hdc, hOldPen);
            DeleteObject(hOuterPen);

            SelectObject(hdc, hOldBrush);

            // 텍스트
            wchar_t text[256] = {};
            GetWindowTextW(pdis->hwndItem, text, 256);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, (pdis->itemState & ODS_DISABLED)
                ? cfg->menubar_textcolor_disabled
                : cfg->menubar_textcolor);
            DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            return TRUE;
        }

        case WM_PAINT:
        {
            if (IsUnityWndClass(hWnd) || IsWndClass(hWnd, L"#32770")) {
                LRESULT lr = DefSubclassProc(hWnd, uMsg, wParam, lParam);
                UAHDrawMenuNCBottomLine(hWnd);
                return lr;
            }
            else if (IsWndClass(hWnd, L"tooltips_class32")) {
                SendMessage(hWnd, TTM_SETTIPBKCOLOR, LoadThemeConfig()->menubar_bgcolor, 0);
                SendMessage(hWnd, TTM_SETTIPTEXTCOLOR, LoadThemeConfig()->menubar_textcolor, 0);
            }
            else if (IsWndClass(hWnd, L"SysTreeView32")) { // left pane of config dialog
                TreeView_SetBkColor(hWnd, LoadThemeConfig()->menubar_bgcolor);
                TreeView_SetTextColor(hWnd, LoadThemeConfig()->menubar_textcolor);
            }
            else if (IsWndClass(hWnd, L"SysListView32")) { // left pane of FX dialog
                ListView_SetBkColor(hWnd, LoadThemeConfig()->menubar_bgcolor);
                ListView_SetTextBkColor(hWnd, LoadThemeConfig()->menubar_bgcolor);
                ListView_SetTextColor(hWnd, LoadThemeConfig()->menubar_textcolor);
            }
            break;
        }
        case WM_STYLECHANGING:
        case WM_STYLECHANGED:
        {
            if (IsUnityWndClass(hWnd)) {
                // prevent propagation to prevent menu bar from going back to the standard one...?!? FIXME
                return true;
            }
            break;
        }
        case WM_MOUSEMOVE:
        {
            if (IsWndClass(hWnd, L"Button") && !GetProp(hWnd, L"Hovering")) {
                SetProp(hWnd, L"Hovering", (HANDLE)1);
                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
                TrackMouseEvent(&tme);
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            break;
        }
        case WM_MOUSELEAVE:
        {
            if (IsWndClass(hWnd, L"Button")) {
                RemoveProp(hWnd, L"Hovering");
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            break;
        }
        case WM_THEMECHANGED:
        {
            if (IsUnityWndClass(hWnd) || IsWndClass(hWnd, L"#32770")) {
                if (g_menuTheme) {
                    CloseThemeData(g_menuTheme);
                    g_menuTheme = nullptr;
                }
            }
            break;
        }
        // https://stackoverflow.com/questions/77985210/how-to-set-menu-bar-color-in-win32
        case WM_UAHDRAWMENU:
        {
            if (IsUnityWndClass(hWnd) || IsWndClass(hWnd, L"#32770")) {
                UAHMENU* pUDM = (UAHMENU*)lParam;
                RECT rc = { 0 };
                {
                    MENUBARINFO mbi = { sizeof(mbi) };
                    GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi);

                    RECT rcWindow;
                    GetWindowRect(hWnd, &rcWindow);
                    // the rcBar is offset by the window rect
                    rc = mbi.rcBar;
                    OffsetRect(&rc, -rcWindow.left, -rcWindow.top);
                }
                FillRect(pUDM->hdc, &rc, LoadThemeConfig()->menubar_bgbrush);
                UAHDrawMenuNCBottomLine(hWnd);
                return true;
            }
            break;
        }
        case WM_UAHDRAWMENUITEM:
        {
            if (IsUnityWndClass(hWnd) || IsWndClass(hWnd, L"#32770")) {
                UAHDRAWMENUITEM* pUDMI = (UAHDRAWMENUITEM*)lParam;

                const HBRUSH* pbrBackground = &LoadThemeConfig()->menubaritem_bgbrush;
                // get the menu item string
                wchar_t menuString[256] = { 0 };
                MENUITEMINFO mii = { sizeof(mii), MIIM_STRING };
                {
                    mii.dwTypeData = menuString;
                    mii.cch = (sizeof(menuString) / 2) - 1;

                    GetMenuItemInfo(pUDMI->um.hmenu, pUDMI->umi.iPosition, TRUE, &mii);
                }
                // get the item state for drawing
                DWORD dwFlags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;
                int iTextStateID = 0;

                if ((pUDMI->dis.itemState & ODS_INACTIVE) | (pUDMI->dis.itemState & ODS_DEFAULT)) {
                    // normal display
                    iTextStateID = MPI_NORMAL;
                }
                if (pUDMI->dis.itemState & ODS_HOTLIGHT) {
                    // hot tracking
                    iTextStateID = MPI_HOT;
                    pbrBackground = &LoadThemeConfig()->menubaritem_bgbrush_hot;
                }
                if (pUDMI->dis.itemState & ODS_SELECTED) {
                    // clicked -- MENU_POPUPITEM has no state for this, though MENU_BARITEM does
                    iTextStateID = MPI_HOT;
                    pbrBackground = &LoadThemeConfig()->menubaritem_bgbrush_selected;
                }
                if ((pUDMI->dis.itemState & ODS_GRAYED) || (pUDMI->dis.itemState & ODS_DISABLED)) {
                    // disabled / grey text
                    iTextStateID = MPI_DISABLED;
                }
                if (pUDMI->dis.itemState & ODS_NOACCEL) {
                    dwFlags |= DT_HIDEPREFIX;
                }

                if (!g_menuTheme) {
                    g_menuTheme = OpenThemeData(hWnd, L"Menu");
                }

                DTTOPTS opts = { sizeof(opts), DTT_TEXTCOLOR, iTextStateID != MPI_DISABLED ? LoadThemeConfig()->menubar_textcolor : LoadThemeConfig()->menubar_textcolor_disabled };
                FillRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, *pbrBackground);

                DrawThemeTextEx(g_menuTheme, pUDMI->um.hdc, MENU_BARITEM, MBI_NORMAL, menuString, mii.cch, dwFlags, &pUDMI->dis.rcItem, &opts);
                return true;
            }
            break;
        }
        case WM_UAHMEASUREMENUITEM:
        {
            if (!IsUnityWndClass(hWnd) && !IsWndClass(hWnd, L"#32770")) {

                //UAHMEASUREMENUITEM* pMmi = (UAHMEASUREMENUITEM*)lParam;
                // allow the default window procedure to handle the message
                // since we don't really care about changing the width
                //*lr = DefWindowProc(hWnd, message, wParam, lParam);
                LRESULT lr = DefSubclassProc(hWnd, uMsg, wParam, lParam);
                // but we can modify it here to make it 1/3rd wider for example
                //pMmi->mis.itemWidth = (pMmi->mis.itemWidth * 4) / 3;
                return lr;
            }
            break;
        }
        default:
            break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// C# 에서 P/Invoke로 호출: 테마 모드 전환 및 모든 창 갱신
extern "C" __declspec(dllexport) void SetThemeMode(bool isDark) {
    if (g_isDarkMode == isDark) return;
    g_isDarkMode = isDark;

    // 테마 메뉴 캐시 초기화
    if (g_menuTheme) {
        CloseThemeData(g_menuTheme);
        g_menuTheme = nullptr;
    }

    // 모든 Unity 프로세스 창에 다크/라이트 모드 재적용 후 강제 갱신
    std::vector<HWND> windowHandles;
    GetAllWindowsByProcessID(GetCurrentProcessId(), windowHandles);
    for (const HWND& hWnd : windowHandles) {
        EnableDarkMode(hWnd);
        // 이미 열려 있는 다이얼로그/창의 버튼 자식들도 소급 갱신
        ApplyButtonThemeToChildren(hWnd);
        RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    }

    // 팝업/컨텍스트 메뉴 테마 캐시 강제 플러시
    // uxtheme.dll 오디널 136 = FlushMenuThemes (비공개 API, Windows 10 1903+)
    HMODULE hUxtheme = GetModuleHandleW(L"uxtheme.dll");
    if (hUxtheme) {
        using fnFlushMenuThemes = void(WINAPI*)();
        auto FlushMenuThemes = (fnFlushMenuThemes)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136));
        if (FlushMenuThemes) FlushMenuThemes();
    }
}

// DLL entry
bool APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpRes) {
    switch (reason)
    {
        case DLL_PROCESS_ATTACH: {
            // enable dark mode for the current process window
            EnableDarkMode(nullptr);

            // catch windows that have been created before we set up the CBTProc hook
            std::vector<HWND> windowHandles;
            GetAllWindowsByProcessID(GetCurrentProcessId(), windowHandles);

            for (const HWND& hWnd : windowHandles) {
                SetWindowSubclass(hWnd, CallWndSubClassProc, 0, 0);
                EnableDarkMode(hWnd);
            }

            // set up the CBTProc hook to catch new windows
            g_hook = SetWindowsHookEx(WH_CBT, CBTProc, nullptr, GetCurrentThreadId());
            break;
        }
        case DLL_PROCESS_DETACH: {
            if (g_hook) {
                UnhookWindowsHookEx(g_hook);
                g_hook = nullptr;
            }
            break;
        }
        default: break;
    }

    return true;
}