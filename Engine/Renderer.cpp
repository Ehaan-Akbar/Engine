#include "Renderer.h"


Renderer::Renderer(VulkanContext& vulkanContext) : vulkanContext{ vulkanContext }
{

}

void Renderer::init()
{
	graphicsCommandPool.initCommandPool(vulkanContext.device.getQueueIndex(vkb::QueueType::graphics));
	swapchain.initSwapchain();
	initUniformBuffers();
	initStorageBuffers();
	initCommandBuffers();
	initSyncObjects();

	initSampler();

	initSwapchainRenderPass();
	initGBufferPass();
	initLightingPass();

	initSwapchainResources();
	initGBufferResources();
	initLightingResources();

	descriptorManager.initDescriptorManager();
	
	initSwapchainPipeline();
	initGBufferPipeline();
	initLightingPipeline();
	

	justFix();
}

void Renderer::destroy()
{
	// Make destroy idempotent by early-return if we've already cleaned up.
	// We'll detect by checking whether sync objects vector is empty AND commandBuffers cleared.
	if (imageAvailableSemaphores.empty() && commandBuffers.empty()) {
		// Either already destroyed or never initialized — nothing to do.
		return;
	}

	// Wait for GPU to finish all work referencing renderer resources
	// Prefer waiting on queues first (finer-grained) then device idle for safety.
	if (vulkanContext.device.graphicsQueue != VK_NULL_HANDLE) {
		vkQueueWaitIdle(vulkanContext.device.graphicsQueue);
	}
	if (vulkanContext.device.presentQueue != VK_NULL_HANDLE) {
		vkQueueWaitIdle(vulkanContext.device.presentQueue);
	}
	vkDeviceWaitIdle(vulkanContext.vulkanResources.device);

	// Destroy per-frame sync objects (only if valid)
	for (size_t i = 0; i < imageAvailableSemaphores.size(); ++i) {
		if (imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
			vkDestroySemaphore(vulkanContext.vulkanResources.device, imageAvailableSemaphores[i], nullptr);
			imageAvailableSemaphores[i] = VK_NULL_HANDLE;
		}
		if (renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
			vkDestroySemaphore(vulkanContext.vulkanResources.device, renderFinishedSemaphores[i], nullptr);
			renderFinishedSemaphores[i] = VK_NULL_HANDLE;
		}
		if (inFlightFences[i] != VK_NULL_HANDLE) {
			vkDestroyFence(vulkanContext.vulkanResources.device, inFlightFences[i], nullptr);
			inFlightFences[i] = VK_NULL_HANDLE;
		}
	}
	imageAvailableSemaphores.clear();
	renderFinishedSemaphores.clear();
	inFlightFences.clear();

	// Free command buffers (safe because device is idle)
	for (auto& cb : commandBuffers) {
		cb.free();
	}
	commandBuffers.clear();

	// Descriptor set layout and sampler
	
	if (textureSampler != VK_NULL_HANDLE) {
		vkDestroySampler(vulkanContext.vulkanResources.device, textureSampler, nullptr);
		textureSampler = VK_NULL_HANDLE;
	}

	if (depthSampler != VK_NULL_HANDLE) {
		vkDestroySampler(vulkanContext.vulkanResources.device, depthSampler, nullptr);
		depthSampler = VK_NULL_HANDLE;
	}

	// Destroy image views / images (Image::destroyImage should also be safe / idempotent)
	gBufferAlbedoImage.destroyImage();
	gBufferNormalImage.destroyImage();
	gBufferMaterialImage.destroyImage();
	gBufferDepthImage.destroyImage();
	lightingImage.destroyImage();

	// Destroy framebuffers (idempotent)
	gBufferFramebuffer.destroyFrameBuffer();
	lightingFramebuffer.destroyFrameBuffer();

	// Destroy swapchain image views
	if (!swapchainImageViews.empty()) {
		swapchain.swapchain.destroy_image_views(swapchainImageViews);
		swapchainImageViews.clear();
	}

	// mark other RAII-managed resources left to their destructors
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

	//Processing scene
	if (ecs.changeFlag) {
		uint32_t currentVertexOffset = 0;
		uint32_t currentIndexOffset = 0;
		uint32_t uboIndex = 0;
		drawInfos.clear();
		drawInfos.reserve(ecs.getEntityCount());
		batchedVertices.clear();
		batchedIndices.clear();

		ecs.onEachEntity([&](std::shared_ptr<Entity> entity) {
			const auto& t = ecs.getComponent<transform>(entity);
			const auto& m = ecs.getComponent<mesh>(entity);

			drawInfos.push_back({
				.indexCount = static_cast<uint32_t>(m->indices->size()),
				.firstIndex = currentIndexOffset,
				.vertexOffset = 0,
				.modelMatrix = t->transformationMatrix(),
				.uboIndex = uboIndex++
				});
			batchedVertices.insert(batchedVertices.end(), m->vertices->begin(), m->vertices->end());
			for (size_t i = 0; i < m->indices->size(); ++i) {
				batchedIndices.push_back((*m->indices)[i] + currentVertexOffset);
			}
			currentVertexOffset += m->vertices->size();
			currentIndexOffset += m->indices->size();
			});
		mainVertexBuffer.initVertexBuffer(std::make_shared<Vertices>(batchedVertices), vulkanContext.device.graphicsQueue, graphicsCommandPool.commandPool);
		mainIndexBuffer.initIndexBuffer(std::make_shared<Indices>(batchedIndices), vulkanContext.device.graphicsQueue, graphicsCommandPool.commandPool);
		isMainVertexBufferInitialized = true;

		ecs.resetChangeFlag();
	}

	mainVertexBuffer.bind(commandBuffers[currentFrame].commandBuffer);
	mainIndexBuffer.bind(commandBuffers[currentFrame].commandBuffer);




	//Writing to UBOs and SSBOs
	{
		std::vector<objectSSBO> ssbo(1000);
		for (int i = 0; i < drawInfos.size(); ++i) {
			ssbo[i] = {
				.model = drawInfos[i].modelMatrix,
			};
		}

		objectStorageBuffers[currentFrame].copy(sizeof(objectSSBO) * ssbo.size(), ssbo.data());
	}
	
	{
		std::vector<lightSSBO> ssbo(1000);

		lightingStorageBuffers[currentFrame].copy(sizeof(lightSSBO) * ssbo.size(), ssbo.data());
	}

	{
		globalUBO ubo{
			.view = camera.getViewMatrix(),
			.projection = camera.getProjectionMatrix(),
			.lightPos = glm::vec4(4.0f, -3.0f, -4.0f, 0.0f),
			.lightDir = glm::normalize(glm::vec4(-1.0f, -1.0f, -1.0f, 0.0f)),
			.camPos = glm::vec4(camera.position, 0.0f),
			.dimensions = glm::vec4(static_cast<float>(swapchain.swapchain.extent.width), static_cast<float>(swapchain.swapchain.extent.height), 0.0f, 0.0f),
			.inverseProjection = camera.getInverseProjectionMatrix(),
			.inverseView = camera.getInverseViewMatrix()
		};

		globalUniformBuffers[currentFrame].copy(sizeof(ubo), &ubo);
	}



	VkClearValue clearColors[4];
	clearColors[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearColors[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearColors[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearColors[3].depthStencil = { 1.0f, 0 };

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


	//G-Buffer Pass
	VkRenderPassBeginInfo gBufferRenderPassBeginInfo{};
	gBufferRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	gBufferRenderPassBeginInfo.renderPass = gBufferRenderPass.renderPass;
	gBufferRenderPassBeginInfo.framebuffer = gBufferFramebuffer.framebuffer;
	gBufferRenderPassBeginInfo.renderArea.offset = { 0,0 };
	gBufferRenderPassBeginInfo.renderArea.extent = swapchain.swapchain.extent;
	gBufferRenderPassBeginInfo.clearValueCount = 4;
	gBufferRenderPassBeginInfo.pClearValues = clearColors;

	vkCmdBeginRenderPass(commandBuffers[currentFrame].commandBuffer, &gBufferRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffers[currentFrame].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gBufferPipeline.pipeline);
	vkCmdSetViewport(commandBuffers[currentFrame].commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffers[currentFrame].commandBuffer, 0, 1, &scissor);

	std::array<VkDescriptorSet, 2> gBufferSets{
		descriptorManager.globalDescriptorSet.descriptorSet,
		descriptorManager.bindlessResourceDescriptorSet.descriptorSet,
	};

	vkCmdBindDescriptorSets(commandBuffers[currentFrame].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gBufferPipeline.pipelineLayout, 0, gBufferSets.size(), gBufferSets.data(), 0, nullptr);


	for (const auto& info : drawInfos) {
		PushConstantMVP push{
			.uboIndex = info.uboIndex,
			.textureIndex = 0 //TODO: Texture index from entity
		};

		vkCmdPushConstants(commandBuffers[currentFrame].commandBuffer, gBufferPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantMVP), &push);
		vkCmdDrawIndexed(commandBuffers[currentFrame].commandBuffer, info.indexCount, 1, info.firstIndex, info.vertexOffset, 0);
	}

	vkCmdEndRenderPass(commandBuffers[currentFrame].commandBuffer);


	VkImage images[4]{};
	images[0] = gBufferAlbedoImage.image;
	images[1] = gBufferNormalImage.image;
	images[2] = gBufferMaterialImage.image;
	images[3] = gBufferDepthImage.image;
	VkImageMemoryBarrier barriers[4]{};
	for (int i = 0; i < 3; ++i) {
		barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barriers[i].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barriers[i].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[i].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barriers[i].image = images[i];
		barriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barriers[i].subresourceRange.baseMipLevel = 0;
		barriers[i].subresourceRange.levelCount = 1;
		barriers[i].subresourceRange.baseArrayLayer = 0;
		barriers[i].subresourceRange.layerCount = 1;
	}

	barriers[3].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barriers[3].oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	barriers[3].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barriers[3].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barriers[3].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barriers[3].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	barriers[3].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barriers[3].image = images[3];
	barriers[3].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	barriers[3].subresourceRange.baseMipLevel = 0;
	barriers[3].subresourceRange.levelCount = 1;
	barriers[3].subresourceRange.baseArrayLayer = 0;
	barriers[3].subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(
		commandBuffers[currentFrame].commandBuffer,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		4, barriers
	);


	//Lighting Pass
	VkRenderPassBeginInfo lightingPassInfo{};
	lightingPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	lightingPassInfo.renderPass = lightingRenderPass.renderPass;
	lightingPassInfo.framebuffer = lightingFramebuffer.framebuffer;
	lightingPassInfo.renderArea.offset = { 0,0 };
	lightingPassInfo.renderArea.extent = swapchain.swapchain.extent;
	lightingPassInfo.clearValueCount = 1;
	lightingPassInfo.pClearValues = &clearColors[0];

	vkCmdBeginRenderPass(commandBuffers[currentFrame].commandBuffer, &lightingPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffers[currentFrame].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lightingPipeline.pipeline);
	vkCmdSetViewport(commandBuffers[currentFrame].commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffers[currentFrame].commandBuffer, 0, 1, &scissor);

	std::array<VkDescriptorSet, 3> lightingSets{
		descriptorManager.globalDescriptorSet.descriptorSet,
		descriptorManager.bindlessResourceDescriptorSet.descriptorSet,
		descriptorManager.targetDescriptorSet.descriptorSet
	};

	vkCmdBindDescriptorSets(commandBuffers[currentFrame].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lightingPipeline.pipelineLayout, 0, lightingSets.size(), lightingSets.data(), 0, nullptr);



		
	PushConstantMVP push{
		.uboIndex = 0,
		.textureIndex = 0 //TODO: Texture index from entity
	};
	vkCmdPushConstants(commandBuffers[currentFrame].commandBuffer, lightingPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantMVP), &push);
	vkCmdDraw(commandBuffers[currentFrame].commandBuffer, 6, 1, 0, 0);

	vkCmdEndRenderPass(commandBuffers[currentFrame].commandBuffer);

	VkImageMemoryBarrier lightingBarrier{};
	lightingBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	lightingBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	lightingBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	lightingBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	lightingBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	lightingBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	lightingBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	lightingBarrier.image = lightingImage.image;
	lightingBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	lightingBarrier.subresourceRange.baseMipLevel = 0;
	lightingBarrier.subresourceRange.levelCount = 1;
	lightingBarrier.subresourceRange.baseArrayLayer = 0;
	lightingBarrier.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(
		commandBuffers[currentFrame].commandBuffer,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &lightingBarrier
	);


	//Final Blit Pass
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

	std::array<VkDescriptorSet, 1> blitSets{
		descriptorManager.targetDescriptorSet.descriptorSet
	};

	vkCmdBindDescriptorSets(commandBuffers[currentFrame].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, swapchainPipeline.pipelineLayout, 0, blitSets.size(), blitSets.data(), 0, nullptr);

	vkCmdPushConstants(commandBuffers[currentFrame].commandBuffer, swapchainPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantMVP), &push);
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
	vkQueueWaitIdle(vulkanContext.device.graphicsQueue);
	vkQueueWaitIdle(vulkanContext.device.presentQueue);
	vkDeviceWaitIdle(vulkanContext.vulkanResources.device);

	destroy();
	framebuffers.clear();
	swapchain.destroySwapchain();

	currentFrame = 0;
	imageIndex = UINT32_MAX;

	swapchain.initSwapchain();
	initSwapchainResources();
	initGBufferResources();
	initLightingResources();

	//initFramebuffers();
	initCommandBuffers();
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
	globalUniformBuffers.reserve(maxFramesInFlight);
	for (size_t i = 0; i < maxFramesInFlight; ++i) {
		globalUniformBuffers.emplace_back(vulkanContext.vulkanResources);
		globalUniformBuffers[i].initUniformBuffer(sizeof(globalUBO));
	}
}

void Renderer::initStorageBuffers()
{
	objectStorageBuffers.reserve(maxFramesInFlight);
	lightingStorageBuffers.reserve(maxFramesInFlight);
	for (size_t i = 0; i < maxFramesInFlight; ++i) {
		objectStorageBuffers.emplace_back(vulkanContext.vulkanResources);
		lightingStorageBuffers.emplace_back(vulkanContext.vulkanResources);

		objectStorageBuffers[i].initStorageBuffer(sizeof(objectSSBO) * 1000);
		lightingStorageBuffers[i].initStorageBuffer(sizeof(lightSSBO) * 10);
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

	VkSamplerCreateInfo depthSamplerInfo{};
	depthSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	depthSamplerInfo.magFilter = VK_FILTER_LINEAR;
	depthSamplerInfo.minFilter = VK_FILTER_LINEAR;
	depthSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	depthSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	depthSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	depthSamplerInfo.anisotropyEnable = VK_FALSE;
	depthSamplerInfo.maxAnisotropy = 1.0f;
	depthSamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	depthSamplerInfo.unnormalizedCoordinates = VK_FALSE;
	depthSamplerInfo.compareEnable = VK_FALSE;
	depthSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	depthSamplerInfo.mipLodBias = 0.0f;
	depthSamplerInfo.minLod = 0.0f;
	depthSamplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(vulkanContext.vulkanResources.device, &depthSamplerInfo, nullptr, &depthSampler) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create depth sampler");
	}
}

void Renderer::initGBufferResources()
{
	gBufferAlbedoImage.initImage(VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, { swapchain.swapchain.extent.width, swapchain.swapchain.extent.height, 1 },
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	gBufferAlbedoImage.initImageView(VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	gBufferNormalImage.initImage(VK_IMAGE_TYPE_2D, VK_FORMAT_R16G16B16A16_SFLOAT, { swapchain.swapchain.extent.width, swapchain.swapchain.extent.height, 1 },
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	gBufferNormalImage.initImageView(VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R16G16B16A16_SFLOAT, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	gBufferMaterialImage.initImage(VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, { swapchain.swapchain.extent.width, swapchain.swapchain.extent.height, 1 },
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	gBufferMaterialImage.initImageView(VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	gBufferDepthImage.initImage(VK_IMAGE_TYPE_2D, VK_FORMAT_D32_SFLOAT, { swapchain.swapchain.extent.width, swapchain.swapchain.extent.height, 1 },
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	gBufferDepthImage.initImageView(VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_D32_SFLOAT, { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 });

	std::vector<VkImageView> gBufferAttachments = { gBufferAlbedoImage.imageView, gBufferNormalImage.imageView, gBufferMaterialImage.imageView, gBufferDepthImage.imageView };
	gBufferFramebuffer.initFrameBuffer(gBufferAttachments, swapchain.swapchain.extent.width, swapchain.swapchain.extent.height, 1, gBufferRenderPass.renderPass);
}

void Renderer::initGBufferPass()
{
	VkAttachmentDescription albedoAttachment{};
	albedoAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
	albedoAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	albedoAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	albedoAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	albedoAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	albedoAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	albedoAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference albedoAttachmentReference{};
	albedoAttachmentReference.attachment = 0;
	albedoAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription normalAttachment{};
	normalAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	normalAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	normalAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	normalAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	normalAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	normalAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	normalAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference normalAttachmentReference{};
	normalAttachmentReference.attachment = 1;
	normalAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription materialAttachment{};
	materialAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
	materialAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	materialAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	materialAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	materialAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	materialAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	materialAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference materialAttachmentReference{};
	materialAttachmentReference.attachment = 2;
	materialAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = VK_FORMAT_D32_SFLOAT;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference{};
	depthAttachmentReference.attachment = 3;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::array<VkAttachmentReference, 3> colorAttachmentsReferences = {
		albedoAttachmentReference,
		normalAttachmentReference,
		materialAttachmentReference
	};

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentsReferences.size());
	subpass.pColorAttachments = colorAttachmentsReferences.data();
	subpass.pDepthStencilAttachment = &depthAttachmentReference;

	std::vector<VkAttachmentDescription> attachments = { albedoAttachment, normalAttachment, materialAttachment, depthAttachment };
	std::vector<VkSubpassDescription> subpasses = { subpass };
	std::vector<VkSubpassDependency> dependencies = { };

	gBufferRenderPass.initRenderPass(attachments, subpasses, dependencies);
}

void Renderer::initGBufferPipeline()
{
	auto vertShader = readFile("gbuffer.vert.spv");
	auto fragShader = readFile("gbuffer.frag.spv");

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
	rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterInfo.depthBiasEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
	depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilInfo.depthTestEnable = VK_TRUE;
	depthStencilInfo.depthWriteEnable = VK_TRUE;
	depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilInfo.stencilTestEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
	multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingInfo.sampleShadingEnable = VK_FALSE;
	multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	std::array<VkPipelineColorBlendAttachmentState, 3> colorAttachments{};
	for (auto& colorBlendAttachmentState : colorAttachments) {
		colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachmentState.blendEnable = VK_FALSE;
	}

	VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
	colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendInfo.logicOpEnable = VK_FALSE;
	colorBlendInfo.attachmentCount = 3;
	colorBlendInfo.pAttachments = colorAttachments.data();


	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PushConstantMVP);

	std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = {
		descriptorManager.globalDescriptorSetLayout,
		descriptorManager.bindlessResourceDescriptorSetLayout,
	};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	gBufferPipeline.initPipeline(vertexShaderModule, fragmentShaderModule, pipelineLayoutInfo, rasterInfo, depthStencilInfo, colorBlendInfo, vertexInputInfo, inputAssemblyInfo, viewportState, multisamplingInfo, gBufferRenderPass.renderPass);

	vkDestroyShaderModule(vulkanContext.vulkanResources.device, vertexShaderModule, nullptr);
	vkDestroyShaderModule(vulkanContext.vulkanResources.device, fragmentShaderModule, nullptr);
}

void Renderer::initLightingResources()
{
	lightingImage.initImage(VK_IMAGE_TYPE_2D, VK_FORMAT_R16G16B16A16_SFLOAT, { swapchain.swapchain.extent.width, swapchain.swapchain.extent.height, 1 },
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	lightingImage.initImageView(VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R16G16B16A16_SFLOAT, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	std::vector<VkImageView> lightingAttachments = { lightingImage.imageView };
	lightingFramebuffer.initFrameBuffer(lightingAttachments, swapchain.swapchain.extent.width, swapchain.swapchain.extent.height, 1, lightingRenderPass.renderPass);
}

void Renderer::initLightingPass()
{
	VkAttachmentDescription lightingAttachment{};
	lightingAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	lightingAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	lightingAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	lightingAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	lightingAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	lightingAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	lightingAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference lightingAttachmentReference{};
	lightingAttachmentReference.attachment = 0;
	lightingAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &lightingAttachmentReference;

	std::vector<VkAttachmentDescription> attachments = { lightingAttachment };
	std::vector<VkSubpassDescription> subpasses = { subpass };
	std::vector<VkSubpassDependency> dependencies = { };

	lightingRenderPass.initRenderPass(attachments, subpasses, dependencies);
}

void Renderer::initLightingPipeline()
{
	auto vertShader = readFile("lighting.vert.spv");
	auto fragShader = readFile("lighting.frag.spv");

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

	std::array<VkDescriptorSetLayout, 3> descriptorSetLayouts = {
		descriptorManager.globalDescriptorSetLayout,
		descriptorManager.bindlessResourceDescriptorSetLayout,
		descriptorManager.targetDescriptorSetLayout,
	};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	lightingPipeline.initPipeline(vertexShaderModule, fragmentShaderModule, pipelineLayoutInfo, rasterInfo, depthStencilInfo, colorBlendInfo, vertexInputInfo, inputAssemblyInfo, viewportState, multisamplingInfo, lightingRenderPass.renderPass);

	vkDestroyShaderModule(vulkanContext.vulkanResources.device, vertexShaderModule, nullptr);
	vkDestroyShaderModule(vulkanContext.vulkanResources.device, fragmentShaderModule, nullptr);
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

	std::array<VkDescriptorSetLayout, 1> descriptorSetLayouts = {
		descriptorManager.targetDescriptorSetLayout
	};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	swapchainPipeline.initPipeline(vertexShaderModule, fragmentShaderModule, pipelineLayoutInfo, rasterInfo, depthStencilInfo, colorBlendInfo, vertexInputInfo, inputAssemblyInfo, viewportState, multisamplingInfo, swapchainRenderPass.renderPass);

	vkDestroyShaderModule(vulkanContext.vulkanResources.device, vertexShaderModule, nullptr);
	vkDestroyShaderModule(vulkanContext.vulkanResources.device, fragmentShaderModule, nullptr);
}