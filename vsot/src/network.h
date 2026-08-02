#pragma once

#include "args.h"

#include <string>
#include <memory>
#include <thread>
#include <functional>
#include <mutex>
#include <list>

#include <vsotnet/Protocol.h>

struct TWaiting_For_Positions {
	sockaddr_in src_addr{};
	socklen_t src_addr_len = sizeof(src_addr);
	vsot::TRefresh_Positions_Request request{};
};

struct TWaiting_For_Any {
	sockaddr_in src_addr{};
	socklen_t src_addr_len = sizeof(src_addr);
};

struct TCallbacks {
	std::function<void(uint16_t /*robot_idx*/, uint16_t /*team_idx*/, float /*left*/, float /*right*/)> on_speed_set;
	std::function<void(const TWaiting_For_Positions& /*request*/)> on_refresh_positions_request;
	std::function<void(const TWaiting_For_Any& /*request*/)> on_metadata_request;
	std::function<void(const TWaiting_For_Any& /*request*/)> on_ball_position_request;
};

class CNetwork {
	private:
		std::string mHost = "localhost";
		int mPort = 10000;

		bool mRunning = false;

		SOCK mNet_Socket = INVALID_SOCKET_VALUE;
		sockaddr_in mNet_Addr{};

		std::unique_ptr<std::thread> mNet_Thread;

		TCallbacks mCallbacks;

	protected:
		void Thread_Fnc();

		int SendTo_Proxy(SOCK socketTarget, const std::vector<uint8_t>& buffer, const sockaddr_in& addr, socklen_t addr_len);

	public:
		CNetwork(const TCLI_Args& args);
		virtual ~CNetwork();

		bool Start();
		bool Stop();

		void Set_Callbacks(const TCallbacks& callbacks);

		void Send_Refresh_Positions_Response(const TWaiting_For_Positions& request, const std::vector<vsot::TNet_Robot_Position>& positions);
		void Send_Metadata_Response(const TWaiting_For_Any& request, const std::vector<uint16_t>& teamCounts, float arenaWidth, float arenaHeight, float goalWidth);
		void Send_Ball_Position_Response(const TWaiting_For_Any& request, uint16_t state, float x, float y, float radius);
};
