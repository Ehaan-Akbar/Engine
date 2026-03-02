#include "VertexBuffer.h"

VertexBuffer::VertexBuffer(VulkanResources& vulkanResources) : vulkanResources{ vulkanResources }, Buffer{ vulkanResources }
{

}

VertexBuffer::~VertexBuffer()
{
	destroyVertexBuffer();
}

void VertexBuffer::initVertexBuffer(std::shared_ptr<Vertices> vertices, VkQueue transferQueue, VkCommandPool commandPool)
{
	vertexCount = static_cast<uint32_t>(vertices->size());
	VkDeviceSize bufferSize = sizeof(Vertex) * vertexCount;

	Buffer::initBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	uploadThroughStaging(vertices->data(), bufferSize, transferQueue, commandPool);
}

void VertexBuffer::bind(VkCommandBuffer commandBuffer)
{
	VkBuffer buffers[] = { Buffer::buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
}

void VertexBuffer::destroyVertexBuffer()
{
	Buffer::destroyBuffer();
}

void VertexBuffer::uploadThroughStaging(void* data, VkDeviceSize size, VkQueue transferQueue, VkCommandPool commandPool)
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
