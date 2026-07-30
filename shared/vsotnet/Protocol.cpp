#include "Protocol.h"

#include <vector>

#undef min
#undef max

namespace vsot {

#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
	int Initialize_Networking() {
		WSADATA wsaData;
		return ::WSAStartup(MAKEWORD(2, 2), &wsaData);
	}

	int Deinitialize_Networking() {
		return ::WSACleanup();
	}
#else
	int Initialize_Networking() {
		return 0;
	}

	int Deinitialize_Networking() {
		return 0;
	}

	int closesocket(SOCK sock) {
		return close(sock);
	}
#endif

	bool Receive_Message(SOCK socket, std::vector<char>& buffer) {
		buffer.resize(1024); // initial size, will be reduced

		TMessage_Header* response_header;

		int res = 0;
		while (res < 1024) {
			int bytes_received = recv(socket, buffer.data() + res, buffer.size() - res, 0);
			if (bytes_received <= 0) {
				buffer.clear();
				return false;
			}
			res += bytes_received;

			if (res >= sizeof(vsot::TMessage_Header)) {
				response_header = reinterpret_cast<vsot::TMessage_Header*>(buffer.data());
				if (res >= sizeof(vsot::TMessage_Header) + response_header->length) {
					buffer.resize(sizeof(vsot::TMessage_Header) + response_header->length);
					break;
				}
			}
		}

		return true;
	}

	bool SetReceiveTimeout(SOCK socket, std::chrono::milliseconds timeout)
	{
		if (timeout.count() < 0)
			return false;

#ifdef _WIN32
		// Windows expects a timeout in milliseconds as a DWORD.
		const auto milliseconds =
			timeout.count() > std::numeric_limits<DWORD>::max()
			? std::numeric_limits<DWORD>::max()
			: static_cast<DWORD>(timeout.count());

		return setsockopt(
			socket,
			SOL_SOCKET,
			SO_RCVTIMEO,
			reinterpret_cast<const char*>(&milliseconds),
			static_cast<int>(sizeof(milliseconds))
		) == 0;

#else
		// Linux/POSIX expects a timeval.
		timeval value{};
		value.tv_sec = static_cast<time_t>(timeout.count() / 1000);
		value.tv_usec = static_cast<suseconds_t>(
			(timeout.count() % 1000) * 1000
			);

		return setsockopt(
			socket,
			SOL_SOCKET,
			SO_RCVTIMEO,
			&value,
			sizeof(value)
		) == 0;
#endif
	}

}
