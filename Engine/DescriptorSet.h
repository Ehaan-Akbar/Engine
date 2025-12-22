#pragma once
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <string>
#include <stdexcept>
#include "Helper.h"
#include "vk_mem_alloc.h"



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

