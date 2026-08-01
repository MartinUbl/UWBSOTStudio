#pragma once

#include <vector>
#include <chrono>
#include <cstdint>

// platform-specific includes for networking here (e.g., <winsock2.h> for Windows, <sys/socket.h> for Unix)
#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#include <WS2tcpip.h>

using socklen_t = int;
using SOCK = SOCKET;

constexpr int INVALID_SOCKET_VALUE = INVALID_SOCKET;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using SOCK = int;

constexpr int INVALID_SOCKET_VALUE = -1;

#endif

#pragma pack(push, 1)

namespace vsot {

	enum class NOpcode : uint16_t {
		Nop = 0x0000,

		// Commands from client to server
		Set_Robot_Speed = 0x0001,
		Refresh_Positions_Request = 0x0002,
		Metadata_Request = 0x0003,
		Ball_Position_Request = 0x0004,

		// Responses from server to client
		Refresh_Positions_Response = 0x0081,
		Metadata_Response = 0x0082,
		Ball_Position_Response = 0x0083,
	};

	struct TSet_Robot_Speed {
		uint16_t robot_idx;
		uint16_t team_idx;
		float left;
		float right;
	};

	struct TRefresh_Positions_Request {
		uint16_t robot_idx; // or Robot_Idx_Any
		uint16_t team_idx;  // or Team_Idx_Any
	};

	struct TNet_Robot_Position {
		uint16_t robot_idx;
		uint16_t team_idx;
		float x;
		float y;
		float orientation;
	};

	struct TRefresh_Positions_Response {
		uint16_t robot_count;
		TNet_Robot_Position positions[0]; // flexible array member
	};

	struct TMetadata_Response {
		float arena_width;
		float arena_height;
		uint16_t team_count;
		uint16_t team_counts[0]; // flexible array member
	};

	struct TBall_Position_Response {
		uint16_t state; // NBall_State
		float x;
		float y;
		float radius;
	};

	struct TMessage_Header {
		NOpcode opcode;
		uint16_t length; // length of the body in bytes
	};

	template<typename TMessage_Body>
	struct TMessage {
		TMessage_Header header;
		TMessage_Body body;
	};

	template<>
	struct TMessage<void> {
		TMessage_Header header;
	};

#pragma pack(pop)

	int Initialize_Networking();
	int Deinitialize_Networking();

	bool Receive_Message(SOCK socket, std::vector<char>& buffer);
	bool SetReceiveTimeout(SOCK socket, std::chrono::milliseconds timeout);

}
