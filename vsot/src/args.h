#pragma once

#include <string>
#include <vector>

struct TCLI_Args {
	// --help
	bool help = false;

	// --teams <int>, or -t <int>
	int teams = 0;
	// --robots-per-team <int>, or -r <int>
	int robots_per_team = 0;

	// --arena-width <float>, or -w <float>
	float arena_width = 0.0f;
	// --arena-height <float>, or -h <float>
	float arena_height = 0.0f;

	// --goal-width <float>, or -g <float>
	float goal_width = 40.0f;

	// --listen-host <string>, or -l <string>
	std::string host = "localhost";
	// --listen-port <int>, or -p <int>
	int port = 3320;

	// --simulation-config <string> or -s <string>
	std::string simulation_config;

	// --resolution-ms <int> or -R <int>
	int resolution_ms = 5;


	// the following parameters will be loaded from the simulation config, if provided
	bool is_simulation = false;
	int runs = 0;
	int win_points = 0;
	std::vector<std::string> controllers;
	std::string scripts_directory;
	int thread_count = 1;
};

/**
 * Parses CLI args, throws an exception when unknown or invalid arguments are provided
 */
TCLI_Args Parse_Args(int argc, char** argv);
