#pragma once

#include <string>

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

	// --listen-host <string>, or -l <string>
	std::string host = "localhost";
	// --listen-port <int>, or -p <int>
	int port = 3320;
};

/**
 * Parses CLI args, throws an exception when unknown or invalid arguments are provided
 */
TCLI_Args Parse_Args(int argc, char** argv);
