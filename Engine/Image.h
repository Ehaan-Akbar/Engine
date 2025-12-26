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

	void transitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange);
	void copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImageLayout imageLayout, uint32_t width, uint32_t height, uint32_t depth = 1, uint32_t baseArrayLayer = 0);


private:
	VulkanResources& vulkanResources;

	VkImage image = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VmaAllocation imageAllocation = nullptr;


	VkImageType imageType;
	VkImageViewType imageViewType;
	VkFormat imageFormat;
	VkFormat imageViewFormat;
	VkExtent3D extent;
};

