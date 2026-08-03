#include "Script.h"

CPython_Script::CPython_Script(std::string scriptName, py::object instance)
	: mScript_Name(std::move(scriptName)), mInstance(std::move(instance)),
	  mInitialize_Method(mInstance.attr("initialize")), mUpdate_Method(mInstance.attr("update")) {

	if (!py::hasattr(mInstance, "initialize")) {
		throw std::runtime_error("Python class instance has no initialize() method");
	}

	if (!py::hasattr(mInstance, "update")) {
		throw std::runtime_error("Python class instance has no update() method");
	}
}


void CPython_Script::Initialize(py::subinterpreter& subinterpreter)
{
	py::subinterpreter_scoped_activate guard(subinterpreter);

	Initialize();
}

void CPython_Script::Initialize()
{
	mInitialize_Method();
}

void CPython_Script::Update(py::subinterpreter& subinterpreter)
{
	py::subinterpreter_scoped_activate guard(subinterpreter);

	Update();
}

void CPython_Script::Update()
{
	mUpdate_Method();
}

const std::string& CPython_Script::Get_Name() const noexcept
{
	return mScript_Name;
}
