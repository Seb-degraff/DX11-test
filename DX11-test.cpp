// DX11-test.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "utils.h"
#include "DX11-test.h"
#include "renderer.hpp"
#include "Windowsx.h" // required for GET_X_LPARAM (mouse related)
#include <math.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE instance_handle;
HWND window_handle;

#include "input.cpp" // NOTE(seb): Including the cpp directly here to simplify experimentation and reduce compile time during dev.

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

    if (input_recieve_event(hwnd, umsg, wparam, lparam)) {
        return 1;
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
    //SetCapture(window_handle);

    return;
}

static float cam_rot_y;
static vec3_t _player_pos;
static vec3_t _player_rot;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow) 
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LOG(L"Hello mother fucking world!", 134);

    int screenWidth, screenHeight;

    // Initialize the width and height of the screen to zero before sending the variables into the function.
    screenWidth = 0;
    screenHeight = 0;

    initialize_window(screenWidth, screenHeight);
    renderer_init(screenWidth, screenHeight, window_handle);

    //
    // Main loop
    //
    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));

    _player_pos.x = 7.5f;
    _player_pos.z = 0.5f;

    // Loop until there is a quit message from the window or the user.
    while (1) {
        // Handle the windows messages.
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
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

    renderer_shutdown();

    return 0;
}

#define __M_PI   3.14159265358979323846264338327950288
#define degToRad(angleInDegrees) ((angleInDegrees) * __M_PI / 180.0)
#define radToDeg(angleInRadians) ((angleInRadians) * 180.0 / __M_PI)

//static unsigned char cells[16 * 16 * 64];

static unsigned char cells[16 * 16 + 1] = {
    "xxxxxxx xxxxxxxx"
    "x              x"
    "x x xxxxxxxxxx x"
    "x x x        x x"
    "x x x xxxxxx x x"
    "x x x    x   x x"
    "x x xxxx x xxx x"
    "x x      x x x x"
    "x xxx xxxx x x x"
    "x xxx xxxx   x x"
    "x x    xxxxx x x"
    "x x    xxxxx x x"
    "x x    x     x x"
    "x xxxxxx xxxxx x"
    "x              x"
    "xxxxxxxx xxxxxxx"
};

bool is_wall(int x, int y)
{
    if (x < 0 || x >= 16) {
        return true;
    }
    if (y < 0 || y >= 16) {
        return true;
    }
    return cells[x + y * 16] != ' ';
}

void draw_frame()
{
    input_tick();
    _player_rot.y += get_mouse_delta().x * 0.1f;
    _player_rot.x += get_mouse_delta().y * 0.1f;
    if (_player_rot.x > 45.f) _player_rot.x = 45.f;
    if (_player_rot.x < -45.f) _player_rot.x = -45.f;

    Vec2i wasd = get_wasd();
    vec2_t movement = vec2(0.02f * wasd.x, 0.02f * wasd.y);
    // TODO: normalize movement before multiplication, so diagonal movement isn't faster.

    vec3_t old_pos = _player_pos;
  
    Vec2i cell_coord;
    cell_coord.x = (int) floorf(_player_pos.x);
    cell_coord.y = (int) floorf(_player_pos.z);
    
    float thick = 0.1f;
    float min_x = is_wall(cell_coord.x - 1, cell_coord.y) ? cell_coord.x + thick : -100000;
    float max_x = is_wall(cell_coord.x + 1, cell_coord.y) ? cell_coord.x + 1 - thick : +100000;
    float min_y = is_wall(cell_coord.x, cell_coord.y - 1) ? cell_coord.y + thick : -100000;
    float max_y = is_wall(cell_coord.x, cell_coord.y + 1) ? cell_coord.y + 1 - thick : +100000;
    //LOG(L"min_x: %f", min_x);

    _player_pos.x += movement.x * cosf((float) degToRad(_player_rot.y)) + movement.y * sinf((float) degToRad(_player_rot.y));
    _player_pos.z += movement.y * cosf((float) degToRad(_player_rot.y)) + movement.x * -sinf((float) degToRad(_player_rot.y));
    
    _player_pos.x = max(min_x, _player_pos.x);
    _player_pos.x = min(max_x, _player_pos.x);

    _player_pos.z = max(min_y, _player_pos.z);
    _player_pos.z = min(max_y, _player_pos.z);

    /*if (is_wall(cell_coord.x, cell_coord.y)) {
        _player_pos = old_pos;
    }*/

    Transforms cam_trans;
    cam_trans.pos = _player_pos + vec3(0, 0.7f, 0);
    cam_trans.rot = _player_rot;

    //LOG(L"_player_pos (%f, %f)", _player_pos.x, _player_pos.z);

    renderer_frame_begin(cam_trans);
    renderer_frame_end();
    //log(L"Frame", 0);
}
