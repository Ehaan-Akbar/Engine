#pragma once
#include "VkBootstrap.h"
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <string>
#include "Helper.h"
#include <stdexcept>


class Framebuffer
{
public:
	friend class VulkanApp;
	friend class Renderer;
	friend class FrameGraph;

	Framebuffer(VulkanResources& vulkanResources);
	void initFrameBuffer(std::vector<VkImageView> attachments, uint32_t width, uint32_t height, uint32_t layers, VkRenderPass renderPass);
	void destroyFrameBuffer();
	~Framebuffer();

private:
	VkFramebuffer framebuffer = VK_NULL_HANDLE;

	VulkanResources& vulkanResources;
};

