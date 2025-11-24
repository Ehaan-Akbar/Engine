#include "Framebuffer.h"

Framebuffer::Framebuffer(VulkanResources& vulkanResources) : vulkanResources{ vulkanResources }
{

}

void Framebuffer::initFrameBuffer(std::vector<VkImageView> attachments, uint32_t width, uint32_t height, uint32_t layers, VkRenderPass renderPass)
{
	VkFramebufferCreateInfo framebufferInfo{};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = width;
	framebufferInfo.height = height;
	framebufferInfo.layers = layers;

	if (vkCreateFramebuffer(vulkanResources.device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create framebuffer");
	}
}

void Framebuffer::destroyFrameBuffer()
{
	if (framebuffer != VK_NULL_HANDLE) {
		vkDestroyFramebuffer(vulkanResources.device, framebuffer, nullptr);
		framebuffer = VK_NULL_HANDLE;
	}
}

Framebuffer::~Framebuffer()
{
	destroyFrameBuffer();
}
