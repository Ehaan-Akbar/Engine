#include "IndexBuffer.h"

IndexBuffer::IndexBuffer(VulkanResources& vulkanResources) : vulkanResources{ vulkanResources }, Buffer{ vulkanResources }
{

}

IndexBuffer::~IndexBuffer()
{
	destroyIndexBuffer();
}

void IndexBuffer::initIndexBuffer(std::shared_ptr<Indices> indices, VkQueue transferQueue, VkCommandPool commandPool)
{
	indexCount = static_cast<uint32_t>(indices->size());
	VkDeviceSize bufferSize = sizeof(uint32_t) * indexCount;

	Buffer::initBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	uploadThroughStaging(indices->data(), bufferSize, transferQueue, commandPool);
}

void IndexBuffer::bind(VkCommandBuffer commandBuffer)
{
	vkCmdBindIndexBuffer(commandBuffer, Buffer::buffer, 0, VK_INDEX_TYPE_UINT32);
}

void IndexBuffer::destroyIndexBuffer()
{
	Buffer::destroyBuffer();
}

void IndexBuffer::uploadThroughStaging(void* data, VkDeviceSize size, VkQueue transferQueue, VkCommandPool commandPool)
{
	Buffer stagingBuffer{ vulkanResources };
	stagingBuffer.initBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
	stagingBuffer.copy(size, data);

	CommandBuffer commandBuffer{ vulkanResources, commandPool };
	commandBuffer.allocate();
	commandBuffer.begin();

	VkBufferCopy copyRegion{};
	copyRegion.size = size;

	vkCmdCopyBuffer(commandBuffer.commandBuffer, stagingBuffer.buffer, Buffer::buffer, 1, &copyRegion);

	commandBuffer.end();

	commandBuffer.submit(transferQueue);
	vkQueueWaitIdle(transferQueue);

	commandBuffer.free();
	stagingBuffer.destroyBuffer();
}
