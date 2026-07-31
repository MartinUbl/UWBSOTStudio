#include "GUI.h"
#include "PluginRegistry.h"

#include <algorithm>
#include <cfloat>
#include <utility>
#include <iostream>
#include <fstream>

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include "cousine-font.h"

std::vector<ImVec4> gTeam_Colors = {
	ImVec4(0.8f, 0.1f, 0.1f, 1.0f), // Team 0: Red
	ImVec4(0.1f, 0.8f, 0.1f, 1.0f), // Team 1: Green
	ImVec4(0.1f, 0.1f, 0.8f, 1.0f), // Team 2: Blue
	ImVec4(0.8f, 0.8f, 0.1f, 1.0f), // Team 3: Yellow
	ImVec4(0.8f, 0.1f, 0.8f, 1.0f), // Team 4: Magenta
	ImVec4(0.1f, 0.8f, 0.8f, 1.0f), // Team 5: Cyan
};

namespace {
	constexpr int WINDOW_MIN_WIDTH = 1024;
	constexpr int WINDOW_MIN_HEIGHT = 360;
	constexpr float TITLE_BAR_HEIGHT = 34.0f;
	constexpr float TITLE_BAR_BUTTON_WIDTH = 46.0f;
	constexpr float RESIZE_BORDER = 7.0f;

	enum EResize_Edge {
		Resize_Edge_None = 0,
		Resize_Edge_Left = 1,
		Resize_Edge_Right = 2,
		Resize_Edge_Top = 4,
		Resize_Edge_Bottom = 8,
	};

	ImVec2 Get_Screen_Cursor_Pos(GLFWwindow* window)
	{
		int window_x = 0;
		int window_y = 0;
		double cursor_x = 0.0;
		double cursor_y = 0.0;
		glfwGetWindowPos(window, &window_x, &window_y);
		glfwGetCursorPos(window, &cursor_x, &cursor_y);
		return ImVec2(window_x + static_cast<float>(cursor_x), window_y + static_cast<float>(cursor_y));
	}

	bool Is_Window_Maximized(GLFWwindow* window)
	{
		return glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE;
	}

	void Toggle_Window_Maximized(GLFWwindow* window)
	{
		if (Is_Window_Maximized(window)) {
			glfwRestoreWindow(window);
		}
		else {
			glfwMaximizeWindow(window);
		}
	}

	void Update_Title_Bar_Drag(GLFWwindow* window, float title_bar_min_x, float title_bar_width)
	{
		static bool is_dragging = false;
		static ImVec2 drag_offset = ImVec2(0.0f, 0.0f);

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			Toggle_Window_Maximized(window);
			return;
		}

		if (ImGui::IsItemActivated() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			ImVec2 screen_cursor = Get_Screen_Cursor_Pos(window);
			int window_x = 0;
			int window_y = 0;
			int window_width = 0;
			int window_height = 0;
			glfwGetWindowPos(window, &window_x, &window_y);
			glfwGetWindowSize(window, &window_width, &window_height);

			if (Is_Window_Maximized(window)) {
				float cursor_ratio = title_bar_width > 0.0f ? (ImGui::GetMousePos().x - title_bar_min_x) / title_bar_width : 0.5f;
				cursor_ratio = std::clamp(cursor_ratio, 0.0f, 1.0f);

				glfwRestoreWindow(window);
				glfwGetWindowSize(window, &window_width, &window_height);
				window_x = static_cast<int>(screen_cursor.x - window_width * cursor_ratio);
				window_y = static_cast<int>(screen_cursor.y - 16.0f);
				glfwSetWindowPos(window, window_x, window_y);
			}

			drag_offset = ImVec2(screen_cursor.x - window_x, screen_cursor.y - window_y);
			is_dragging = true;
		}

		if (is_dragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			ImVec2 screen_cursor = Get_Screen_Cursor_Pos(window);
			glfwSetWindowPos(window,
				static_cast<int>(screen_cursor.x - drag_offset.x),
				static_cast<int>(screen_cursor.y - drag_offset.y));
		}

		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			is_dragging = false;
		}
	}

	int Get_Resize_Edges(const ImGuiViewport* viewport)
	{
		if (viewport == nullptr) {
			return Resize_Edge_None;
		}

		const ImVec2 mouse_pos = ImGui::GetMousePos();
		const ImVec2 min = viewport->Pos;
		const ImVec2 max = ImVec2(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y);

		if (mouse_pos.x < min.x || mouse_pos.x > max.x || mouse_pos.y < min.y || mouse_pos.y > max.y) {
			return Resize_Edge_None;
		}

		int edges = Resize_Edge_None;
		if (mouse_pos.x <= min.x + RESIZE_BORDER) {
			edges |= Resize_Edge_Left;
		}
		if (mouse_pos.x >= max.x - RESIZE_BORDER) {
			edges |= Resize_Edge_Right;
		}
		if (mouse_pos.y <= min.y + RESIZE_BORDER) {
			edges |= Resize_Edge_Top;
		}
		if (mouse_pos.y >= max.y - RESIZE_BORDER) {
			edges |= Resize_Edge_Bottom;
		}
		return edges;
	}

	ImGuiMouseCursor Get_Resize_Cursor(int edges)
	{
		if ((edges & Resize_Edge_Left) != 0 && (edges & Resize_Edge_Top) != 0) {
			return ImGuiMouseCursor_ResizeNWSE;
		}
		if ((edges & Resize_Edge_Right) != 0 && (edges & Resize_Edge_Bottom) != 0) {
			return ImGuiMouseCursor_ResizeNWSE;
		}
		if ((edges & Resize_Edge_Right) != 0 && (edges & Resize_Edge_Top) != 0) {
			return ImGuiMouseCursor_ResizeNESW;
		}
		if ((edges & Resize_Edge_Left) != 0 && (edges & Resize_Edge_Bottom) != 0) {
			return ImGuiMouseCursor_ResizeNESW;
		}
		if ((edges & (Resize_Edge_Left | Resize_Edge_Right)) != 0) {
			return ImGuiMouseCursor_ResizeEW;
		}
		if ((edges & (Resize_Edge_Top | Resize_Edge_Bottom)) != 0) {
			return ImGuiMouseCursor_ResizeNS;
		}
		return ImGuiMouseCursor_Arrow;
	}

	void Update_Window_Resize(GLFWwindow* window, const ImGuiViewport* viewport)
	{
		static bool is_resizing = false;
		static int resize_edges = Resize_Edge_None;
		static ImVec2 resize_start_cursor = ImVec2(0.0f, 0.0f);
		static int resize_start_x = 0;
		static int resize_start_y = 0;
		static int resize_start_width = 0;
		static int resize_start_height = 0;

		const int hover_edges = Is_Window_Maximized(window) ? Resize_Edge_None : Get_Resize_Edges(viewport);
		if (!is_resizing && hover_edges != Resize_Edge_None) {
			ImGui::SetMouseCursor(Get_Resize_Cursor(hover_edges));
		}

		if (!is_resizing && hover_edges != Resize_Edge_None && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
			is_resizing = true;
			resize_edges = hover_edges;
			resize_start_cursor = Get_Screen_Cursor_Pos(window);
			glfwGetWindowPos(window, &resize_start_x, &resize_start_y);
			glfwGetWindowSize(window, &resize_start_width, &resize_start_height);
		}

		if (is_resizing && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			const ImVec2 cursor = Get_Screen_Cursor_Pos(window);
			const int delta_x = static_cast<int>(cursor.x - resize_start_cursor.x);
			const int delta_y = static_cast<int>(cursor.y - resize_start_cursor.y);

			int window_x = resize_start_x;
			int window_y = resize_start_y;
			int window_width = resize_start_width;
			int window_height = resize_start_height;

			if ((resize_edges & Resize_Edge_Left) != 0) {
				const int right = resize_start_x + resize_start_width;
				window_x = resize_start_x + delta_x;
				if (right - window_x < WINDOW_MIN_WIDTH) {
					window_x = right - WINDOW_MIN_WIDTH;
				}
				window_width = right - window_x;
			}
			if ((resize_edges & Resize_Edge_Right) != 0) {
				window_width = resize_start_width + delta_x;
				if (window_width < WINDOW_MIN_WIDTH) {
					window_width = WINDOW_MIN_WIDTH;
				}
			}
			if ((resize_edges & Resize_Edge_Top) != 0) {
				const int bottom = resize_start_y + resize_start_height;
				window_y = resize_start_y + delta_y;
				if (bottom - window_y < WINDOW_MIN_HEIGHT) {
					window_y = bottom - WINDOW_MIN_HEIGHT;
				}
				window_height = bottom - window_y;
			}
			if ((resize_edges & Resize_Edge_Bottom) != 0) {
				window_height = resize_start_height + delta_y;
				if (window_height < WINDOW_MIN_HEIGHT) {
					window_height = WINDOW_MIN_HEIGHT;
				}
			}

			glfwSetWindowPos(window, window_x, window_y);
			glfwSetWindowSize(window, window_width, window_height);
			ImGui::SetMouseCursor(Get_Resize_Cursor(resize_edges));
		}

		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			is_resizing = false;
			resize_edges = Resize_Edge_None;
		}
	}
}

CStudio_GUI::CStudio_GUI(
	GLFWwindow* window,
	std::vector<TPlugin_Descriptor_Internal> sensing_plugins,
	std::vector<TPlugin_Descriptor_Internal> control_plugins)
	: mWindow(window),
	  mSensing_Plugins(std::move(sensing_plugins)),
	  mControl_Plugins(std::move(control_plugins))
{
	Restore_Plugin_Defaults();
}

void CStudio_GUI::Configure_ImGui()
{
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromMemoryCompressedTTF(cousine_compressed_data, cousine_compressed_size, 16.0f);
	io.Fonts->AddFontFromMemoryCompressedTTF(cousine_compressed_data, cousine_compressed_size, 14.0f);
	io.Fonts->AddFontFromMemoryCompressedTTF(cousine_compressed_data, cousine_compressed_size, 11.0f);

	ImGui::StyleColorsDark();
	ImGui::GetStyle().WindowRounding = 0.0f;
}

void CStudio_GUI::Draw()
{
#ifdef IMGUI_HAS_VIEWPORT
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
#else
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
#endif

	Update_Window_Resize(mWindow, ImGui::GetMainViewport());

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("UWB SOT Studio", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
	ImGui::PopStyleVar();

	Draw_Title_Bar();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
	if (!mIs_Configured) {
		Frame_Configuration();
	}
	else {
		Frame_Dashboard();
	}
	ImGui::PopStyleVar();

	ImGui::End();
}

void CStudio_GUI::Draw_Title_Bar()
{
	const float button_area_width = TITLE_BAR_BUTTON_WIDTH * 3.0f;
	const float available_width = ImGui::GetContentRegionAvail().x;
	const float drag_width = available_width > button_area_width ? available_width - button_area_width : 0.0f;

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.08f, 0.10f, 1.00f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

	if (ImGui::BeginChild("AppTitleBar", ImVec2(0.0f, TITLE_BAR_HEIGHT), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
		const ImVec2 title_bar_min = ImGui::GetCursorScreenPos();
		const ImVec2 title_pos(title_bar_min.x + 12.0f, title_bar_min.y + (TITLE_BAR_HEIGHT - ImGui::GetTextLineHeight()) * 0.5f);

		ImGui::InvisibleButton("##TitleBarDrag", ImVec2(drag_width, TITLE_BAR_HEIGHT));
		Update_Title_Bar_Drag(mWindow, ImGui::GetItemRectMin().x, drag_width);

		ImGui::GetWindowDrawList()->AddText(title_pos, ImGui::GetColorU32(ImGuiCol_Text), "UWB SOT Studio");

		ImGui::SameLine();
		if (ImGui::Button("_##Minimize", ImVec2(TITLE_BAR_BUTTON_WIDTH, TITLE_BAR_HEIGHT))) {
			glfwIconifyWindow(mWindow);
		}

		ImGui::SameLine();
		const char* maximize_label = Is_Window_Maximized(mWindow) ? "][##Restore" : "[]##Maximize";
		if (ImGui::Button(maximize_label, ImVec2(TITLE_BAR_BUTTON_WIDTH, TITLE_BAR_HEIGHT))) {
			Toggle_Window_Maximized(mWindow);
		}

		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.78f, 0.16f, 0.18f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.62f, 0.08f, 0.10f, 1.00f));
		if (ImGui::Button("x##Close", ImVec2(TITLE_BAR_BUTTON_WIDTH, TITLE_BAR_HEIGHT))) {
			glfwSetWindowShouldClose(mWindow, GLFW_TRUE);
		}
		ImGui::PopStyleColor(2);
	}

	ImGui::EndChild();
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor();
}

void CStudio_GUI::Store_Plugin_Defaults()
{
	// store currently selected plugin IDs and parameter values to a configuration file or persistent storage

	std::ofstream config_file("plugin_defaults.cfg");
	if (config_file.is_open()) {
		config_file << mSensing_Plugins[mSelected_Sensing_Plugin_Index].id << std::endl;
		config_file << mSensing_Module_Parameters.size() << std::endl;
		for (const auto& param_value : mSensing_Module_Parameters) {
			config_file << param_value << std::endl;
		}
		config_file << mControl_Plugins[mSelected_Control_Plugin_Index].id << std::endl;
		config_file << mControl_Module_Parameters.size() << std::endl;
		for (const auto& param_value : mControl_Module_Parameters) {
			config_file << param_value << std::endl;
		}
		config_file.close();
	}
}

void CStudio_GUI::Restore_Plugin_Defaults()
{
	// restore previously stored plugin IDs and parameter values from a configuration file or persistent storage
	std::ifstream config_file("plugin_defaults.cfg");
	if (config_file.is_open()) {
		std::string sensing_plugin_id;
		std::getline(config_file, sensing_plugin_id);
		for (size_t i = 0; i < mSensing_Plugins.size(); ++i) {
			if (mSensing_Plugins[i].id == sensing_plugin_id) {
				mSelected_Sensing_Plugin_Index = static_cast<int>(i);
				break;
			}
		}
		size_t sensing_param_count;
		config_file >> sensing_param_count;
		config_file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		mSensing_Module_Parameters.resize(sensing_param_count);
		for (size_t i = 0; i < sensing_param_count; ++i) {
			std::getline(config_file, mSensing_Module_Parameters[i]);
		}
		std::string control_plugin_id;
		std::getline(config_file, control_plugin_id);
		for (size_t i = 0; i < mControl_Plugins.size(); ++i) {
			if (mControl_Plugins[i].id == control_plugin_id) {
				mSelected_Control_Plugin_Index = static_cast<int>(i);
				break;
			}
		}
		size_t control_param_count;
		config_file >> control_param_count;
		config_file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		mControl_Module_Parameters.resize(control_param_count);
		for (size_t i = 0; i < control_param_count; ++i) {
			std::getline(config_file, mControl_Module_Parameters[i]);
		}
		config_file.close();
	}
}
