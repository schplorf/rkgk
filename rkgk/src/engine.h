#pragma once
#include <cstdint>

#include "color.h"
#include "imgui/imgui.h"


unsigned char* buffer_{};
ImVec2 stroke_pos_;
bool stroking_ = false;
float prev_pressure_ = 0;

void start_stroke()
{

}

void stroke_to()
{

}

void end_stroke()
{

}

void alpha_blend(uint8_t* dst, uint8_t* src, uint8_t alpha)
{
	uint8_t inv_alpha = 255 - alpha;
	dst[0] = (src[0] * alpha + dst[0] * inv_alpha) / 255;
	dst[1] = (src[1] * alpha + dst[1] * inv_alpha) / 255;
	dst[2] = (src[2] * alpha + dst[2] * inv_alpha) / 255;
	dst[3] = (src[3] * alpha + dst[3] * inv_alpha) / 255;
}

void set_pixel(const int x, const int y, const color new_color, const int width, const int height, unsigned char* pixels)
{
	if (x < 0 || y < 0 || x >= width || y >= height)
	{
		return;
	}

	const int bytes_per_pixel = 4;
	const int packed_pos = y * width + x;
	buffer_[packed_pos + 0] = new_color.r;
	buffer_[packed_pos + 1] = new_color.g;
	buffer_[packed_pos + 2] = new_color.b;
	buffer_[packed_pos + 3] = new_color.a;

	const unsigned char rA = new_color.r;
	const unsigned char gA = new_color.g;
	const unsigned char bA = new_color.b;
	const unsigned char aA = new_color.a;

	const unsigned char rB = buffer_[packed_pos + 0];
	const unsigned char gB = buffer_[packed_pos + 1];
	const unsigned char bB = buffer_[packed_pos + 2];
	const unsigned char aB = buffer_[packed_pos + 3];

	const int stride = ((32 * width) + 31) / 32 * 4;
	// Pointer to the first byte of the 4-byte color data
	unsigned char* data = pixels + (y * stride) + (x * bytes_per_pixel);

	uint8_t src[4] = { rA, gA, bA, 255 };
	uint8_t alpha = aA;
	alpha_blend(data, src, alpha);
}

void dab(float cx, float cy, const float pressure, const brush& brush, const color new_color, const int width, const int height, unsigned char* pixels)
{
	const float size = brush.get_size(pressure);

	float r, fudge;
	// arbitrary value fudging to make small brush sizes look nice
	if (size < 2)
	{
		fudge = size / 2;
		r = 1;
		// fix weird off by one issue
		cx--; cy--;
	}
	else
	{
		fudge = 1;
		r = size / 2;
	}

	const float cpx = floor(cx) + .5f;
	const float cpy = floor(cy) + .5f;
	for (float x = cpx - r; x <= cpx + r; x++)
	{
		for (float y = cpy - r; y <= cpy + r; y++)
		{
			const auto v1 = ImVec2(x, y);
			const auto v2 = ImVec2(cx, cy);
			const float dist = distance(v1, v2);
			if (dist > r) continue;
			const float aa = r - lerp(r, dist, brush.aa);
			const float alpha = std::max(0.0f, std::min((float)new_color.a, aa * fudge * 255));
			const int rx = lround(x);
			const int ry = lround(y);
			set_pixel(rx, ry, color(new_color.r, new_color.g, new_color.b, alpha), width, height, pixels);
		}
	}
}


