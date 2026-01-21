#pragma once
#include "Helper.h"


class RenderPass
{
public:
	friend class VulkanApp;
	friend class Renderer;
	friend class FrameGraph;

	RenderPass(VulkanResources& vulkanResources);
	void initRenderPass(
		std::vector<VkAttachmentDescription> attachments,
		std::vector<VkSubpassDescription> subpasses,
		std::vector<VkSubpassDependency> dependencies);
	void destroyRenderPass();
	~RenderPass();

private:
	VkRenderPass renderPass = VK_NULL_HANDLE;

	std::vector<VkAttachmentDescription> attachments;
	std::vector<VkSubpassDescription> subpasses;
	std::vector<VkSubpassDependency> dependencies;

	VulkanResources& vulkanResources;
};

