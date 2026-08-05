#include "PluginRegistry.h"
#include <iostream>

std::unique_ptr<CPlugin_Registry> CPlugin_Registry::mInstance = nullptr;

CPlugin_Registry& CPlugin_Registry::Instance() {
	if (!mInstance) {
		mInstance = std::unique_ptr<CPlugin_Registry>(new CPlugin_Registry());
	}
	return *mInstance;
}

CPlugin_Registry::CPlugin_Registry() {
}

CPlugin_Registry::~CPlugin_Registry() {
}

void CPlugin_Registry::Load_Plugins() {
	// find all DLLs in "plugins" subdirectory
	CShared_Library_Loader loader;
	auto plugin_paths = loader.Enumerate();

	// load each plugin and query its descriptors
	for (const auto& path : plugin_paths) {
		try {
			auto library = loader.Load(path);
			auto get_descriptors = library->Resolve<TGet_Plugin_Descriptors>(Fnc_Name_Get_Plugin_Descriptors);
			TPlugin_Descriptor* descriptors;
			uint16_t count = 0;
			if (get_descriptors(&descriptors, &count) != NLib_Result::Success) {
				continue;
			}

			bool loaded_any = false;

			for (uint16_t i = 0; i < count; ++i) {
				const auto& descriptor = descriptors[i];
				if (descriptor.type == NPlugin_Type::Sensing) {
					mSensing_Plugins.push_back(std::make_pair(library, std::make_unique<TPlugin_Descriptor>(descriptor)));
					loaded_any = true;
				}
				else if (descriptor.type == NPlugin_Type::Control) {
					mControl_Plugins.push_back(std::make_pair(library, std::make_unique<TPlugin_Descriptor>(descriptor)));
					loaded_any = true;
				}
			}
		}
		catch (const std::exception& ex) {
			std::cerr << "Failed to load plugin from " << path << ": " << ex.what() << std::endl;
		}
	}
}

CPlugin_Registry& gPlugin_Registry() {
	return CPlugin_Registry::Instance();
}

std::vector<TPlugin_Descriptor_Internal> CPlugin_Registry::Convert_Descriptors(const std::vector<std::pair<std::shared_ptr<CShared_Library>, std::unique_ptr<TPlugin_Descriptor>>>& plugins) const {
	std::vector<TPlugin_Descriptor_Internal> result;
	for (const auto& plugin : plugins) {
		const auto& descriptor = plugin.second;
		TPlugin_Descriptor_Internal internal_descriptor;
		internal_descriptor.id = descriptor->id;
		internal_descriptor.name = descriptor->name;
		internal_descriptor.description = descriptor->description;
		internal_descriptor.type = descriptor->type;
		for (uint16_t i = 0; i < descriptor->parameter_count; ++i) {
			TPlugin_Descriptor_Internal::TParameter parameter;
			parameter.name = descriptor->config_parameters[i];
			parameter.description = descriptor->config_parameter_descriptions[i];
			parameter.type = descriptor->config_parameter_types[i];
			internal_descriptor.parameters.push_back(parameter);
		}
		result.push_back(internal_descriptor);
	}
	return result;
}

std::vector<TPlugin_Descriptor_Internal> CPlugin_Registry::Get_Sensing_Plugins() const {
	return Convert_Descriptors(mSensing_Plugins);
}

std::vector<TPlugin_Descriptor_Internal> CPlugin_Registry::Get_Control_Plugins() const {
	return Convert_Descriptors(mControl_Plugins);
}

std::shared_ptr<IPlugin> CPlugin_Registry::Create_Plugin(NPlugin_Type type, const std::string& id) const {

	auto& src = (type == NPlugin_Type::Sensing) ? mSensing_Plugins : mControl_Plugins;
	for (const auto& plugin : src) {
		const auto& library = plugin.first;
		const auto& descriptor = plugin.second;
		if (descriptor->id == id) {
			auto create_plugin = library->Resolve<TCreate_Plugin>(Fnc_Name_Create_Plugin);
			IPlugin* instance = nullptr;
			if (create_plugin(type, id.c_str(), &instance) == NLib_Result::Success) {
				return std::shared_ptr<IPlugin>(instance);
			}
		}
	}

	return nullptr;
}
