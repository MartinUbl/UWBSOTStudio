#pragma once

#include <pybind11/embed.h>

#include <filesystem>
#include <string>
#include <utility>

namespace py = pybind11;

class CPython_Script {

	public:
		explicit CPython_Script(std::string scriptName, py::object instance);

		CPython_Script(const CPython_Script&) = delete;
		CPython_Script& operator=(const CPython_Script&) = delete;

		CPython_Script(CPython_Script&&) noexcept = default;
		CPython_Script& operator=(CPython_Script&&) noexcept = default;

		void Initialize();
		void Update();

		[[nodiscard]]
		const std::string& Get_Name() const noexcept;

	private:
		std::string mScript_Name;
		py::object mInstance;
};
