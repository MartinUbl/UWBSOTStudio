#include "GUI.h"

#include <GLFW/glfw3.h>
#include <nfd.hpp>

#include <uwbsot/Utils.h>

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>

namespace {
	ImU32 Team_Row_Color(uint16_t team_idx)
	{
		const ImVec4 team_color = gTeam_Colors[team_idx % gTeam_Colors.size()];
		return IM_COL32(
			static_cast<int>(team_color.x * 255.0f),
			static_cast<int>(team_color.y * 255.0f),
			static_cast<int>(team_color.z * 255.0f),
			58);
	}

	const char* Log_Severity_Label(NLog_Severity severity)
	{
		switch (severity) {
			case NLog_Severity::Info:
				return "INFO";
			case NLog_Severity::Warning:
				return "WARN";
			case NLog_Severity::Error:
				return "ERROR";
			default:
				return "INFO";
		}
	}

	ImU32 Log_Severity_Color(NLog_Severity severity)
	{
		switch (severity) {
			case NLog_Severity::Info:
				return IM_COL32(180, 210, 255, 255);
			case NLog_Severity::Warning:
				return IM_COL32(255, 205, 92, 255);
			case NLog_Severity::Error:
				return IM_COL32(255, 120, 120, 255);
			default:
				return IM_COL32(220, 220, 220, 255);
		}
	}
}

void CStudio_GUI::Frame_Dashboard()
{
	if (TClock::now() - mLast_Update_Time >= std::chrono::milliseconds(10)) {
		mLast_Update_Time = TClock::now();
		if (mSensing_Plugin_Instance) {

			if (mArena_Width == 0 || mArena_Height == 0 || mGoal_Width == 0) {
				mSensing_Plugin_Instance->Get_Arena_Size(&mArena_Width, &mArena_Height, &mGoal_Width);
			}

			TRobot_Position* pos;
			uint16_t pcnt;

			mSensing_Plugin_Instance->Get_Robot_Positions(&pos, &pcnt, Team_Idx_Any);

			mRobot_Positions.assign(pos, pos + pcnt);
			if (mRobot_Cached_Info.size() != mRobot_Positions.size()) {
				mRobot_Cached_Info.resize(mRobot_Positions.size());
			}

			delete[] pos;

			mSensing_Plugin_Instance->Get_Ball_Position(&mBall.state, &mBall.x, &mBall.y, &mBall.radius);

			int camW = mCamera_Texture.width, camH = mCamera_Texture.height;

			if (mCamera_Texture.width == 0 || mCamera_Texture.height == 0 || mCamera_Buffer == nullptr) {
				// Initialize camera texture dimensions
				auto res = mSensing_Plugin_Instance->Get_Camera_Image(nullptr, &camW, &camH);
				if (res == NLib_Result::Success) {
					mCamera_Buffer = new uint8_t[camW * camH * 4]; // Assuming RGBA
				}
			}

			if (mCamera_Buffer != nullptr) {
				auto res = mSensing_Plugin_Instance->Get_Camera_Image(mCamera_Buffer, &camW, &camH);
				if (res == NLib_Result::Success) {
					Update_Camera_Texture(mCamera_Buffer, camW, camH);
				}
			}
		}

		if (mPython_Runtime) {
			try {
				mPython_Runtime->Update_All();
			}
			catch (std::exception& e) {
				std::cerr << "Error updating Python scripts: " << e.what() << std::endl;
				Log_Message(NLog_Severity::Error, "Python", e.what());
			}
		}
	}

	if (ImGui::BeginChild("MainContent", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_None)) {

		const auto windowWidth = ImGui::GetContentRegionAvail().x;
		const auto windowHeight = ImGui::GetContentRegionAvail().y;
		const auto sectionWidth = windowWidth / 3.0f;

		auto baseX = ImGui::GetCursorPosX();
		auto curPosY = ImGui::GetCursorPosY() + 8;

		ImVec2 rowMin = ImGui::GetCursorScreenPos();
		float rowWidth = ImGui::GetContentRegionAvail().x + 8;
		float rowHeight = ImGui::GetTextLineHeight() + 8 * 2.0f;

		ImGui::GetWindowDrawList()->AddRectFilled(
			ImVec2(rowMin.x - 8, rowMin.y - 6),
			ImVec2(rowMin.x + rowWidth, rowMin.y + rowHeight),
			Header_BG_Color
		);

		const float sectionHeight = windowHeight * 0.45f;

		ImGui::SetCursorPosX(baseX + 8);
		ImGui::SetCursorPosY(curPosY);

		ImGui::PushStyleColor(ImGuiCol_Text, Highlight_Color);  ImGui::Text("Top-Down view"); ImGui::PopStyleColor();
		Draw_Scaled_TopDown_Arena(ImVec2(baseX, curPosY + 58), ImVec2(sectionWidth, sectionHeight));
		ImGui::SetCursorPosX(baseX + sectionWidth + 8);
		ImGui::SetCursorPosY(curPosY);

		ImGui::PushStyleColor(ImGuiCol_Text, Highlight_Color);  ImGui::Text("3D view"); ImGui::PopStyleColor();
		Draw_3D_Arena(ImVec2(baseX + sectionWidth, curPosY + 58), ImVec2(sectionWidth, sectionHeight));
		ImGui::SetCursorPosX(baseX + 2 * sectionWidth + 8);
		ImGui::SetCursorPosY(curPosY);

		ImGui::PushStyleColor(ImGuiCol_Text, Highlight_Color);  ImGui::Text("Camera view"); ImGui::PopStyleColor();
		Draw_Camera_View(ImVec2(baseX + 2 * sectionWidth, curPosY + 58), ImVec2(sectionWidth, sectionHeight));
		ImGui::SetCursorPosX(baseX);
		ImGui::SetCursorPosY(curPosY + sectionHeight + 24);

		const float lowerAreaWidth = ImGui::GetContentRegionAvail().x;
		const float lowerAreaHeight = ImGui::GetContentRegionAvail().y;
		const float paneWidth = lowerAreaWidth / 3.0f;

		auto draw_speed_gauge = [](const char* label, float value) {
			const float clamped_value = std::clamp(value, -1.0f, 1.0f);
			const ImVec2 start = ImGui::GetCursorScreenPos();
			const float width = std::max(60.0f, ImGui::GetContentRegionAvail().x);
			const float height = ImGui::GetTextLineHeight() + 6.0f;
			const ImVec2 end(start.x + width, start.y + height);
			const float center_x = start.x + width * 0.5f;
			const float fill_extent = (width * 0.5f - 3.0f) * std::abs(clamped_value);
			const ImU32 fill_color = clamped_value >= 0.0f ? IM_COL32(73, 178, 112, 230) : IM_COL32(220, 82, 82, 230);

			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			draw_list->AddRectFilled(start, end, IM_COL32(18, 22, 30, 255), 3.0f);
			if (clamped_value >= 0.0f) {
				draw_list->AddRectFilled(ImVec2(center_x, start.y + 2.0f), ImVec2(center_x + fill_extent, end.y - 2.0f), fill_color, 2.0f);
			}
			else {
				draw_list->AddRectFilled(ImVec2(center_x - fill_extent, start.y + 2.0f), ImVec2(center_x, end.y - 2.0f), fill_color, 2.0f);
			}
			draw_list->AddLine(ImVec2(center_x, start.y + 1.0f), ImVec2(center_x, end.y - 1.0f), IM_COL32(235, 235, 235, 190), 1.0f);
			draw_list->AddRect(start, end, IM_COL32(255, 255, 255, 70), 3.0f);

			char value_text[32];
			snprintf(value_text, sizeof(value_text), "%.2f", clamped_value);
			const ImVec2 text_size = ImGui::CalcTextSize(value_text);
			draw_list->AddText(
				ImVec2(start.x + (width - text_size.x) * 0.5f, start.y + (height - text_size.y) * 0.5f),
				IM_COL32(245, 245, 245, 255),
				value_text);

			ImGui::InvisibleButton(label, ImVec2(width, height));
		};

		ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 128, 180)));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y));

		if (ImGui::BeginChild("ScriptWorkspacePane", ImVec2(paneWidth, lowerAreaHeight), true)) {
			ImGui::TextUnformatted("Script workspace");
			ImGui::Separator();

			if (ImGui::Button("Choose folder", ImVec2(-1.0f, 0.0f))) {
				NFD::Guard nfdGuard;
				NFD::UniquePath outPath;

				nfdresult_t result = NFD::PickFolder(outPath);
				if (result == NFD_OKAY) {
					mScript_Directory = outPath.get();
					Store_Script_Dir();

					Refresh_Script_List(outPath.get());
					Log_Message(NLog_Severity::Info, "Dashboard", "Script directory selected: " + mScript_Directory.string());
				}
			}

			if (!mScript_Directory.empty()) {
				ImGui::TextWrapped("%s", mScript_Directory.string().c_str());
			}
			else {
				ImGui::TextDisabled("No script folder selected");
			}

			ImGui::Separator();

			ImGui::Spacing();

			std::vector<size_t> controller_indices;
			for (size_t i = 0; i < mScript_List.size(); ++i) {
				if (mScript_List[i].type == NScript_Type::Controller) {
					controller_indices.push_back(i);
				}
			}

			std::vector<uint16_t> team_indices;
			for (const auto& robot : mRobot_Positions) {
				if (std::find(team_indices.begin(), team_indices.end(), robot.team_idx) == team_indices.end()) {
					team_indices.push_back(robot.team_idx);
				}
			}
			if (team_indices.empty()) {
				for (size_t i = 0; i < mTeam_Sizes.size(); ++i) {
					team_indices.push_back(static_cast<uint16_t>(i));
				}
			}
			std::sort(team_indices.begin(), team_indices.end());

			static std::map<uint16_t, int> selected_controller_by_team;
			for (uint16_t team_index : team_indices) {
				int& selected_controller_index = selected_controller_by_team[team_index];
				if (selected_controller_index >= static_cast<int>(controller_indices.size())) {
					selected_controller_index = controller_indices.empty() ? -1 : 0;
				}
				if (selected_controller_index < 0 && !controller_indices.empty()) {
					selected_controller_index = 0;
				}
			}

			if (team_indices.empty()) {
				ImGui::TextDisabled("No teams present");
			}

			for (uint16_t team_index : team_indices) {
				const int selected_controller_index = selected_controller_by_team[team_index];
				const char* selected_controller_name = selected_controller_index >= 0
					? mScript_List[controller_indices[selected_controller_index]].name.c_str()
					: "No controllers found";

				const std::string combo_label = "##Team " + std::to_string(team_index);
				ImGui::Text("Team %d:", team_index);
				ImGui::SetNextItemWidth(-1.0f);
				if (ImGui::BeginCombo(combo_label.c_str(), selected_controller_name)) {
					for (size_t i = 0; i < controller_indices.size(); ++i) {
						const bool is_selected = selected_controller_by_team[team_index] == static_cast<int>(i);
						const auto& script = mScript_List[controller_indices[i]];
						if (ImGui::Selectable(script.name.c_str(), is_selected)) {
							selected_controller_by_team[team_index] = static_cast<int>(i);
						}
						if (is_selected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
			}

			const bool can_start_controller = mPython_Runtime && !controller_indices.empty() && !team_indices.empty();
			if (!can_start_controller) {
				ImGui::BeginDisabled();
			}
			if (ImGui::Button("Start match", ImVec2(-1.0f, 0.0f))) {
				mController_Start_Time = TClock::now();

				for (uint16_t team_index : team_indices) {
					const int selected_controller_index = selected_controller_by_team[team_index];
					if (selected_controller_index < 0 || selected_controller_index >= static_cast<int>(controller_indices.size())) {
						continue;
					}

					const auto& script = mScript_List[controller_indices[selected_controller_index]];
					std::cout << "Loading controller script: " << script.path << std::endl;

					auto fname = script.path.filename().stem().string();

					try {
						auto& ctl = mPython_Runtime->Create_Controller_Script(fname, team_index);
						ctl.Initialize();
						Log_Message(NLog_Severity::Info, "Python", "Loaded controller script: " + fname + " for team " + std::to_string(team_index));
					}
					catch (py::error_already_set& e) {
						std::cerr << "Error loading controller script: " << e.what() << std::endl;
						Log_Message(NLog_Severity::Error, "Python", e.what());
					}
				}
			}
			if (!can_start_controller) {
				ImGui::EndDisabled();
			}
		}
		ImGui::EndChild();

		ImGui::SameLine(0.0f, 0.0f);

		if (ImGui::BeginChild("RobotScriptsPane", ImVec2(paneWidth, lowerAreaHeight), true)) {
			ImGui::TextUnformatted("Robot scripts");

			std::vector<size_t> sorted_indices(mRobot_Positions.size());
			for (size_t i = 0; i < sorted_indices.size(); ++i) {
				sorted_indices[i] = i;
			}
			std::sort(sorted_indices.begin(), sorted_indices.end(), [&](size_t lhs, size_t rhs) {
				const TRobot_Position& left = mRobot_Positions[lhs];
				const TRobot_Position& right = mRobot_Positions[rhs];
				if (left.team_idx != right.team_idx) {
					return left.team_idx < right.team_idx;
				}
				return left.robot_idx < right.robot_idx;
			});

			if (ImGui::BeginTable("RobotScriptsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 0.0f))) {
				ImGui::TableSetupColumn("Robot (team)", ImGuiTableColumnFlags_WidthFixed, 140.0f);
				ImGui::TableSetupColumn("Script", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthFixed, 120.0f);
				ImGui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthFixed, 120.0f);
				ImGui::TableHeadersRow();

				for (size_t index : sorted_indices) {
					const auto& robot = mRobot_Positions[index];
					const auto& cached_info = mRobot_Cached_Info[index];
					ImGui::TableNextRow();
					ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, Team_Row_Color(robot.team_idx));

					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%d (%d)", robot.robot_idx, robot.team_idx);
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%s", cached_info.scriptName.c_str());
					ImGui::TableSetColumnIndex(2);
					ImGui::PushID(static_cast<int>(index));
					draw_speed_gauge("LeftSpeedGauge", cached_info.leftSpeed);
					ImGui::TableSetColumnIndex(3);
					draw_speed_gauge("RightSpeedGauge", cached_info.rightSpeed);
					ImGui::PopID();
				}
				ImGui::EndTable();
			}
		}
		ImGui::EndChild();

		ImGui::SameLine(0.0f, 0.0f);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImGui::GetStyle().WindowPadding);
		if (ImGui::BeginChild("DashboardLogPane", ImVec2(0.0f, lowerAreaHeight), true)) {
			ImGui::TextUnformatted("Log");
			ImGui::Separator();

			if (ImGui::BeginChild("DashboardLogScroll", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar)) {
				for (const auto& entry : mLog_Entries) {
					ImGui::TextDisabled("%s", entry.timestamp.c_str());
					ImGui::SameLine(0.0f, 12.0f);
					ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Log_Severity_Color(entry.severity)), "%s", Log_Severity_Label(entry.severity));
					ImGui::SameLine(0.0f, 12.0f);
					ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Highlight_Color), "%s", entry.source.c_str());
					ImGui::SameLine(0.0f, 12.0f);
					ImGui::TextWrapped("%s", entry.message.c_str());
				}
				if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
					ImGui::SetScrollHereY(1.0f);
				}
			}
			ImGui::EndChild();
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();

		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}
	ImGui::EndChild();
}

void CStudio_GUI::Refresh_Script_List(const std::filesystem::path& script_directory)
{
	mPython_Runtime = std::make_unique<CPython_Runtime>(script_directory);

	py::module_ env_module = py::module_::import("env");

	env_module.attr("control") = py::cast(
		this,
		py::return_value_policy::reference
	);

	mScript_List.clear();
	if (std::filesystem::exists(script_directory) && std::filesystem::is_directory(script_directory)) {
		for (const auto& entry : std::filesystem::directory_iterator(script_directory)) {
			if (entry.is_regular_file() && entry.path().extension() == ".py") {

				// open the file, read lines while they start with #, and if the line starts with "#NAME: ", use the rest of the line as the name, otherwise use the filename
				std::ifstream ifs(entry.path());
				std::string name = entry.path().filename().string();
				std::string id = entry.path().stem().string();
				NScript_Type type = NScript_Type::Agent;

				// Read lines starting with '#' to find a name
				std::string line;
				while (std::getline(ifs, line)) {
					if (line.empty() || line[0] != '#') {
						break; // Stop if the line doesn't start with '#'
					}
					if (line.rfind("#NAME: ", 0) == 0) { // Check if the line starts with "#NAME: "
						name = line.substr(7); // Extract the name after "#NAME: "
					}
					if (line.rfind("#ID: ", 0) == 0) { // Check if the line starts with "#ID: "
						id = line.substr(5); // Extract the id after "#ID: "
					}
					if (line.rfind("#TYPE: ", 0) == 0) { // Check if the line starts with "#TYPE: "
						std::string stype = line.substr(7); // Extract the type after "#TYPE: "
						if (stype == "agent") {
							type = NScript_Type::Agent;
						}
						else if (stype == "controller") {
							type = NScript_Type::Controller;
						}
						else {
							type = NScript_Type::None;
						}
					}
				}

				if (type == NScript_Type::None) {
					continue;
				}

				mScript_List.push_back({ id, name, type, entry.path() });
			}
		}
	}
}

void CStudio_GUI::Log_Message(NLog_Severity severity, const std::string& source_module, const std::string& message)
{
	const auto now = std::chrono::system_clock::now();
	const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
	std::tm local_time{};
#ifdef _WIN32
	localtime_s(&local_time, &now_time);
#else
	localtime_r(&now_time, &local_time);
#endif

	std::ostringstream timestamp;
	timestamp << std::put_time(&local_time, "%H:%M:%S");

	mLog_Entries.push_back({
		severity,
		timestamp.str(),
		source_module,
		message
	});

	constexpr size_t Max_Log_Entries = 1000;
	if (mLog_Entries.size() > Max_Log_Entries) {
		mLog_Entries.erase(mLog_Entries.begin(), mLog_Entries.begin() + (mLog_Entries.size() - Max_Log_Entries));
	}
}


int CStudio_GUI::Get_Robot_Count(size_t team_index) const {
	return mTeam_Sizes.size() > team_index ? mTeam_Sizes[team_index] : 0;
}

void CStudio_GUI::Set_Robot_Script(size_t team_index, size_t robot_index, const std::string& script_id)
{
	std::cout << "Setting script for robot " << robot_index << " to script ID: " << script_id << std::endl;

	if (robot_index >= mRobot_Positions.size()) {
		return;
	}

	auto it = std::find_if(mScript_List.begin(), mScript_List.end(), [&](const TScript_Entry& script) {
		return script.id == script_id;
	});

	if (it == mScript_List.end()) {
		return;
	}

	try {
		int targetIdx = -1;
		for (size_t i = 0; i < mRobot_Positions.size(); ++i) {
			if (mRobot_Positions[i].robot_idx == static_cast<int>(robot_index) && mRobot_Positions[i].team_idx == static_cast<int>(team_index)) {
				targetIdx = static_cast<int>(i);
				break;
			}
		}

		if (targetIdx == -1) {
			return;
		}

		mRobot_Cached_Info[targetIdx].scriptName = it->name;

		mPython_Runtime->Create_Agent_Script(it->path.filename().stem().string(), static_cast<uint16_t>(team_index), static_cast<uint16_t>(robot_index)).Initialize();
		Log_Message(NLog_Severity::Info, "Python", "Assigned script '" + it->name + "' to team " + std::to_string(team_index) + ", robot " + std::to_string(robot_index));
	}
	catch (py::error_already_set& e) {
		std::cerr << "Error loading script module: " << e.what() << std::endl;
		Log_Message(NLog_Severity::Error, "Python", e.what());
		return;
	}
}

long long CStudio_GUI::Get_Elapsed_Time_Milliseconds() const {
	auto now = TClock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - mController_Start_Time);
	return elapsed.count();
}

void CStudio_GUI::Set_Robot_Speed(size_t team_index, size_t robot_index, float speed_left, float speed_right) {
	if (robot_index >= mRobot_Positions.size()) {
		return;
	}

	if (mControl_Plugin_Instance) {
		mControl_Plugin_Instance->Set_Robot_Speed(static_cast<int>(robot_index), static_cast<int>(team_index), speed_left, speed_right);

		for (size_t i = 0; i < mRobot_Positions.size(); ++i) {
			if (mRobot_Positions[i].robot_idx == static_cast<int>(robot_index) && mRobot_Positions[i].team_idx == static_cast<int>(team_index)) {
				mRobot_Cached_Info[i].leftSpeed = speed_left;
				mRobot_Cached_Info[i].rightSpeed = speed_right;
				break;
			}
		}
	}
}

py::tuple CStudio_GUI::Get_Robot_Position(size_t team_index, size_t robot_index) const {

	int targetIdx = -1;
	for (size_t i = 0; i < mRobot_Positions.size(); ++i) {
		if (mRobot_Positions[i].robot_idx == static_cast<int>(robot_index) && mRobot_Positions[i].team_idx == static_cast<int>(team_index)) {
			targetIdx = static_cast<int>(i);
			break;
		}
	}

	if (targetIdx == -1) {
		return py::make_tuple(0.0, 0.0, 0.0); // Return default values if robot not found
	}

	return py::make_tuple(mRobot_Positions[targetIdx].x, mRobot_Positions[targetIdx].y, mRobot_Positions[targetIdx].orientation);
}

py::tuple CStudio_GUI::Get_Ball_Position() const {
	return py::make_tuple(mBall.x, mBall.y, mBall.radius);
}

PYBIND11_EMBEDDED_MODULE(env, m)
{
	py::class_<CStudio_GUI>(m, "env")
		.def(
			"get_robot_count",
			&CStudio_GUI::Get_Robot_Count,
			py::arg("team_index")
		)
		.def(
			"set_robot_script",
			&CStudio_GUI::Set_Robot_Script,
			py::arg("team_index"),
			py::arg("robot_index"),
			py::arg("script_id")
		)
		.def(
			"get_time_ms",
			&CStudio_GUI::Get_Elapsed_Time_Milliseconds
		)
		.def(
			"set_robot_speed",
			&CStudio_GUI::Set_Robot_Speed,
			py::arg("team_index"),
			py::arg("robot_index"),
			py::arg("speed_left"),
			py::arg("speed_right")
		)
		.def("get_robot_position",
			&CStudio_GUI::Get_Robot_Position,
			py::arg("team_index"),
			py::arg("robot_index")
		)
		.def("get_ball_position",
			&CStudio_GUI::Get_Ball_Position
		);
}
