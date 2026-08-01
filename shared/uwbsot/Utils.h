#pragma once

#include <map>
#include <string>
#include <optional>
#include <filesystem>

namespace uwbsot {
	namespace utils {

		class CParameter_Map {
			public:
				CParameter_Map(const char** names, const char** values, size_t size) {
					for (size_t i = 0; i < size; ++i) {
						mParameter_Map[names[i]] = values[i];
					}
				}

				template<typename T>
				std::optional<T> get(const std::string& name) const {
					auto it = mParameter_Map.find(name);
					if (it != mParameter_Map.end()) {
						if constexpr (std::is_same_v<T, std::string>) {
							return it->second;
						}
						else if constexpr (std::is_integral_v<T>) {
							if constexpr (std::is_signed_v<T>) {
								return static_cast<T>(std::stoll(it->second));
							}
							else {
								return static_cast<T>(std::stoull(it->second));
							}
						}
						else if constexpr (std::is_floating_point_v<T>) {
							return static_cast<T>(std::stod(it->second));
						}
					}
					return std::nullopt;
				}

			private:
				std::map<std::string, std::string> mParameter_Map;
		};


		std::filesystem::path GetAppDataDirectory(const std::string& appName);
	}
}
