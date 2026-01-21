#pragma once
#include "Helper.h"




class Surface
{
public:
	friend class VulkanApp;

	Surface(VulkanResources& vulkanResources);
	void initSurface(GLFWwindow* window);
	void destroySurface();
	~Surface();

private:
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	VulkanResources& vulkanResources;
};

