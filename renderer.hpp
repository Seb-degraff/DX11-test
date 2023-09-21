#pragma once

struct vec3_t vec3(float x, float y, float z);

struct vec3_t
{
	float x;
	float y;
	float z;

	inline vec3_t operator+(const vec3_t& other)
	{
		return vec3(x + other.x, y + other.y, z + other.z);
	}
};

inline vec3_t vec3(float x, float y, float z)
{
	vec3_t vec;
	
	vec.x = x;
	vec.y = y;
	vec.z = z;

	return vec;
}

struct Transforms
{
	vec3_t pos;
	vec3_t rot;
};

bool renderer_init(int screen_width, int screen_height, HWND hwnd);
void renderer_shutdown();
void renderer_frame_begin(Transforms cam_transforms);
void renderer_frame_end();
