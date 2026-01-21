#pragma once
#include "Helper.h"

#include "Buffer.h"
#include "CommandBuffer.h"

using Indices = std::vector<uint32_t>;

class IndexBuffer : protected Buffer
{
public:

	friend class Renderer;

	IndexBuffer(VulkanResources& vulkanResources);
	void initIndexBuffer(std::shared_ptr<Indices> indices, VkQueue transferQueue, VkCommandPool commandPool);
	void bind(VkCommandBuffer commandBuffer);
	void destroyIndexBuffer();
	~IndexBuffer();


	uint32_t indexCount;

private:
	void uploadThroughStaging(void* data, VkDeviceSize size, VkQueue transferQueue, VkCommandPool commandPool);

private:
	VulkanResources& vulkanResources;

};

