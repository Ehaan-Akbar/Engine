#include "Image.h"

Image::Image(VulkanResources& vulkanResources) : vulkanResources{ vulkanResources }
{

}

void Image::initImage(VkImageType type, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, uint32_t mipLevels, uint32_t arrayLayers, VkSampleCountFlagBits samples, VkImageTiling tiling, VkImageCreateFlags flags)
{
	this->imageType = type;
	this->imageFormat = format;
	this->extent = extent;

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
	imageInfo.flags = flags;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = memoryUsage;

	if (vmaCreateImage(vulkanResources.allocator, &imageInfo, &allocInfo, &image, &imageAllocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to make rendered iamges ahh");
	}
}

void Image::initImageView(VkImageViewType type, VkFormat format, VkImageSubresourceRange subresourceRange)
{
	this->imageViewType = type;
	this->imageViewFormat = format;

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

void Image::transitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange)
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange = subresourceRange;
	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("Unsupported layout transition!");
	}
	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

void Image::copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImageLayout imageLayout, uint32_t width, uint32_t height, uint32_t depth, uint32_t baseArrayLayer)
{
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = baseArrayLayer;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		depth
	};
	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		imageLayout,
		1,
		&region
	);
}

VkImageView Image::createFaceView(uint32_t face)
{
	VkImageViewCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.image = image;
	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.format = imageFormat;
	info.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, face, 1 };

	VkImageView view;
	vkCreateImageView(vulkanResources.device, &info, nullptr, &view);
	transientViews.push_back(view);
	return view;
}

VkImageView Image::createFaceMipView(uint32_t face, uint32_t mip)
{
	VkImageViewCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.image = image;
	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.format = imageFormat;
	info.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, mip, 1, face, 1 };

	VkImageView view;
	vkCreateImageView(vulkanResources.device, &info, nullptr, &view);
	transientViews.push_back(view);
	return view;
}

void Image::destroyTransientViews()
{
	for (auto& view : transientViews) {
		vkDestroyImageView(vulkanResources.device, view, nullptr);
	}
}
