#include "layer.h"

#include "canvas.h"

layer::layer(const canvas* canvas, const std::string& name)
{
	canvas_ = canvas;
	this->name = name;
	pixels = new unsigned char[canvas_->byte_count()];
}

void layer::clear(const color color) const
{
	for (auto i = 0; i < canvas_->byte_count(); i += 4)
	{
		if (i % canvas_->p6 == 0)
		{
			pixels[i] = color.r;
			pixels[i + 1] = color.g;
			pixels[i + 2] = color.b;
			pixels[i + 3] = color.a;
		}
		else
		{
			pixels[i] = 222;
			pixels[i + 1] = 222;
			pixels[i + 2] = 222;
			pixels[i + 3] = 255;
		}
	}
}
