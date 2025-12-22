#include "StorageBuffer.h"

StorageBuffer::StorageBuffer(VulkanResources& vulkanResources) : vulkanResources{ vulkanResources }, Buffer{ vulkanResources }
{

}

void StorageBuffer::initStorageBuffer(VkDeviceSize bufferSize)
{
	//TODO: Transfer Queue, memory usage gpu only?
	Buffer::initBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	mappedData = Buffer::map();
}

void StorageBuffer::bind(VkCommandBuffer commandBuffer)
{
}

void StorageBuffer::destroyStorageBuffer()
{
	Buffer::destroyBuffer();
}

StorageBuffer::~StorageBuffer()
{
	destroyStorageBuffer();
}
