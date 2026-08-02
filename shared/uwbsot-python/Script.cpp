#include "Script.h"

CPython_Script::CPython_Script(std::string scriptName, py::object instance)
	: mScript_Name(std::move(scriptName)), mInstance(std::move(instance)) {
}


void CPython_Script::Initialize()
{
	mInstance.attr("initialize")();
}

void CPython_Script::Update()
{
	mInstance.attr("update")();
}

const std::string& CPython_Script::Get_Name() const noexcept
{
	return mScript_Name;
}
