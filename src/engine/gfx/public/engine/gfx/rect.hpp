#pragma once

namespace ze::gfx
{

struct Rect2D
{
	int32_t x;
	int32_t y;
	uint32_t width;
	uint32_t height;

	Rect2D(const int32_t in_x = 0,
		const int32_t in_y = 0,
		const uint32_t in_width = 0,
		const uint32_t in_height = 0) : x(in_x), y(in_y), width(in_width), height(in_height) {}
};

}