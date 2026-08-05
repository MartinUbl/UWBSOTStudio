#pragma once

#include <uwbsot/PluginInterface.h>

#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

class CTable_Sensing_Plugin : public ISensing_Plugin {

	private:
		Pylon::CInstantCamera mCamera;

	public:
		// IPlugin iface
		virtual NLib_Result Initialize(const char** config_parameters, const char** config_parameter_values, uint16_t parameter_count) override;
		virtual NLib_Result Shutdown() override;

		// ISensing_Plugin iface
		virtual const uint64_t Get_Logical_Clock() const override;
		virtual uint16_t Get_Team_Count() const override;
		virtual NLib_Result Get_Team_Sizes(uint16_t** sizes, uint16_t* sizes_count) const override;
		virtual NLib_Result Get_Arena_Size(float* width, float* height, float* goal_width) const override;
		virtual NLib_Result Get_Robot_Positions(TRobot_Position** positions, uint16_t* positions_count, uint16_t team_idx = Team_Idx_Any) override;
		virtual NLib_Result Get_Ball_Position(NBall_State* state, float* x, float* y, float* radius) override;
		virtual NLib_Result Get_Camera_Image(void* rgbaPixels, int* width, int* height) override;
};
