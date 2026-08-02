#include "GUI.h"

void CStudio_GUI::Draw_Scaled_TopDown_Arena(ImVec2 cursorPos, ImVec2 size) {

	constexpr float Display_Margin = 10.0f;

	ImVec2 adjustedSize = ImVec2(size.x - 2 * Display_Margin, size.y - 2 * Display_Margin);

	ImGui::GetWindowDrawList()->AddRectFilled(cursorPos, ImVec2(cursorPos.x + size.x, cursorPos.y + size.y), IM_COL32(0, 0, 32, 255));
	ImGui::GetWindowDrawList()->AddRect(cursorPos, ImVec2(cursorPos.x + size.x, cursorPos.y + size.y), IM_COL32(0, 0, 128, 180), 0.0f, 0, 2.0f);

	float factorX = adjustedSize.x / static_cast<float>(mArena_Width);
	float factorY = adjustedSize.y / static_cast<float>(mArena_Height);

	// preserve aspect ratio of the arena
	factorX = factorY = std::min(factorX, factorY);

	ImVec2 arenaOffset(cursorPos.x + Display_Margin + (adjustedSize.x - mArena_Width * factorX) / 2.0f, cursorPos.y + Display_Margin + (adjustedSize.y - mArena_Height * factorY) / 2.0f);

	// arena outline based on mArena_Width and mArena_Height
	ImGui::GetWindowDrawList()->AddRect(arenaOffset, ImVec2(arenaOffset.x + mArena_Width * factorX, arenaOffset.y + mArena_Height * factorY), IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);

	// draw the goals on the left and right sides of the arena (in the middle of the height)
	ImVec2 leftGoalTop(arenaOffset.x - 4, arenaOffset.y + (mArena_Height * factorY - mGoal_Width * factorY) / 2.0f);
	ImVec2 leftGoalBottom(arenaOffset.x + 2, arenaOffset.y + (mArena_Height * factorY + mGoal_Width * factorY) / 2.0f);

	ImVec2 rightGoalTop(arenaOffset.x + mArena_Width * factorX - 2, arenaOffset.y + (mArena_Height * factorY - mGoal_Width * factorY) / 2.0f);
	ImVec2 rightGoalBottom(arenaOffset.x + mArena_Width * factorX + 4, arenaOffset.y + (mArena_Height * factorY + mGoal_Width * factorY) / 2.0f);

	ImGui::GetWindowDrawList()->AddRectFilled(leftGoalTop, leftGoalBottom, IM_COL32(128, 0, 0, 255));
	ImGui::GetWindowDrawList()->AddRectFilled(rightGoalTop, rightGoalBottom, IM_COL32(128, 0, 0, 255));

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
		ImVec2 orientation_end = ImVec2(robotPos.x + 15.0f * cos(pos.orientation), robotPos.y + 15.0f * sin(pos.orientation));
		ImGui::GetWindowDrawList()->AddLine(robotPos, orientation_end, IM_COL32(0, 255, 0, 255), 2.0f);

		// draw the text background
		ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(robotPos.x - 6.0f, robotPos.y - 8.0f), ImVec2(robotPos.x + 6.0f, robotPos.y + 8.0f), IM_COL32(0, 0, 0, 128));

		// draw the robot ID as text
		ImGui::GetWindowDrawList()->AddText(ImVec2(robotPos.x - 4.0f, robotPos.y - 6.0f), IM_COL32(255, 255, 255, 255), std::to_string(pos.robot_idx).c_str());
	}

	// draw the ball
	ImVec2 ballPos(arenaOffset.x + mBall.x * factorX, arenaOffset.y + mBall.y * factorY);
	ImGui::GetWindowDrawList()->AddCircleFilled(ballPos, mBall.radius * factorX, mBall.state == NBall_State::Ground ? IM_COL32(255, 92, 0, 255) : IM_COL32(127, 127, 64, 255));
}
