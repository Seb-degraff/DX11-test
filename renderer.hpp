#pragma once

bool renderer_init(int screen_width, int screen_height, HWND hwnd);
void renderer_shutdown();
void renderer_frame_begin();
void renderer_frame_end();
