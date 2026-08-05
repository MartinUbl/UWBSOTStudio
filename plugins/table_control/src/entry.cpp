#include <uwbsot/PluginInterface.h>
#include "table_control.h"

namespace table_control {

	constexpr size_t config_parameter_count = 1;

	const char* config_parameters[config_parameter_count] = {
		"serial_port"
	};

	const char* config_parameter_descriptions[config_parameter_count] = {
		"Serial port of control dongle"
	};

	NParameter_Type config_parameter_types[config_parameter_count] = {
		NParameter_Type::String
	};

	TPlugin_Descriptor descriptor = {
		.id = "table_control_plugin",
		.name = "Table Control Plugin",
		.description = "Table control plugin. This plugin connects to the table control environment and provides control functionality on robots defined within this environment.",
		.type = NPlugin_Type::Control,
		.parameter_count = config_parameter_count,
		.config_parameters = config_parameters,
		.config_parameter_descriptions = config_parameter_descriptions,
		.config_parameter_types = config_parameter_types,
	};

}

extern "C" PLUGIN_INTERFACE_API NLib_Result Get_Plugin_Descriptors(TPlugin_Descriptor** descriptors, uint16_t* count) {
	*descriptors = &table_control::descriptor;
	*count = 1;

	return NLib_Result::Success;
}

extern "C" PLUGIN_INTERFACE_API NLib_Result Create_Plugin(NPlugin_Type type, const char* id, IPlugin** plugin_instance) {
	if (type == NPlugin_Type::Control && std::string(id) == table_control::descriptor.id) {
		*plugin_instance = new CTable_Control_Plugin();
		return NLib_Result::Success;
	}
	return NLib_Result::Not_Found;
}
