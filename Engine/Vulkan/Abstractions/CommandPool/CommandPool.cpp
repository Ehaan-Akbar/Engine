#include "CommandPool.h"

CommandPool::CommandPool(VulkanResources& vulkanResources) : vulkanResources{ vulkanResources }
{
}

void CommandPool::initCommandPool(uint32_t queueIndex)
{
	VkCommandPoolCreateInfo commandPoolInfo{};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolInfo.queueFamilyIndex = queueIndex;

	if (vkCreateCommandPool(vulkanResources.device, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool");
	}
}

void CommandPool::destroyCommandPool()
{
	if (commandPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(vulkanResources.device, commandPool, nullptr);
		commandPool = VK_NULL_HANDLE;
	}
}

CommandPool::~CommandPool()
{
	destroyCommandPool();
}
