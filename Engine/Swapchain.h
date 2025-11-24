#pragma once
#include "VkBootstrap.h"
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <string>
#include "Helper.h"
#include <stdexcept>


class Swapchain
{
public:
	friend class VulkanApp;
	friend class Renderer;

	Swapchain(VulkanResources& vulkanResources);
	void initSwapchain();
	void destroySwapchain();
	~Swapchain();

private:
	vkb::Swapchain swapchain;

	VulkanResources& vulkanResources;
};

