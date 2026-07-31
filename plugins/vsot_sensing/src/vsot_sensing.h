#pragma once

#include <uwbsot/PluginInterface.h>
#include <vsotnet/Protocol.h>

class CVSOT_Sensing_Plugin : public ISensing_Plugin {

	private:
		std::string mEnv_Address;
		uint16_t mEnv_Port = 0;

		sockaddr_in mAddr{};
		SOCK mSocket = INVALID_SOCKET_VALUE;

		// only for dummy camera image generation
		struct TCached_Robot_Position {
			float x;
			float y;
			uint16_t team_idx;
		};

		std::vector<TCached_Robot_Position> mCached_Robot_Positions;

	protected:
		bool Send_Metadata_Request(vsot::TMetadata_Response** response) const;

	public:
		// IPlugin iface
		virtual NLib_Result Initialize(const char** config_parameters, const char** config_parameter_values, uint16_t parameter_count) override;
		virtual NLib_Result Shutdown() override;

		// ISensing_Plugin iface
		virtual const uint64_t Get_Logical_Clock() const override;
		virtual uint16_t Get_Team_Count() const override;
		virtual NLib_Result Get_Team_Sizes(uint16_t** sizes, uint16_t* sizes_count) const override;
		virtual NLib_Result Get_Arena_Size(float* width, float* height) const override;
		virtual NLib_Result Get_Robot_Positions(TRobot_Position** positions, uint16_t* positions_count, uint16_t team_idx = Team_Idx_Any) override;
		virtual NLib_Result Get_Camera_Image(void* rgbaPixels, int* width, int* height) override;
};
