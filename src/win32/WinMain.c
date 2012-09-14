/*! @file WinMain.c
 *  @brief Main entry point
 *  @author Kyle
 *  @date 9/13/2012 3:41:54 PM
 *  @copyright Copyright (c) 2012 Kyle. All rights reserved.
 */
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <Shellapi.h>
#include "application.h"

/*
 * Internal 
 */
static const char _class_name[] = "deferred";

/* Variables */
static HWND _hwnd = NULL;
static int  _fullscreen = 0;

/*
 * Functions
 */
void* app_get_window(void) { return _hwnd; }

static LRESULT CALLBACK _WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

static void _create_application(HINSTANCE hInstance, const char* name)
{
    WNDCLASSEX wcex;
    wcex.cbSize         = sizeof(WNDCLASSEX); 
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = _WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = NULL;
    wcex.hCursor        = (HCURSOR)LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = NULL;
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = name;
    wcex.hIconSm        = NULL;
    if(!RegisterClassEx(&wcex))
        MessageBox(NULL, "Could not register class", "Error", MB_OK);
}
static void _find_screen_top_left(int* x, int* y) 
{
    RECT rect;
    HWND taskBar;
    int width;

    *y = 0;
    *x = 0;
    taskBar = FindWindow("Shell_traywnd", NULL);
    width = GetSystemMetrics(SM_CXSCREEN);

    if(taskBar && GetWindowRect(taskBar, &rect)) {
        if(rect.right == width && rect.top == 0) /* Top taskbar */
            *y = rect.bottom;
        else if(rect.right != width) /* Left taskbar */
            *x = rect.right;
    }
}

static HWND _create_window(HINSTANCE hInstance, const char* name) 
{
    RECT rect;
    HWND hwnd;
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    UINT uiStyle = WS_OVERLAPPEDWINDOW;
    int x;
    int y;
    _find_screen_top_left(&x, &y);

    rect.left = 0;
    rect.top = 0;
    rect.right = width/2;
    rect.bottom = height/2;

    AdjustWindowRect(&rect, uiStyle, FALSE);
    hwnd = CreateWindow(name, 
                        "Main Window", 
                        uiStyle, 
                        x, y,
                        rect.right-rect.left, 
                        rect.bottom-rect.top, 
                        NULL, NULL, 
                        hInstance, NULL);
    if(hwnd == NULL)
        MessageBox(NULL, "Could not create window", "Error", MB_OK);
    return hwnd;
}

static void _toggle_fullscreen(void) 
{
    _fullscreen = !_fullscreen;
    if(_fullscreen) {
        int width = GetSystemMetrics(SM_CXSCREEN);
        int height = GetSystemMetrics(SM_CYSCREEN);
        SetWindowLongPtr(_hwnd, GWL_STYLE, 
            WS_SYSMENU | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE); 
        MoveWindow(_hwnd, 0, 0, width, height, TRUE); 
    } else {
        int width = GetSystemMetrics(SM_CXSCREEN);
        int height = GetSystemMetrics(SM_CYSCREEN);
        RECT rect; 
        rect.left = 0; 
        rect.top = 0; 
        rect.right = width/2; 
        rect.bottom = height/2; 
        SetWindowLongPtr(_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE); 
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE); 
        MoveWindow(_hwnd, 0, 0, rect.right-rect.left, rect.bottom-rect.top, TRUE);
    }
}

LRESULT CALLBACK _WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam ) 
{
    switch(message) {
    case WM_CREATE: 
        return 0;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        switch(wParam) {
        case VK_ESCAPE:
            PostQuitMessage(0);
            return 0;
        case VK_RETURN:
            if ((HIWORD(lParam) & KF_ALTDOWN)) {
                _toggle_fullscreen();
            }
            return 0;
        }
    case WM_MENUCHAR: /* identify alt+enter, make it not beep since we're handling it: */
        if( LOWORD(wParam) & VK_RETURN )
            return MAKELRESULT(0, MNC_CLOSE);
        return MAKELRESULT(0, MNC_IGNORE);
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 * External 
 */
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
    int argc = 0;
    int ii;
    LPWSTR cmd_line = GetCommandLineW();
    LPWSTR* argvw = CommandLineToArgvW(cmd_line, &argc);
    char args[256][32] = {0};
    const char* argv[32] = {0};
    for(ii=0;ii<argc;++ii) {
        WideCharToMultiByte(CP_ACP, 0, argvw[ii], -1, args[ii], sizeof(args[ii]), NULL, NULL);
        argv[ii] = args[ii];
    }
    return ApplicationMain(argc, argv);
    (void)sizeof(hInstance);
    (void)sizeof(hPrevInstance);
    (void)sizeof(nCmdShow);
    (void)sizeof(lpCmdLine);
}
int ApplicationMain(int argc, const char* argv[])
{
    MSG msg = {0};
    HINSTANCE hInstance = GetModuleHandle(NULL);
    _create_application(hInstance, _class_name);
    _hwnd = _create_window(hInstance, _class_name);
    ShowWindow(_hwnd, SW_SHOWNORMAL);

    /* TODO: Initialization code */
    on_init(argc, argv);

    /* Main loop */
    do {
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg); 
            DispatchMessage(&msg); 
        } else {
            /* TODO: Per-frame code */
            on_frame();
        }
    } while(msg.message != WM_QUIT);

    /* TODO: Shutdown code */
    on_shutdown();

    return 0;
}
