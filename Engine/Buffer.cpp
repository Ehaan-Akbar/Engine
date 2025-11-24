#include "Buffer.h"

Buffer::Buffer(VulkanResources& vulkanResources) : vulkanResources{ vulkanResources }
{

}

void Buffer::initBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags, VkSharingMode sharingMode)
{
	destroyBuffer();

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = sharingMode;

	VmaAllocationCreateInfo allocateInfo{};
	allocateInfo.usage = memoryUsage;
	allocateInfo.flags = flags;

	if (vmaCreateBuffer(vulkanResources.allocator, &bufferInfo, &allocateInfo, &buffer, &allocation, &this->allocateInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer");
	}
}

void Buffer::destroyBuffer()
{
	if (buffer != VK_NULL_HANDLE) {
		unmap();
		vmaDestroyBuffer(vulkanResources.allocator, buffer, allocation);
		buffer = VK_NULL_HANDLE;
		allocation = nullptr;
		isMapped = false;
	}
}

void* Buffer::map()
{
	if (!isMapped) {
		void* data;
		if (vmaMapMemory(vulkanResources.allocator, allocation, &data) != VK_SUCCESS) {
			throw std::runtime_error("Failed to map buffer");
		}
		isMapped = true;
		return data;
	}
	return nullptr;
}

void Buffer::unmap()
{
	if (isMapped) {
		vmaUnmapMemory(vulkanResources.allocator, allocation);
		isMapped = false;
	}
}

void Buffer::copy(VkDeviceSize size, void* data, VkDeviceSize offset)
{
	void* dst = nullptr;
	if (vmaMapMemory(vulkanResources.allocator, allocation, &dst) != VK_SUCCESS) {
		throw std::runtime_error("Failed to map buffer for data copy");
	}
	std::memcpy(static_cast<char*>(dst) + offset, data, size);
	vmaUnmapMemory(vulkanResources.allocator, allocation);
}

Buffer::~Buffer()
{
	destroyBuffer();
}
