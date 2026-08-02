#pragma once

#include <cstdint>
#include <limits>
#include <vector>
#include <string>

#ifdef WIN32
	#define PLUGIN_INTERFACE_API __declspec(dllexport)

	#undef max
	#undef min
#endif

enum class NLib_Result : uint32_t {
	Success,
	Unsupported,

	Invalid_Parameters,
	Illegal_State,
	Not_Found,
	Out_Of_Memory,
};

enum class NParameter_Type : uint8_t {
	None,

	String,
	Integer,
	Boolean,
	Float,

	Meta_Serial_Port,
};

enum class NPlugin_Type : uint16_t {
	Sensing,
	Control,

	None,
};

enum class NBall_State : uint8_t {
	Not_Found,	// the detector did not find the ball in the current frame
	Ground,		// the ball is on the ground
	Lifted,		// the ball is visually found, but lifted from the ground
};

struct TPlugin_Descriptor {
	const char* id;
	const char* name;
	const char* description;

	NPlugin_Type type;

	uint16_t parameter_count;
	const char** config_parameters;
	const char** config_parameter_descriptions;
	NParameter_Type* config_parameter_types;
};

struct TPlugin_Descriptor_Internal {
	std::string id;
	std::string name;
	std::string description;

	NPlugin_Type type;

	struct TParameter {
		std::string name;
		std::string description;
		NParameter_Type type;
	} parameter;

	std::vector<TParameter> parameters;
};

constexpr const char* Fnc_Name_Get_Plugin_Descriptors = "Get_Plugin_Descriptors";
constexpr const char* Fnc_Name_Create_Plugin = "Create_Plugin";

using TGet_Plugin_Descriptors = NLib_Result (*)(TPlugin_Descriptor** descriptors, uint16_t* count);

class IPlugin {
	public:
		virtual ~IPlugin() = default;

		/**
		 * Initializes the plugin with the provided configuration parameters. The config_parameters and config_parameter_values arrays must have a length equal to parameter_count, and the parameters must match those defined in the plugin descriptor.
		 */
		virtual NLib_Result Initialize(const char** config_parameters, const char** config_parameter_values, uint16_t parameter_count) = 0;

		/**
		 * Shuts down the plugin and releases any resources it holds.
		 */
		virtual NLib_Result Shutdown() = 0;
};

enum class NSensing_Plugin_State : uint8_t {
	Not_Started,
	Initializing,
	Operational,
	Failed,
};

constexpr uint16_t Robot_Idx_Any = std::numeric_limits<uint16_t>::max();
constexpr uint16_t Team_Idx_Any = std::numeric_limits<uint16_t>::max();
constexpr float Position_Unknown = std::numeric_limits<float>::quiet_NaN();

struct TRobot_Position {
	uint16_t robot_idx = Robot_Idx_Any;
	float x = Position_Unknown;
	float y = Position_Unknown;
	float orientation = Position_Unknown;
	uint16_t team_idx = Team_Idx_Any;
};

class ISensing_Plugin : public IPlugin {
	public:
		/**
		 * Retrieves logical clock value - this value is incremented by the sensing plugin every time it updates its internal state,
		 * and can be used to determine if the data retrieved from the plugin is up-to-date
		 */
		virtual const uint64_t Get_Logical_Clock() const = 0;

		/**
		 * Retrieves the number of teams detected by the sensing plugin.
		 */
		virtual uint16_t Get_Team_Count() const = 0;

		/**
		 * Retrieves the number of robots detected for each team. The sizes_count must be obtained through Get_Team_Count.
		 */
		virtual NLib_Result Get_Team_Sizes(uint16_t** sizes, uint16_t* sizes_count) const = 0;

		/**
		 * Retrieves the arena size detected by the sensing plugin. The arena size is represented as a pair of floats (width, height) and the goal width as a float
		 */
		virtual NLib_Result Get_Arena_Size(float* width, float* height, float* goal_width) const = 0;

		/**
		 * Retrieves the positions of the robots. The positions_count must be obtained through Get_Team_Sizes.
		 * The team_idx can be Team_Idx_Any to retrieve all robots, or a specific team index to retrieve only that team's robots.
		 */
		virtual NLib_Result Get_Robot_Positions(TRobot_Position** positions, uint16_t* positions_count, uint16_t team_idx = Team_Idx_Any) = 0;

		/**
		 * Retrieves the position and state of the ball. The state indicates whether the ball is on the ground, lifted, or not found.
		 */
		virtual NLib_Result Get_Ball_Position(NBall_State* state, float* x, float* y, float* radius) = 0;

		/**
		 * Retrieves the camera image from the sensing plugin.
		 * If rgbaPixels is nullptr, only width and height will be set to the dimensions of the image
		 * The caller is responsible for allocating and freeing the memory for rgbaPixels. The image is returned in RGBA format, with 4 bytes per pixel.
		 */
		virtual NLib_Result Get_Camera_Image(void* rgbaPixels, int* width, int* height) { return NLib_Result::Unsupported; }
};

class IControl_Plugin : public IPlugin {
	public:
		/**
		 * Sets robot speed; speed is in an interval of <-1; 1>, -1 meaning full backward speed, 1 meaning full forward speed
		 * robot_idx and team_idx must be set to a valid value, _Any value is not accepted
		 */
		virtual NLib_Result Set_Robot_Speed(uint16_t robot_idx, uint16_t team_idx, float speed_left, float speed_right) = 0;
};

using TCreate_Plugin = NLib_Result(*)(NPlugin_Type type, const char* name, IPlugin** plugin_instance);
