// DX11-test.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "DX11-test.h"
#include <stdio.h>
#include <vadefs.h>

#define MAX_LOADSTRING 100

#define WIDE2(x) L##x
#define WIDE1(x) WIDE2(x)

#define log(fmt, ...) log_impl(fmt, WIDE1(__FILE__), __LINE__, __VA_ARGS__)

void log_impl(LPCWSTR format, const wchar_t* file, int line, ...)
{
    WCHAR buffer[2048];
    WCHAR buffer2[2048];
    va_list arg;
    va_start(arg, line);
    _vsnwprintf_s(buffer, sizeof(buffer), format, arg);
    _snwprintf_s(buffer2, sizeof(buffer2), L"%s (%s:%i)\n", buffer, file, line);
    va_end(arg);

    OutputDebugString(buffer2);
}

// Global Variables:
HINSTANCE instance_handle;
HWND window_handle;
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

static LRESULT CALLBACK WndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch (umsg) {
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }

        // Check if the window is being closed.
        case WM_CLOSE: {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, umsg, wparam, lparam);
}

void draw_frame();

void initialize_window(int& screenWidth, int& screenHeight)
{
    WNDCLASSEX wc;
    DEVMODE dmScreenSettings;
    int posX, posY;

    // Get the instance of this application.
    instance_handle = GetModuleHandle(NULL);

    LPCWSTR application_name = L"Hello";

    // Setup the windows class with default settings.
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance_handle;
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc.hIconSm = wc.hIcon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = application_name;
    wc.cbSize = sizeof(WNDCLASSEX);

    // Register the window class.
    RegisterClassEx(&wc);

    // Determine the resolution of the clients desktop screen.
    screenWidth = GetSystemMetrics(SM_CXSCREEN);
    screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Setup the screen settings depending on whether it is running in full screen or in windowed mode.
    bool full_screen = 0;
    if (full_screen)
    {
        // If full screen set the screen to maximum size of the users desktop and 32bit.
        memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
        dmScreenSettings.dmSize = sizeof(dmScreenSettings);
        dmScreenSettings.dmPelsWidth = (unsigned long)screenWidth;
        dmScreenSettings.dmPelsHeight = (unsigned long)screenHeight;
        dmScreenSettings.dmBitsPerPel = 32;
        dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

        // Change the display settings to full screen.
        ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

        // Set the position of the window to the top left corner.
        posX = posY = 0;
    }
    else
    {
        // If windowed then set it to 800x600 resolution.
        screenWidth = 800;
        screenHeight = 600;

        // Place the window in the middle of the screen.
        posX = (GetSystemMetrics(SM_CXSCREEN) - screenWidth) / 2;
        posY = (GetSystemMetrics(SM_CYSCREEN) - screenHeight) / 2;
    }

    // Create the window with the screen settings and get the handle to it.
    DWORD window_style = 0;
    window_style = WS_SYSMENU; // show close menu

    //window_style |= WS_CLIPSIBLINGS;
    //window_style |= WS_CLIPCHILDREN;
    //window_style |= WS_POPUP;

    window_handle = CreateWindowEx(WS_EX_APPWINDOW, application_name, application_name,
        window_style,
        posX, posY, screenWidth, screenHeight, NULL, NULL, instance_handle, NULL);

    // Bring the window up on the screen and set it as main focus.
    ShowWindow(window_handle, SW_SHOW);
    SetForegroundWindow(window_handle);
    SetFocus(window_handle);

    // Hide the mouse cursor.
    //ShowCursor(false);

    return;
}


// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    log(L"Hello mother fucking world!", 134);

    int screenWidth, screenHeight;

    // Initialize the width and height of the screen to zero before sending the variables into the function.
    screenWidth = 0;
    screenHeight = 0;

    initialize_window(screenWidth, screenHeight);

    //
    // Main loop
    //
    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));

    // Loop until there is a quit message from the window or the user.
    while (1) {
        // Handle the windows messages.
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (msg.message == WM_QUIT) {
            break;
        }
        else {
            draw_frame();
        }

    }

    return 0;
}

void draw_frame()
{
    log(L"Frame", 0);
}
