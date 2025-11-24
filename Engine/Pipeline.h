#pragma once
#include "VkBootstrap.h"
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <string>
#include "Helper.h"
#include <stdexcept>


class Pipeline
{
public:
	friend class VulkanApp;
	friend class Renderer;

	Pipeline(VulkanResources& vulkanResources);
	void initPipeline(
		VkShaderModule& vertexShaderModule,
		VkShaderModule& fragmentShaderModule,
		VkPipelineLayoutCreateInfo pipelineLayoutInfo,
		VkPipelineRasterizationStateCreateInfo rasterizationStateInfo,
		VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo,
		VkPipelineColorBlendStateCreateInfo colorBlendStateInfo,
		VkPipelineVertexInputStateCreateInfo vertexInputStateInfo,
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo,
		VkPipelineViewportStateCreateInfo viewportStateInfo,
		VkPipelineMultisampleStateCreateInfo multisamplingStateInfo,
		VkRenderPass renderPass);
	void destroyPipeline();
	~Pipeline();

	static VkShaderModule createShaderModule(VkDevice& device, const std::vector<char>& code);
	static void destroyShaderModule(VkDevice& device, VkShaderModule& shaderModule);

private:
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo;
	VkPipelineRasterizationStateCreateInfo rasterizationStateInfo;
	VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo;
	VkPipelineColorBlendStateCreateInfo colorBlendStateInfo;
	VkPipelineVertexInputStateCreateInfo vertexInputStateInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo;
	VkPipelineViewportStateCreateInfo viewportStateInfo;
	VkPipelineMultisampleStateCreateInfo multisamplingStateInfo;

	VulkanResources& vulkanResources;
};

