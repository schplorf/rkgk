#pragma once

#include <string>
#include "color.h"

struct layer
{
	std::string name;
	unsigned char opacity = 255;
	unsigned char* pixels;

	layer(const std::string& name, const int byte_count)
	{
		this->name = name;
		pixels = new unsigned char[byte_count];
	}
	
	void clear(const color color, const int byte_count) const
	{
		for (auto i = 0; i < byte_count; i += 4)
		{
			pixels[i] = color.r;
			pixels[i + 1] = color.g;
			pixels[i + 2] = color.b;
			pixels[i + 3] = color.a;
		}
	}
};

