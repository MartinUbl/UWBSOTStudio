#include <iostream>
#include "arena.h"
#include "args.h"

void Print_Help() {
	std::cout << "Usage: program [options]\n";
	std::cout << "Options:\n";
	std::cout << "  --help                 Show this help message\n";
	std::cout << "  --teams <int>, -t <int>         Number of teams (must be > 0)\n";
	std::cout << "  --robots-per-team <int>, -r <int>  Number of robots per team (must be > 0)\n";
	std::cout << "  --arena-width <float>, -w <float>  Width of the arena (must be > 0)\n";
	std::cout << "  --arena-height <float>, -h <float> Height of the arena (must be > 0)\n";
	std::cout << "  --listen-host <string>, -l <string>     Host address (default: localhost)\n";
	std::cout << "  --listen-port <int>, -p <int>           Port number (default: 3320)\n";
}

int main(int argc, char** argv) {

	try {
		TCLI_Args args = Parse_Args(argc, argv);
		if (args.help) {
			Print_Help();
			return 0;
		}

		CArena arena(args);
		if (!arena.Start()) {
			std::cerr << "Failed to start the arena.\n";
			return 1;
		}

		return arena.Run();
	}
	catch (const std::invalid_argument& e) {
		std::cerr << "Error: " << e.what() << "\n\n";
		Print_Help();
		return 1;
	}

	return 0;
}
