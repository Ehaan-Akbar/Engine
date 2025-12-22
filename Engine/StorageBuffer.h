#pragma once
#include "Buffer.h"


class StorageBuffer : protected Buffer
{
public:
	friend class Renderer;

	StorageBuffer(VulkanResources& vulkanResources);
	void initStorageBuffer(VkDeviceSize bufferSize);
	void bind(VkCommandBuffer commandBuffer);
	void destroyStorageBuffer();
	~StorageBuffer();


private:
	VulkanResources& vulkanResources;
	void* mappedData;
};

