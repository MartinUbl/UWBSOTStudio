#pragma once

#include <vector>
#include <list>
#include <memory>
#include <uwbsot/PluginInterface.h>

#include "SharedLibrary.h"

class CPlugin_Registry {
	private:
		static std::unique_ptr<CPlugin_Registry> mInstance;
		
		CPlugin_Registry();

	private:
		std::vector<std::pair<std::shared_ptr<CShared_Library>, std::unique_ptr<TPlugin_Descriptor>>> mSensing_Plugins;
		std::vector<std::pair<std::shared_ptr<CShared_Library>, std::unique_ptr<TPlugin_Descriptor>>> mControl_Plugins;

		std::vector<TPlugin_Descriptor_Internal> Convert_Descriptors(const std::vector<std::pair<std::shared_ptr<CShared_Library>, std::unique_ptr<TPlugin_Descriptor>>>& plugins) const;

	public:
		static CPlugin_Registry& Instance();
		~CPlugin_Registry();

		void Load_Plugins();

		std::vector<TPlugin_Descriptor_Internal> Get_Sensing_Plugins() const;
		std::vector<TPlugin_Descriptor_Internal> Get_Control_Plugins() const;

		std::shared_ptr<IPlugin> Create_Plugin(NPlugin_Type type, const std::string& id) const;
};

extern CPlugin_Registry& gPlugin_Registry();
