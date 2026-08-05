#include <uwbsot/PluginInterface.h>

#include "table_sensing.h"

#include <string_view>

namespace table_sensing {

	constexpr size_t config_parameter_count = 1;

	const char* config_parameters[config_parameter_count] = {
		"camera_name"
	};

	const char* config_parameter_descriptions[config_parameter_count] = {
		"Name of the camera to use"

	};

	NParameter_Type config_parameter_types[config_parameter_count] = {
		NParameter_Type::String
	};

	TPlugin_Descriptor descriptor = {
		.id = "table_sensing_plugin",
		.name = "Table Sensing Plugin",
		.description = "Table sensing plugin. This plugin connects to a real camera and provides sensing functionality on robots defined within this environment.",
		.type = NPlugin_Type::Sensing,
		.parameter_count = config_parameter_count,
		.config_parameters = config_parameters,
		.config_parameter_descriptions = config_parameter_descriptions,
		.config_parameter_types = config_parameter_types,
	};

}

extern "C" PLUGIN_INTERFACE_API NLib_Result Get_Plugin_Descriptors(TPlugin_Descriptor** descriptors, uint16_t* count) {
	*descriptors = &table_sensing::descriptor;
	*count = 1;

	return NLib_Result::Success;
}

extern "C" PLUGIN_INTERFACE_API NLib_Result Create_Plugin(NPlugin_Type type, const char* id, IPlugin** plugin_instance) {
	if (type == NPlugin_Type::Sensing && std::string_view(id) == table_sensing::descriptor.id) {
		*plugin_instance = new CTable_Sensing_Plugin();
		return NLib_Result::Success;
	}

	return NLib_Result::Not_Found;
}
