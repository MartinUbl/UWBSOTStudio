#include "GUI.h"

#include "PluginRegistry.h"
#include <uwbsot/PluginInterface.h>

#include "misc/cpp/imgui_stdlib.h"

void CStudio_GUI::Frame_Configuration() {

	const auto xsize = ImGui::GetContentRegionAvail().x;
	const auto ysize = ImGui::GetContentRegionAvail().y;

	constexpr float config_margin = 24.0f;
	constexpr float max_config_width = 1400.0f;
	constexpr float max_config_height = 600.0f;

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

			if (Restore_Script_Dir()) {
				Refresh_Script_List(mScript_Directory);
			}
		}
	}
	ImGui::EndChild();

	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor();
}

void CStudio_GUI::Draw_Module_Configuration(const std::string& title, const std::string& combo_id, const std::string& details_table_id, const std::string& input_prefix,
	const std::vector<TPlugin_Descriptor_Internal>& plugins, int& selected_plugin_index, std::vector<std::string>& parameter_values) {

	ImGui::BeginGroup();

	const char* text = title.c_str();
	const ImVec2 padding(6.0f, 3.0f);
	const ImU32 bgColor = Header_BG_Color;

	ImVec2 rowMin = ImGui::GetCursorScreenPos();
	float rowWidth = ImGui::GetContentRegionAvail().x + 8;
	float rowHeight = ImGui::GetTextLineHeight() + padding.y * 2.0f;

	ImGui::GetWindowDrawList()->AddRectFilled(
		ImVec2(rowMin.x - 8, rowMin.y - 6),
		ImVec2(rowMin.x + rowWidth, rowMin.y + rowHeight),
		bgColor
	);

	ImVec2 rowPos{ rowMin.x + padding.x - 4, rowMin.y + padding.y - 2 };

	ImGui::SetCursorScreenPos(rowPos);
	ImGui::PushStyleColor(ImGuiCol_Text, Highlight_Color);
	ImGui::TextUnformatted(text);
	ImGui::PopStyleColor();
	ImGui::SetCursorScreenPos(ImVec2(rowMin.x, rowMin.y + rowHeight + 4));

	ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
	ImGui::Text("Select the plugin from the drop-down menu");
	ImGui::PopFont();

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
			ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 160.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);

			ImGui::TableNextRow();
			ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, Header_BG_Color);
			ImGui::TableSetColumnIndex(0);
			ImGui::PushStyleColor(ImGuiCol_Text, Highlight_Color); ImGui::Text("Plugin information"); ImGui::PopStyleColor();


			ImGui::TableNextRow();
			ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, Row_BG_Color);
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("ID:");
			ImGui::TableSetColumnIndex(1);
			ImGui::TextWrapped("%s", selected_plugin.id.c_str());

			ImGui::TableNextRow();
			ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, Row_BG_Color);
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Name:");
			ImGui::TableSetColumnIndex(1);
			ImGui::TextWrapped("%s", selected_plugin.name.c_str());

			ImGui::TableNextRow();
			ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, Row_BG_Color);
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Description:");
			ImGui::TableSetColumnIndex(1);
			ImGui::TextWrapped("%s", selected_plugin.description.c_str());

			ImGui::TableNextRow();
			ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, Header_BG_Color);
			ImGui::TableSetColumnIndex(0);
			ImGui::PushStyleColor(ImGuiCol_Text, Highlight_Color); ImGui::Text("Configuration"); ImGui::PopStyleColor();

			for (size_t i = 0; i < selected_plugin.parameters.size(); ++i) {
				const auto& param = selected_plugin.parameters[i];

				ImGui::TableNextRow();
				ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, Row_BG_Color);
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
