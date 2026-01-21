#pragma once
#include "Helper.h"


class Instance
{
public:

	friend class VulkanApp;
	friend class VulkanContext;

	Instance(VulkanResources& vulkanResources);
	void initInstance(std::string title);
	void destroyInstance();
	~Instance();

private:
	vkb::Instance instance;

	std::string title;

	VulkanResources& vulkanResources;
};

