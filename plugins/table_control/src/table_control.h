#pragma once

#include <uwbsot/PluginInterface.h>
#include <vsotnet/Protocol.h>

#include <set>

class CTable_Control_Plugin : public IControl_Plugin {

	private:
		uint64_t mDevice_Handle = 0;

		std::set<uint32_t> mInitialized_Robots;

	protected:
		static inline constexpr uint32_t Make_Robot_Hash(uint16_t robot_idx, uint16_t team_idx) {
			return (static_cast<uint32_t>(team_idx) << 16) | static_cast<uint32_t>(robot_idx);
		}

	public:
		// IPlugin iface
		virtual NLib_Result Initialize(const char** config_parameters, const char** config_parameter_values, uint16_t parameter_count) override;
		virtual NLib_Result Shutdown() override;
		// IControl_Plugin iface
		virtual NLib_Result Set_Robot_Speed(uint16_t robot_idx, uint16_t team_idx, float speed_left, float speed_right) override;
};