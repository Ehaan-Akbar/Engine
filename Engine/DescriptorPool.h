#pragma once
#include "Helper.h"


class DescriptorPool
{
public:
	friend class Renderer;
	friend class DescriptorManager;

	DescriptorPool(VulkanResources& vulkanResources);
	void initDescriptorPool(std::vector<std::pair<VkDescriptorType, uint32_t>> types, uint32_t maxSets, VkDescriptorPoolCreateFlags flags = 0);
	void destroyDescriptorPool();
	~DescriptorPool();

private:
	VulkanResources& vulkanResources;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

};

