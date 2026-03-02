#include "StagingBuffer.h"

StagingBuffer::StagingBuffer(VulkanResources& vulkanResources) : vulkanResources{ vulkanResources }, Buffer{ vulkanResources }
{

}

void StagingBuffer::initStagingBuffer(VkDeviceSize size)
{
	Buffer::initBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
	mappedData = Buffer::map();
}

void StagingBuffer::bind(VkCommandBuffer& commandBuffer)
{
}

void StagingBuffer::destroyStagingBuffer()
{
	Buffer::destroyBuffer();
}

StagingBuffer::~StagingBuffer()
{
	destroyStagingBuffer();
}
