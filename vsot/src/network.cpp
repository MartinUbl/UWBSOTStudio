#include "network.h"
#include <iostream>

#include <spdlog/spdlog.h>

#include <chrono>
#undef min
#undef max

namespace {
	bool IsTimeout() {
#ifdef _WIN32
		return WSAGetLastError() == WSAETIMEDOUT;
#else
		return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
	}
}

CNetwork::CNetwork(const TCLI_Args& args) : mHost(args.host), mPort(args.port) {
	if (vsot::Initialize_Networking() != 0) {
		spdlog::error("Failed to initialize networking");
		throw std::runtime_error("Failed to initialize networking");
	}
}

CNetwork::~CNetwork() {
	Stop();
	vsot::Deinitialize_Networking();
}

bool CNetwork::Start() {

	// initialize UDP socket on mHost:mPort, start network thread to listen for incoming messages, etc.
	mNet_Socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (mNet_Socket == INVALID_SOCKET_VALUE) {
		spdlog::error("Failed to create socket");
		return false;
	}

	int res = 0;

	mNet_Addr.sin_family = AF_INET;
	mNet_Addr.sin_port = htons(mPort);
	inet_pton(AF_INET, mHost.c_str(), &mNet_Addr.sin_addr);

	res = bind(mNet_Socket, (sockaddr*)&mNet_Addr, sizeof(mNet_Addr));
	if (res < 0) {
		spdlog::error("Failed to bind socket");
		return false;
	}

	mRunning = true;
	mNet_Thread = std::make_unique<std::thread>(&CNetwork::Thread_Fnc, this);

	spdlog::info("Network started on {}:{}", mHost, mPort);

	return true;
}

bool CNetwork::Stop() {
	mRunning = false;
	closesocket(mNet_Socket);

	if (mNet_Thread && mNet_Thread->joinable()) {
		mNet_Thread->join();
	}

	return true;
}

void CNetwork::Set_Callbacks(const TCallbacks& callbacks) {
	mCallbacks = callbacks;
}

void CNetwork::Thread_Fnc() {

	int res = 0;
	std::vector<char> buffer(65536); // 64KB buffer for incoming messages
	sockaddr_in src_addr{};
	socklen_t src_addr_len = sizeof(src_addr);

	vsot::SetReceiveTimeout(mNet_Socket, std::chrono::milliseconds(1000));

	while (mRunning) {

		// for now, let's assume no errors and localhost-only communication!

		src_addr_len = sizeof(src_addr);
		res = recvfrom(mNet_Socket, buffer.data(), buffer.size(), 0, reinterpret_cast<sockaddr*>(&src_addr), &src_addr_len);
		if (!mRunning) {
			break;
		}

		if (res < 0) {
			if (!IsTimeout()) {
				// TODO: handle error
				spdlog::error("Error receiving data (header, res = {}, errno = {}, WSAGetLastError = {})", res, errno, WSAGetLastError());
			}
			continue;
		}

		int total_received = res;

		bool error_occurred = false;

		if (res < sizeof(vsot::TMessage_Header)) {
			// call recvfrom in a loop until we have enough data for the header
			while (total_received < sizeof(vsot::TMessage_Header)) {
				res = recvfrom(mNet_Socket, buffer.data() + total_received, buffer.size() - total_received, 0, reinterpret_cast<sockaddr*>(&src_addr), &src_addr_len);
				if (!mRunning) {
					break;
				}
				if (res < 0) {
					// TODO: handle error
					spdlog::error("Error receiving data (header continuation, res = {})", res);
					error_occurred = true;
					break;
				}
				total_received += res;
			}
		}

		// Now we have at least the header, let's parse it
		vsot::TMessage_Header* header = reinterpret_cast<vsot::TMessage_Header*>(buffer.data());

		if (error_occurred) {
			continue;
		}

		// check if we have the full message body
		while (total_received < sizeof(vsot::TMessage_Header) + header->length) {
			res = recvfrom(mNet_Socket, buffer.data() + total_received, buffer.size() - total_received, 0, reinterpret_cast<sockaddr*>(&src_addr), &src_addr_len);
			if (!mRunning) {
				break;
			}
			if (res < 0) {
				error_occurred = true;
				spdlog::error("Error receiving data (body continuation, res = {})", res);
				break;
			}
			total_received += res;

			if (total_received > buffer.size()) {
				error_occurred = true;
				spdlog::error("Received message exceeds buffer size, discarding");
				break;
			}
		}

		if (error_occurred) {
			continue;
		}

		//spdlog::info("Received message from {}:{}, opcode = {}, length = {}", inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port), static_cast<uint16_t>(header->opcode), header->length);

		if (header->opcode == vsot::NOpcode::Nop) {
			spdlog::info("Received Nop message, ignoring");
			continue;
		}
		else if (header->opcode == vsot::NOpcode::Set_Robot_Speed) {
			if (header->length != sizeof(vsot::TSet_Robot_Speed)) {
				spdlog::error("Invalid Set_Robot_Speed message length");
				continue;
			}
			vsot::TSet_Robot_Speed* msg = reinterpret_cast<vsot::TSet_Robot_Speed*>(buffer.data() + sizeof(vsot::TMessage_Header));
			if (mCallbacks.on_speed_set) {
				mCallbacks.on_speed_set(msg->robot_idx, msg->team_idx, msg->left, msg->right);
			}
		}
		else if (header->opcode == vsot::NOpcode::Refresh_Positions_Request) {
			if (header->length != sizeof(vsot::TRefresh_Positions_Request)) {
				spdlog::error("Invalid Refresh_Positions_Request message length");
				continue;
			}
			vsot::TRefresh_Positions_Request* msg = reinterpret_cast<vsot::TRefresh_Positions_Request*>(buffer.data() + sizeof(vsot::TMessage_Header));
			if (mCallbacks.on_refresh_positions_request) {
				mCallbacks.on_refresh_positions_request({ src_addr, src_addr_len, *msg });
			}
		}
		else if (header->opcode == vsot::NOpcode::Metadata_Request) {
			if (header->length != 0) {
				spdlog::error("Invalid Metadata_Request message length");
				continue;
			}
			if (mCallbacks.on_metadata_request) {
				mCallbacks.on_metadata_request({ src_addr, src_addr_len });
			}
		}
		else if (header->opcode == vsot::NOpcode::Ball_Position_Request) {
			if (header->length != 0) {
				spdlog::error("Invalid Ball_Position_Request message length");
				continue;
			}
			if (mCallbacks.on_ball_position_request) {
				mCallbacks.on_ball_position_request({ src_addr, src_addr_len });
			}
		}
		else {
			spdlog::error("Unknown opcode received: {}", static_cast<uint16_t>(header->opcode));
		}

	}

}

void CNetwork::Send_Refresh_Positions_Response(const TWaiting_For_Positions& request, const std::vector<vsot::TNet_Robot_Position>& positions) {
	vsot::TMessage<vsot::TRefresh_Positions_Response> response;
	response.header.opcode = vsot::NOpcode::Refresh_Positions_Response;
	response.header.length = sizeof(uint16_t) /* robot_count */ + positions.size() * sizeof(vsot::TNet_Robot_Position) /* robot_positions */;

	response.body.robot_count = static_cast<uint16_t>(positions.size());

	std::vector<uint8_t> buffer(sizeof(vsot::TMessage_Header) + response.header.length);
	std::copy(reinterpret_cast<const char*>(&response.header), reinterpret_cast<const char*>(&response.header) + sizeof(vsot::TMessage_Header), buffer.begin());
	std::copy(reinterpret_cast<const char*>(&response.body.robot_count), reinterpret_cast<const char*>(&response.body.robot_count) + sizeof(uint16_t), buffer.begin() + sizeof(vsot::TMessage_Header));
	std::copy(reinterpret_cast<const char*>(positions.data()), reinterpret_cast<const char*>(positions.data()) + positions.size() * sizeof(vsot::TNet_Robot_Position), buffer.begin() + sizeof(vsot::TMessage_Header) + sizeof(uint16_t));

	//spdlog::info("Sending Refresh_Positions_Response to {}:{}, robot_count = {}", inet_ntoa(request.src_addr.sin_addr), ntohs(request.src_addr.sin_port), response.body.robot_count);

	// send the response back to the requester
	int res = SendTo_Proxy(mNet_Socket, buffer, request.src_addr, request.src_addr_len);
	if (res < 0) {
		std::cerr << "Error sending Refresh_Positions_Response\n";
	}
}

void CNetwork::Send_Metadata_Response(const TWaiting_For_Any& request, const std::vector<uint16_t>& teamCounts, float arenaWidth, float arenaHeight, float goalWidth) {
	vsot::TMessage<vsot::TMetadata_Response> response;
	response.header.opcode = vsot::NOpcode::Metadata_Response;
	response.header.length = sizeof(uint16_t) /* team_count */ + teamCounts.size() * sizeof(uint16_t) /* team_counts */ + sizeof(float) * 3 /* arena_width, arena_height and goal_width */;

	response.body.team_count = static_cast<uint16_t>(teamCounts.size());
	response.body.arena_width = arenaWidth;
	response.body.arena_height = arenaHeight;
	response.body.goal_width = goalWidth;

	std::vector<uint8_t> buffer(sizeof(vsot::TMessage_Header) + response.header.length);
	std::copy(reinterpret_cast<const char*>(&response.header), reinterpret_cast<const char*>(&response.header) + sizeof(vsot::TMessage_Header), buffer.begin());
	std::copy(reinterpret_cast<const char*>(&response.body.arena_width), reinterpret_cast<const char*>(&response.body.arena_width) + sizeof(float), buffer.begin() + sizeof(vsot::TMessage_Header));
	std::copy(reinterpret_cast<const char*>(&response.body.arena_height), reinterpret_cast<const char*>(&response.body.arena_height) + sizeof(float), buffer.begin() + sizeof(vsot::TMessage_Header) + sizeof(float));
	std::copy(reinterpret_cast<const char*>(&response.body.goal_width), reinterpret_cast<const char*>(&response.body.goal_width) + sizeof(float), buffer.begin() + sizeof(vsot::TMessage_Header) + sizeof(float) * 2);
	std::copy(reinterpret_cast<const char*>(&response.body.team_count), reinterpret_cast<const char*>(&response.body.team_count) + sizeof(uint16_t), buffer.begin() + sizeof(vsot::TMessage_Header) + sizeof(float) * 3);
	std::copy(reinterpret_cast<const char*>(teamCounts.data()), reinterpret_cast<const char*>(teamCounts.data()) + teamCounts.size() * sizeof(uint16_t), buffer.begin() + sizeof(vsot::TMessage_Header) + sizeof(float) * 3 + sizeof(uint16_t));

	spdlog::info("Sending Metadata_Response to {}:{}, bufsize = {}, length = {}, team_count = {}", inet_ntoa(request.src_addr.sin_addr), ntohs(request.src_addr.sin_port), buffer.size(), response.header.length, response.body.team_count);

	// send the response back to the requester
	int res = SendTo_Proxy(mNet_Socket, buffer, request.src_addr, request.src_addr_len);
	if (res < 0) {
		std::cerr << "Error sending Metadata_Response\n";
	}
}

void CNetwork::Send_Ball_Position_Response(const TWaiting_For_Any& request, uint16_t state, float x, float y, float radius) {
	vsot::TMessage<vsot::TBall_Position_Response> response;
	response.header.opcode = vsot::NOpcode::Ball_Position_Response;
	response.header.length = sizeof(uint16_t) /* state */ + sizeof(float) * 3 /* x, y and radius */;
	response.body.state = state;
	response.body.x = x;
	response.body.y = y;
	response.body.radius = radius;
	std::vector<uint8_t> buffer(sizeof(vsot::TMessage_Header) + response.header.length);
	std::copy(reinterpret_cast<const char*>(&response.header), reinterpret_cast<const char*>(&response.header) + sizeof(vsot::TMessage_Header), buffer.begin());
	std::copy(reinterpret_cast<const char*>(&response.body.state), reinterpret_cast<const char*>(&response.body.state) + sizeof(uint16_t), buffer.begin() + sizeof(vsot::TMessage_Header));
	std::copy(reinterpret_cast<const char*>(&response.body.x), reinterpret_cast<const char*>(&response.body.x) + sizeof(float), buffer.begin() + sizeof(vsot::TMessage_Header) + sizeof(uint16_t));
	std::copy(reinterpret_cast<const char*>(&response.body.y), reinterpret_cast<const char*>(&response.body.y) + sizeof(float), buffer.begin() + sizeof(vsot::TMessage_Header) + sizeof(uint16_t) + sizeof(float));
	std::copy(reinterpret_cast<const char*>(&response.body.radius), reinterpret_cast<const char*>(&response.body.radius) + sizeof(float), buffer.begin() + sizeof(vsot::TMessage_Header) + sizeof(uint16_t) + sizeof(float) * 2);
	//spdlog::info("Sending Ball_Position_Response to {}:{}, bufsize = {}, length = {}, state = {}", inet_ntoa(request.src_addr.sin_addr), ntohs(request.src_addr.sin_port), buffer.size(), response.header.length, response.body.state);
	// send the response back to the requester
	int res = SendTo_Proxy(mNet_Socket, buffer, request.src_addr, request.src_addr_len);
	if (res < 0) {
		std::cerr << "Error sending Ball_Position_Response\n";
	}
}

int CNetwork::SendTo_Proxy(SOCK socketTarget, const std::vector<uint8_t>& buffer, const sockaddr_in& addr, socklen_t addr_len) {

	// byte dump the buffer
	/*
	std::ostringstream oss;
	for (size_t i = 0; i < buffer.size(); ++i) {
		oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]) << " ";
	}
	spdlog::info("SendTo: {}", oss.str());
	*/

	int res = sendto(socketTarget, reinterpret_cast<const char*>(buffer.data()), buffer.size(), 0, reinterpret_cast<const sockaddr*>(&addr), addr_len);
	if (res < 0) {
		std::cerr << "Error sending to proxy\n";
	}

	return res;
}
