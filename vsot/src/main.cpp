#include <iostream>
#include "arena.h"
#include "args.h"

#include <spdlog/spdlog.h>
#include <csignal>

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

	signal(SIGINT, [](int signo) {
		std::cout << "SIGINT received, exiting...\n";
		std::exit(0);
	});

	try {
		TCLI_Args args = Parse_Args(argc, argv);
		if (args.help) {
			Print_Help();
			return 0;
		}

		if (args.is_simulation) {
			// simulation run:
			py::initialize_interpreter();

			std::vector<std::unique_ptr<std::thread>> mThreads;

			// GIL release scope
			{
				py::gil_scoped_release release_main_gil;

				for (size_t i = 0; i < args.thread_count; i++) {
					mThreads.push_back(std::make_unique<std::thread>([&, i]() {

						for (size_t run = 0; run < args.runs; run++) {
							spdlog::info("[{}] Starting run {}", i, run + 1);

							CArena arena(args, i);
							{
								py::gil_scoped_acquire gil;
								if (!arena.Start()) {
									spdlog::error("[{}] Failed to start the arena for run {}", i, run + 1);
									return;
								}
							}

							int result = arena.Run();
							if (result != 0) {
								spdlog::error("[{}] Arena run {} exited with error code {}", i, run + 1, result);
							}
						}
						}));
				}

				for (auto& thread : mThreads) {
					if (thread->joinable()) {
						thread->join();
					}
				}
			}

			py::finalize_interpreter();

			return 0;
		}
		else {
			// daemon run:

			CArena arena(args);
			if (!arena.Start()) {
				std::cerr << "Failed to start the arena.\n";
				return 1;
			}

			return arena.Run();
		}
	}
	catch (const std::invalid_argument& e) {
		std::cerr << "Error: " << e.what() << "\n\n";
		Print_Help();
		return 1;
	}

	return 0;
}
