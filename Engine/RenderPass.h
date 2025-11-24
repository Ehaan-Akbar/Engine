#pragma once
#include "VkBootstrap.h"
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <string>
#include "Helper.h"
#include <stdexcept>


class RenderPass
{
public:
	friend class VulkanApp;
	friend class Renderer;

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

