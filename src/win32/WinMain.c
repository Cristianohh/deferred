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
#include <windowsx.h>
#include <Shellapi.h>
#include "application.h"
#include "unit_test.h"

/*
 * Internal 
 */
static const char _class_name[] = "deferred";

/* Variables */
static uint32_t     _width = 0;
static uint32_t     _height = 0;
static int          _prev_mouse_x = -1;
static int          _prev_mouse_y = -1;
static HWND         _hwnd = NULL;
static int          _fullscreen = 0;
static char         _keys[KEY_MAX_KEYS] = {0};
static char         _mouse_buttons[MOUSE_MAX_BUTTONS] = {0};
static SystemEvent  _event_queue[1024];
static uint32_t     _write_pos = 0;
static uint32_t     _read_pos = 0;
static HCURSOR      _cursor = NULL;

void _app_push_event(SystemEvent event) {
    _event_queue[_write_pos%1024] = event;
    _write_pos++;
}
int app_is_key_down(Key key) { return _keys[key]; }
int app_is_mouse_button_down(MouseButton button) { return _mouse_buttons[button]; }

const SystemEvent* app_pop_event(void) {
    if(_read_pos == _write_pos)
        return NULL;
    return &_event_queue[(_read_pos++) % 1024];
}
void app_lock_and_hide_cursor(void) { 
    RECT rect;
    GetClientRect(_hwnd, &rect);
    ClipCursor(&rect);
    SetCursor(NULL);
}
void app_unlock_and_show_cursor(void) {
    SetCursor(_cursor);
    ClipCursor(NULL);
}

static Key convert_keycode(uint8_t key)
{   
    if(key >= 'A' && key <= 'Z')
        return (key - 'A') + KEY_A;
    
    if(key >= '0' && key <= '9')
        return (key - '0') + KEY_0;

    if(key >= VK_F1 && key <= VK_F12)
        return (key - VK_F1) + KEY_F1;

    switch(key)
    {
    case VK_ESCAPE:
        return KEY_ESCAPE;

    case VK_SPACE:  
        return KEY_SPACE;

    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT:
        return KEY_SHIFT;
    case VK_CONTROL:
    case VK_LCONTROL:
    case VK_RCONTROL:
        return KEY_CTRL;
    case VK_MENU:
    case VK_LMENU:
    case VK_RMENU:
        return KEY_ALT;

    case VK_RETURN:
        return KEY_ENTER;

    case VK_LEFT:
        return KEY_LEFT;
    case VK_RIGHT:
        return KEY_RIGHT;
    case VK_UP:
        return KEY_UP;
    case VK_DOWN:
        return KEY_DOWN;

    default:
        return -1;
    }
}

/*
 * Functions
 */
void* app_get_window(void) { return _hwnd; }
MessageBoxResult message_box(const char* header, const char* message) {
    int result = MessageBox(_hwnd, message, header, MB_RETRYCANCEL);

    if(result == IDCANCEL) 
        return kMBCancel;
    if(result == IDRETRY)
        return kMBRetry;

    return kMBOK;
}

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
    wcex.hCursor        = NULL;
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
    Key key;
    switch(message) {
    case WM_CREATE:
        _cursor = GetCursor();
        return 0;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        key = convert_keycode((uint8_t)wParam);
        switch(wParam) {
        case VK_RETURN:
            if ((HIWORD(lParam) & KF_ALTDOWN)) {
                _toggle_fullscreen();
            }
            return 0;
        }
        if(key == -1)
            break;
        if(_keys[key] == 0) {
            SystemEvent event;
            event.type = kEventKeyDown;
            event.data.key = key;
            _app_push_event(event);
        }
        _keys[key] = 1;
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        key = convert_keycode((uint8_t)wParam);
        if(key == -1)
            break;
        _keys[key] = 0;
        break;
    case WM_MENUCHAR: /* identify alt+enter, make it not beep since we're handling it: */
        if( LOWORD(wParam) & VK_RETURN )
            return MAKELRESULT(0, MNC_CLOSE);
        return MAKELRESULT(0, MNC_IGNORE);
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    case WM_ERASEBKGND:
    case WM_SIZING:
        return 0;
    case WM_SIZE:
        {
            SystemEvent event;
            RECT rect;
            GetClientRect(_hwnd, &rect);
            _width = rect.right - rect.left;
            _height = rect.bottom - rect.top;
            event.data.resize.width = _width;
            event.data.resize.height = _height;
            event.type = kEventResize;
            _app_push_event(event);
        }
        return 0;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        {
            SystemEvent event;
            int x = GET_X_LPARAM(lParam);
            int y = _height - GET_Y_LPARAM(lParam);
            event.data.mouse.x = x;
            event.data.mouse.y = y;
            // L button
            if(wParam & MK_LBUTTON) {
                if(_mouse_buttons[MOUSE_LEFT] == 0) {
                    event.type = kEventMouseDown;
                    event.data.mouse.button = MOUSE_LEFT;
                    _app_push_event(event);
                }
                _mouse_buttons[MOUSE_LEFT] = 1;
            } else {
                if(_mouse_buttons[MOUSE_LEFT] == 1) {
                    event.type = kEventMouseUp;
                    event.data.mouse.button = MOUSE_LEFT;
                    _app_push_event(event);
                }
                _mouse_buttons[MOUSE_LEFT] = 0;
            }
            // R button
            if(wParam & MK_RBUTTON) {
                if(_mouse_buttons[MOUSE_RIGHT] == 0) {
                    event.type = kEventMouseDown;
                    event.data.mouse.button = MOUSE_RIGHT;
                    _app_push_event(event);
                }
                _mouse_buttons[MOUSE_RIGHT] = 1;
            } else {
                if(_mouse_buttons[MOUSE_RIGHT] == 1) {
                    event.type = kEventMouseUp;
                    event.data.mouse.button = MOUSE_RIGHT;
                    _app_push_event(event);
                }
                _mouse_buttons[MOUSE_RIGHT] = 0;
            }
            // M button
            if(wParam & MK_MBUTTON) {
                if(_mouse_buttons[MOUSE_MIDDLE] == 0) {
                    event.type = kEventMouseDown;
                    event.data.mouse.button = MOUSE_MIDDLE;
                    _app_push_event(event);
                }
                _mouse_buttons[MOUSE_MIDDLE] = 1;
            } else {
                if(_mouse_buttons[MOUSE_MIDDLE] == 1) {
                    event.type = kEventMouseUp;
                    event.data.mouse.button = MOUSE_MIDDLE;
                    _app_push_event(event);
                }
                _mouse_buttons[MOUSE_MIDDLE] = 0;
            }
        }
        return 0;
    case WM_MOUSEMOVE:
        {
            SystemEvent event;
            int x = GET_X_LPARAM(lParam);
            int y = _height - GET_Y_LPARAM(lParam);
            event.data.mouse.x = x - _prev_mouse_x;
            event.data.mouse.y = _prev_mouse_y - y;
            event.type = kEventMouseMove;
            _app_push_event(event);

            _prev_mouse_x = x;
            _prev_mouse_y = y;
        }
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
    char args[32][256] = {0};
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
    HINSTANCE hInstance = GetModuleHandle(NULL);
    MSG msg = {0};
    RUN_ALL_TESTS(argc, argv, "-t");
    
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
            if(on_frame())
                break;
        }
    } while(msg.message != WM_QUIT);

    /* TODO: Shutdown code */
    on_shutdown();

    return 0;
}
