#include <assert.h>

static int last_mouse_pos_x;
static int last_mouse_pos_y;
static int mouse_pos_x;
static int mouse_pos_y;
static float mouse_delta_x;
static float mouse_delta_y;

#define KEY_A 0x41
#define KEY_COUNT (0xFE + 1)
static bool keys[KEY_COUNT];
//static bool keys_last[];

static bool cursor_locked = false;
static bool mouse_delta_is_fresh = false;

static void lock_cursor()
{
    /* while the mouse is locked, make the mouse cursor invisible and
           confine the mouse movement to a small rectangle inside our window
           (so that we dont miss any mouse up events)
        */
    POINT pos;
    BOOL res = GetCursorPos(&pos);
    assert(res);
    (void*)res;
    LONG locked_x = pos.x;
    LONG locked_y = pos.y;

    RECT client_rect = {
        locked_x,
        locked_y,
        locked_x,
        locked_y
    };

    ShowCursor(false); // does not seem to work :(

    ClipCursor(&client_rect);

    /* enable raw input for mouse, starts sending WM_INPUT messages to WinProc (see GLFW) */
    const RAWINPUTDEVICE rid = {
        0x01,   // usUsagePage: HID_USAGE_PAGE_GENERIC
        0x02,   // usUsage: HID_USAGE_GENERIC_MOUSE
        0,      // dwFlags
        window_handle    // hwndTarget
    };

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        LOG(L"RegisterRawInputDevices() failed (on mouse lock).\n");
        exit(1);
    }
}

static bool input_recieve_event (HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch (umsg) {
        case WM_MOUSEMOVE: {
            mouse_pos_x = GET_X_LPARAM(lparam);
            mouse_pos_y = GET_Y_LPARAM(lparam);
            return true;
        }
        case WM_LBUTTONDOWN: {
            lock_cursor();
        }
        case WM_KEYDOWN: {
            if (wparam >= 0 && wparam < KEY_COUNT) {
                if (wparam == VK_ESCAPE) {
                    exit(0);
                }
                keys[wparam] = true;
                return true;
            }
        }
        case WM_KEYUP: {
            if (wparam >= 0 && wparam < KEY_COUNT) {
                keys[wparam] = false;
                return true;
            }
        }
        case WM_INPUT: {
            // NOTE(seb): raw mouse input during mouse-lock, taken from sokol_app.h
            HRAWINPUT ri = (HRAWINPUT) lparam;
            uint8_t raw_input_data[sizeof(RAWINPUT)];
            UINT size = sizeof(raw_input_data);
            // see: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getrawinputdata
            if ((UINT)-1 == GetRawInputData(ri, RID_INPUT, &raw_input_data, &size, sizeof(RAWINPUTHEADER))) {
                LOG(L"GetRawInputData() failed\n");
                exit(1);
                break;
            }
            const RAWINPUT* raw_mouse_data = (const RAWINPUT*) &raw_input_data;
            if ((raw_mouse_data->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == false) { // We expect movement deltas, see sokol_app for the absolute case.
                /* mouse reports movement delta (this seems to be the common case) */
                mouse_delta_x = (float)raw_mouse_data->data.mouse.lLastX;
                mouse_delta_y = (float)raw_mouse_data->data.mouse.lLastY;
                mouse_delta_is_fresh = true;
                return true;
            }
            else {
                //LOG(L"damn");
            }
        }
    }
    return false;
}

void input_tick()
{
    if (last_mouse_pos_x != 0 && last_mouse_pos_y != 0) {
        //mouse_delta_x = mouse_pos_x - last_mouse_pos_x;
        //mouse_delta_y = mouse_pos_y - last_mouse_pos_y;
        //LOG(L"(%i - %i = %f; %f)", mouse_pos_x, last_mouse_pos_x, mouse_delta_x, mouse_delta_y);
    }

    //if (GetActiveWindow() == window_handle) {
    //    RECT rect;
    //    int res = GetWindowRect(window_handle, &rect);
    //    assert(res != 0);
    //    mouse_pos_x = 800;//(rect.left + rect.right) / 2;
    //    mouse_pos_y = 500;//(rect.top + rect.bottom) / 2;
    //    //ShowCursor(false);
    //    res = SetCursorPos(mouse_pos_x, mouse_pos_y);
    //    assert(res != 0);
    //}

    last_mouse_pos_x = mouse_pos_x;
    last_mouse_pos_y = mouse_pos_y;

    if (!mouse_delta_is_fresh) {
        mouse_delta_x = 0;
        mouse_delta_y = 0;
    }
    mouse_delta_is_fresh = false;
}

struct Vec2i {
    int x;
    int y;
};

Vec2i get_mouse_delta()
{
    Vec2i ret;
    ret.x = mouse_delta_x;
    ret.y = mouse_delta_y;
    return ret;
}

Vec2i get_wasd()
{
    // https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
    Vec2i ret;
    ret.x = 0;
    ret.y = 0;
    ret.x -= keys[KEY_A + ('A' - 'A')];
    ret.x += keys[KEY_A + ('D' - 'A')];
    ret.y -= keys[KEY_A + ('S' - 'A')];
    ret.y += keys[KEY_A + ('W' - 'A')];
    return ret;
}
