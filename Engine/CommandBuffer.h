#pragma once
#include "Helper.h"

//Add allocation batching for multiple command buffers at once


class CommandBuffer
{
public:
	friend class VulkanApp;
	friend class Renderer;
	friend class VertexBuffer;
	friend class IndexBuffer;

	CommandBuffer(VulkanResources& vulkanResources, VkCommandPool& commandPool);
	CommandBuffer& allocate(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	void begin(VkCommandBufferUsageFlags usageFlags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	void end();
	void submit(VkQueue queue, VkFence fence = VK_NULL_HANDLE, VkSemaphore waitSemaphore = VK_NULL_HANDLE, VkSemaphore signalSemaphore = VK_NULL_HANDLE, VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	void free();
	~CommandBuffer();

private:
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

	VulkanResources& vulkanResources;
	VkCommandPool& commandPool;
};

