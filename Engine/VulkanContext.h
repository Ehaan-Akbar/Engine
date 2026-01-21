#pragma once
#include "Helper.h"
#include "Window.h"
#include "Device.h"
#include "Instance.h"
#include "Surface.h"



class VulkanContext {
public:

	VulkanContext(GLFWwindow* window);
	~VulkanContext();

	VulkanResources vulkanResources;

	Instance instance{ vulkanResources };
	Surface surface{ vulkanResources };
	Device device{ vulkanResources };
};



