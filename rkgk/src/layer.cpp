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
	for (auto i = 0; i < canvas_->byte_count(); i++)
	{
		if(i%40==0)
		{
			pixels[i] = color.r/2;
		pixels[i+1] = color.g/2;
		pixels[i+2] = color.b/2;
		pixels[i+3] = color.a;	
		}else
		{
			
		pixels[i] = color.r;
		pixels[i+1] = color.g;
		pixels[i+2] = color.b;
		pixels[i+3] = color.a;
		}
	}
}
