#pragma once

#include "Script.h"

#include <pybind11/embed.h>
#include <pybind11/subinterpreter.h>

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <uwbsot/PluginInterface.h>

namespace py = pybind11;

enum class NScript_Type {
	None,
	Agent,
	Controller
};

struct TScript_Entry {
	std::string id;
	std::string name;
	NScript_Type type;
	std::filesystem::path path;
};

/**
 * CPython_UWBSOT_API is an abstract interface that defines the methods for interacting with the Python scripts in the UWB SOT environment
 */
class CPython_UWBSOT_API {
	public:
		virtual ~CPython_UWBSOT_API() = default;

		/**
		 * Get the number of robots in a given team
		 */
		virtual int Get_Robot_Count(size_t team_index) const = 0;

		/**
		 * Set the script for a given robot in a team; the script is the "name" field in script headers
		 */
		virtual void Set_Robot_Script(size_t team_index, size_t robot_index, const std::string& script_id) = 0;

		/**
		 * Get the elapsed time in milliseconds since the start of the controller operation
		 */
		virtual long long Get_Elapsed_Time_Milliseconds() const = 0;

		/**
		 * Set the speed of a given robot in a team; speed_left and speed_right are the speeds of the left and right wheels, respectively
		 */
		virtual void Set_Robot_Speed(size_t team_index, size_t robot_index, float speed_left, float speed_right) = 0;

		/**
		 * Get the position of a given robot in a team; returns a tuple (x, y, orientation)
		 */
		virtual py::tuple Get_Robot_Position(size_t team_index, size_t robot_index) const = 0;

		/**
		 * Get the position of the ball; returns a tuple (x, y, state)
		 */
		virtual py::tuple Get_Ball_Position() const = 0;
};

/**
 * CPython_Runtime is responsible for managing the Python interpreter and scripts.
 * It initializes the interpreter, loads scripts, and provides methods to create and manage controller and agent scripts.
 */
class CPython_Runtime {
	private:
		py::subinterpreter mSubInterpreter;

		std::vector<std::unique_ptr<CPython_Script>> mScripts;

	public:
		explicit CPython_Runtime(const std::filesystem::path& scriptsDirectory);
		~CPython_Runtime();

		CPython_Runtime(const CPython_Runtime&) = delete;
		CPython_Runtime& operator=(const CPython_Runtime&) = delete;

		/**
		 * Create a controller script instance for a given script name and team index.
		 * Returns a reference to the created CPython_Script instance.
		 */
		CPython_Script& Create_Controller_Script(const std::string& scriptName, uint16_t team_index, bool initialize = false);

		/**
		 * Create an agent script instance for a given script name, team index, and robot index.
		 * Returns a reference to the created CPython_Script instance.
		 */
		CPython_Script& Create_Agent_Script(const std::string& scriptName, uint16_t team_index, uint16_t robot_index, bool initialize = false);

		/**
		 * Inject the CPython_UWBSOT_API interface into the Python environment, allowing scripts to interact with the UWB SOT environment.
		 */
		void Inject_API(CPython_UWBSOT_API& api);

		void Initialize_All();
		void Update_All();

		static bool Load_Script_List(const std::filesystem::path& scriptsDirectory, std::vector<TScript_Entry>& outScriptList);

	protected:
		/**
		 * Create a script instance for a given script name, team index, and robot index.
		 * Returns a reference to the created CPython_Script instance.
		 */
		CPython_Script& Create_Script(const std::string& scriptName, uint16_t team_index = Team_Idx_Any, uint16_t robot_index = Robot_Idx_Any, bool initialize = false);
};