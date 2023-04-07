 #ifdef NDEBUG
	#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
 #else
	#pragma comment(linker, "/SUBSYSTEM:console /ENTRY:mainCRTStartup")
#endif

#include <iostream>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#define EASYTAB_IMPLEMENTATION
#include "easytab.h"

#include "canvas.h"
#include "brush.h"
#include "mathstuff.h"
#include "gui.h"
#include "portable-file-dialogs.h"

canvas cur_canvas(1024, 1024, "foobar");
ImVector<brush> brushes;
int cur_brush = 0;
float* cur_color = new float[3] {0, 0, 0};

static void glfw_error_callback(const int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main()
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit()) return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	GLFWwindow* window = glfwCreateWindow(1280, 720, "rkgk", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	if (glewInit() != GLEW_OK) return -1;

	std::cout << glGetString(GL_VERSION) << std::endl;
	cur_canvas.create_opengl_texture();

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");

	constexpr auto clearColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

	HWND hwnd = glfwGetWin32Window(window);
	printf("EasyTab initialization code: %i\n", EasyTab_Load(hwnd));

	float pressure = 0;
	float xx = 0, prevPressure = 0;
	int x = 0, y = 0;


	brushes.push_back(brush("hard round"));

	setup_gui_style(io);

	while (!glfwWindowShouldClose(window))
	{
		MSG msg;
		// https://github.com/glfw/glfw/issues/403#issuecomment-974796203
		while (PeekMessageW(&msg, hwnd, WT_PACKET, WT_MAX, PM_REMOVE))
		{
			if (EasyTab_HandleEvent(msg.hwnd, msg.message, msg.lParam, msg.wParam) == EASYTAB_OK)
			{
				prevPressure = pressure;
				pressure = EasyTab->Pressure;
				x = EasyTab->PosX;
				y = EasyTab->PosY;
			}
		}

		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (!io.WantCaptureMouse && ImGui::IsMousePosValid())
		{
			// if the mouse button is down but pressure is 0, we are likely using the mouse
			cur_canvas.handle_inputs(io, io.MouseDown[0] && pressure <= 0.0f ? 1 : pressure, color( cur_color[0]*255, cur_color[1]*255, cur_color[2]*255, 255), brushes[cur_brush]);
		}

		if (ImGui::IsKeyPressed(ImGuiKey_LeftBracket))
		{
			brushes[cur_brush].size--;
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_RightBracket))
		{
			brushes[cur_brush].size++;
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_Delete))
		{
			cur_canvas.layers[0].clear(color_white, cur_canvas.byte_count());
			cur_canvas.invalidate_opengl_texture();
		}

		ImGui::ShowDemoWindow();

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open", "CTRL+O"))
				{
					auto dialog = pfd::open_file("Select a file", ".",
						{ "Image Files", "*.png *.jpg *.jpeg *.bmp" });

					if (!dialog.result().empty())
					{
						cur_canvas.open(dialog.result()[0]);
					}
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
				if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
				ImGui::Separator();
				if (ImGui::MenuItem("Cut", "CTRL+X")) {}
				if (ImGui::MenuItem("Copy", "CTRL+C")) {}
				if (ImGui::MenuItem("Paste", "CTRL+V")) {}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		ImGui::Begin("Debug");
		ImGui::Text("mat: m11: %g, m12: %g, m21: %g, m22: %g, m31: %g, m32: %g",
			cur_canvas.matrix.m11, cur_canvas.matrix.m12, cur_canvas.matrix.m21, cur_canvas.matrix.m22, cur_canvas.matrix.m31, cur_canvas.matrix.m32);

		auto inverse = cur_canvas.matrix.invert();
		auto inverse_vec = inverse.transform_vector(io.MousePos);
		ImGui::Text("(%g, %g)", io.MousePos.x, io.MousePos.y);
		ImGui::Text("inverse (%g, %g)", inverse_vec.x, inverse_vec.y);
		ImGui::Text("x: %i, y: %i, pressure: %.2f, prevPressure: %.2f", x, y, pressure, prevPressure);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::DragFloat("p1", &cur_canvas.p1, 0.01f, -5, 5);
		ImGui::DragFloat("p2", &cur_canvas.p2, 0.01f, -5, 5);
		ImGui::DragFloat("p3", &cur_canvas.p3, 0.01f, -5, 5);
		ImGui::DragFloat("p4", &cur_canvas.p4, 0.01f, -5, 45);
		ImGui::DragFloat("p5", &cur_canvas.p5, 0.01f, -5, 5);
		ImGui::DragInt("p6", &cur_canvas.p6, 0.1f, -100, 100);


		if (ImGui::Button("Reset canvas view"))
		{
			cur_canvas.matrix = matrix3x2();
			cur_canvas.pan = ImVec2();
			cur_canvas.zoom = 1;
			cur_canvas.invalidate_render_quad();
		}
		if (ImGui::Button("Regen img"))
		{
			cur_canvas.layers[0].clear(color_white, cur_canvas.byte_count());
			cur_canvas.invalidate_opengl_texture();
		}
		if (ImGui::Button("Save"))
		{
			cur_canvas.save();
		}
		ImGui::End();


		ImGui::Begin("Layers");
		if (ImGui::Button("+"))
		{
			cur_canvas.add_layer();
		}
		ImGui::SameLine();
		if (ImGui::Button("-"))
		{
			cur_canvas.remove_layer(cur_canvas.cur_layer);
		}
		for (int i = cur_canvas.layers.size(); i-- > 0;)
		{
			auto& layer = cur_canvas.layers[i];
			if (ImGui::Selectable(layer.name.c_str(), cur_canvas.cur_layer == i))
			{
				cur_canvas.cur_layer = i;
			}
		}
		ImGui::End();

		ImGui::Begin("Brushes");
		for (int i = 0; i < brushes.size(); i++)
		{
			auto& brush = brushes[i];
			if (ImGui::Selectable(brush.name.c_str(), cur_brush == i))
			{
				cur_canvas.cur_layer = i;
			}
		}
		auto& brush = brushes[cur_brush];
		ImGui::DragFloat("Size", &brush.size, 0.1f, 0.1f, 100);
		ImGui::Checkbox("Size pressure", &brush.size_pressure);
		if (brush.size_pressure)
		{
			ImGui::SliderFloat("Min size", &brush.min_size, 0.0f, 100);
		}
		ImGui::SliderInt("Opacity", &brush.opacity, 1, 255);
		ImGui::Checkbox("Opacity pressure", &brush.opacity_pressure);
		ImGui::SliderFloat("Spacing", &brush.spacing, 0.01f, 1.0f);
		ImGui::SliderFloat("Anti-aliasing", &brush.aa, 0.1f, 1.0f);
		ImGui::End();

		ImGui::Begin("Color");
		ImGui::ColorPicker3("", cur_color);
		ImGui::End();

		const auto drawlist = ImGui::GetBackgroundDrawList();

		cur_canvas.render(drawlist);

		drawlist->AddCircle(io.MousePos, brushes[cur_brush].size * cur_canvas.matrix.m11, IM_COL32(0, 0, 0, 255));
		
		// Rendering
		ImGui::Render();

		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);

		glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w, clearColor.z * clearColor.w, clearColor.w);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	EasyTab_Unload();

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}