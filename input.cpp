
static int last_mouse_pos_x;
static int last_mouse_pos_y;
static int mouse_pos_x;
static int mouse_pos_y;
static int mouse_delta_x;
static int mouse_delta_y;

static bool input_recieve_event (HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch (umsg) {
        case WM_MOUSEMOVE: {
            mouse_pos_x = GET_X_LPARAM(lparam);
            mouse_pos_y = GET_Y_LPARAM(lparam);
            return true;
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
