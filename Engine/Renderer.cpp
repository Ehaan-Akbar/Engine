#include "Renderer.h"


Renderer::Renderer(VulkanContext& vulkanContext) : vulkanContext{ vulkanContext }
{

}

void Renderer::init()
{
	graphicsCommandPool.initCommandPool(vulkanContext.device.getQueueIndex(vkb::QueueType::graphics));
	swapchain.initSwapchain();
	initUniformBuffers();
	initCommandBuffers();
	initSyncObjects();

	initSampler();

	initRenderingPass();
	initSwapchainRenderPass();

	initRenderingResources();
	initSwapchainResources();

	//initDescriptorPools();
	//initDescriptorSets();

	initBindlessDescriptors();
	
	
	initRenderingPipeline();
	initSwapchainPipeline();
	
}

void Renderer::destroy()
{
	vkDeviceWaitIdle(vulkanContext.vulkanResources.device);

	for (size_t i = 0; i < maxFramesInFlight; ++i) {
		vkDestroySemaphore(vulkanContext.vulkanResources.device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(vulkanContext.vulkanResources.device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(vulkanContext.vulkanResources.device, inFlightFences[i], nullptr);
	}


	vkDestroyDescriptorSetLayout(vulkanContext.vulkanResources.device, samplerDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(vulkanContext.vulkanResources.device, uboDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(vulkanContext.vulkanResources.device, bindlessDescriptorSetLayout, nullptr);


	vkDestroySampler(vulkanContext.vulkanResources.device, textureSampler, nullptr);

	swapchain.swapchain.destroy_image_views(swapchainImageViews);

	renderedDepthImage.destroyImage();
	renderedImage.destroyImage();

	

	//Members destroyed through RAII
}

Renderer::~Renderer()
{
	destroy();
}

bool Renderer::beginFrame()
{
	vkWaitForFences(vulkanContext.vulkanResources.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	auto result = vkAcquireNextImageKHR(vulkanContext.vulkanResources.device, swapchain.swapchain.swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapchain();
		return false;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("ahh");
	}

	vkResetFences(vulkanContext.vulkanResources.device, 1, &inFlightFences[currentFrame]);


	vkResetCommandBuffer(commandBuffers[currentFrame].commandBuffer, 0);

	return true;
}

void Renderer::submit(ECS& ecs, Camera& camera)
{

	commandBuffers[currentFrame].begin();

	vkCmdBindDescriptorSets(commandBuffers[currentFrame].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderedPipeline.pipelineLayout, 0, 1, &bindlessDescriptorSet.descriptorSet, 0, nullptr);

	VkClearValue clearColors[2];
	clearColors[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearColors[1].depthStencil = { 1.0f, 0 };

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapchain.swapchain.extent.width);
	viewport.height = static_cast<float>(swapchain.swapchain.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;


	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = swapchain.swapchain.extent;

	VkRenderPassBeginInfo renderingPassBeginInfo{};
	renderingPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderingPassBeginInfo.renderPass = renderedRenderPass.renderPass;
	renderingPassBeginInfo.framebuffer = renderedFramebuffer.framebuffer;
	renderingPassBeginInfo.renderArea.offset = { 0,0 };
	renderingPassBeginInfo.renderArea.extent = swapchain.swapchain.extent;
	renderingPassBeginInfo.clearValueCount = 2;
	renderingPassBeginInfo.pClearValues = clearColors;

	vkCmdBeginRenderPass(commandBuffers[currentFrame].commandBuffer, &renderingPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffers[currentFrame].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderedPipeline.pipeline);

	vkCmdSetViewport(commandBuffers[currentFrame].commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffers[currentFrame].commandBuffer, 0, 1, &scissor);


	uint32_t currentVertexOffset = 0;
	modelTransforms.clear();
	batchedVertices.clear();
	batchedIndices.clear();

	ecs.onEachEntity([&](std::shared_ptr<Entity> entity) {
		const auto& t = ecs.getComponent<transform>(entity);
		const auto& m = ecs.getComponent<mesh>(entity);

		modelTransforms.push_back(t->transformationMatrix());
		batchedVertices.insert(batchedVertices.end(), m->vertices->begin(), m->vertices->end());
		for (size_t i = 0; i < m->indices->size(); ++i) {
			batchedIndices.push_back((*m->indices)[i] + currentVertexOffset);
		}
		currentVertexOffset += m->vertices->size();

	});

	if (!isMainVertexBufferInitialized) {
		mainVertexBuffer.initVertexBuffer(std::make_shared<Vertices>(batchedVertices), vulkanContext.device.graphicsQueue, graphicsCommandPool.commandPool);
		mainIndexBuffer.initIndexBuffer(std::make_shared<Indices>(batchedIndices), vulkanContext.device.graphicsQueue, graphicsCommandPool.commandPool);
		isMainVertexBufferInitialized = true;
	}

	mainVertexBuffer.bind(commandBuffers[currentFrame].commandBuffer);
	mainIndexBuffer.bind(commandBuffers[currentFrame].commandBuffer);

	for (const auto& transform : modelTransforms) {

		PushConstantMVP push{
			.uboIndex = static_cast<uint32_t>(currentFrame),
			.textureIndex = 0 //TODO: Texture index from entity
		};

		UBO ubo{
			.model = transform,
			.view = camera.getViewMatrix(),
			.projection = camera.getProjectionMatrix(),
			.lightPos = glm::vec4(4.0f, -3.0f, -4.0f, 0.0f),
			.lightDir = glm::normalize(glm::vec4(-1.0f, -1.0f, -1.0f, 0.0f)),
			.camPos = glm::vec4(camera.position, 0.0f)
		};

		uniformBuffers[currentFrame].copy(sizeof(ubo), &ubo);
		
		vkCmdPushConstants(commandBuffers[currentFrame].commandBuffer, renderedPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantMVP), &push);
		vkCmdDrawIndexed(commandBuffers[currentFrame].commandBuffer, static_cast<uint32_t>(batchedIndices.size()), 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(commandBuffers[currentFrame].commandBuffer);


	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.image = renderedImage.image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(
		commandBuffers[currentFrame].commandBuffer,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);



	VkClearValue swapClearColor;
	swapClearColor.color = { 0.0f, 0.0f, 0.0f, 1.0f };
	VkRenderPassBeginInfo swapchainRenderPassBeginInfo{};
	swapchainRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	swapchainRenderPassBeginInfo.renderPass = swapchainRenderPass.renderPass;
	swapchainRenderPassBeginInfo.framebuffer = framebuffers[imageIndex].framebuffer;
	swapchainRenderPassBeginInfo.renderArea.offset = { 0,0 };
	swapchainRenderPassBeginInfo.renderArea.extent = swapchain.swapchain.extent;
	swapchainRenderPassBeginInfo.clearValueCount = 1;
	swapchainRenderPassBeginInfo.pClearValues = &swapClearColor;

	vkCmdBeginRenderPass(commandBuffers[currentFrame].commandBuffer, &swapchainRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	
	vkCmdBindPipeline(commandBuffers[currentFrame].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, swapchainPipeline.pipeline);

	vkCmdSetViewport(commandBuffers[currentFrame].commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffers[currentFrame].commandBuffer, 0, 1, &scissor);

	PushConstantMVP push{
			.uboIndex = static_cast<uint32_t>(currentFrame),
			.textureIndex = 0 //TODO: Texture index from entity
	};
	vkCmdPushConstants(commandBuffers[currentFrame].commandBuffer, renderedPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantMVP), &push);
	vkCmdDraw(commandBuffers[currentFrame].commandBuffer, 6, 1, 0, 0);
	
	vkCmdEndRenderPass(commandBuffers[currentFrame].commandBuffer);


	commandBuffers[currentFrame].end();
	//Submit
	commandBuffers[currentFrame].submit(vulkanContext.device.graphicsQueue, inFlightFences[currentFrame], imageAvailableSemaphores[currentFrame], renderFinishedSemaphores[currentFrame]);
}

void Renderer::endFrame()
{
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];


	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain.swapchain.swapchain;
	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(vulkanContext.device.presentQueue, &presentInfo);

	currentFrame = (currentFrame + 1) % maxFramesInFlight;
}

void Renderer::recreateSwapchain()
{
	//TODO : Handle minimization properly and remake resources

	destroy();
	framebuffers.clear();
	swapchain.destroySwapchain();

	currentFrame = 0;
	imageIndex = UINT32_MAX;

	swapchain.initSwapchain();
	//initFramebuffers();
	initSyncObjects();
}

void Renderer::initCommandBuffers()
{
	commandBuffers.reserve(maxFramesInFlight);

	for (int i = 0; i < maxFramesInFlight; ++i) {
		commandBuffers.emplace_back(vulkanContext.vulkanResources, graphicsCommandPool.commandPool);
		commandBuffers.back().allocate();
	}
}

void Renderer::initSyncObjects()
{
	imageAvailableSemaphores.resize(maxFramesInFlight);
	renderFinishedSemaphores.resize(maxFramesInFlight);
	inFlightFences.resize(maxFramesInFlight);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < maxFramesInFlight; ++i) {
		if (vkCreateSemaphore(vulkanContext.vulkanResources.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(vulkanContext.vulkanResources.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(vulkanContext.vulkanResources.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create sync objects");
		}
	}
}

void Renderer::initUniformBuffers()
{
	uniformBuffers.reserve(maxFramesInFlight);
	for (int i = 0; i < maxFramesInFlight; ++i) {
		uniformBuffers.emplace_back(vulkanContext.vulkanResources);
		uniformBuffers.back().initUniformBuffer(sizeof(UBO));
	}
}

void Renderer::initSampler()
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(vulkanContext.vulkanResources.device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create texture sampler");
	}
}

void Renderer::initSwapchainResources()
{
	swapchainImages = swapchain.swapchain.get_images().value();
	swapchainImageViews = swapchain.swapchain.get_image_views().value();
	framebuffers.reserve(swapchainImageViews.size());


	for (size_t i = 0; i < swapchainImageViews.size(); ++i) {
		std::vector<VkImageView> attachments = { swapchainImageViews[i] };

		framebuffers.emplace_back(vulkanContext.vulkanResources);
		framebuffers.back().initFrameBuffer(attachments, swapchain.swapchain.extent.width, swapchain.swapchain.extent.height, 1, swapchainRenderPass.renderPass);
	}
}

void Renderer::initSwapchainRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapchain.swapchain.image_format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentReference{};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;

	//Look at later and learn more
	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::vector<VkAttachmentDescription> attachments = { colorAttachment };
	std::vector<VkSubpassDescription> subpasses = { subpass };
	std::vector<VkSubpassDependency> dependencies = { dependency };

	swapchainRenderPass.initRenderPass(attachments, subpasses, dependencies);
}

void Renderer::initSwapchainPipeline()
{
	auto vertShader = readFile("blit.vert.spv");
	auto fragShader = readFile("blit.frag.spv");

	VkShaderModule vertexShaderModule = Pipeline::createShaderModule(vulkanContext.vulkanResources.device, vertShader);
	VkShaderModule fragmentShaderModule = Pipeline::createShaderModule(vulkanContext.vulkanResources.device, fragShader);

	VkPipelineShaderStageCreateInfo vertStateInfo{};
	vertStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertStateInfo.module = vertexShaderModule;
	vertStateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragStateInfo{};
	fragStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStateInfo.module = fragmentShaderModule;
	fragStateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertStateInfo, fragStateInfo };



	std::vector<VkDynamicState> dynamicStates{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};


	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;


	VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
	dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicStateInfo.pDynamicStates = dynamicStates.data();

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterInfo{};
	rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterInfo.depthClampEnable = VK_FALSE;
	rasterInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterInfo.lineWidth = 1.0f;
	//rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterInfo.cullMode = VK_CULL_MODE_NONE;
	rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterInfo.depthBiasEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
	depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilInfo.depthTestEnable = VK_FALSE;
	depthStencilInfo.depthWriteEnable = VK_FALSE;
	depthStencilInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilInfo.stencilTestEnable = VK_FALSE;
	/*depthStencilInfo.depthTestEnable = VK_TRUE;
	depthStencilInfo.depthWriteEnable = VK_TRUE;
	depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilInfo.stencilTestEnable = VK_FALSE;*/

	VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
	multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingInfo.sampleShadingEnable = VK_FALSE;
	multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
	colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendInfo.logicOpEnable = VK_FALSE;
	colorBlendInfo.attachmentCount = 1;
	colorBlendInfo.pAttachments = &colorBlendAttachmentState;

	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PushConstantMVP);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &bindlessDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	swapchainPipeline.initPipeline(vertexShaderModule, fragmentShaderModule, pipelineLayoutInfo, rasterInfo, depthStencilInfo, colorBlendInfo, vertexInputInfo, inputAssemblyInfo, viewportState, multisamplingInfo, swapchainRenderPass.renderPass);

	vkDestroyShaderModule(vulkanContext.vulkanResources.device, vertexShaderModule, nullptr);
	vkDestroyShaderModule(vulkanContext.vulkanResources.device, fragmentShaderModule, nullptr);
}

void Renderer::initRenderingResources()
{
	VkExtent3D extent = { swapchain.swapchain.extent.width, swapchain.swapchain.extent.height, 1 };

	renderedImage.initImage(VK_IMAGE_TYPE_2D, VK_FORMAT_R16G16B16A16_SFLOAT, extent, 
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
		VMA_MEMORY_USAGE_GPU_ONLY);
	renderedImage.initImageView(VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R16G16B16A16_SFLOAT, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });


	renderedDepthImage.initImage(VK_IMAGE_TYPE_2D, VK_FORMAT_D32_SFLOAT, extent, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	renderedDepthImage.initImageView(VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_D32_SFLOAT, { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 });


	std::vector<VkImageView> renderedAttachments = { renderedImage.imageView, renderedDepthImage.imageView };
	renderedFramebuffer.initFrameBuffer(renderedAttachments, extent.width, extent.height, 1, renderedRenderPass.renderPass);

}

void Renderer::initRenderingPass()
{

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentReference{};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = VK_FORMAT_D32_SFLOAT;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference{};
	depthAttachmentReference.attachment = 1;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;
	subpass.pDepthStencilAttachment = &depthAttachmentReference;

	std::vector<VkAttachmentDescription> attachments = { colorAttachment, depthAttachment };
	std::vector<VkSubpassDescription> subpasses = { subpass };
	std::vector<VkSubpassDependency> dependencies = { };

	renderedRenderPass.initRenderPass(attachments, subpasses, dependencies);
}

void Renderer::initRenderingPipeline()
{
	auto vertShader = readFile("render.vert.spv");
	auto fragShader = readFile("render.frag.spv");

	VkShaderModule vertexShaderModule = Pipeline::createShaderModule(vulkanContext.vulkanResources.device, vertShader);
	VkShaderModule fragmentShaderModule = Pipeline::createShaderModule(vulkanContext.vulkanResources.device, fragShader);

	VkPipelineShaderStageCreateInfo vertStateInfo{};
	vertStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertStateInfo.module = vertexShaderModule;
	vertStateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragStateInfo{};
	fragStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStateInfo.module = fragmentShaderModule;
	fragStateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertStateInfo, fragStateInfo };



	std::vector<VkDynamicState> dynamicStates{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};


	auto bindingDescription = VertexBuffer::getBindingDescription();
	auto attributeDescriptions = VertexBuffer::getAttributeDescription();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;


	VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
	dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicStateInfo.pDynamicStates = dynamicStates.data();

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterInfo{};
	rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterInfo.depthClampEnable = VK_FALSE;
	rasterInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterInfo.lineWidth = 1.0f;
	rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	//rasterInfo.cullMode = VK_CULL_MODE_NONE;
	rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterInfo.depthBiasEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
	depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	/*depthStencilInfo.depthTestEnable = VK_FALSE;
	depthStencilInfo.depthWriteEnable = VK_FALSE;
	depthStencilInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilInfo.stencilTestEnable = VK_FALSE;*/
	depthStencilInfo.depthTestEnable = VK_TRUE;
	depthStencilInfo.depthWriteEnable = VK_TRUE;
	depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilInfo.stencilTestEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
	multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingInfo.sampleShadingEnable = VK_FALSE;
	multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
	colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendInfo.logicOpEnable = VK_FALSE;
	colorBlendInfo.attachmentCount = 1;
	colorBlendInfo.pAttachments = &colorBlendAttachmentState;


	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PushConstantMVP);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &bindlessDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	renderedPipeline.initPipeline(vertexShaderModule, fragmentShaderModule, pipelineLayoutInfo, rasterInfo, depthStencilInfo, colorBlendInfo, vertexInputInfo, inputAssemblyInfo, viewportState, multisamplingInfo, renderedRenderPass.renderPass);

	vkDestroyShaderModule(vulkanContext.vulkanResources.device, vertexShaderModule, nullptr);
	vkDestroyShaderModule(vulkanContext.vulkanResources.device, fragmentShaderModule, nullptr);
}

void Renderer::initDescriptorPools()
{
	sharedDescriptorPool.initDescriptorPool({ {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxFramesInFlight}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1} }, maxFramesInFlight+1);
}

void Renderer::initDescriptorSets()
{
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;


		VkDescriptorSetLayoutCreateInfo uboLayoutInfo{};
		uboLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		uboLayoutInfo.bindingCount = 1;
		uboLayoutInfo.pBindings = &uboLayoutBinding;

		if (vkCreateDescriptorSetLayout(vulkanContext.vulkanResources.device, &uboLayoutInfo, nullptr, &uboDescriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout smh");
		}

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 0;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo samplerLayoutInfo{};
		samplerLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		samplerLayoutInfo.bindingCount = 1;
		samplerLayoutInfo.pBindings = &samplerLayoutBinding;

		if (vkCreateDescriptorSetLayout(vulkanContext.vulkanResources.device, &samplerLayoutInfo, nullptr, &samplerDescriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout smh");
		}
	}
	
	{
		uboDescriptorSets.reserve(maxFramesInFlight);
		for (size_t i = 0; i < maxFramesInFlight; ++i) {
			uboDescriptorSets.emplace_back(vulkanContext.vulkanResources);
			uboDescriptorSets.back().initDescriptorSet(uboDescriptorSetLayout, sharedDescriptorPool.descriptorPool);
		}

		samplerDescriptorSet.initDescriptorSet(samplerDescriptorSetLayout, sharedDescriptorPool.descriptorPool);
	}

	for (size_t i = 0; i < maxFramesInFlight; ++i) {
		uboDescriptorSets[i].update(0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {uniformBuffers[i].buffer, 0, sizeof(UBO)});
	}

	samplerDescriptorSet.update(0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, { textureSampler, renderedImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });



}

void Renderer::initBindlessDescriptors()
{
	bindlessDescriptorPool.initDescriptorPool({ {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_RESOURCE_COUNT::UNIFORM_BUFFER}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_RESOURCE_COUNT::COMBINED_IMAGE_SAMPLER} }, 1, 
		VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);

	std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].descriptorCount = MAX_RESOURCE_COUNT::UNIFORM_BUFFER;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[0].pImmutableSamplers = nullptr;

	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[1].descriptorCount = MAX_RESOURCE_COUNT::COMBINED_IMAGE_SAMPLER;
	bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[1].pImmutableSamplers = nullptr;

	std::array<VkDescriptorBindingFlags, 2> bindingFlags = {
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

	if (vkCreateDescriptorSetLayout(vulkanContext.vulkanResources.device, &layoutInfo, nullptr, &bindlessDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create bindless descriptor set layout");
	}

	bindlessDescriptorSet.initDescriptorSet(bindlessDescriptorSetLayout, bindlessDescriptorPool.descriptorPool);

	for (size_t i = 0; i < maxFramesInFlight; ++i) {
		bindlessDescriptorSet.update(0, i, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, { uniformBuffers[i].buffer, 0, sizeof(UBO) });
	}
	bindlessDescriptorSet.update(1, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, { textureSampler, renderedImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	
	


}
