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

#include <array>

namespace {
	constexpr int WINDOW_MIN_WIDTH = 1024;
	constexpr int WINDOW_MIN_HEIGHT = 360;
	constexpr float TITLE_BAR_HEIGHT = 34.0f;
	constexpr float TITLE_BAR_BUTTON_WIDTH = 46.0f;
	constexpr float RESIZE_BORDER = 7.0f;

	std::array<ImVec4, 6> gTeam_Colors = {
		ImVec4(0.8f, 0.1f, 0.1f, 1.0f), // Team 0: Red
		ImVec4(0.1f, 0.8f, 0.1f, 1.0f), // Team 1: Green
		ImVec4(0.1f, 0.1f, 0.8f, 1.0f), // Team 2: Blue
		ImVec4(0.8f, 0.8f, 0.1f, 1.0f), // Team 3: Yellow
		ImVec4(0.8f, 0.1f, 0.8f, 1.0f), // Team 4: Magenta
		ImVec4(0.1f, 0.8f, 0.8f, 1.0f), // Team 5: Cyan
	};

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
	io.Fonts->AddFontFromMemoryCompressedTTF(cousine_compressed_data, cousine_compressed_size, 12.0f);

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
		Draw_Configuration();
	}
	else {
		Draw_Main_Content();
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

void CStudio_GUI::Draw_Configuration()
{
	const auto xsize = ImGui::GetContentRegionAvail().x;
	const auto ysize = ImGui::GetContentRegionAvail().y;

	constexpr float config_margin = 24.0f;
	constexpr float max_config_width = 1024.0f;
	constexpr float max_config_height = 550.0f;

	const float config_width = std::max(0.0f, std::min(max_config_width, xsize - config_margin * 2.0f));
	const float config_height = std::max(0.0f, std::min(max_config_height, ysize - config_margin * 2.0f));

	ImGui::SetCursorPosX(std::max(config_margin, (xsize - config_width) * 0.5f));
	ImGui::SetCursorPosY(std::max(config_margin, (ysize - config_height) * 0.5f));

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.20f, 1.00f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 24.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 10.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 6.0f));

	if (ImGui::BeginChild("Configuration", ImVec2(config_width, config_height), false, ImGuiWindowFlags_NoScrollbar)) {
		const ImVec2 start_text_size = ImGui::CalcTextSize("Start");
		const ImVec2 start_button_size(
			start_text_size.x + ImGui::GetStyle().FramePadding.x * 2.0f + 40.0f,
			start_text_size.y + ImGui::GetStyle().FramePadding.y * 2.0f + 10.0f);

		const float footer_height = start_button_size.y + ImGui::GetStyle().ItemSpacing.y;

		if (ImGui::BeginChild("ConfigurationModules", ImVec2(0.0f, -footer_height), false, ImGuiWindowFlags_NoScrollbar)) {
			if (ImGui::BeginTable("Config Table", 2, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_PadOuterX)) {
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				Draw_Module_Configuration(
					"Sensing module",
					"##SensingModule",
					"##SENSColumns",
					"##SENS_",
					mSensing_Plugins,
					mSelected_Sensing_Plugin_Index,
					mSensing_Module_Parameters);

				ImGui::TableSetColumnIndex(1);
				Draw_Module_Configuration(
					"Control module",
					"##ControlModule",
					"##CTRLColumns",
					"##CTRL_",
					mControl_Plugins,
					mSelected_Control_Plugin_Index,
					mControl_Module_Parameters);

				ImGui::EndTable();
			}
		}
		ImGui::EndChild();

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - start_button_size.x);
		if (ImGui::Button("Start", start_button_size)) {

			Store_Plugin_Defaults();

			mSensing_Plugin_Instance = std::dynamic_pointer_cast<ISensing_Plugin>(
				gPlugin_Registry().Create_Plugin(NPlugin_Type::Sensing, mSensing_Plugins[mSelected_Sensing_Plugin_Index].id));

			mControl_Plugin_Instance = std::dynamic_pointer_cast<IControl_Plugin>(
				gPlugin_Registry().Create_Plugin(NPlugin_Type::Control, mControl_Plugins[mSelected_Control_Plugin_Index].id));

			if (mSensing_Plugin_Instance && mControl_Plugin_Instance) {

				auto convertParameterNames = [](const std::vector<TPlugin_Descriptor_Internal::TParameter>& parameters) {
					std::vector<const char*> names;
					names.reserve(parameters.size());
					for (const auto& param : parameters) {
						names.push_back(param.name.c_str());
					}
					return names;
				};

				auto convertParameterValues = [](const std::vector<std::string>& values) {
					std::vector<const char*> c_values;
					c_values.reserve(values.size());
					for (const auto& value : values) {
						c_values.push_back(value.c_str());
					}
					return c_values;
				};

				auto sensing_parameter_names = convertParameterNames(mSensing_Plugins[mSelected_Sensing_Plugin_Index].parameters);
				auto control_parameter_names = convertParameterNames(mControl_Plugins[mSelected_Control_Plugin_Index].parameters);

				auto sensing_parameter_values = convertParameterValues(mSensing_Module_Parameters);
				auto control_parameter_values = convertParameterValues(mControl_Module_Parameters);

				mSensing_Plugin_Instance->Initialize(
					sensing_parameter_names.data(),
					sensing_parameter_values.data(),
					static_cast<uint16_t>(mSensing_Plugins[mSelected_Sensing_Plugin_Index].parameters.size()));
				mControl_Plugin_Instance->Initialize(
					control_parameter_names.data(),
					control_parameter_values.data(),
					static_cast<uint16_t>(mControl_Plugins[mSelected_Control_Plugin_Index].parameters.size()));

				uint16_t* cnts = nullptr;
				uint16_t cnt_of_cnts = 0;
				mSensing_Plugin_Instance->Get_Team_Sizes(&cnts, &cnt_of_cnts);

				mTeam_Sizes.resize(cnt_of_cnts);
				for (uint16_t i = 0; i < cnt_of_cnts; ++i) {
					mTeam_Sizes[i] = cnts[i];
				}
				if (cnts) {
					delete[] cnts;
				}

				mLast_Update_Time = TClock::now();
			}

			mIs_Configured = true;
		}
	}
	ImGui::EndChild();

	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor();
}

void CStudio_GUI::Draw_Module_Configuration(
	const std::string& title,
	const std::string& combo_id,
	const std::string& details_table_id,
	const std::string& input_prefix,
	const std::vector<TPlugin_Descriptor_Internal>& plugins,
	int& selected_plugin_index,
	std::vector<std::string>& parameter_values)
{
	ImGui::BeginGroup();
	ImGui::Text("%s", title.c_str());

	const char* selected_name = selected_plugin_index == -1 ? "Select..." : plugins[selected_plugin_index].name.c_str();
	if (ImGui::BeginCombo(combo_id.c_str(), selected_name)) {
		for (size_t i = 0; i < plugins.size(); ++i) {
			const auto& plugin = plugins[i];
			const bool selected = selected_plugin_index == static_cast<int>(i);
			if (ImGui::Selectable(plugin.name.c_str(), selected)) {
				if (!selected) {
					selected_plugin_index = static_cast<int>(i);
					parameter_values.clear();
					parameter_values.resize(plugin.parameters.size());
				}
			}

			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	if (selected_plugin_index >= 0) {
		const auto& selected_plugin = plugins[selected_plugin_index];

		if (ImGui::BeginTable(details_table_id.c_str(), 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX)) {
			ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("ID:");
			ImGui::TableSetColumnIndex(1);
			ImGui::TextWrapped("%s", selected_plugin.id.c_str());

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Name:");
			ImGui::TableSetColumnIndex(1);
			ImGui::TextWrapped("%s", selected_plugin.name.c_str());

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Description:");
			ImGui::TableSetColumnIndex(1);
			ImGui::TextWrapped("%s", selected_plugin.description.c_str());

			for (size_t i = 0; i < selected_plugin.parameters.size(); ++i) {
				const auto& param = selected_plugin.parameters[i];

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextWrapped("%s:", param.description.c_str());
				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputText((std::string(input_prefix) + param.name).c_str(), &parameter_values[i]);
			}

			ImGui::PopFont();
			ImGui::EndTable();
		}
	}

	ImGui::EndGroup();
}

void CStudio_GUI::Draw_Main_Content()
{
	if (TClock::now() - mLast_Update_Time >= std::chrono::milliseconds(10)) {
		mLast_Update_Time = TClock::now();
		if (mSensing_Plugin_Instance) {

			if (mArena_Width == 0 || mArena_Height == 0) {
				mSensing_Plugin_Instance->Get_Arena_Size(&mArena_Width, &mArena_Height);
			}

			TRobot_Position* pos;
			uint16_t pcnt;

			mSensing_Plugin_Instance->Get_Robot_Positions(&pos, &pcnt, Team_Idx_Any);

			mRobot_Positions.assign(pos, pos + pcnt);

			delete[] pos;
		}
	}

	if (ImGui::BeginChild("MainContent", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_MenuBar)) {
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Connect...", "Ctrl+N")) {
					//
				}
				if (ImGui::MenuItem("Close", "Ctrl+Q")) {
					glfwSetWindowShouldClose(mWindow, GLFW_TRUE);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		if (ImGui::BeginTable("Table", 2)) {
			ImGui::TableSetupColumn("Connection", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Field", ImGuiTableColumnFlags_WidthStretch, 10.0f);

			ImGui::TableNextColumn();
			ImGui::Text("Connection status");

			ImGui::TableNextColumn();
			ImGui::Text("Field view");

			ImGui::EndTable();
		}

		Draw_Scaled_TopDown_Arena(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 50), ImVec2(600, 600));
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 600);

		if (ImGui::Button("Test")) {
			mControl_Plugin_Instance->Set_Robot_Speed(0, 0, 0.05f, 0.95f);
		}
	}
	ImGui::EndChild();
}

void CStudio_GUI::Draw_Scaled_TopDown_Arena(ImVec2 cursorPos, ImVec2 size) {

	constexpr float Display_Margin = 10.0f;

	ImVec2 adjustedSize = ImVec2(size.x - 2 * Display_Margin, size.y - 2 * Display_Margin);

	ImGui::GetWindowDrawList()->AddRectFilled(cursorPos, ImVec2(cursorPos.x + size.x, cursorPos.y + size.y), IM_COL32(0, 0, 32, 255));

	float factorX = adjustedSize.x / static_cast<float>(mArena_Width);
	float factorY = adjustedSize.y / static_cast<float>(mArena_Height);

	// preserve aspect ratio of the arena
	factorX = factorY = std::min(factorX, factorY);

	ImVec2 arenaOffset(cursorPos.x + Display_Margin + (adjustedSize.x - mArena_Width * factorX) / 2.0f, cursorPos.y + Display_Margin + (adjustedSize.y - mArena_Height * factorY) / 2.0f);

	// arena outline based on mArena_Width and mArena_Height
	ImGui::GetWindowDrawList()->AddRect(arenaOffset, ImVec2(arenaOffset.x + mArena_Width * factorX, arenaOffset.y + mArena_Height * factorY), IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);

	for (const auto& pos : mRobot_Positions) {
		ImVec2 robotPos(arenaOffset.x + pos.x * factorX, arenaOffset.y + pos.y * factorY);

		ImVec4 teamColor = gTeam_Colors[pos.team_idx % gTeam_Colors.size()];

		float half_size = 5.0f * factorX;
		std::vector<ImVec2> corners = {
			ImVec2(-half_size, -half_size),
			ImVec2(half_size, -half_size),
			ImVec2(half_size, half_size),
			ImVec2(-half_size, half_size)
		};

		// transform the points according to the robot's orientation
		float cos_theta = std::cos(pos.orientation);
		float sin_theta = std::sin(pos.orientation);
		for (auto& corner : corners) {
			corner = ImVec2(corner.x * cos_theta - corner.y * sin_theta, corner.x * sin_theta + corner.y * cos_theta);
		}

		// translate the points to the robot's position
		for (auto& corner : corners) {
			corner.x += robotPos.x;
			corner.y += robotPos.y;
		}

		// fill the rectangle
		ImGui::GetWindowDrawList()->AddConvexPolyFilled(corners.data(), static_cast<int>(corners.size()), IM_COL32(static_cast<int>(teamColor.x * 255), static_cast<int>(teamColor.y * 255), static_cast<int>(teamColor.z * 255), static_cast<int>(teamColor.w * 255)));

		// draw a line from center to perimeter to indicate orientation
		ImVec2 orientation_end = ImVec2(robotPos.x + 10.0f * cos(pos.orientation), robotPos.y + 10.0f * sin(pos.orientation));
		ImGui::GetWindowDrawList()->AddLine(robotPos, orientation_end, IM_COL32(0, 255, 0, 255), 2.0f);
	}
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
