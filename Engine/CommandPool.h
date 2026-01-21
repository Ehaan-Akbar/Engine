#pragma once
#include "Helper.h"



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

