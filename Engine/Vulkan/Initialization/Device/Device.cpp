#include "Device.h"


Device::Device(VulkanResources& vulkanResources) : vulkanResources{ vulkanResources }
{

}

void Device::initDevice()
{
	//Physical Device
	vkb::PhysicalDeviceSelector selector{ vulkanResources.vkb_instance };
	auto physicalDeviceReturn = selector.set_surface(vulkanResources.surface)
		.add_required_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)
		.add_required_extension(VK_KHR_MAINTENANCE3_EXTENSION_NAME)
		.add_required_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
		.add_required_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
		.add_required_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)
		.add_required_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)
		.select();
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
	descriptorIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
	descriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
	descriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;

	VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
	bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;

	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
	accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	accelerationStructureFeatures.accelerationStructure = VK_TRUE;

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
	rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	rayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;


	//Logical Device
	vkb::DeviceBuilder deviceBuilder{ physicalDeviceReturn.value() };
	auto deviceReturn = deviceBuilder.add_pNext(&descriptorIndexingFeatures).add_pNext(&bufferDeviceAddressFeatures).add_pNext(&accelerationStructureFeatures).add_pNext(&rayTracingPipelineFeatures).build();
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
	VmaVulkanFunctions vulkanFunctions{};
	
	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.device = vulkanResources.device;
	allocatorInfo.physicalDevice = vulkanResources.physicalDevice;
	allocatorInfo.device = vulkanResources.device;
	allocatorInfo.instance = vulkanResources.instance;
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
	vmaImportVulkanFunctionsFromVolk(&allocatorInfo, &vulkanFunctions);
	allocatorInfo.pVulkanFunctions = &vulkanFunctions;

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
