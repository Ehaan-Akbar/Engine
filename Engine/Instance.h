#pragma once
#include "VkBootstrap.h"
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <string>
#include "Helper.h"
#include <stdexcept>



class Instance
{
public:

	friend class VulkanApp;

	Instance(VulkanResources& vulkanResources);
	void initInstance(std::string title);
	void destroyInstance();
	~Instance();

private:
	vkb::Instance instance;

	std::string title;

	VulkanResources& vulkanResources;
};

