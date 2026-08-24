#include "table_sensing.h"

#include <uwbsot/Utils.h>

#include <iostream>
#include <numbers>

#undef min
#undef max

namespace mock {
	constexpr size_t Teams_Count = 2;
	constexpr size_t Robots_Per_Team = 4;

	constexpr int Camera_Target_Width = 240;
	constexpr int Camera_Target_Height = 200;
}

NLib_Result CTable_Sensing_Plugin::Initialize(const char** config_parameters, const char** config_parameter_values, uint16_t parameter_count) {

	uwbsot::utils::CParameter_Map parameter_map(config_parameters, config_parameter_values, parameter_count);

	auto cameraIdentifier = parameter_map.get<std::string>("camera_name").value_or("");

	Pylon::PylonInitialize();

	Pylon::CDeviceInfo selectedCamera;
	Pylon::IPylonDevice* pDevice = nullptr;

	Pylon::DeviceInfoList_t devices;
	Pylon::CTlFactory::GetInstance().EnumerateDevices(devices);
	if (devices.size() == 0) {
		return NLib_Result::Not_Found;
	}

	bool foundCamera = false;
	if (!cameraIdentifier.empty()) {

		for (const auto& device : devices) {
			if (cameraIdentifier == device.GetFriendlyName().c_str() || cameraIdentifier == device.GetDeviceGUID().c_str() || cameraIdentifier == device.GetSerialNumber().c_str()) {
				selectedCamera = device;
				foundCamera = true;
				break;
			}
		}

		if (!foundCamera) {
			return NLib_Result::Not_Found;
		}

		pDevice = Pylon::CTlFactory::GetInstance().CreateDevice(selectedCamera);
	}
	else {
		pDevice = Pylon::CTlFactory::GetInstance().CreateFirstDevice();
	}

	if (!pDevice) {
		return NLib_Result::Not_Found;
	}

	mCamera.Attach(pDevice);

	mCamera.StartGrabbing();

	return NLib_Result::Success;
}

NLib_Result CTable_Sensing_Plugin::Shutdown() {
	if (mCamera.IsPylonDeviceAttached()) {
		mCamera.StopGrabbing();
		mCamera.Close();
	}

	return NLib_Result::Success;
}

const uint64_t CTable_Sensing_Plugin::Get_Logical_Clock() const {
	return 1;
}

uint16_t CTable_Sensing_Plugin::Get_Team_Count() const {

	// TODO: implement actual team count retrieval

	return static_cast<uint16_t>(mock::Teams_Count);
}

NLib_Result CTable_Sensing_Plugin::Get_Team_Sizes(uint16_t** sizes, uint16_t* sizes_count) const {

	// TODO: implement actual team sizes retrieval

	*sizes_count = static_cast<uint16_t>(mock::Teams_Count);

	*sizes = new uint16_t[*sizes_count];
	(*sizes)[0] = static_cast<uint16_t>(mock::Robots_Per_Team);
	(*sizes)[1] = static_cast<uint16_t>(mock::Robots_Per_Team);
	
	return NLib_Result::Success;
}

NLib_Result CTable_Sensing_Plugin::Get_Arena_Size(float* width, float* height, float* goal_width) const {

	// TODO: implement actual arena size retrieval

	*width = 220.0f;
	*height = 180.0f;
	*goal_width = 40.0f;

	return NLib_Result::Success;
}

NLib_Result CTable_Sensing_Plugin::Get_Robot_Positions(TRobot_Position** positions, uint16_t* positions_count, uint16_t team_idx) {

	// TODO: implement actual robot positions retrieval

	const size_t position_count = mock::Teams_Count * mock::Robots_Per_Team;

	*positions = new TRobot_Position[position_count];
	*positions_count = static_cast<uint16_t>(position_count);

	for (size_t i = 0; i < position_count; ++i) {
		(*positions)[i].robot_idx = static_cast<uint16_t>(i / 2);
		(*positions)[i].team_idx = static_cast<uint16_t>(i % 2);
		(*positions)[i].x = 0.0f;
		(*positions)[i].y = 0.0f;
		(*positions)[i].orientation = (*positions)[i].team_idx * std::numbers::pi_v<float>;
	}

	return NLib_Result::Success;
}

NLib_Result CTable_Sensing_Plugin::Get_Ball_Position(NBall_State* state, float* x, float* y, float* radius) {

	// TODO: implement actual ball position retrieval

	*state = NBall_State::Ground;
	*x = 110.0f;
	*y = 90.0f;
	*radius = 5.0f;

	return NLib_Result::Success;
}

NLib_Result CTable_Sensing_Plugin::Get_Camera_Image(void* rgbaPixels, int* width, int* height) {

	if (rgbaPixels == nullptr) {
		*width = mock::Camera_Target_Width;
		*height = mock::Camera_Target_Height;
		return NLib_Result::Success;
	}

	// check for fit
	if (*width * *height < mock::Camera_Target_Width * mock::Camera_Target_Height) {
		return NLib_Result::Invalid_Parameters;
	}

	// fill the image black
	for (int y = 0; y < mock::Camera_Target_Height; ++y) {
		for (int x = 0; x < mock::Camera_Target_Width; ++x) {
			uint8_t* pixel = reinterpret_cast<uint8_t*>(rgbaPixels) + (y * mock::Camera_Target_Width + x) * 4;
			pixel[0] = 0; // R
			pixel[1] = 0; // G
			pixel[2] = 0; // B
			pixel[3] = 255; // A
		}
	}

	// set the width and height
	*width = mock::Camera_Target_Width;
	*height = mock::Camera_Target_Height;

	if (!mCamera.IsOpen()) {
		return NLib_Result::Illegal_State;
	}

	Pylon::CGrabResultPtr ptrGrabResult;
	
	mCamera.RetrieveResult(5000, ptrGrabResult, Pylon::TimeoutHandling_ThrowException);

	if (ptrGrabResult->GrabSucceeded()) {
		Pylon::CImageFormatConverter converter;
		converter.OutputPixelFormat = Pylon::PixelType_RGBA8packed;
		converter.OutputBitAlignment = Pylon::OutputBitAlignment_MsbAligned;

		Pylon::CPylonImage rgbaImage;

		converter.Convert(rgbaImage, ptrGrabResult);

		auto* rgba = static_cast<std::uint8_t*>(rgbaImage.GetBuffer());

		// resample the image (rgba) to fit the output buffer
		for (int y = 0; y < mock::Camera_Target_Height; ++y) {
			for (int x = 0; x < mock::Camera_Target_Width; ++x) {
				const int srcX = x * rgbaImage.GetWidth() / mock::Camera_Target_Width;
				const int srcY = y * rgbaImage.GetHeight() / mock::Camera_Target_Height;
				const uint8_t* srcPixel = rgba + (srcY * rgbaImage.GetWidth() + srcX) * 4;
				uint8_t* const dstPixel = reinterpret_cast<uint8_t*>(rgbaPixels) + (y * mock::Camera_Target_Width + x) * 4;
				dstPixel[0] = srcPixel[0]; // R
				dstPixel[1] = srcPixel[1]; // G
				dstPixel[2] = srcPixel[2]; // B
				dstPixel[3] = srcPixel[3]; // A
			}
		}
	}

	// return success
	return NLib_Result::Success;
}
