static int last_mouse_pos_x;
static int last_mouse_pos_y;
static int mouse_pos_x;
static int mouse_pos_y;
static int mouse_delta_x;
static int mouse_delta_y;

#define KEY_A 0x41
#define KEY_COUNT (0xFE + 1)
static bool keys[KEY_COUNT];
//static bool keys_last[];

static bool input_recieve_event (HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch (umsg) {
        case WM_MOUSEMOVE: {
            mouse_pos_x = GET_X_LPARAM(lparam);
            mouse_pos_y = GET_Y_LPARAM(lparam);
            return true;
        }
        case WM_KEYDOWN: {
            if (wparam >= 0 && wparam < KEY_COUNT) {
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
    }
    return false;
}

void input_tick()
{
    if (last_mouse_pos_x != 0 && last_mouse_pos_y != 0) {
        mouse_delta_x = mouse_pos_x - last_mouse_pos_x;
        mouse_delta_y = mouse_pos_y - last_mouse_pos_y;
    }
    last_mouse_pos_x = mouse_pos_x;
    last_mouse_pos_y = mouse_pos_y;
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
    Vec2i ret;
    ret.x = 0;
    ret.y = 0;
    ret.x -= keys[KEY_A + ('A' - 'A')];
    ret.x += keys[KEY_A + ('D' - 'A')];
    ret.y -= keys[KEY_A + ('S' - 'A')];
    ret.y += keys[KEY_A + ('W' - 'A')];
    return ret;
}
