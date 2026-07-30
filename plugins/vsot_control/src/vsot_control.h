#pragma once

#include <uwbsot/PluginInterface.h>
#include <vsotnet/Protocol.h>

class CVSOT_Control_Plugin : public IControl_Plugin {

	private:
		std::string mEnv_Address;
		uint16_t mEnv_Port = 0;

		sockaddr_in mAddr{};
		SOCK mSocket = INVALID_SOCKET_VALUE;

	public:
		// IPlugin iface
		virtual NLib_Result Initialize(const char** config_parameters, const char** config_parameter_values, uint16_t parameter_count) override;
		virtual NLib_Result Shutdown() override;
		// IControl_Plugin iface
		virtual NLib_Result Set_Robot_Speed(uint16_t robot_idx, uint16_t team_idx, float speed_left, float speed_right) override;
};