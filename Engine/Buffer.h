#pragma once
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <string>
#include <stdexcept>
#include "Helper.h"
#include "vk_mem_alloc.h"




class Buffer
{
public:
	friend class VulkanApp;
	friend class Renderer;
	friend class VertexBuffer;
	friend class IndexBuffer;

	Buffer(VulkanResources& vulkanResources);
	void initBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags = 0, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE);
	void destroyBuffer();
	void* map();
	void unmap();
	void copy(VkDeviceSize size, void* data, VkDeviceSize offset = 0);
	~Buffer();

protected:
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = nullptr;
	VmaAllocationInfo allocateInfo;
	bool isMapped = false;

	VulkanResources& vulkanResources;
};
