#include "vsot_sensing.h"

#include <uwbsot/Utils.h>

#include <iostream>

#undef min
#undef max

NLib_Result CVSOT_Sensing_Plugin::Initialize(const char** config_parameters, const char** config_parameter_values, uint16_t parameter_count) {

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

NLib_Result CVSOT_Sensing_Plugin::Shutdown() {
	vsot::Deinitialize_Networking();

	return NLib_Result::Success;
}

const uint64_t CVSOT_Sensing_Plugin::Get_Logical_Clock() const {
	return 1;
}

bool CVSOT_Sensing_Plugin::Send_Metadata_Request(vsot::TMetadata_Response** response) const {
	// request metadata from the environment and parse the response to get the number of teams
	vsot::TMessage<void> request;
	request.header.opcode = vsot::NOpcode::Metadata_Request;
	request.header.length = 0;

	int res = send(mSocket, reinterpret_cast<const char*>(&request), sizeof(request), 0);
	if (res <= 0) {
		std::cerr << "Failed to send metadata request\n";
		return false;
	}

	std::vector<char> buffer;
	if (!vsot::Receive_Message(mSocket, buffer)) {
		std::cerr << "Failed to receive metadata response\n";
		return false;
	}

	const size_t body_size = buffer.size() - sizeof(vsot::TMessage_Header);

	char* body_copy = new char[body_size];
	memcpy(body_copy, buffer.data() + sizeof(request), body_size);

	*response = reinterpret_cast<vsot::TMetadata_Response*>(body_copy);

	return true;
}

uint16_t CVSOT_Sensing_Plugin::Get_Team_Count() const {

	vsot::TMetadata_Response* response = nullptr;
	if (!Send_Metadata_Request(&response)) {
		return 0;
	}

	uint16_t team_count = response->team_count;
	delete[] reinterpret_cast<char*>(response);
	return team_count;
}

NLib_Result CVSOT_Sensing_Plugin::Get_Team_Sizes(uint16_t** sizes, uint16_t* sizes_count) const {

	vsot::TMetadata_Response* response = nullptr;
	if (!Send_Metadata_Request(&response)) {
		*sizes_count = 0;
		return NLib_Result::Illegal_State;
	}

	*sizes_count = response->team_count;

	*sizes = new uint16_t[*sizes_count];
	for (uint16_t i = 0; i < *sizes_count; ++i) {
		(*sizes)[i] = response->team_counts[i];
	}
	
	delete[] reinterpret_cast<char*>(response);
	return NLib_Result::Success;
}

NLib_Result CVSOT_Sensing_Plugin::Get_Arena_Size(float* width, float* height) const {

	vsot::TMetadata_Response* response = nullptr;
	if (!Send_Metadata_Request(&response)) {
		*width = 0.0f;
		*height = 0.0f;
		return NLib_Result::Illegal_State;
	}

	*width = response->arena_width;
	*height = response->arena_height;

	delete[] reinterpret_cast<char*>(response);
	return NLib_Result::Success;
}

NLib_Result CVSOT_Sensing_Plugin::Get_Robot_Positions(TRobot_Position** positions, uint16_t* positions_count, uint16_t team_idx) {

	vsot::TMessage<vsot::TRefresh_Positions_Request> request;
	request.header.opcode = vsot::NOpcode::Refresh_Positions_Request;
	request.header.length = sizeof(vsot::TRefresh_Positions_Request);
	request.body.robot_idx = Robot_Idx_Any;
	request.body.team_idx = team_idx;

	int res = send(mSocket, reinterpret_cast<const char*>(&request), sizeof(request), 0);
	if (res <= 0) {
		std::cerr << "Failed to send refresh positions request\n";
		return NLib_Result::Illegal_State;
	}

	std::vector<char> buffer(1024);
	vsot::Receive_Message(mSocket, buffer);

	vsot::TRefresh_Positions_Response* response = reinterpret_cast<vsot::TRefresh_Positions_Response*>(buffer.data() + sizeof(vsot::TMessage_Header));

	*positions_count = response->robot_count;
	*positions = new TRobot_Position[*positions_count];

	mCached_Robot_Positions.clear();
	for (uint16_t i = 0; i < *positions_count; ++i) {
		(*positions)[i].robot_idx = response->positions[i].robot_idx;
		(*positions)[i].x = response->positions[i].x;
		(*positions)[i].y = response->positions[i].y;
		(*positions)[i].orientation = response->positions[i].orientation;
		(*positions)[i].team_idx = response->positions[i].team_idx;
		mCached_Robot_Positions.push_back({response->positions[i].x, response->positions[i].y, response->positions[i].team_idx});
	}

	return NLib_Result::Success;
}

NLib_Result CVSOT_Sensing_Plugin::Get_Camera_Image(void* rgbaPixels, int* width, int* height) {
	if (rgbaPixels == nullptr) {
		*width = 120;
		*height = 100;
		return NLib_Result::Success;
	}

	// check for fit
	if (*width * *height < 120 * 100) {
		return NLib_Result::Invalid_Parameters;
	}

	// fill the image black
	for (int y = 0; y < 100; ++y) {
		for (int x = 0; x < 120; ++x) {
			uint8_t* pixel = reinterpret_cast<uint8_t*>(rgbaPixels) + (y * 120 + x) * 4;
			pixel[0] = 0; // R
			pixel[1] = 0; // G
			pixel[2] = 0; // B
			pixel[3] = 255; // A
		}
	}

	// set the width and height
	*width = 120;
	*height = 100;

	// draw a pixel for each robot position
	for (const auto& pos : mCached_Robot_Positions) {
		int x = static_cast<int>(pos.x / 220.0f * 120.0f);
		int y = static_cast<int>(pos.y / 180.0f * 100.0f);
		if (x < 0 || x >= 120 || y < 0 || y >= 100) {
			continue;
		}

		const int nbh = 2;

		// fill the pixel and 1 more sorrounding pixels to make it more visible
		for (int dy = -nbh; dy <= nbh; ++dy) {
			for (int dx = -nbh; dx <= nbh; ++dx) {
				int px = x + dx;
				int py = y + dy;
				if (px < 0 || px >= 120 || py < 0 || py >= 100) {
					continue;
				}
				uint8_t* pixel = reinterpret_cast<uint8_t*>(rgbaPixels) + (py * 120 + px) * 4;
				pixel[0] = 255; // R
				pixel[1] = 0;   // G
				pixel[2] = 0;   // B
				pixel[3] = 255; // A
			}
		}
	}

	// return success
	return NLib_Result::Success;
}
