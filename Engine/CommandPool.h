#pragma once
#include "VkBootstrap.h"
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <string>
#include "Helper.h"
#include <stdexcept>



class CommandPool
{
public:
	friend class VulkanApp;
	friend class App;
	friend class Renderer;

	CommandPool(VulkanResources& vulkanResources);
	void initCommandPool(uint32_t queueIndex);
	void destroyCommandPool();
	~CommandPool();

private:
	VkCommandPool commandPool = VK_NULL_HANDLE;

	VulkanResources& vulkanResources;
};

