#include <uwbsot/PluginInterface.h>
#include "vsot_control.h"

namespace vsot_control {

	constexpr size_t config_parameter_count = 2;

	const char* config_parameters[config_parameter_count] = {
		"env_address",
		"env_port"
	};

	const char* config_parameter_descriptions[config_parameter_count] = {
		"IP/host of VSOT environment",
		"Port of VSOT environment"
	};

	NParameter_Type config_parameter_types[config_parameter_count] = {
		NParameter_Type::String,
		NParameter_Type::Integer
	};

	TPlugin_Descriptor descriptor = {
		.id = "vsot_control_plugin",
		.name = "VSOT Control Plugin",
		.description = "Virtual SOT (VSOT) control plugin. This plugin connects to the virtual SOT environment and provides control functionality on robots defined within this environment.",
		.type = NPlugin_Type::Control,
		.parameter_count = config_parameter_count,
		.config_parameters = config_parameters,
		.config_parameter_descriptions = config_parameter_descriptions,
		.config_parameter_types = config_parameter_types,
	};

}

extern "C" PLUGIN_INTERFACE_API NLib_Result Get_Plugin_Descriptors(TPlugin_Descriptor** descriptors, uint16_t* count) {
	*descriptors = &vsot_control::descriptor;
	*count = 1;

	return NLib_Result::Success;
}

extern "C" PLUGIN_INTERFACE_API NLib_Result Create_Plugin(NPlugin_Type type, const char* id, IPlugin** plugin_instance) {
	if (type == NPlugin_Type::Control && std::string(id) == vsot_control::descriptor.id) {
		*plugin_instance = new CVSOT_Control_Plugin();
		return NLib_Result::Success;
	}
	return NLib_Result::Not_Found;
}
