#include "GUI.h"

#include <GLFW/glfw3.h>

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
	}

	if (ImGui::BeginChild("MainContent", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_None/*ImGuiWindowFlags_MenuBar*/)) {
		/*if (ImGui::BeginMenuBar()) {
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
		}*/

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

		if (ImGui::Button("Test")) {
			mControl_Plugin_Instance->Set_Robot_Speed(0, 0, 0.05f, 0.95f);
		}
	}
	ImGui::EndChild();
}
