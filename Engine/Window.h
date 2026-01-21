#pragma once
#include "Helper.h"



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

