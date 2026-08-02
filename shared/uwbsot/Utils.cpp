#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#else
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace uwbsot {
	namespace utils {

		std::filesystem::path GetAppDataDirectory(const std::string& appName)
		{
			namespace fs = std::filesystem;

			if (appName.empty()) {
				throw std::invalid_argument("Application name must not be empty.");
			}

			fs::path directory;

#if defined(_WIN32)

			PWSTR appDataPath = nullptr;

			const HRESULT result = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &appDataPath);

			if (FAILED(result) || !appDataPath) {
				throw std::runtime_error("Could not determine the Windows AppData directory.");
			}

			directory = fs::path(appDataPath) / fs::u8path(appName);
			CoTaskMemFree(appDataPath);

#elif defined(__APPLE__)

			const char* home = std::getenv("HOME");

			if (!home || *home == '\0') {
				const passwd* user = getpwuid(getuid());
				home = user != nullptr ? user->pw_dir : nullptr;
			}

			if (!home || *home == '\0') {
				throw std::runtime_error("Could not determine the user's home directory.");
			}

			directory = fs::path(home) / "Library" / "Application Support" / fs::u8path(appName);

#elif defined(__linux__) || defined(__unix__)

			const char* xdgDataHome = std::getenv("XDG_DATA_HOME");

			if (xdgDataHome != nullptr && *xdgDataHome != '\0') {
				directory = fs::path(xdgDataHome) / fs::u8path(appName);
			}
			else {
				const char* home = std::getenv("HOME");

				if (!home || *home == '\0') {
					const passwd* user = getpwuid(getuid());
					home = user != nullptr ? user->pw_dir : nullptr;
				}

				if (!home || *home == '\0') {
					throw std::runtime_error("Could not determine the user's home directory.");
				}

				directory = fs::path(home) / ".local" / "share" / fs::u8path(appName);
			}

#else
#error Unsupported platform
#endif

			std::error_code error;
			fs::create_directories(directory, error);

			if (error) {
				throw fs::filesystem_error("Could not create the application data directory.", directory, error);
			}

			if (!fs::is_directory(directory, error) || error) {
				throw fs::filesystem_error("Application data path is not a directory.", directory, error);
			}

			return directory;
		}

	}
}
