#pragma once
#include "Helper.h"


class Swapchain
{
public:
	friend class VulkanApp;
	friend class Renderer;

	Swapchain(VulkanResources& vulkanResources);
	void initSwapchain();
	void destroySwapchain();
	~Swapchain();

private:
	vkb::Swapchain swapchain;

	VulkanResources& vulkanResources;
};

