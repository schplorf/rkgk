#pragma once
#include "../imgui/imgui.h"

struct color
{
	unsigned char r, g, b, a;
	color(): r(0), g(0), b(0), a(0)
	{
	}

	color(const unsigned char r, const unsigned char g, const unsigned char b, const unsigned char a): r(r), g(g), b(b), a(a)
	{
	}
};


const color color_white(255,255,255,255);
const color color_black(0,0,0,255);

class canvas;
struct brush;

extern canvas cur_canvas;
extern ImVector<brush> brushes;
extern int cur_brush;
extern float* cur_color;

