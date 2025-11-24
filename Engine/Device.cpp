#include "Device.h"


Device::Device(VulkanResources& vulkanResources) : vulkanResources{ vulkanResources }
{

}

void Device::initDevice()
{
	//Physical Device
	vkb::PhysicalDeviceSelector selector{ vulkanResources.vkb_instance };
	auto physicalDeviceReturn = selector.set_surface(vulkanResources.surface).add_required_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME).add_required_extension(VK_KHR_MAINTENANCE3_EXTENSION_NAME).select();
	if (!physicalDeviceReturn) {
		throw std::runtime_error("Failed to create physical device");
	}


	VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
	descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
	descriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
	descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
	descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	descriptorIndexingFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
	descriptorIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
	descriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;


	//Logical Device
	vkb::DeviceBuilder deviceBuilder{ physicalDeviceReturn.value() };
	auto deviceReturn = deviceBuilder.add_pNext(&descriptorIndexingFeatures).build();
	if (!deviceReturn) {
		throw std::runtime_error("Failed to create logical device");
	}

	device = deviceReturn.value();
	physicalDevice = physicalDeviceReturn.value();

	vulkanResources.device = device.device;
	vulkanResources.physicalDevice = physicalDevice.physical_device;
	vulkanResources.vkb_device = device;

}

void Device::initQueues()
{
	//Graphics queue
	auto graphicsQueueReturn = device.get_queue(vkb::QueueType::graphics);
	if (!graphicsQueueReturn) {
		throw std::runtime_error("Failed to create graphics queue");
	}
	graphicsQueue = graphicsQueueReturn.value();

	//Present queue
	auto presentQueueReturn = device.get_queue(vkb::QueueType::present);
	if (!presentQueueReturn) {
		throw std::runtime_error("Failed to create present queue");
	}
	presentQueue = presentQueueReturn.value();
}

void Device::initAllocator()
{
	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.device = vulkanResources.device;
	allocatorInfo.physicalDevice = vulkanResources.physicalDevice;
	allocatorInfo.instance = vulkanResources.instance;
	vmaCreateAllocator(&allocatorInfo, &vulkanResources.allocator);
}

void Device::destroyDevice()
{
	if (device.device != VK_NULL_HANDLE) {
		vkb::destroy_device(device);
	}
}

void Device::destroyAllocator()
{
	vmaDestroyAllocator(vulkanResources.allocator);
}

Device::~Device()
{
	destroyAllocator();
	destroyDevice();
}

uint32_t Device::getQueueIndex(vkb::QueueType queueType)
{
	return device.get_queue_index(queueType).value();
}
