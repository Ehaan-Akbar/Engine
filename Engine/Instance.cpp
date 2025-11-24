#include "Instance.h"

Instance::Instance(VulkanResources& vulkanResources) : vulkanResources{ vulkanResources }
{

}

void Instance::initInstance(std::string title)
{
	this->title = title;

#ifdef NDEBUG
	const bool debugMode = false;
#else
	const bool debugMode = true;
#endif

	vkb::InstanceBuilder instanceBuilder{};

	auto instanceReturn = instanceBuilder.set_app_name(title.c_str())
#ifndef NDEBUG
		.request_validation_layers(true)
#endif
	.set_engine_name("Game Engine idk bro").require_api_version(1, 0, 0).build();


	if (!instanceReturn) {
		throw std::runtime_error("Failed to create instance");
	}

	instance = instanceReturn.value();

	//Instance level Extensions | Layers - Validation layer

	auto systemInfoRet = vkb::SystemInfo::get_system_info();
	auto systemInfo = systemInfoRet.value();


	if (systemInfo.validation_layers_available && debugMode) {
		instanceBuilder.use_default_debug_messenger();
	}

	vulkanResources.instance = instance.instance;
	vulkanResources.vkb_instance = instance;
}

void Instance::destroyInstance()
{
	if (instance.instance != VK_NULL_HANDLE) {
		vkDestroyInstance(instance, nullptr);
	}
}

Instance::~Instance()
{
	destroyInstance();
}
