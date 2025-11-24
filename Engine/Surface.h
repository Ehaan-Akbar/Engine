#pragma once
#include "VkBootstrap.h"
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <string>
#include "Helper.h"
#include <stdexcept>





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

