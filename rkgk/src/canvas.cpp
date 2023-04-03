#include "canvas.h"

#include <string>

#include "globals.h"
#include "mathstuff.h"
#include "cmath"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "brush.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


canvas::canvas(const int width, const int height, const std::string& name)
{
	width_ = width;
	height_ = height;
	invalidate_canvas_quad();

	this->name = name;

	add_layer();
}

void canvas::render(ImDrawList* drawlist) const
{
	drawlist->AddImageQuad((void*)(intptr_t)texture_,
		quad_[0], quad_[1],
		quad_[2], quad_[3]);
}

bool canvas::get_transformed_pos(const ImVec2 pos, ImVec2& transformed_pos) const
{
	// get relative cursor position after transformations are applied
	transformed_pos = matrix.invert().transform_vector(pos);
	return transformed_pos.x >= 0 && transformed_pos.x < width_ && transformed_pos.y >= 0 && transformed_pos.y < height_;
}

void canvas::handle_inputs(const ImGuiIO& io, float pressure)
{
	if (ImGui::IsKeyPressed(ImGuiKey_MouseWheelY))
	{
		constexpr float step = .2f; // todo this will be a setting
		zoom = io.MouseWheel > 0 ? 1 + step : 1 - step; // positive delta = zoom in
		auto m = matrix3x2();
		m.scale_at(zoom, zoom, io.MousePos.x, io.MousePos.y);
		matrix = m * matrix;
		invalidate_canvas_quad();
	}

	const bool rotating = ImGui::IsKeyDown(ImGuiKey_R);
	const bool panning = ImGui::IsKeyDown(ImGuiKey_Space);
	// rotation
	if (rotating && !panning)
	{
		angle = 1 * (3.14 / 180.0);
		auto m = matrix3x2();
		m = m.rotate(angle, ImVec2(ImGui::GetWindowWidth() / 2, ImGui::GetWindowHeight() / 2));
		matrix = m * matrix;
		invalidate_canvas_quad();
		return;
	}

	// panning
	if (!rotating && panning)
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		if (io.MouseDown[0])
		{
			pan.x = io.MouseDelta.x;
			pan.y = io.MouseDelta.y;
			auto m = matrix3x2();
			m.translate(pan.x, pan.y);
			matrix = m * matrix;
			invalidate_canvas_quad();
		}
		return;
	}

	// stroke started
	if (io.MouseClicked[0])
	{
		ImVec2 transformed_pos;
		get_transformed_pos(io.MousePos, transformed_pos);
		buffer_ = new unsigned char[byte_count()];
		stroke_pos_ = transformed_pos;
		stroking_ = true;
		prev_pressure_ = pressure;
		//layers[cur_layer].clear(color_white);
		dab(stroke_pos_.x, stroke_pos_.y, pressure
			,brushes[cur_brush], color(cur_color[0] * 255, cur_color[1] * 255, cur_color[2] * 255,
				brushes[cur_brush].get_alpha(pressure)));
		invalidate_texture();
		glfwSwapInterval(0); // disable v-sync, we want many inputs as we can get so our lines aren't choppy
		return;
	}

	// stroking
	if (io.MouseDown[0] && stroking_)
	{
		ImVec2 new_pos;
		get_transformed_pos(io.MousePos, new_pos);

		const auto dab_distance = distance(stroke_pos_, new_pos);
		const auto& brush = brushes[cur_brush];
		const auto stroke_size = brush.get_size(pressure);
		const auto spacing = std::max(.5f, stroke_size * brush.spacing);
		if (dab_distance < spacing)
		{
			return;
		}

		float nx, ny, np;
		const auto df = spacing / dab_distance;
		for (auto f = df; f <= 1; f += df)
		{
			nx = (f * new_pos.x) + ((1 - f) * stroke_pos_.x);
			ny = (f * new_pos.y) + ((1 - f) * stroke_pos_.y);
			np = f * pressure + (1 - f) * prev_pressure_;
			dab(nx, ny, np, brush, color(cur_color[0] * 255, cur_color[1] * 255, cur_color[2] * 255, brush.get_alpha(np)));
		}

		stroke_pos_ = ImVec2(nx, ny);
		prev_pressure_ = (np);
		invalidate_texture();
		return;
	}

	// stroke ended
	if (io.MouseReleased[0] && stroking_)
	{
		delete[]buffer_;
		stroking_ = false;
		glfwSwapInterval(1); // reenable v-sync, waste of gpu power to have it off while we're not painting
	}
}

void canvas::dab(const float cx, const  float cy, const  float pressure, const brush& brush, const color new_color)
{
	const float size = brush.get_size(pressure);

	float r;
	// arbitrary value fudging to make small brush sizes look nice
	if (size < 2)
	{
		p3 = size / 2;
		r = 1;
	}
	else
	{
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
			const float alpha = std::max(0.0f, std::min((float)new_color.a, (aa * 255) * p3));
			const int rx = lround(x);
			const int ry = lround(y);
			set_pixel(rx, ry, color(new_color.r, new_color.g, new_color.b, alpha));
		}
	}
}

void alpha_blend(uint8_t* dst, uint8_t* src, uint8_t alpha)
{
	uint8_t inv_alpha = 255 - alpha;
	dst[0] = (src[0] * alpha + dst[0] * inv_alpha) / 255;
	dst[1] = (src[1] * alpha + dst[1] * inv_alpha) / 255;
	dst[2] = (src[2] * alpha + dst[2] * inv_alpha) / 255;
	dst[3] = (src[3] * alpha + dst[3] * inv_alpha) / 255;
}

void canvas::set_pixel(const int x, const int y, const color new_color)
{
	if (x < 0 || y < 0 || x >= width_ || y >= height_)
	{
		return;
	}

	const int bytes_per_pixel = 4;
	const int packed_pos = y * width_ + x;
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

	const int stride = ((32 * width_) + 31) / 32 * 4;
	// Pointer to the first byte of the 4-byte color data
	unsigned char* data = layers[0].pixels + (y * stride) + (x * bytes_per_pixel);

	uint8_t src[4] = { rA, gA, bA, 255 };
	uint8_t alpha = aA;
	alpha_blend(data, src, alpha);
}

void canvas::invalidate_canvas_quad()
{
	quad_[0] = matrix.transform_vector(ImVec2());
	quad_[1] = matrix.transform_vector(ImVec2(width_, 0));
	quad_[2] = matrix.transform_vector(ImVec2(width_, height_));
	quad_[3] = matrix.transform_vector(ImVec2(0, height_));
}

void canvas::opengl_initialized()
{
	// Create a OpenGL texture identifier
	glGenTextures(1, &texture_);
	glBindTexture(GL_TEXTURE_2D, texture_);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

	layers[0].clear(color_white);
	invalidate_texture();
}

void canvas::invalidate_texture() const
{
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, layers[0].pixels);
}

void canvas::add_layer()
{
	const layer layer(this, "Layer " + std::to_string(layers.size() + 1));
	layers.insert(layers.begin() + cur_layer + 1, layer);
	cur_layer++;
}

void canvas::remove_layer(const int idx)
{
	if (idx < 0 || layers.size() <= 1) return;
	delete[]layers[idx].pixels;
	layers.erase(layers.begin() + idx);
	if (cur_layer != 0)
	{
		cur_layer--;
	}
}

void canvas::save()
{
	stbi_write_bmp("img.bmp", width_, height_,4, layers[0].pixels);
}
