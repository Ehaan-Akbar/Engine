#pragma once
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <string>
#include <stdexcept>
#include "vk_mem_alloc.h"
#include <array>
#include "Buffer.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
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

