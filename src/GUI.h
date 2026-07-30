#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>

#include <uwbsot/PluginInterface.h>

#include <imgui.h>

using TClock = std::chrono::steady_clock;

struct GLFWwindow;

class CStudio_GUI {
	public:
		CStudio_GUI(
			GLFWwindow* window,
			std::vector<TPlugin_Descriptor_Internal> sensing_plugins,
			std::vector<TPlugin_Descriptor_Internal> control_plugins);

		static void Configure_ImGui();

		void Draw();

	private:
		void Draw_Title_Bar();
		void Draw_Configuration();
		void Draw_Module_Configuration(
			const std::string& title,
			const std::string& combo_id,
			const std::string& details_table_id,
			const std::string& input_prefix,
			const std::vector<TPlugin_Descriptor_Internal>& plugins,
			int& selected_plugin_index,
			std::vector<std::string>& parameter_values);
		void Draw_Main_Content();

		void Store_Plugin_Defaults();
		void Restore_Plugin_Defaults();

		void Draw_Scaled_TopDown_Arena(ImVec2 cursorPos, ImVec2 size);

	private:
		GLFWwindow* mWindow = nullptr;
		bool mIs_Configured = false;

		std::vector<TPlugin_Descriptor_Internal> mSensing_Plugins;
		std::vector<TPlugin_Descriptor_Internal> mControl_Plugins;

		std::shared_ptr<ISensing_Plugin> mSensing_Plugin_Instance;
		std::shared_ptr<IControl_Plugin> mControl_Plugin_Instance;

		int mSelected_Sensing_Plugin_Index = -1;
		int mSelected_Control_Plugin_Index = -1;

		std::vector<std::string> mSensing_Module_Parameters;
		std::vector<std::string> mControl_Module_Parameters;

		std::vector<size_t> mTeam_Sizes;

		std::chrono::time_point<TClock> mLast_Update_Time;
		std::vector<TRobot_Position> mRobot_Positions;

		float mArena_Width = 0.0f;
		float mArena_Height = 0.0f;
};
