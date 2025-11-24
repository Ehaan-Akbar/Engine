#include "DescriptorPool.h"

DescriptorPool::DescriptorPool(VulkanResources& vulkanResources) : vulkanResources{vulkanResources}
{

}

void DescriptorPool::initDescriptorPool(std::vector<std::pair<VkDescriptorType, uint32_t>> types, uint32_t maxSets, VkDescriptorPoolCreateFlags flags)
{
	std::vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.reserve(types.size());

	for (const auto& [type, count] : types) {
		VkDescriptorPoolSize poolSize{};
		poolSize.type = type;
		poolSize.descriptorCount = count;
		poolSizes.push_back(poolSize);
	}

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = maxSets;
	poolInfo.flags = flags;

	if (vkCreateDescriptorPool(vulkanResources.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void DescriptorPool::destroyDescriptorPool()
{
	if (descriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(vulkanResources.device, descriptorPool, nullptr);
		descriptorPool = VK_NULL_HANDLE;
	}
}

DescriptorPool::~DescriptorPool()
{
	destroyDescriptorPool();
}
