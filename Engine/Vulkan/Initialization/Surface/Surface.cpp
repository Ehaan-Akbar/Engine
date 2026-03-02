#include "Surface.h"

Surface::Surface(VulkanResources& vulkanResources) : vulkanResources{ vulkanResources }
{

}

void Surface::initSurface(GLFWwindow* window)
{
	this->vulkanResources = vulkanResources;

	if (glfwCreateWindowSurface(vulkanResources.instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create surface");
	}

	vulkanResources.surface = surface;
}

void Surface::destroySurface()
{
	if (surface != VK_NULL_HANDLE) {
		vkb::destroy_surface(vulkanResources.instance, surface);
		surface = VK_NULL_HANDLE;
	}
}

Surface::~Surface()
{
	destroySurface();
}
