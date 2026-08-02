#include "vsot_control.h"

#include <iostream>
#include <uwbsot/Utils.h>
#include <vsotnet/Protocol.h>

NLib_Result CVSOT_Control_Plugin::Initialize(const char** config_parameters, const char** config_parameter_values, uint16_t parameter_count) {
	uwbsot::utils::CParameter_Map parameter_map(config_parameters, config_parameter_values, parameter_count);

	mEnv_Address = parameter_map.get<std::string>("env_address").value_or("localhost");
	mEnv_Port = parameter_map.get<uint16_t>("env_port").value_or(0);

	vsot::Initialize_Networking();

	mSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (mSocket == INVALID_SOCKET_VALUE) {
		std::cerr << "Failed to create socket\n";
		return NLib_Result::Illegal_State;
	}

	int res = 0;
	res = vsot::Initialize_Networking();
	if (res != 0) {
		std::cerr << "Failed to initialize networking\n";
		return NLib_Result::Illegal_State;
	}

	mAddr.sin_family = AF_INET;
	res = inet_pton(AF_INET, mEnv_Address.c_str(), &mAddr.sin_addr);
	if (res <= 0) {
		std::cerr << "Invalid address: " << mEnv_Address << "\n";
		return NLib_Result::Invalid_Parameters;
	}

	mAddr.sin_port = htons(mEnv_Port);

	res = connect(mSocket, (sockaddr*)&mAddr, sizeof(mAddr));
	if (res != 0) {
		std::cerr << "Failed to connect the UDP socket with target address\n";
		return NLib_Result::Illegal_State;
	}

	return NLib_Result::Success;
}

NLib_Result CVSOT_Control_Plugin::Shutdown() {

	if (mSocket != INVALID_SOCKET_VALUE) {
		closesocket(mSocket);
		mSocket = INVALID_SOCKET_VALUE;
	}

	vsot::Deinitialize_Networking();

	return NLib_Result::Success;
}

NLib_Result CVSOT_Control_Plugin::Set_Robot_Speed(uint16_t robot_idx, uint16_t team_idx, float speed_left, float speed_right) {

	CPacket pkt;

	vsot::TMessage<vsot::TSet_Robot_Speed> message;
	message.header.opcode = vsot::NOpcode::Set_Robot_Speed;
	message.header.length = sizeof(vsot::TSet_Robot_Speed);

	message.body.robot_idx = robot_idx;
	message.body.team_idx = team_idx;
	message.body.left = speed_left;
	message.body.right = speed_right;

	message.Serialize_To(pkt);

	int res = send(mSocket, reinterpret_cast<const char*>(pkt.Get_Raw_Data().data()), pkt.Get_Raw_Data().size(), 0);
	if (res <= 0) {
		std::cerr << "Failed to send set robot speed message\n";
		return NLib_Result::Illegal_State;
	}

	return NLib_Result::Success;
}
