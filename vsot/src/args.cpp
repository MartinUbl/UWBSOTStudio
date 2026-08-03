#include "args.h"
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace {

	bool Load_Simulation_Config(const std::string& path, TCLI_Args& args) {
		std::ifstream file(path);
		if (!file.is_open()) {
			std::cerr << "Failed to open simulation config file: " << path << std::endl;
			return false;
		}

		std::string line;
		while (std::getline(file, line)) {
			// Remove whitespace
			line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
			// Skip empty lines and comments
			if (line.empty() || line[0] == '#') {
				continue;
			}
			auto delimiter_pos = line.find('=');
			if (delimiter_pos == std::string::npos) {
				std::cerr << "Invalid line in config: " << line << std::endl;
				continue;
			}
			std::string key = line.substr(0, delimiter_pos);
			std::string value = line.substr(delimiter_pos + 1);
			if (key == "runs") {
				args.runs = std::stoi(value);
			}
			else if (key == "win-points") {
				args.win_points = std::stoi(value);
			}
			else if (key == "teams") {
				args.teams = std::stoi(value);
			}
			else if (key == "robots") {
				args.robots_per_team = std::stoi(value);
			}
			else if (key == "scripts-directory") {
				args.scripts_directory = value;
			}
			else if (key == "thread-count") {
				args.thread_count = std::stoi(value);
			}
			else if (key == "resolution-ms") {
				args.resolution_ms = std::stoi(value);
			}
			else if (key == "arena-width") {
				args.arena_width = std::stof(value);
			}
			else if (key == "arena-height") {
				args.arena_height = std::stof(value);
			}
			else if (key == "goal-width") {
				args.goal_width = std::stof(value);
			}
			else if (key.find("controllers[") == 0) {
				// Extract the index
				size_t start = key.find('[') + 1;
				size_t end = key.find(']');
				if (start != std::string::npos && end != std::string::npos && end > start) {
					int index = std::stoi(key.substr(start, end - start));
					if (index >= args.controllers.size()) {
						args.controllers.resize(index + 1);
					}
					args.controllers[index] = value;
				}
				else {
					std::cerr << "Invalid controller key in config: " << key << std::endl;
				}
			}
			else {
				std::cerr << "Unknown key in config: " << key << std::endl;
			}
		}
		return true;
	}

}

TCLI_Args Parse_Args(int argc, char** argv) {
	TCLI_Args args;

	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--help") {
			args.help = true;
		}
		else if (arg == "--teams" || arg == "-t") {
			if (i + 1 < argc) {
				args.teams = std::stoi(argv[++i]);
			}
			else {
				throw std::invalid_argument("Missing value for --teams");
			}
		}
		else if (arg == "--robots-per-team" || arg == "-r") {
			if (i + 1 < argc) {
				args.robots_per_team = std::stoi(argv[++i]);
			}
			else {
				throw std::invalid_argument("Missing value for --robots-per-team");
			}
		}
		else if (arg == "--arena-width" || arg == "-w") {
			if (i + 1 < argc) {
				args.arena_width = std::stof(argv[++i]);
			}
			else {
				throw std::invalid_argument("Missing value for --arena-width");
			}
		}
		else if (arg == "--arena-height" || arg == "-h") {
			if (i + 1 < argc) {
				args.arena_height = std::stof(argv[++i]);
			}
			else {
				throw std::invalid_argument("Missing value for --arena-height");
			}
		}
		else if (arg == "--goal-width" || arg == "-g") {
			if (i + 1 < argc) {
				args.goal_width = std::stof(argv[++i]);
			}
			else {
				throw std::invalid_argument("Missing value for --goal-width");
			}
		}
		else if (arg == "--listen-host" || arg == "-l") {
			if (i + 1 < argc) {
				args.host = argv[++i];
			}
			else {
				throw std::invalid_argument("Missing value for --listen-host");
			}
		}
		else if (arg == "--listen-port" || arg == "-p") {
			if (i + 1 < argc) {
				args.port = std::stoi(argv[++i]);
			}
			else {
				throw std::invalid_argument("Missing value for --listen-port");
			}
		}
		else if (arg == "--simulation-config" || arg == "-s") {
			if (i + 1 < argc) {
				args.simulation_config = argv[++i];
				args.is_simulation = true;
			}
			else {
				throw std::invalid_argument("Missing value for --simulation-config");
			}
		}
		else if (arg == "--resolution-ms" || arg == "-R") {
			if (i + 1 < argc) {
				args.resolution_ms = std::stoi(argv[++i]);
			}
			else {
				throw std::invalid_argument("Missing value for --resolution-ms");
			}
		}
		else {
			throw std::invalid_argument("Unknown argument: " + arg);
		}
	}

	if (args.is_simulation) {
		if (!Load_Simulation_Config(args.simulation_config, args)) {
			throw std::invalid_argument("Failed to load simulation config: " + args.simulation_config);
		}
	}

	if (args.teams <= 0) {
		throw std::invalid_argument("Number of teams must be greater than 0");
	}
	if (args.robots_per_team <= 0) {
		throw std::invalid_argument("Number of robots per team must be greater than 0");
	}
	if (args.arena_width <= 0.0f) {
		throw std::invalid_argument("Arena width must be greater than 0");
	}
	if (args.arena_height <= 0.0f) {
		throw std::invalid_argument("Arena height must be greater than 0");
	}
	if (args.goal_width <= 0.0f) {
		throw std::invalid_argument("Goal width must be greater than 0");
	}
	if (args.goal_width >= args.arena_height) {
		throw std::invalid_argument("Goal width must be less than arena height");
	}
	if (args.port <= 0 || args.port > 65535) {
		throw std::invalid_argument("Port number must be between 1 and 65535");
	}
	if (args.host.empty()) {
		throw std::invalid_argument("Host cannot be empty");
	}
	if (args.resolution_ms <= 0) {
		throw std::invalid_argument("Resolution must be greater than 0");
	}

	return args;
}
