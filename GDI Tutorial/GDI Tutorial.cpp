// GDI Tutorial.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "framework.h"
#include "GDI Tutorial.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

class AutoGetDC {
public:
    AutoGetDC(HWND hWnd) {
        m_hWnd = hWnd;
        m_hdc = GetDC(hWnd);
    }

    ~AutoGetDC() 
    {
        ReleaseDC(m_hWnd, m_hdc);
    }

    operator HDC() {
        return m_hdc;
    }

private:
    HWND m_hWnd = NULL;
    HDC m_hdc = NULL;
};

template<typename T>
class AutoGDIObject {
public:
    AutoGDIObject<T>& operator = (T obj) {
        if (m_obj && m_obj != obj) {
            DeleteObject(m_obj);
        }
        m_obj = obj;
        return *this;
    }

    operator T() const {
        return m_obj;
    }

    ~AutoGDIObject() {
        DeleteObject(m_obj);
    }

private:
    T m_obj = NULL;
};

using AutoPen = AutoGDIObject<HPEN>;
using AutoBrush = AutoGDIObject<HBRUSH>;
using AutoBitmap = AutoGDIObject<HBITMAP>;
using AutoFont = AutoGDIObject<HFONT>;

class Application {
public:
    enum class Mode {
        DRAW_NONE,
        DRAW_LINE,
        DRAW_RECT,
        DRAW_TEXT,
    };

public:
    Application() {}
    Application(Application&) = delete;
    Application& operator = (Application&) = delete;

    void Initialize(HWND hWnd, uint32_t width, uint32_t height) {
        AutoGetDC hdc(hWnd);
        m_width = width;
        m_height = height;
        m_memDc = CreateCompatibleDC(hdc);
        m_bmp = CreateCompatibleBitmap(hdc, width, height);
        SelectObject(m_memDc, m_bmp);
        SelectObject(m_memDc, GetStockObject(WHITE_PEN));
        Rectangle(m_memDc, 0, 0, width, height);
        auto fontPtSize = -MulDiv(18, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        SetPenColor(RGB(0, 0, 0));
        SetBrushColor(RGB(0, 0, 0));
        LOGFONT lf = {
        fontPtSize, //height
            0,          // width
            0,          // escapement
            0,          // orientation
            FW_NORMAL,  // weight
            FALSE,      // bold
            FALSE,      // italic
            FALSE,      // strikeout
            DEFAULT_CHARSET, // char set
            OUT_DEFAULT_PRECIS, // output precision
            CLIP_DEFAULT_PRECIS, // clip precision
            DEFAULT_QUALITY,  // rendering quality
            DEFAULT_PITCH | FF_DONTCARE, // pitch and family
            L"Arial" // font name
        };
        SetFont(lf);
    }

    ~Application() {
        DeleteDC(m_memDc);
    }

    void SetPenColor(COLORREF color) {
        m_pen = ::CreatePen(PS_SOLID, 2, color);
    }

    void SetBrushColor(COLORREF color) {
        m_brush = CreateSolidBrush(color);
    }

    void SetFont(const LOGFONT& lf) {
        m_font = ::CreateFontIndirect(&lf);
    }

    void SetMode(Mode mode) {
        m_mode = mode;
    }

    Mode GetMode() const {
        return m_mode;
    }

    void SetPen(HPEN pen) {
        if (m_pen) {
            DeleteObject(m_pen);
        }
        m_pen = pen;
    }

    HPEN GetPen() const {
        return m_pen;
    }

    void SetBrush(HBRUSH brush) {
        if (m_brush) {
            DeleteObject(m_brush);
        }
        m_brush = brush;
    }

    HBRUSH GetBrush() const {
        return m_brush;
    }

    void SetFont(HFONT font) {
        if (m_font) {
            DeleteObject(m_font);
        }
        m_font = font;
    }

    HFONT GetFont() const {
        return m_font;
    }

    COLORREF GetFontColor() {
        return m_textForegroundColor;
    }

    void SetFontColor(COLORREF textForegroundColor) {
        m_textForegroundColor = textForegroundColor;
    }

    void Draw(HDC hdc, POINT prev, POINT curr) {
        BitBlt(hdc, 0, 0, m_width, m_height, m_memDc, 0, 0, SRCCOPY);
        switch (m_mode) {
            case Mode::DRAW_LINE:
            {
                HPEN oldPen = static_cast<HPEN>(GetCurrentObject(hdc, OBJ_PEN));
                if (GetPen())
                {
                    SelectObject(hdc, m_pen);
                }
                MoveToEx(hdc, prev.x, prev.y, nullptr);
                LineTo(hdc, curr.x, curr.y);
                SelectObject(hdc, oldPen);
            }
                break;
            case Mode::DRAW_RECT:
            {
                HBRUSH oldBrush = static_cast<HBRUSH>(GetCurrentObject(hdc, OBJ_BRUSH));
                HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, GetStockObject(NULL_PEN)));
                if (GetBrush()) {
                    SelectObject(hdc, m_brush);
                }
                else {
                    SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
                }
                Rectangle(hdc, prev.x, prev.y, curr.x, curr.y);
                SelectObject(hdc, oldBrush);
            }
                break;
            case Mode::DRAW_TEXT:
            {
                auto oldFont = SelectObject(hdc, m_font);
                auto oldTextColor = SetTextColor(hdc, m_textForegroundColor);
                auto oldBackgroundColor = SetBkColor(hdc, m_textBackgroundColor);
                auto oldAlign = SetTextAlign(hdc, TA_LEFT | TA_BASELINE);

                std::wstring  sample = L"Sample Text";
                TextOut(hdc, curr.x, curr.y, sample.c_str(), sample.size());

                SetTextAlign(hdc, oldAlign);
                SetTextColor(hdc, oldTextColor);
                SetBkColor(hdc, oldBackgroundColor);
                SelectObject(hdc, oldFont);
            }
                break;
        }
    }

    void UpdateDrawing(HDC hdc) {
        BitBlt(m_memDc, 0, 0, m_width, m_height, hdc, 0, 0, SRCCOPY);
    }

    void Repaint(HDC hdc) {
        BitBlt(hdc, 0, 0, m_width, m_height, m_memDc, 0, 0, SRCCOPY);
    }

    void Print(HDC /*printerDC*/ hdc) {
        DOCINFO docInfo;
        ZeroMemory(&docInfo, sizeof(docInfo));
        docInfo.cbSize = sizeof(docInfo);
        docInfo.lpszDocName = L"GDI Tutorial";
		StartDoc(hdc, &docInfo);
		StartPage(hdc);

		// Drawing code begin
		//    
        BitBlt(hdc, 0, 0, m_width, m_height, m_memDc, 0, 0, SRCCOPY);
		//
		// Drawing code end

		EndPage(hdc);
		EndDoc(hdc);
    }

private:
    Mode m_mode = Mode::DRAW_LINE;

    HDC m_memDc = NULL;
    AutoBitmap m_bmp;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    COLORREF m_textForegroundColor = RGB(0, 0, 0);
    COLORREF m_textBackgroundColor = RGB(255, 255, 255);

    AutoPen m_pen;
    AutoBrush m_brush;
    AutoFont m_font;
};


Application app;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_GDITUTORIAL, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GDITUTORIAL));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GDITUTORIAL));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_GDITUTORIAL);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPED|WS_SYSMENU|WS_CAPTION|WS_BORDER|WS_MINIMIZEBOX,
        CW_USEDEFAULT, 0, 800, 600, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static POINT prevPoint = {};
    static bool buttonDown = false;
    static COLORREF customColors[16];

    switch (message)
    {
    case WM_CREATE:
        break;

    case WM_SIZE:
        {
            uint32_t width = LOWORD(lParam);
            uint32_t height = HIWORD(lParam);
            app.Initialize(hWnd, width, height);
        }
        break;

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;

        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;

        case ID_EDIT_PEN:
        {
            LOGPEN lp;
            GetObject(app.GetPen(), sizeof(LOGPEN), &lp);
            CHOOSECOLOR clr = {sizeof(CHOOSECOLOR), hWnd, NULL, lp.lopnColor, customColors};
            if (ChooseColor(&clr)) {
                app.SetPenColor(clr.rgbResult);
            }
        }
            break;

        case ID_EDIT_BRUSH:
        {
            LOGBRUSH lb;
            GetObject(app.GetPen(), sizeof(LOGBRUSH), &lb);
            CHOOSECOLOR clr = { sizeof(CHOOSECOLOR), hWnd, NULL, lb.lbColor, customColors };
            if (ChooseColor(&clr)) {
                app.SetBrushColor(clr.rgbResult);
            }
        }
        break;

        case ID_EDIT_FONT:
        {
            LOGFONT lf;
            GetObject(app.GetFont(), sizeof(LOGFONT), &lf);
            CHOOSEFONT cf = {sizeof(CHOOSEFONT), hWnd, NULL, &lf, 0, CF_BOTH| CF_INITTOLOGFONTSTRUCT | CF_EFFECTS,  app.GetFontColor()};

            if (ChooseFont(&cf)) {
                app.SetFontColor(cf.rgbColors);
                app.SetFont(lf);
            }
        }
        break;

        case ID_DRAW_LINE:
            app.SetMode(Application::Mode::DRAW_LINE);
            break;
        case ID_DRAW_RECT:
            app.SetMode(Application::Mode::DRAW_RECT);
            break;
        case ID_DRAW_TEXT:
            app.SetMode(Application::Mode::DRAW_TEXT);
            break;

        case ID_FILE_PRINT:
        {
            PRINTDLG pd = { 0 };
            pd.lStructSize = sizeof(pd);
            pd.hwndOwner = hWnd;
            pd.Flags = PD_RETURNDC;

            // Retrieves the printer DC
            if (PrintDlg(&pd))
            {
                app.Print(pd.hDC);
                DeleteObject(pd.hDC);
            }
        }
        break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;

    case WM_LBUTTONDOWN:
    {
        prevPoint = { LOWORD(lParam), HIWORD(lParam) };
        buttonDown = true;
    }
    break;

    case WM_MOUSEMOVE:
    if (buttonDown) {
        POINT currPoint = { LOWORD(lParam), HIWORD(lParam) };
        AutoGetDC hdc(hWnd);
        app.Draw(hdc, prevPoint, currPoint);
    }
    break;

    case WM_LBUTTONUP:
    if (buttonDown) {
        POINT currPoint = { LOWORD(lParam), HIWORD(lParam) };
        AutoGetDC hdc(hWnd);
        app.Draw(hdc, prevPoint, currPoint);
        app.UpdateDrawing(hdc);
        buttonDown = false;
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        app.Repaint(hdc);
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
