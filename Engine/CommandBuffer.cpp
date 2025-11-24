#include "CommandBuffer.h"

CommandBuffer::CommandBuffer(VulkanResources& vulkanResources, VkCommandPool& commandPool) : vulkanResources{ vulkanResources }, commandPool{ commandPool }
{
}

CommandBuffer& CommandBuffer::allocate(VkCommandBufferLevel level)
{
	VkCommandBufferAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = commandPool;
	allocateInfo.level = level;
	allocateInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(vulkanResources.device, &allocateInfo, &commandBuffer)) {
		throw std::runtime_error("Failed to allocate command buffer");
	}
}

void CommandBuffer::begin(VkCommandBufferUsageFlags usageFlags)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = usageFlags;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to begin command buffer");
	}
}

void CommandBuffer::end() {
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to end command buffer");
	}
}

void CommandBuffer::submit(VkQueue queue, VkFence fence, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, VkPipelineStageFlags waitStage)
{
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	if (waitSemaphore != VK_NULL_HANDLE) {
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &waitSemaphore;
		submitInfo.pWaitDstStageMask = &waitStage;
	}

	if (signalSemaphore != VK_NULL_HANDLE) {
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &signalSemaphore;
	}

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	if (vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit command buffer");
	}
}

void CommandBuffer::free()
{
	if (commandBuffer != VK_NULL_HANDLE) {
		vkFreeCommandBuffers(vulkanResources.device, commandPool, 1, &commandBuffer);
		commandBuffer = VK_NULL_HANDLE;
	}
}

CommandBuffer::~CommandBuffer()
{
	free();
}
