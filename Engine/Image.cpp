#include "Image.h"

Image::Image(VulkanResources& vulkanResources) : vulkanResources{ vulkanResources }
{

}

void Image::initImage(VkImageType type, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, uint32_t mipLevels, uint32_t arrayLayers, VkSampleCountFlagBits samples, VkImageTiling tiling)
{
	this->imageType = type;
	this->imageFormat = format;

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = type;
	imageInfo.format = format;
	imageInfo.extent = { extent.width, extent.height, extent.depth };
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = arrayLayers;
	imageInfo.samples = samples;
	imageInfo.tiling = tiling;
	imageInfo.usage = usage;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = memoryUsage;

	if (vmaCreateImage(vulkanResources.allocator, &imageInfo, &allocInfo, &image, &imageAllocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to make rendered iamges ahh");
	}
}

void Image::initImageView(VkImageViewType type, VkFormat format, VkImageSubresourceRange subresourceRange)
{
	this->imageViewType = type;
	this->imageFormat = format;

	VkImageViewCreateInfo imageViewInfo{};
	imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewInfo.image = image;
	imageViewInfo.viewType = type;
	imageViewInfo.format = format;
	imageViewInfo.subresourceRange = subresourceRange;

	if (vkCreateImageView(vulkanResources.device, &imageViewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create rendered image views ahh");
	}
}

void Image::destroyImage()
{
	if (image != VK_NULL_HANDLE) {
		vmaDestroyImage(vulkanResources.allocator, image, imageAllocation);
		image = VK_NULL_HANDLE;
		imageAllocation = nullptr;
		imageType = VK_IMAGE_TYPE_MAX_ENUM;
		imageFormat = VK_FORMAT_UNDEFINED;
	}
	if (imageView != VK_NULL_HANDLE) {
		vkDestroyImageView(vulkanResources.device, imageView, nullptr);
		imageView = VK_NULL_HANDLE;
	}
}

Image::~Image()
{
	destroyImage();
}
