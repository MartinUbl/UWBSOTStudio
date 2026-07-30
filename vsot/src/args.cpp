#include "args.h"
#include <iostream>
#include <stdexcept>

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
		else {
			throw std::invalid_argument("Unknown argument: " + arg);
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
	if (args.port <= 0 || args.port > 65535) {
		throw std::invalid_argument("Port number must be between 1 and 65535");
	}
	if (args.host.empty()) {
		throw std::invalid_argument("Host cannot be empty");
	}

	return args;
}
