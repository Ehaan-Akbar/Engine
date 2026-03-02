#include "VulkanContext.h"

VulkanContext::VulkanContext(GLFWwindow* window)
{
	volkInitialize();
	instance.initInstance("");
	volkLoadInstance(instance.instance);
	surface.initSurface(window);
	device.initDevice();
	volkLoadDevice(device.device.device);
	device.initQueues();
	device.initAllocator();
}

VulkanContext::~VulkanContext()
{
	//RAII
}
