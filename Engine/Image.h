#pragma once
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <string>
#include <stdexcept>
#include "Helper.h"
#include "vk_mem_alloc.h"


class Image
{
public:

	friend class Renderer;
	friend class App;

	Image(VulkanResources& vulkanResources);
	void initImage(VkImageType type, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, uint32_t mipLevels = 1, uint32_t arrayLayers = 1, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL);
	void initImageView(VkImageViewType type, VkFormat format, VkImageSubresourceRange subresourceRange);
	void destroyImage();
	~Image();


private:
	VulkanResources& vulkanResources;

	VkImage image = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VmaAllocation imageAllocation = nullptr;


	VkImageType imageType;
};

