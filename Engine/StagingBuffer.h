#pragma once
#include "Buffer.h"

class StagingBuffer : public Buffer
{

public:
	StagingBuffer(VulkanResources& vulkanResources);
	void initStagingBuffer(VkDeviceSize size);
	void bind(VkCommandBuffer& commandBuffer);
	void destroyStagingBuffer();
	~StagingBuffer();
	
private:
	VulkanResources& vulkanResources;
	void* mappedData;

};

