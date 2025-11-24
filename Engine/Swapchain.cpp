#include "Swapchain.h"


Swapchain::Swapchain(VulkanResources& vulkanResources) : vulkanResources{ vulkanResources }
{

}

void Swapchain::initSwapchain()
{
	vkb::SwapchainBuilder swapchainBuilder{ vulkanResources.vkb_device };
	auto swapchainReturn = swapchainBuilder.build();
	if (!swapchainReturn) {
		throw std::runtime_error("Failed to create swapchain");
	}
	swapchain = swapchainReturn.value();
}

void Swapchain::destroySwapchain()
{
	if (swapchain.swapchain != VK_NULL_HANDLE) {
		vkb::destroy_swapchain(swapchain);
	}
}

Swapchain::~Swapchain()
{
	destroySwapchain();
}
