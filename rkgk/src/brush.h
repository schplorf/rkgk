#pragma once
#include <string>
#include "mathstuff.h"

struct brush
{
	std::string name;
	float size = 1, min_size = 0;
	int opacity = 255, min_opacity = 0;
	bool size_pressure = false, opacity_pressure = false;
	float spacing = 0.05f, aa = 0.5f;

	explicit brush(const std::string& name)
	{
		this->name = name;
	}

	float get_size(const float pressure) const
	{
		return size_pressure ? lerp(min_size, size, pressure) : size;
	}

	unsigned char get_alpha(const float pressure) const
	{
		return opacity_pressure ? (int)lerp(min_opacity, opacity, pressure) : opacity;
	}
};
