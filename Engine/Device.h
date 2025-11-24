#pragma once
#include "VkBootstrap.h"
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <string>
#include "Helper.h"
#include <stdexcept>


class Device
{
public:
	friend class VulkanApp;
	friend class App;
	friend class Renderer;

	Device(VulkanResources& vulkanResources);
	void initDevice();
	void initQueues();
	void initAllocator();
	void destroyDevice();
	void destroyAllocator();
	~Device();

	uint32_t getQueueIndex(vkb::QueueType queueType);

private:
	vkb::Device device;
	vkb::PhysicalDevice physicalDevice;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VulkanResources& vulkanResources;
};

