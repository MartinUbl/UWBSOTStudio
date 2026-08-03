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

	if (mSocket != INVALID_SOCKET_VALUE) {
		closesocket(mSocket);
		mSocket = INVALID_SOCKET_VALUE;
	}

	vsot::Deinitialize_Networking();

	return NLib_Result::Success;
}

const uint64_t CVSOT_Sensing_Plugin::Get_Logical_Clock() const {
	return 1;
}

bool CVSOT_Sensing_Plugin::Send_Empty_Request(CPacket& response, vsot::NOpcode opcode) const {

	CPacket pkt;

	// request metadata from the environment
	vsot::TMessage<void> request;
	request.header.opcode = opcode;
	request.header.length = 0;

	pkt.Put(request);

	int res = send(mSocket, reinterpret_cast<const char*>(pkt.Get_Raw_Data().data()), pkt.Get_Raw_Data().size(), 0);
	if (res <= 0) {
		std::cerr << "Failed to send metadata request\n";
		return false;
	}

	std::vector<char> buffer;
	if (!vsot::Receive_Message(mSocket, buffer)) {
		std::cerr << "Failed to receive metadata response\n";
		return false;
	}

	if (buffer.size() < sizeof(vsot::TMessage_Header)) {
		std::cerr << "Received metadata response is too short\n";
		return false;
	}

	response.assign(buffer);

	return true;
}

uint16_t CVSOT_Sensing_Plugin::Get_Team_Count() const {

	CPacket response;

	if (!Send_Empty_Request(response)) {
		return 0;
	}

	vsot::TMessage<vsot::TMetadata_Response> metadata_response;
	metadata_response.Deserialize_From(response);

	return static_cast<uint16_t>(metadata_response.body.team_counts.size());
}

NLib_Result CVSOT_Sensing_Plugin::Get_Team_Sizes(uint16_t** sizes, uint16_t* sizes_count) const {

	CPacket response;

	if (!Send_Empty_Request(response)) {
		*sizes_count = 0;
		return NLib_Result::Illegal_State;
	}

	vsot::TMessage<vsot::TMetadata_Response> metadata_response;
	metadata_response.Deserialize_From(response);

	*sizes_count = static_cast<uint16_t>(metadata_response.body.team_counts.size());

	*sizes = new uint16_t[*sizes_count];
	for (uint16_t i = 0; i < *sizes_count; ++i) {
		(*sizes)[i] = metadata_response.body.team_counts[i];
	}
	
	return NLib_Result::Success;
}

NLib_Result CVSOT_Sensing_Plugin::Get_Arena_Size(float* width, float* height, float* goal_width) const {

	CPacket response;

	if (!Send_Empty_Request(response)) {
		*width = 0.0f;
		*height = 0.0f;
		return NLib_Result::Illegal_State;
	}

	vsot::TMessage<vsot::TMetadata_Response> metadata_response;
	metadata_response.Deserialize_From(response);

	*width = metadata_response.body.arena_width;
	*height = metadata_response.body.arena_height;
	*goal_width = metadata_response.body.goal_width;

	return NLib_Result::Success;
}

NLib_Result CVSOT_Sensing_Plugin::Get_Robot_Positions(TRobot_Position** positions, uint16_t* positions_count, uint16_t team_idx) {

	CPacket pkt;

	vsot::TMessage<vsot::TRefresh_Positions_Request> request;
	request.header.opcode = vsot::NOpcode::Refresh_Positions_Request;
	request.header.length = sizeof(vsot::TRefresh_Positions_Request);
	request.body.robot_idx = Robot_Idx_Any;
	request.body.team_idx = team_idx;

	request.Serialize_To(pkt);

	int res = send(mSocket, reinterpret_cast<const char*>(pkt.Get_Raw_Data().data()), pkt.Get_Raw_Data().size(), 0);
	if (res <= 0) {
		std::cerr << "Failed to send refresh positions request\n";
		return NLib_Result::Illegal_State;
	}

	std::vector<char> buffer(1024);
	vsot::Receive_Message(mSocket, buffer);

	CPacket response(buffer);

	vsot::TMessage<vsot::TRefresh_Positions_Response> response_body;
	response_body.Deserialize_From(response);

	size_t position_count = response_body.body.positions.size();

	// Allocate the correct amount of memory
	*positions = new TRobot_Position[position_count];
	*positions_count = static_cast<uint16_t>(position_count);

	mCached_Robot_Positions.clear();

	for (size_t i = 0; i < position_count; ++i) {
		(*positions)[i].robot_idx = response_body.body.positions[i].robot_idx;
		(*positions)[i].x = response_body.body.positions[i].x;
		(*positions)[i].y = response_body.body.positions[i].y;
		(*positions)[i].orientation = response_body.body.positions[i].orientation;
		(*positions)[i].team_idx = response_body.body.positions[i].team_idx;
		mCached_Robot_Positions.push_back({ response_body.body.positions[i].x, response_body.body.positions[i].y, response_body.body.positions[i].team_idx });
	}

	return NLib_Result::Success;
}

NLib_Result CVSOT_Sensing_Plugin::Get_Ball_Position(NBall_State* state, float* x, float* y, float* radius) {

	CPacket response;

	if (!Send_Empty_Request(response, vsot::NOpcode::Ball_Position_Request)) {
		*state = NBall_State::Not_Found;
		*x = 0.0f;
		*y = 0.0f;
		*radius = 0.0f;
		return NLib_Result::Illegal_State;
	}

	vsot::TMessage<vsot::TBall_Position_Response> response_body;
	response_body.Deserialize_From(response);

	*state = static_cast<NBall_State>(response_body.body.state);
	*x = response_body.body.x;
	*y = response_body.body.y;
	*radius = response_body.body.radius;

	mCached_Ball_Position.state = static_cast<NBall_State>(response_body.body.state);
	mCached_Ball_Position.x = response_body.body.x;
	mCached_Ball_Position.y = response_body.body.y;
	mCached_Ball_Position.radius = response_body.body.radius;

	return NLib_Result::Success;
}

namespace {

#pragma warning(push)
#pragma warning(disable: 4838) // disable warning for narrowing conversions (for convenience)

	const char letter_V[] = {
		0b10000001,
		0b10000001,
		0b01000010,
		0b01000010,
		0b00100100,
		0b00100100,
		0b00011000,
		0b00011000
	};
	const char letter_S[] = {
		0b00111100,
		0b01000010,
		0b01000000,
		0b00111100,
		0b00000010,
		0b01000010,
		0b00111100,
		0b00000000
	};
	const char letter_O[] = {
		0b00111100,
		0b01000010,
		0b01000010,
		0b01000010,
		0b01000010,
		0b01000010,
		0b00111100,
		0b00000000
	};
	const char letter_T[] = {
		0b01111110,
		0b00010000,
		0b00010000,
		0b00010000,
		0b00010000,
		0b00010000,
		0b00010000,
		0b00000000
	};

	const char letter_C[] = {
		0b00111100,
		0b01000010,
		0b01000000,
		0b01000000,
		0b01000000,
		0b01000010,
		0b00111100,
		0b00000000
	};

	const char letter_A[] = {
		0b00111100,
		0b01000010,
		0b01000010,
		0b01111110,
		0b01000010,
		0b01000010,
		0b01000010,
		0b00000000
	};

	const char letter_M[] = {
		0b01000010,
		0b01100110,
		0b01011010,
		0b01000010,
		0b01000010,
		0b01000010,
		0b01000010,
		0b00000000
	};

#pragma warning(pop)
}

NLib_Result CVSOT_Sensing_Plugin::Get_Camera_Image(void* rgbaPixels, int* width, int* height) {

	constexpr int mock_cameraWidth = 240;
	constexpr int mock_cameraHeight = 200;

	if (rgbaPixels == nullptr) {
		*width = mock_cameraWidth;
		*height = mock_cameraHeight;
		return NLib_Result::Success;
	}

	// check for fit
	if (*width * *height < mock_cameraWidth * mock_cameraHeight) {
		return NLib_Result::Invalid_Parameters;
	}

	// fill the image black
	for (int y = 0; y < mock_cameraHeight; ++y) {
		for (int x = 0; x < mock_cameraWidth; ++x) {
			uint8_t* pixel = reinterpret_cast<uint8_t*>(rgbaPixels) + (y * mock_cameraWidth + x) * 4;
			pixel[0] = 0; // R
			pixel[1] = 0; // G
			pixel[2] = 0; // B
			pixel[3] = 255; // A
		}
	}

	// set the width and height
	*width = mock_cameraWidth;
	*height = mock_cameraHeight;

	// draw a dummy text
	auto drawLetter = [&](const char* letter, int letterWidth, int letterHeight, int offsetX, int offsetY) {
		for (int y = 0; y < letterHeight; ++y) {
			for (int x = 0; x < letterWidth; ++x) {
				if (letter[y] & (1 << (7 - x))) {
					int px = offsetX + x;
					int py = offsetY + y;
					if (px < 0 || px >= mock_cameraWidth || py < 0 || py >= mock_cameraHeight) {
						continue;
					}
					uint8_t* pixel = reinterpret_cast<uint8_t*>(rgbaPixels) + (py * mock_cameraWidth + px) * 4;
					pixel[0] = 64; // R
					pixel[1] = 64; // G
					pixel[2] = 64; // B
					pixel[3] = 255; // A
				}
			}
		}
	};

	drawLetter(letter_V, 8, 8, 30, 46);
	drawLetter(letter_S, 8, 8, 40, 46);
	drawLetter(letter_O, 8, 8, 50, 46);
	drawLetter(letter_T, 8, 8, 60, 46);

	drawLetter(letter_C, 8, 8, 30, 56);
	drawLetter(letter_A, 8, 8, 40, 56);
	drawLetter(letter_M, 8, 8, 50, 56);

	// draw a pixel for each robot position
	for (const auto& pos : mCached_Robot_Positions) {
		int x = static_cast<int>(pos.x / 220.0f * mock_cameraWidth);
		int y = static_cast<int>(pos.y / 180.0f * mock_cameraHeight);
		if (x < 0 || x >= mock_cameraWidth || y < 0 || y >= mock_cameraHeight) {
			continue;
		}

		const int nbh = 3;

		// fill the pixel and 1 more sorrounding pixels to make it more visible
		for (int dy = -nbh; dy <= nbh; ++dy) {
			for (int dx = -nbh; dx <= nbh; ++dx) {
				int px = x + dx;
				int py = y + dy;
				if (px < 0 || px >= mock_cameraWidth || py < 0 || py >= mock_cameraHeight) {
					continue;
				}
				uint8_t* pixel = reinterpret_cast<uint8_t*>(rgbaPixels) + (py * mock_cameraWidth + px) * 4;
				pixel[0] = 255; // R
				pixel[1] = 255; // G
				pixel[2] = 255; // B
				pixel[3] = 255; // A
			}
		}
	}

	// draw orange circle for the ball position
	{
		int x = static_cast<int>(mCached_Ball_Position.x / 220.0f * mock_cameraWidth);
		int y = static_cast<int>(mCached_Ball_Position.y / 180.0f * mock_cameraHeight);
		int radius = static_cast<int>(mCached_Ball_Position.radius / 220.0f * mock_cameraWidth);
		for (int dy = -radius; dy <= radius; ++dy) {
			for (int dx = -radius; dx <= radius; ++dx) {
				if (dx * dx + dy * dy <= radius * radius) {
					int px = x + dx;
					int py = y + dy;
					if (px < 0 || px >= mock_cameraWidth || py < 0 || py >= mock_cameraHeight) {
						continue;
					}
					uint8_t* pixel = reinterpret_cast<uint8_t*>(rgbaPixels) + (py * mock_cameraWidth + px) * 4;
					pixel[0] = 255; // R
					pixel[1] = 75;  // G
					pixel[2] = 0;   // B
					pixel[3] = 255; // A
				}
			}
		}
	}

	// return success
	return NLib_Result::Success;
}
