#pragma once
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <string>
#include <stdexcept>
#include "Helper.h"
#include "vk_mem_alloc.h"

class DescriptorPool
{
public:
	friend class Renderer;

	DescriptorPool(VulkanResources& vulkanResources);
	void initDescriptorPool(std::vector<std::pair<VkDescriptorType, uint32_t>> types, uint32_t maxSets, VkDescriptorPoolCreateFlags flags = 0);
	void destroyDescriptorPool();
	~DescriptorPool();

private:
	VulkanResources& vulkanResources;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

};

