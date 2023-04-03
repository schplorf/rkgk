#pragma once
#include "../imgui/imgui.h"
#include <GL/glew.h>

#include "globals.h"
#include "mathstuff.h"
#include "layer.h"

class canvas
{
	int width_, height_;
	ImVec2 quad_[4];
	GLuint texture_ = 0;
	unsigned char* buffer_{};
	ImVec2 stroke_pos_;
	bool stroking_ = false;
	float prev_pressure_ = 0;
public:
	std::string name;
	float zoom = 1, angle = 0;
	ImVec2 pan;
	matrix3x2 matrix = matrix3x2();
	ImVector<layer> layers;
	int cur_layer = -1;
	float p1,p2,p3=1,p4=1,p5;
	int p6 = 1;

	canvas(int width, int height, const std::string& name);

	void opengl_initialized();
	void render(ImDrawList* drawlist) const;
	bool get_transformed_pos(ImVec2 pos, ImVec2& transformed_pos) const;
	void handle_inputs(const ImGuiIO& io, float pressure);
	void dab(const float cx, const float cy, const float pressure, const brush& brush, const color new_color);
	void set_pixel( int x, int y, color new_color);

	void invalidate_canvas_quad();
	void invalidate_texture() const;

	void add_layer();
	void remove_layer( int idx);

	void save();

	int width() const { return width_; }
	int height() const { return height_; }
	int byte_count() const { return width_ * height_ * 4; } // 4 channels  (rgba)
};

 