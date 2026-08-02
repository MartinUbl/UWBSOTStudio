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

/**
 * Result codes for the plugin interface
 */
enum class NLib_Result : uint32_t {
	Success,				// OK
	Unsupported,			// unsupported operation on given object

	Invalid_Parameters,		// invalid parameters passed to function
	Illegal_State,			// illegal state of object, e.g. not initialized
	Not_Found,				// requested item not found
	Out_Of_Memory,			// the program ran out of memory during the operation
};

/**
 * Types of parameters that can be used in plugin configuration
 */
enum class NParameter_Type : uint8_t {
	None,

	// basic types

	String,
	Integer,
	Boolean,
	Float,

	// special types (always derived from basic types; adds validation or purpose)

	Meta_Serial_Port,		// [String] a serial port name, e.g. COM1, /dev/ttyUSB0, etc.
};

/**
 * Types of plugins in the system
 */
enum class NPlugin_Type : uint16_t {
	Sensing,
	Control,

	None,
};

/**
 * States of the ball detected by the sensing plugin
 */
enum class NBall_State : uint8_t {
	Not_Found,				// the detector did not find the ball in the current frame
	Ground,					// the ball is on the ground
	Lifted,					// the ball is visually found, but lifted from the ground
};

/**
 * Descriptor for a plugin, containing its metadata and configuration parameters
 */
struct TPlugin_Descriptor {
	const char* id;									// unique identifier for the plugin
	const char* name;								// human-readable name of the plugin
	const char* description;						// human-readable description of the plugin

	NPlugin_Type type;								// type of the plugin (Sensing, Control, etc.)

	uint16_t parameter_count;						// number of configuration parameters for the plugin
	const char** config_parameters;					// array of configuration parameter names
	const char** config_parameter_descriptions;		// array of configuration parameter descriptions
	NParameter_Type* config_parameter_types;		// array of configuration parameter types
};

/**
 * Internal representation of a plugin descriptor, used for easier manipulation in code
 */
struct TPlugin_Descriptor_Internal {
	std::string id;
	std::string name;
	std::string description;

	NPlugin_Type type = NPlugin_Type::None;

	struct TParameter {
		std::string name;
		std::string description;
		NParameter_Type type = NParameter_Type::None;
	} parameter;

	std::vector<TParameter> parameters;
};

// "any" may be used as "all" selector or "unknown" value for robot and team indices in several situations

constexpr uint16_t Robot_Idx_Any = std::numeric_limits<uint16_t>::max();
constexpr uint16_t Team_Idx_Any = std::numeric_limits<uint16_t>::max();
constexpr float Position_Unknown = std::numeric_limits<float>::quiet_NaN();

/**
 * Represents the position and orientation of a robot in the environment
 */
struct TRobot_Position {
	uint16_t robot_idx = Robot_Idx_Any;
	float x = Position_Unknown;
	float y = Position_Unknown;
	float orientation = Position_Unknown;
	uint16_t team_idx = Team_Idx_Any;
};

/**
 * Interface for a plugin, providing methods for initialization and shutdown
 */
class IPlugin {
	public:
		virtual ~IPlugin() = default;

		/**
		 * Initializes the plugin with the provided configuration parameters. The config_parameters and config_parameter_values arrays must have a length equal
		 * to parameter_count, and the parameters must match those defined in the plugin descriptor.
		 */
		virtual NLib_Result Initialize(const char** config_parameters, const char** config_parameter_values, uint16_t parameter_count) = 0;

		/**
		 * Shuts down the plugin and releases any resources it holds.
		 */
		virtual NLib_Result Shutdown() = 0;
};

/**
 * Interface for a sensing plugin, providing methods to retrieve information about the environment and detected objects
 */
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

/**
 * Interface for a control plugin, providing methods to control the robots in the environment
 */
class IControl_Plugin : public IPlugin {
	public:
		/**
		 * Sets robot speed; speed is in an interval of <-1; 1>, -1 meaning full backward speed, 1 meaning full forward speed
		 * robot_idx and team_idx must be set to a valid value, _Any value is not accepted
		 */
		virtual NLib_Result Set_Robot_Speed(uint16_t robot_idx, uint16_t team_idx, float speed_left, float speed_right) = 0;
};

// Function names for plugin operations

constexpr const char* Fnc_Name_Get_Plugin_Descriptors = "Get_Plugin_Descriptors";
constexpr const char* Fnc_Name_Create_Plugin = "Create_Plugin";

// Function pointer types for plugin operations

using TGet_Plugin_Descriptors = NLib_Result(*)(TPlugin_Descriptor** descriptors, uint16_t* count);
using TCreate_Plugin = NLib_Result(*)(NPlugin_Type type, const char* name, IPlugin** plugin_instance);
