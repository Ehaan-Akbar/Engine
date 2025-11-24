#include "Window.h"


Window::Window()
{
}

void Window::initWindow(int width, int height, std::string title)
{
	this->width = width;
	this->height = height;
	this->title = title;

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
}

void Window::destroyWindow()
{
	if (window != nullptr) {
		glfwDestroyWindow(window);
		window = nullptr;
		width = 0;
		height = 0;
		title.clear();
		glfwTerminate();
	}
}

Window::~Window()
{
	destroyWindow();
}
