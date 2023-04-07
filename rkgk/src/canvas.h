#pragma once
#include <vector>

#include "imgui/imgui.h"
#include <GL/glew.h>

#include "brush.h"
#include "mathstuff.h"
#include "layer.h"

#include "portable-file-dialogs.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <GLFW/glfw3.h>

#include "stb_image_write.h"
#include "engine.h"

struct canvas
{
private:
	// rendering
	ImVec2 render_quad_[4];
	GLuint texture_ = 0;
	// ..
	int width_, height_;
public:
	std::string name;
	float zoom = 1, angle = 0;
	ImVec2 pan;
	matrix3x2 matrix = matrix3x2();
	ImVector<layer> layers;
	int cur_layer = -1;
	// debug
	float p1 = 0, p2 = 0, p3 = 1, p4 = 1, p5 = 0;
	int p6 = 1;

	canvas(const int width, const int height, const std::string& name)
	{
		width_ = width;
		height_ = height;
		invalidate_render_quad();

		this->name = name;

		add_layer();
		layers[0].clear(color_white, byte_count());
	}

#pragma region rendering

	void invalidate_render_quad()
	{
		render_quad_[0] = matrix.transform_vector(ImVec2());
		render_quad_[1] = matrix.transform_vector(ImVec2(width_, 0));
		render_quad_[2] = matrix.transform_vector(ImVec2(width_, height_));
		render_quad_[3] = matrix.transform_vector(ImVec2(0, height_));
	}

	void create_opengl_texture()
	{
		// Create a OpenGL texture identifier
		glGenTextures(1, &texture_);
		glBindTexture(GL_TEXTURE_2D, texture_);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		invalidate_opengl_texture();
	}

	void invalidate_opengl_texture()
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, layers[0].pixels);
	}

	void render(ImDrawList* drawlist) const
	{
		drawlist->AddImageQuad((void*)(intptr_t)texture_,
			render_quad_[0], render_quad_[1],
			render_quad_[2], render_quad_[3]);
	}

#pragma endregion rendering

	bool get_transformed_pos(const ImVec2 pos, ImVec2& transformed_pos) const
	{
		// get relative position after transformations are applied
		transformed_pos = matrix.invert().transform_vector(pos);
		return transformed_pos.x >= 0 && transformed_pos.x < width_ && transformed_pos.y >= 0 && transformed_pos.y < height_;
	}

	void handle_inputs(const ImGuiIO& io, float pressure, color color, brush& brush)
	{
		// zooming
		if (ImGui::IsKeyPressed(ImGuiKey_MouseWheelY))
		{
			constexpr float step = .2f; // todo this will be a setting
			zoom = io.MouseWheel > 0 ? 1 + step : 1 - step; // positive delta = zoom in
			auto m = matrix3x2();
			m.scale_at(zoom, zoom, io.MousePos.x, io.MousePos.y);
			matrix = m * matrix;

			if (matrix.m11 >= 2.0f)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}

			invalidate_render_quad();
			return;
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
			invalidate_render_quad();
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
				invalidate_render_quad();
			}
			return;
		}

		// stroke started
		if (io.MouseClicked[0])
		{
			start_stroke();
			ImVec2 transformed_pos;
			get_transformed_pos(io.MousePos, transformed_pos);
			buffer_ = new unsigned char[byte_count()];
			stroke_pos_ = transformed_pos;
			stroking_ = true;
			prev_pressure_ = pressure;
			//layers[cur_layer].clear(color_white);
			dab(stroke_pos_.x, stroke_pos_.y, pressure, brush, color, width_, height_, layers[0].pixels);
			invalidate_opengl_texture();
			glfwSwapInterval(0); // disable v-sync, we want many inputs as we can get so our lines aren't choppy
			return;
		}

		// stroking
		if (io.MouseDown[0] && stroking_)
		{
			ImVec2 new_pos;
			get_transformed_pos(io.MousePos, new_pos);

			const auto dab_distance = distance(stroke_pos_, new_pos);
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
				dab(nx, ny, np, brush, color, width_, height_, layers[0].pixels);
			}

			stroke_pos_ = ImVec2(nx, ny);
			prev_pressure_ = np;
			invalidate_opengl_texture();
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

	void add_layer()
	{
		const layer layer("Layer " + std::to_string(layers.size() + 1), byte_count());
		layers.insert(layers.begin() + cur_layer + 1, layer);
		cur_layer++;
	}

	void remove_layer(const int idx)
	{
		if (idx < 0 || layers.size() <= 1) return;
		delete[]layers[idx].pixels;
		layers.erase(layers.begin() + idx);
		if (cur_layer != 0)
		{
			cur_layer--;
		}
	}

#pragma region saving/loading

	void save() const
	{
		stbi_write_bmp("img.bmp", width_, height_, 4, layers[0].pixels);
	}

	void open(const std::string& path)
	{
		int image_width, image_height, channels;
		const unsigned char* image_data = stbi_load(path.c_str(), &image_width, &image_height, &channels, 4);
		if (image_data == nullptr || channels != 4)
		{
			pfd::message("Problem", "An error occurred while doing things", pfd::choice::ok, pfd::icon::error);
			return;
		}

		for (auto i = 0; i < byte_count(); i += 4)
		{
			layers[0].pixels[i] = image_data[i];
			layers[0].pixels[i + 1] = image_data[i + 1];
			layers[0].pixels[i + 2] = image_data[i + 2];
			layers[0].pixels[i + 3] = image_data[i + 3];
		}
		invalidate_opengl_texture();
	}

#pragma endregion saving/loading

	int width() const { return width_; }
	int height() const { return height_; }
	int byte_count() const { return width_ * height_ * 4; } // 4 channels (rgba)
};

