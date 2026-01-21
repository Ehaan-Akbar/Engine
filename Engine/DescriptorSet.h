#pragma once
#include "Helper.h"


class DescriptorSet
{
public:
	friend class Renderer;
	friend class DescriptorManager;

	DescriptorSet(VulkanResources& vulkanResources);
	void initDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool);
	void update(uint32_t binding, uint32_t arrayElement, VkDescriptorType type, const VkDescriptorImageInfo& info);
	void update(uint32_t binding, uint32_t arrayElement, VkDescriptorType type, const VkDescriptorBufferInfo& info);
	void destroyDescriptorSet();
	~DescriptorSet();

private:
	VulkanResources& vulkanResources;
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

