#pragma once
#include "Helper.h"



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
	VkDeviceAddress getDeviceAddress();
	~Buffer();

protected:
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = nullptr;
	VmaAllocationInfo allocateInfo;
	bool isMapped = false;

	VulkanResources& vulkanResources;
};
