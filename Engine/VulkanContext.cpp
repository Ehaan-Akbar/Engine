#include "VulkanContext.h"

VulkanContext::VulkanContext(GLFWwindow* window)
{
	instance.initInstance("");
	surface.initSurface(window);
	device.initDevice();
	device.initQueues();
	device.initAllocator();
}

VulkanContext::~VulkanContext()
{
	//RAII
}
