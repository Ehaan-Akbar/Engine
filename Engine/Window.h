#pragma once
#include "VkBootstrap.h"
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <string>
#include "Helper.h"
#include <stdexcept>



class Window
{
public:
	friend class VulkanApp;

	Window();
	void initWindow(int width, int height, std::string title);
	void destroyWindow();
	~Window();

	GLFWwindow* getWindow() {
		return window;
	}

private:
	GLFWwindow* window = nullptr;

	int width;
	int height;
	std::string title;
};

