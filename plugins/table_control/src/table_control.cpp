#include "table_control.h"

#include <iostream>
#include <uwbsot/Utils.h>
#include <vsotnet/Protocol.h>

#include <msotctl.h>

#include <string_view>

NLib_Result CTable_Control_Plugin::Initialize(const char** config_parameters, const char** config_parameter_values, uint16_t parameter_count) {
	uwbsot::utils::CParameter_Map parameter_map(config_parameters, config_parameter_values, parameter_count);

	std::string port = parameter_map.get<std::string>("serial_port").value_or("");

	MSOTResult res = MSOTR_ERROR;

	if (port.empty()) {

		TCommDescription* comm_desc = nullptr;
		uint32_t count = 0;

		res = msotctl_enumerate_ports(&comm_desc, &count);
		if (res != MSOTResult::MSOTR_OK) {
			std::cerr << "Failed to enumerate serial ports." << std::endl;
			return NLib_Result::Not_Found;
		}

		// go through ports, and if desc contains "MiroSOT", use that port
		for (uint32_t i = 0; i < count; ++i) {
			if (std::string_view(comm_desc[i].desc).find("MiroSOT") != std::string::npos) {
				port = comm_desc[i].portpath;
				break;
			}
		}

		msotctl_free_port_list(comm_desc);

		if (port.empty()) {
			std::cerr << "No MiroSOT dongle found." << std::endl;
			return NLib_Result::Not_Found;
		}
	}

	res = msotctl_connect(port.c_str(), &mDevice_Handle);
	if (res != MSOTResult::MSOTR_OK) {
		std::cerr << "Failed to connect to the table control device on port " << port << std::endl;
		return NLib_Result::Not_Found;
	}

	return NLib_Result::Success;
}

NLib_Result CTable_Control_Plugin::Shutdown() {

	if (mDevice_Handle != 0) {
		msotctl_disconnect(mDevice_Handle);
	}

	return NLib_Result::Success;
}

NLib_Result CTable_Control_Plugin::Set_Robot_Speed(uint16_t robot_idx, uint16_t team_idx, float speed_left, float speed_right) {

	auto hash = Make_Robot_Hash(robot_idx, team_idx);

	if (!mInitialized_Robots.contains(hash)) {

		auto res = msotctl_msg_init(mDevice_Handle, robot_idx, 1);

		if (res != MSOTResult::MSOTR_OK) {
			std::cerr << "Failed to initialize robot " << robot_idx << " on team " << team_idx << std::endl;
			return NLib_Result::Illegal_State;
		}

		mInitialized_Robots.insert(hash);
	}

	const uint8_t speed_left_byte = static_cast<uint8_t>(std::clamp(speed_left, 0.0f, 1.0f) * 255.0f);
	const uint8_t speed_right_byte = static_cast<uint8_t>(std::clamp(speed_right, 0.0f, 1.0f) * 255.0f);

	auto res = msotctl_msg_immediate_move(mDevice_Handle, robot_idx, speed_left_byte, speed_right_byte, 500);

	return NLib_Result::Success;
}
