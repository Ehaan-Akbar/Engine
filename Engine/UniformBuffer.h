#pragma once
#include "Helper.h"

#include "Buffer.h"
#include "CommandBuffer.h"


class UniformBuffer : protected Buffer
{
public:
	friend class Renderer;

	UniformBuffer(VulkanResources& vulkanResources);
	void initUniformBuffer(VkDeviceSize bufferSize);
	void bind();
	void destroyUniformBuffer();
	~UniformBuffer();

private:
	VulkanResources& vulkanResources;
	void* mappedData;
};

