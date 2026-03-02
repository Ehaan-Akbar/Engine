#pragma once
#include "../Helper/Helper.h"
#include "../Initialization/Window/Window.h"
#include "../Initialization/Device/Device.h"
#include "../Initialization/Instance/Instance.h"
#include "../Initialization/Surface/Surface.h"



class VulkanContext {
public:

	VulkanContext(GLFWwindow* window);
	~VulkanContext();

	VulkanResources vulkanResources;

	Instance instance{ vulkanResources };
	Surface surface{ vulkanResources };
	Device device{ vulkanResources };
};



