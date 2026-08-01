#pragma once

#include "Script.h"

#include <pybind11/embed.h>

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <uwbsot/PluginInterface.h>

namespace py = pybind11;

class CPython_Runtime {
	public:
		explicit CPython_Runtime(const std::filesystem::path& scriptsDirectory);

		CPython_Runtime(const CPython_Runtime&) = delete;
		CPython_Runtime& operator=(const CPython_Runtime&) = delete;

		CPython_Script& Create_Controller_Script(const std::string& scriptName, uint16_t team_index);
		CPython_Script& Create_Agent_Script(const std::string& scriptName, uint16_t team_index, uint16_t robot_index);

		void Initialize_All();
		void Update_All();

	protected:
		CPython_Script& Create_Script(const std::string& scriptName, uint16_t team_index = Team_Idx_Any, uint16_t robot_index = Robot_Idx_Any);

	private:
		py::scoped_interpreter mInterpreter;

		std::vector<std::unique_ptr<CPython_Script>> mScripts;
};