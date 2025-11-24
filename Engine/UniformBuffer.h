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

