#include "UniformBuffer.h"

UniformBuffer::UniformBuffer(VulkanResources& vulkanResources) : Buffer{ vulkanResources }, vulkanResources{ vulkanResources }
{

}

void UniformBuffer::initUniformBuffer(VkDeviceSize bufferSize)
{

	Buffer::initBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	mappedData = Buffer::map();
}

void UniformBuffer::bind()
{
}

void UniformBuffer::destroyUniformBuffer()
{
	Buffer::destroyBuffer();
}

UniformBuffer::~UniformBuffer()
{
	destroyUniformBuffer();
}
