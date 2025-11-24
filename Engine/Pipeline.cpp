#include "Pipeline.h"

Pipeline::Pipeline(VulkanResources& vulkanResources) : vulkanResources{ vulkanResources }
{

}

void Pipeline::initPipeline(
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
	VkRenderPass renderPass)
{
	this->pipelineLayoutInfo = pipelineLayoutInfo;
	this->rasterizationStateInfo = rasterizationStateInfo;
	this->depthStencilStateInfo = depthStencilStateInfo;
	this->colorBlendStateInfo = colorBlendStateInfo;
	this->vertexInputStateInfo = vertexInputStateInfo;
	this->inputAssemblyStateInfo = inputAssemblyStateInfo;
	this->viewportStateInfo = viewportStateInfo;
	this->multisamplingStateInfo = multisamplingStateInfo;

	if (vkCreatePipelineLayout(vulkanResources.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}

	VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
	vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderStageInfo.module = vertexShaderModule;
	vertexShaderStageInfo.pName = "main";


	VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
	fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStageInfo.module = fragmentShaderModule;
	fragmentShaderStageInfo.pName = "main";

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertexShaderStageInfo, fragmentShaderStageInfo };

	std::vector<VkDynamicState> dynamicStates{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
	dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicStateInfo.pDynamicStates = dynamicStates.data();

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputStateInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyStateInfo;
	pipelineInfo.pViewportState = &viewportStateInfo;
	pipelineInfo.pRasterizationState = &rasterizationStateInfo;
	pipelineInfo.pMultisampleState = &multisamplingStateInfo;
	pipelineInfo.pColorBlendState = &colorBlendStateInfo;
	pipelineInfo.pDepthStencilState = &depthStencilStateInfo;
	pipelineInfo.pDynamicState = &dynamicStateInfo;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(vulkanResources.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline");
	}
}

void Pipeline::destroyPipeline()
{
	if (pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(vulkanResources.device, pipeline, nullptr);
		pipeline = VK_NULL_HANDLE;
		rasterizationStateInfo = {};
		depthStencilStateInfo = {};
		colorBlendStateInfo = {};
		vertexInputStateInfo = {};
		inputAssemblyStateInfo = {};
		viewportStateInfo = {};
		multisamplingStateInfo = {};
	}
	
	if (pipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(vulkanResources.device, pipelineLayout, nullptr);
		pipelineLayout = VK_NULL_HANDLE;
		pipelineLayoutInfo = {};
	}
}

Pipeline::~Pipeline()
{
	destroyPipeline();
}

VkShaderModule Pipeline::createShaderModule(VkDevice& device, const std::vector<char>& code)
{
	VkShaderModuleCreateInfo shaderModuleInfo{};
	shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleInfo.codeSize = code.size();
	shaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module");
	}

	return shaderModule;
}

void Pipeline::destroyShaderModule(VkDevice& device, VkShaderModule& shaderModule)
{
	vkDestroyShaderModule(device, shaderModule, nullptr);
}
