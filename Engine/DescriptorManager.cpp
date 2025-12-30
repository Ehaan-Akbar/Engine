#include "DescriptorManager.h"

DescriptorManager::DescriptorManager(VulkanResources& vulkanResources) : vulkanResources{ vulkanResources }
{

}

void DescriptorManager::initDescriptorManager()
{
	initDescriptorPool();
	initGlobalDescriptorSet();
	initBindlessResourceDescriptorSet();
	initTargetDescriptorSet();
}

void DescriptorManager::destroy()
{
	descriptorPool.destroyDescriptorPool();
	vkDestroyDescriptorSetLayout(vulkanResources.device, targetDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(vulkanResources.device, bindlessResourceDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(vulkanResources.device, globalDescriptorSetLayout, nullptr);
}

DescriptorManager::~DescriptorManager()
{
	destroy();
}

void DescriptorManager::initDescriptorPool()
{
	descriptorPool.initDescriptorPool(
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 + 1},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6000 + 5}
		},
		3, // 3 sets
		VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT); // For bindless


}

void DescriptorManager::initGlobalDescriptorSet() //Non-Bindless
{
	std::array<VkDescriptorSetLayoutBinding, 3> bindings{};
	//Binding 0 - Global UBO
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[0].pImmutableSamplers = nullptr;

	//Binding 1 - Object SSBO
	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[1].pImmutableSamplers = nullptr;

	//Binding 1 - Lights SSBO
	bindings[2].binding = 2;
	bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[2].descriptorCount = 1;
	bindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[2].pImmutableSamplers = nullptr;

	std::array<VkDescriptorBindingFlags, 3> bindingFlags = {
		VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT,
		VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT,
		VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT
	};

	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
	bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	bindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
	bindingFlagsInfo.pBindingFlags = bindingFlags.data();

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
	layoutInfo.pNext = &bindingFlagsInfo;

	if (vkCreateDescriptorSetLayout(vulkanResources.device, &layoutInfo, nullptr, &globalDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create global descriptor set layout");
	}

	globalDescriptorSet.initDescriptorSet(globalDescriptorSetLayout, descriptorPool.descriptorPool);
}

void DescriptorManager::initBindlessResourceDescriptorSet() //Bindless
{
	std::array<VkDescriptorSetLayoutBinding, 1> bindings{};
	//Binding 0 - Textures
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[0].descriptorCount = 6000;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[0].pImmutableSamplers = nullptr;


	std::array<VkDescriptorBindingFlags, 1> bindingFlags = {
		VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT
	};

	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
	bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	bindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
	bindingFlagsInfo.pBindingFlags = bindingFlags.data();


	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
	layoutInfo.pNext = &bindingFlagsInfo;

	if (vkCreateDescriptorSetLayout(vulkanResources.device, &layoutInfo, nullptr, &bindlessResourceDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create bindless descriptor set layout");
	}

	bindlessResourceDescriptorSet.initDescriptorSet(bindlessResourceDescriptorSetLayout, descriptorPool.descriptorPool);
}

void DescriptorManager::initTargetDescriptorSet() //Non-Bindless
{
	std::array<VkDescriptorSetLayoutBinding, 5> bindings{};
	//Binding 0 - Albedo Image
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[0].pImmutableSamplers = nullptr;

	//Binding 1 - Normal Image
	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[1].pImmutableSamplers = nullptr;

	//Binding 2 - Material Image
	bindings[2].binding = 2;
	bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[2].descriptorCount = 1;
	bindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[2].pImmutableSamplers = nullptr;

	//Binding 3 - Lighting Image
	bindings[3].binding = 3;
	bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[3].descriptorCount = 1;
	bindings[3].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[3].pImmutableSamplers = nullptr;

	//Binding 4 - Depth Image
	bindings[4].binding = 4;
	bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[4].descriptorCount = 1;
	bindings[4].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[4].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
	bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	bindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	bindingFlagsInfo.pBindingFlags = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(vulkanResources.device, &layoutInfo, nullptr, &targetDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create global descriptor set layout");
	}

	targetDescriptorSet.initDescriptorSet(targetDescriptorSetLayout, descriptorPool.descriptorPool);
}
