#include "RenderPass.h"

RenderPass::RenderPass(VulkanResources& vulkanResources) : vulkanResources{ vulkanResources }
{

}

void RenderPass::initRenderPass(
	std::vector<VkAttachmentDescription> attachments,
	std::vector<VkSubpassDescription> subpasses,
	std::vector<VkSubpassDependency> dependencies)
{
	this->attachments = attachments;
	this->subpasses = subpasses;
	this->dependencies = dependencies;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
	renderPassInfo.pSubpasses = subpasses.data();
	if (!dependencies.empty()) {
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();
	}
	else {
		renderPassInfo.dependencyCount = 0;
		renderPassInfo.pDependencies = nullptr;

	}

	if (vkCreateRenderPass(vulkanResources.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass");
	}
}

void RenderPass::destroyRenderPass()
{
	if (renderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(vulkanResources.device, renderPass, nullptr);
		renderPass = VK_NULL_HANDLE;
	}
}

RenderPass::~RenderPass()
{
	destroyRenderPass();
}
