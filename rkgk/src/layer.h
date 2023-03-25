#pragma once
#include <string>

#include "globals.h"

class canvas;

class layer
{
	const canvas* canvas_;
public:
	unsigned char* pixels;
	std::string name;
	unsigned char opacity = 255;

	explicit layer(const canvas* canvas, const std::string& name);

	void clear(color color) const;
};

