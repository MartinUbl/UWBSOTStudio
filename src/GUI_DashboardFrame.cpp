#include "GUI.h"

#include <GLFW/glfw3.h>
#include <nfd.hpp>

#include <uwbsot/Utils.h>

#include <iostream>
#include <fstream>

void CStudio_GUI::Frame_Dashboard()
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
			if (mRobot_Cached_Info.size() != mRobot_Positions.size()) {
				mRobot_Cached_Info.resize(mRobot_Positions.size());
			}

			delete[] pos;

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
			catch (py::error_already_set& e) {
				std::cerr << "Error updating Python scripts: " << e.what() << std::endl;
			}
		}
	}

	if (ImGui::BeginChild("MainContent", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_None)) {

		const auto windowWidth = ImGui::GetContentRegionAvail().x;
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

		ImGui::SetCursorPosX(baseX + 8);
		ImGui::SetCursorPosY(curPosY);

		ImGui::PushStyleColor(ImGuiCol_Text, Highlight_Color);  ImGui::Text("Top-Down view"); ImGui::PopStyleColor();
		Draw_Scaled_TopDown_Arena(ImVec2(baseX, curPosY + 58), ImVec2(sectionWidth, 600));
		ImGui::SetCursorPosX(baseX + sectionWidth + 8);
		ImGui::SetCursorPosY(curPosY);

		ImGui::PushStyleColor(ImGuiCol_Text, Highlight_Color);  ImGui::Text("3D view"); ImGui::PopStyleColor();
		Draw_3D_Arena(ImVec2(baseX + sectionWidth, curPosY + 58), ImVec2(sectionWidth, 600));
		ImGui::SetCursorPosX(baseX + 2 * sectionWidth + 8);
		ImGui::SetCursorPosY(curPosY);

		ImGui::PushStyleColor(ImGuiCol_Text, Highlight_Color);  ImGui::Text("Camera view"); ImGui::PopStyleColor();
		Draw_Camera_View(ImVec2(baseX + 2 * sectionWidth, curPosY + 58), ImVec2(sectionWidth, 600));
		ImGui::SetCursorPosX(baseX);
		ImGui::SetCursorPosY(curPosY + 600 + 24);

		if (ImGui::Button("Select script directory")) {
			NFD::Guard nfdGuard;
			NFD::UniquePath outPath;

			nfdresult_t result = NFD::PickFolder(outPath);
			if (result == NFD_OKAY) {
				mScript_Directory = outPath.get();
				Store_Script_Dir();

				Refresh_Script_List(outPath.get());
			}
		}

		for (const auto& script : mScript_List) {
			if (script.type != NScript_Type::Controller) {
				continue;
			}

			if (ImGui::Button(script.name.c_str())) {

				std::cout << "Loading controller script: " << script.path << std::endl;

				auto fname = script.path.filename().stem().string();

				try {

					mController_Start_Time = TClock::now();

					auto& ctl = mPython_Runtime->Create_Controller_Script(fname, 0);

					ctl.Initialize();
				}
				catch (py::error_already_set& e) {
					std::cerr << "Error loading controller script: " << e.what() << std::endl;
				}
			}
		}

		ImGui::SetCursorPosX(baseX + 300);
		ImGui::SetCursorPosY(curPosY + 600 + 24);

		ImGui::BeginGroup();

		ImGui::BeginTable("RobotScriptsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable);
		for (size_t i = 0; i < mRobot_Positions.size(); ++i) {
			const auto& robot = mRobot_Positions[i];
			const auto& cached_info = mRobot_Cached_Info[i];
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Team %d, Robot %d", robot.team_idx, robot.robot_idx);
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%s", cached_info.scriptName.c_str());
			ImGui::TableSetColumnIndex(2);
			ImGui::ProgressBar((cached_info.leftSpeed + 1.0f) / 2.0f, ImVec2(-1.0f, 0.0f));
			ImGui::TableSetColumnIndex(3);
			ImGui::ProgressBar((cached_info.rightSpeed + 1.0f) / 2.0f, ImVec2(-1.0f, 0.0f));
		}
		ImGui::EndTable();

		ImGui::EndGroup();
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


int CStudio_GUI::Get_Robot_Count() const
{
	return static_cast<int>(mRobot_Positions.size());
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
	}
	catch (py::error_already_set& e) {
		std::cerr << "Error loading script module: " << e.what() << std::endl;
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

PYBIND11_EMBEDDED_MODULE(env, m)
{
	py::class_<CStudio_GUI>(m, "env")
		.def(
			"get_robot_count",
			&CStudio_GUI::Get_Robot_Count
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
		);
}
