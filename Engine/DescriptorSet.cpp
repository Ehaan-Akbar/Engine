#include "DescriptorSet.h"

DescriptorSet::DescriptorSet(VulkanResources& vulkanResources) : vulkanResources{ vulkanResources }
{

}

void DescriptorSet::initDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool)
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	if (vkAllocateDescriptorSets(vulkanResources.device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}
}

void DescriptorSet::update(uint32_t binding, uint32_t arrayElement, VkDescriptorType type, const VkDescriptorImageInfo& info)
{
	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = binding;
	descriptorWrite.dstArrayElement = arrayElement;
	descriptorWrite.descriptorType = type;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &info;

	vkUpdateDescriptorSets(vulkanResources.device, 1, &descriptorWrite, 0, nullptr);
}

void DescriptorSet::update(uint32_t binding, uint32_t arrayElement, VkDescriptorType type, const VkDescriptorBufferInfo& info)
{
	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = binding;
	descriptorWrite.dstArrayElement = arrayElement;
	descriptorWrite.descriptorType = type;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = &info;

	vkUpdateDescriptorSets(vulkanResources.device, 1, &descriptorWrite, 0, nullptr);
}

void DescriptorSet::destroyDescriptorSet()
{

}

DescriptorSet::~DescriptorSet()
{
	destroyDescriptorSet();
}
