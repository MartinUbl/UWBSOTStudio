#include "Runtime.h"

#include <fstream>
#include <sstream>

CPython_Runtime::CPython_Runtime(const std::filesystem::path& scriptsDirectory)
	: mInterpreter{ } {

	const std::filesystem::path absolutePath = std::filesystem::absolute(scriptsDirectory);

	if (!std::filesystem::is_directory(absolutePath)) {
		throw std::runtime_error("Python script directory does not exist: " + absolutePath.string());
	}

	py::module_ sys = py::module_::import("sys");

	sys.attr("path").attr("insert")(0, absolutePath.string());
}

CPython_Script& CPython_Runtime::Create_Script(const std::string& scriptName, uint16_t team_index, uint16_t robot_index) {

	try {
		py::module_ module = py::module_::import(scriptName.c_str());

		const std::string className = (robot_index == Robot_Idx_Any) ? "Controller" : "Agent";

		if (!py::hasattr(module, className.c_str())) {
			throw std::runtime_error("Python module '" + scriptName + "' does not contain class '" + className + "'");
		}

		py::object scriptClass = module.attr(className.c_str());

		// calls the constructor: if robot_index is Robot_Idx_Any, call with team_index only, else call with both team_index and robot_index
		py::object instance = (robot_index == Robot_Idx_Any) ? scriptClass(team_index) : scriptClass(team_index, robot_index);

		if (!py::hasattr(instance, "initialize")) {
			throw std::runtime_error("Python class '" + className + "' has no initialize() method");
		}

		if (!py::hasattr(instance, "update")) {
			throw std::runtime_error("Python class '" + className + "' has no update() method");
		}

		auto script = std::make_unique<CPython_Script>(scriptName, std::move(instance));

		CPython_Script& result = *script;
		mScripts.push_back(std::move(script));

		return result;
	}
	catch (const py::error_already_set& error) {
		throw std::runtime_error("Failed to load Python script '" + scriptName + "':\n" + error.what());
	}
}

CPython_Script& CPython_Runtime::Create_Controller_Script(const std::string& scriptName, uint16_t team_index) {
	return Create_Script(scriptName, team_index);
}

CPython_Script& CPython_Runtime::Create_Agent_Script(const std::string& scriptName, uint16_t team_index, uint16_t robot_index) {
	return Create_Script(scriptName, team_index, robot_index);
}

void CPython_Runtime::Initialize_All() {
	for (const auto& script : mScripts) {
		try {
			script->Initialize();
		}
		catch (const py::error_already_set& error) {
			throw std::runtime_error("initialize() failed in script '" + script->Get_Name() + "':\n" + error.what());
		}
	}
}

void CPython_Runtime::Update_All() {
	for (const auto& script : mScripts) {
		try {
			script->Update();
		}
		catch (const py::error_already_set& error) {
			throw std::runtime_error("update() failed in script '" + script->Get_Name() + "':\n" + error.what());
		}
	}
}

bool CPython_Runtime::Load_Script_List(const std::filesystem::path& scriptsDirectory, std::vector<TScript_Entry>& outScriptList) {
	if (!std::filesystem::is_directory(scriptsDirectory)) {
		return false;
	}

	outScriptList.clear();
	if (std::filesystem::exists(scriptsDirectory) && std::filesystem::is_directory(scriptsDirectory)) {
		for (const auto& entry : std::filesystem::directory_iterator(scriptsDirectory)) {
			if (entry.is_regular_file() && entry.path().extension() == ".py") {

				// open the file, read lines while they start with #, and if the line starts with "#NAME: ", use the rest of the line as the name, otherwise use the filename
				std::ifstream ifs(entry.path());
				std::string name = entry.path().filename().string();
				std::string id = entry.path().stem().string();
				NScript_Type type = NScript_Type::Agent;

				// Read lines starting with '#' to find a name
				std::string line;
				while (std::getline(ifs, line)) {
					if (line.empty() || line[0] != '#') {
						break; // Stop if the line doesn't start with '#'
					}
					if (line.rfind("#NAME: ", 0) == 0) { // Check if the line starts with "#NAME: "
						name = line.substr(7); // Extract the name after "#NAME: "
					}
					if (line.rfind("#ID: ", 0) == 0) { // Check if the line starts with "#ID: "
						id = line.substr(5); // Extract the id after "#ID: "
					}
					if (line.rfind("#TYPE: ", 0) == 0) { // Check if the line starts with "#TYPE: "
						std::string stype = line.substr(7); // Extract the type after "#TYPE: "
						if (stype == "agent") {
							type = NScript_Type::Agent;
						}
						else if (stype == "controller") {
							type = NScript_Type::Controller;
						}
						else {
							type = NScript_Type::None;
						}
					}
				}

				if (type == NScript_Type::None) {
					continue;
				}

				outScriptList.push_back({ id, name, type, entry.path() });
			}
		}
	}
	return true;
}
