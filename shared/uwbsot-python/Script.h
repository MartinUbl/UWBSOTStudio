#pragma once

#include <pybind11/embed.h>
#include <pybind11/subinterpreter.h>

#include <filesystem>
#include <string>
#include <utility>

namespace py = pybind11;

/**
 * CPython_Script represents a Python script instance in the UWB SOT environment.
 * It holds the script name and the corresponding Python object instance
 */
class CPython_Script {
	private:
		std::string mScript_Name;
		py::object mInstance;

		py::detail::str_attr_accessor mInitialize_Method;
		py::detail::str_attr_accessor mUpdate_Method;

	public:
		explicit CPython_Script(std::string scriptName, py::object instance);

		CPython_Script(const CPython_Script&) = delete;
		CPython_Script& operator=(const CPython_Script&) = delete;

		CPython_Script(CPython_Script&&) noexcept = default;
		CPython_Script& operator=(CPython_Script&&) noexcept = default;

		/**
		 * Initialize the script by calling its initialize() method
		 */
		void Initialize(py::subinterpreter& subinterpreter);

		/**
		 * Initialize the script by calling its initialize() method without a subinterpreter
		 */
		void Initialize();

		/**
		 * Update the script by calling its update() method
		 */
		void Update(py::subinterpreter& subinterpreter);

		/**
		 * Update the script by calling its update() method without a subinterpreter
		 */
		void Update();
		
		/**
		 * Get the name of the script
		 */
		[[nodiscard]]
		const std::string& Get_Name() const noexcept;
};
