#pragma once

#include <vector>
#include <chrono>
#include <cstdint>

#include "Packet.h"

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
		uint16_t robot_idx = 0;
		uint16_t team_idx = 0;
		float left = 0.0f;
		float right = 0.0f;

		void Serialize_To(CPacket& pkt) const {
			pkt.Put(robot_idx);
			pkt.Put(team_idx);
			pkt.Put(left);
			pkt.Put(right);
		}

		void Deserialize_From(CPacket& pkt) {
			robot_idx = pkt.Get<uint16_t>();
			team_idx = pkt.Get<uint16_t>();
			left = pkt.Get<float>();
			right = pkt.Get<float>();
		}
	};

	struct TRefresh_Positions_Request {
		uint16_t robot_idx = 0;		// or Robot_Idx_Any
		uint16_t team_idx = 0;		// or Team_Idx_Any

		void Serialize_To(CPacket& pkt) const {
			pkt.Put(robot_idx);
			pkt.Put(team_idx);
		}

		void Deserialize_From(CPacket& pkt) {
			robot_idx = pkt.Get<uint16_t>();
			team_idx = pkt.Get<uint16_t>();
		}
	};

	struct TNet_Robot_Position {
		uint16_t robot_idx = 0;
		uint16_t team_idx = 0;
		float x = 0.0f;
		float y = 0.0f;
		float orientation = 0.0f;

		void Serialize_To(CPacket& pkt) const {
			pkt.Put(robot_idx);
			pkt.Put(team_idx);
			pkt.Put(x);
			pkt.Put(y);
			pkt.Put(orientation);
		}

		void Deserialize_From(CPacket& pkt) {
			robot_idx = pkt.Get<uint16_t>();
			team_idx = pkt.Get<uint16_t>();
			x = pkt.Get<float>();
			y = pkt.Get<float>();
			orientation = pkt.Get<float>();
		}
	};

	struct TRefresh_Positions_Response {
		std::vector<TNet_Robot_Position> positions;

		void Serialize_To(CPacket& pkt) const {
			pkt.Put_Array(positions);
		}

		void Deserialize_From(CPacket& pkt) {
			positions = pkt.Get_Array<TNet_Robot_Position>();
		}
	};

	struct TMetadata_Response {
		float arena_width = 0.0f;
		float arena_height = 0.0f;
		float goal_width = 0.0f;
		std::vector<uint16_t> team_counts;

		void Serialize_To(CPacket& pkt) const {
			pkt.Put(arena_width);
			pkt.Put(arena_height);
			pkt.Put(goal_width);
			pkt.Put_Array(team_counts);
		}

		void Deserialize_From(CPacket& pkt) {
			arena_width = pkt.Get<float>();
			arena_height = pkt.Get<float>();
			goal_width = pkt.Get<float>();
			team_counts = pkt.Get_Array<uint16_t>();
		}
	};

	struct TBall_Position_Response {
		uint16_t state = 0;		// NBall_State
		float x = 0.0f;
		float y = 0.0f;
		float radius = 1.0f;

		void Serialize_To(CPacket& pkt) const {
			pkt.Put(state);
			pkt.Put(x);
			pkt.Put(y);
			pkt.Put(radius);
		}

		void Deserialize_From(CPacket& pkt) {
			state = pkt.Get<uint16_t>();
			x = pkt.Get<float>();
			y = pkt.Get<float>();
			radius = pkt.Get<float>();
		}
	};

#pragma pack(push, 1)

	struct TMessage_Header {
		NOpcode opcode = NOpcode::Nop;
		uint16_t length = 0;					// length of the body in bytes

		void Serialize_To(CPacket& pkt) const {
			pkt.Put(opcode);
			pkt.Put(length);
		}

		void Deserialize_From(CPacket& pkt) {
			opcode = pkt.Get<NOpcode>();
			length = pkt.Get<uint16_t>();
		}
	};

#pragma pack(pop)

	template<typename TMessage_Body>
	struct TMessage {
		TMessage_Header header;
		TMessage_Body body;

		void Serialize_To(CPacket& pkt) {
			header.Serialize_To(pkt);
			body.Serialize_To(pkt);

			header.length = static_cast<uint16_t>(pkt.Get_Raw_Data().size() - sizeof(TMessage_Header));
			pkt.Put_At(header, 0);
		}

		void Deserialize_From(CPacket& pkt) {
			header.Deserialize_From(pkt);
			body.Deserialize_From(pkt);
		}
	};

	template<>
	struct TMessage<void> {
		TMessage_Header header;

		void Serialize_To(CPacket& pkt) const {
			header.Serialize_To(pkt);
		}

		void Deserialize_From(CPacket& pkt) {
			header.Deserialize_From(pkt);
		}
	};

	int Initialize_Networking();
	int Deinitialize_Networking();

	bool Receive_Message(SOCK socket, std::vector<char>& buffer);
	bool SetReceiveTimeout(SOCK socket, std::chrono::milliseconds timeout);

}
