#include <uwbsot/PluginInterface.h>

#include "vsot_sensing.h"

#include <string_view>

namespace vsot_sensing {

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
		.id = "vsot_sensing_plugin",
		.name = "VSOT Sensing Plugin",
		.description = "Virtual SOT (VSOT) sensing plugin",
		.type = NPlugin_Type::Sensing,
		.parameter_count = config_parameter_count,
		.config_parameters = config_parameters,
		.config_parameter_descriptions = config_parameter_descriptions,
		.config_parameter_types = config_parameter_types,
	};

}

extern "C" PLUGIN_INTERFACE_API NLib_Result Get_Plugin_Descriptors(TPlugin_Descriptor** descriptors, uint16_t* count) {
	*descriptors = &vsot_sensing::descriptor;
	*count = 1;

	return NLib_Result::Success;
}

extern "C" PLUGIN_INTERFACE_API NLib_Result Create_Plugin(NPlugin_Type type, const char* id, IPlugin** plugin_instance) {
	if (type == NPlugin_Type::Sensing && std::string_view(id) == vsot_sensing::descriptor.id) {
		*plugin_instance = new CVSOT_Sensing_Plugin();
		return NLib_Result::Success;
	}

	return NLib_Result::Not_Found;
}
