#pragma once
#include "Helper.h"


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

