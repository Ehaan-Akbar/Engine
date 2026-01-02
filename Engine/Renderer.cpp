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
	initPreprocessIBLPasses();

	initSwapchainResources();
	initGBufferResources();
	initLightingResources();
	initPreprocessIBLResources();

	descriptorManager.initDescriptorManager();
	
	initSwapchainPipeline();
	initGBufferPipeline();
	initSkyboxPipeline();
	initLightingPipeline();
	initPreprocessIBLPipelines();

	size_t maxImageSize = 4096 * 4096 * 4;
	stagingBuffer.initStagingBuffer(maxImageSize);
	

	justFix();
}

void Renderer::destroy()
{
	vkDeviceWaitIdle(vulkanContext.vulkanResources.device);

	
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

	if (cubemapSampler != VK_NULL_HANDLE) {
		vkDestroySampler(vulkanContext.vulkanResources.device, cubemapSampler, nullptr);
	}

	// Destroy image views / images (Image::destroyImage should also be safe / idempotent)

	gBufferAlbedoImage.destroyImage();
	gBufferNormalImage.destroyImage();
	gBufferMaterialImage.destroyImage();
	gBufferDepthImage.destroyImage();
	lightingImage.destroyImage();
	for (auto& image : images) {
		delete image;
	}

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

	//if (handleResourcesUpload(resourceManager, commandBuffers[currentFrame].commandBuffer)) {
	//	commandBuffers[currentFrame].end();
	//	//Submit
	//	commandBuffers[currentFrame].submit(vulkanContext.device.graphicsQueue, inFlightFences[currentFrame], imageAvailableSemaphores[currentFrame], renderFinishedSemaphores[currentFrame]);
	//	return;
	//};

	//Processing scene
	uint32_t currentVertexOffset = 0;
	uint32_t currentIndexOffset = 0;
	uint32_t uboIndex = 0;
	drawInfos.clear();
	lightInfos.clear();
	drawInfos.reserve(ecs.getEntityCount());
	lightInfos.reserve(ecs.getEntityCount());
	batchedVertices.clear();
	batchedIndices.clear();

	ecs.onEachEntity([&](std::shared_ptr<Entity> entity) {
		const auto& l = ecs.getComponent<Light>(entity);
		if (l) {
			lightInfos.push_back({
				.type = l->type,
				.direction = l->direction,
				.position = l->position,
				.color = l->color
				});
			return;
		}
		const auto& t = ecs.getComponent<Transform>(entity);
		const auto& m = ecs.getComponent<Mesh>(entity);
		const auto& ma = ecs.getComponent<Material>(entity);

		drawInfos.push_back({
			.indexCount = static_cast<uint32_t>(m->indices->size()),
			.firstIndex = currentIndexOffset,
			.vertexOffset = 0,
			.ssboIndex = uboIndex++,
			.transform = t,
			.material = ma
			});

		batchedVertices.insert(batchedVertices.end(), m->vertices->begin(), m->vertices->end());
		for (size_t i = 0; i < m->indices->size(); ++i) {
			batchedIndices.push_back((*m->indices)[i] + currentVertexOffset);
		}
		currentVertexOffset += m->vertices->size();
		currentIndexOffset += m->indices->size();
		});

	if (!isMainVertexBufferInitialized) {
		mainVertexBuffer.initVertexBuffer(std::make_shared<Vertices>(batchedVertices), vulkanContext.device.graphicsQueue, graphicsCommandPool.commandPool);
		mainIndexBuffer.initIndexBuffer(std::make_shared<Indices>(batchedIndices), vulkanContext.device.graphicsQueue, graphicsCommandPool.commandPool);
		isMainVertexBufferInitialized = true;
	}
	



	mainVertexBuffer.bind(commandBuffers[currentFrame].commandBuffer);
	mainIndexBuffer.bind(commandBuffers[currentFrame].commandBuffer);


	//Updating UBOs and SSBOs
	descriptorManager.globalDescriptorSet.update(DescriptorManager::GLOBAL_BINDING::GLOBAL_UBO, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, { globalUniformBuffers[currentFrame].buffer, 0, VK_WHOLE_SIZE });
	descriptorManager.globalDescriptorSet.update(DescriptorManager::GLOBAL_BINDING::OBJECT_SSBO, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, { objectStorageBuffers[currentFrame].buffer, 0, VK_WHOLE_SIZE });
	descriptorManager.globalDescriptorSet.update(DescriptorManager::GLOBAL_BINDING::LIGHTING_SSBO, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, { lightStorageBuffers[currentFrame].buffer, 0, VK_WHOLE_SIZE });

	//Writing to UBOs and SSBOs
	{
		std::vector<objectSSBO> ssbo(1000);
		for (int i = 0; i < drawInfos.size(); ++i) {
			auto& drawInfo = drawInfos[i];
			ssbo[i] = {
				.model = drawInfo.transform->transformationMatrix(),
				.albedoIndex = drawInfo.material->albedoIndex,
				.roughnessIndex = drawInfo.material->roughnessIndex,
				.normalIndex = drawInfo.material->normalIndex,
				.occlusionIndex = drawInfo.material->occlusionIndex,
				.emissiveIndex = drawInfo.material->emissiveIndex
			};

		}

		/*for (size_t i = 0; i < std::min<size_t>(drawInfos.size(), 8); ++i) {
			auto& d = drawInfos[i];
			std::cout << "[SSBO] draw=" << d.ssboIndex
				<< " albedo=" << d.material->albedoIndex
				<< " normal=" << d.material->normalIndex
				<< " roughness=" << d.material->roughnessIndex
				<< " occlusion=" << d.material->occlusionIndex
				<< " emissive=" << d.material->emissiveIndex
				<< "\n";
		}*/

		objectStorageBuffers[currentFrame].copy(sizeof(objectSSBO) * ssbo.size(), ssbo.data());
	}

	{
		std::vector<lightSSBO> ssbo(100);
		for (int i = 0; i < lightInfos.size(); ++i) {
			ssbo[i] = {
				.lightType = glm::vec4(lightInfos[i].type, 0.0f, 0.0f, 0.0f),
				.lightDir = lightInfos[i].direction,
				.lightPos = lightInfos[i].position,
				.lightColor = lightInfos[i].color
			};
		}

		lightStorageBuffers[currentFrame].copy(sizeof(lightSSBO) * ssbo.size(), ssbo.data());
	}

	{
		globalUBO ubo{
			.view = camera.getViewMatrix(),
			.projection = camera.getProjectionMatrix(),
			.camPos = glm::vec4(camera.position, 0.0f),
			.dimensions = glm::vec4(static_cast<float>(swapchain.swapchain.extent.width), static_cast<float>(swapchain.swapchain.extent.height), 0.0f, 0.0f),
			.inverseProjection = camera.getInverseProjectionMatrix(),
			.inverseView = camera.getInverseViewMatrix(),
			.numberOfEntities = glm::vec4(drawInfos.size(), lightInfos.size(), 0.0f, 0.0f)
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
		PushConstant push{
			.ssboIndex = info.ssboIndex,
		};

		vkCmdPushConstants(commandBuffers[currentFrame].commandBuffer, gBufferPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstant), &push);
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
	vkCmdSetViewport(commandBuffers[currentFrame].commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffers[currentFrame].commandBuffer, 0, 1, &scissor);

	PushConstant push{
		.ssboIndex = 0,
		.skyboxIndex = 0
	};


	vkCmdBindPipeline(commandBuffers[currentFrame].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline.pipeline);
	std::array<VkDescriptorSet, 3> skyboxSets{
		descriptorManager.globalDescriptorSet.descriptorSet,
		descriptorManager.bindlessResourceDescriptorSet.descriptorSet,
		descriptorManager.targetDescriptorSet.descriptorSet
	};
	vkCmdBindDescriptorSets(commandBuffers[currentFrame].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline.pipelineLayout, 0, skyboxSets.size(), skyboxSets.data(), 0, nullptr);
	vkCmdPushConstants(commandBuffers[currentFrame].commandBuffer, skyboxPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstant), &push);
	vkCmdDraw(commandBuffers[currentFrame].commandBuffer, 36, 1, 0, 0);


	vkCmdBindPipeline(commandBuffers[currentFrame].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lightingPipeline.pipeline);
	std::array<VkDescriptorSet, 3> lightingSets{
		descriptorManager.globalDescriptorSet.descriptorSet,
		descriptorManager.bindlessResourceDescriptorSet.descriptorSet,
		descriptorManager.targetDescriptorSet.descriptorSet
	};
	vkCmdBindDescriptorSets(commandBuffers[currentFrame].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lightingPipeline.pipelineLayout, 0, lightingSets.size(), lightingSets.data(), 0, nullptr);
	vkCmdPushConstants(commandBuffers[currentFrame].commandBuffer, lightingPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstant), &push);
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

	vkCmdPushConstants(commandBuffers[currentFrame].commandBuffer, swapchainPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstant), &push);
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

bool Renderer::preprocess(ResourceManager& resourceManager)
{
	vkResetCommandBuffer(initializationCommandBuffer.commandBuffer, 0);
	initializationCommandBuffer.begin();

	auto upload = handleResourcesUpload(resourceManager, initializationCommandBuffer.commandBuffer);

	//if upload not done, end and come back
	if (!upload) {
		initializationCommandBuffer.end();
		initializationCommandBuffer.submit(vulkanContext.device.graphicsQueue);
		vkDeviceWaitIdle(vulkanContext.vulkanResources.device);
		return false;
	}

	bool ibl = computeSkyBoxMaps(initializationCommandBuffer.commandBuffer);

	initializationCommandBuffer.end();
	initializationCommandBuffer.submit(vulkanContext.device.graphicsQueue);

	vkDeviceWaitIdle(vulkanContext.vulkanResources.device);



	//Destroy iamge views and frame buffers (both can stay but really not needed) FB -> IV

	for (auto& fb : irradianceFramebuffers) {
		fb.destroyFrameBuffer();
	}
	for (auto& v : prefilterFramebuffers) {
		for (auto& fb : v) {
			fb.destroyFrameBuffer();
		}
	}
	lutFramebuffer.destroyFrameBuffer();

	irradianceCubeMapImage.destroyTransientViews();
	prefilterCubeMapImage.destroyTransientViews();
	brdfLUTImage.destroyTransientViews();

	return upload && ibl ? true : false;
}

bool Renderer::handleResourcesUpload(ResourceManager& resourceManager, VkCommandBuffer& commandBuffer)
{
	//TODO: Upload First then worry about rendering
	images.reserve(images.size() + resourceManager.uploadQueue.size());

	if (!resourceManager.uploadQueue.empty()) {
		auto imageResource = resourceManager.uploadQueue.front();
		const uint32_t layerCount = static_cast<uint32_t>(imageResource->layers.size());

		if (imageResource->gpuState == ResourceManager::UNLOADED) {
			imageResource->setGPUState(ResourceManager::LOADING);

			images.emplace_back(new Image{ vulkanContext.vulkanResources });

			//std::cout << imageResource->width + " " << imageResource->height << "\n";
			images.back()->initImage(
				VK_IMAGE_TYPE_2D,
				VK_FORMAT_R8G8B8A8_SRGB,
				VkExtent3D{ static_cast<uint32_t>(imageResource->layers[0].width), static_cast<uint32_t>(imageResource->layers[0].height), 1 },
				VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY,
				1U,
				layerCount,
				VK_SAMPLE_COUNT_1_BIT,
				VK_IMAGE_TILING_OPTIMAL,
				imageResource->type == ResourceManager::TEXTURES ? 0 : VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
			);

			images.back()->transitionImageLayout(
				commandBuffer,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layerCount }
			);
		}
		
		static int i = 0;
		if (i < layerCount) {
			uint32_t imageSize = imageResource->layers[i].width * imageResource->layers[i].height * 4;
			stagingBuffer.copy(imageSize, imageResource->layers[i].pixels);

			images.back()->copyBufferToImage(
				commandBuffer,
				stagingBuffer.buffer,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				static_cast<uint32_t>(imageResource->layers[i].width),
				static_cast<uint32_t>(imageResource->layers[i].height),
				1U,
				i
			);
			i++;
			return false;
		}		

		images.back()->transitionImageLayout(
			commandBuffer,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layerCount }
		);

		images.back()->initImageView(
			imageResource->type == ResourceManager::TEXTURES ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_CUBE,
			VK_FORMAT_R8G8B8A8_SRGB,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layerCount }
		);
		//std::cout << '\n' << imageResource->getID() << std::endl;


		uint32_t appendedIndex = static_cast<uint32_t>(images.size() - 1);
		std::cout << "[TextureUpload] resID=" << imageResource->getID()
			<< " appendedIndex=" << appendedIndex
			<< " imageView=0x" << std::hex << images.back()->imageView << std::dec
			<< " path=\"" << imageResource->path << "\"\n";

		descriptorManager.bindlessResourceDescriptorSet.update(
			static_cast<uint32_t>(imageResource->type),
			imageResource->getID(),
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			{ imageResource->type == ResourceManager::TEXTURES ? textureSampler : cubemapSampler, images.back()->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
		);

		imageResource->setGPUState(ResourceManager::ResourceState::LOADED);
		//imageResource->free();

		resourceManager.uploadQueue.pop();
		i = 0;
		return false;
	}
	return true;
}

bool Renderer::computeSkyBoxMaps(VkCommandBuffer& commandBuffer)
{
	//TODO: find why we truly need to do this and render pass cant
	//TODO: Layouts start at undefined but this only works at start, if reusing this for dynamic upload then need to change old layout
	//TODO: Batch barriers </3
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.image = irradianceCubeMapImage.image;
	barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6 };

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	barrier.image = prefilterCubeMapImage.image;
	barrier.subresourceRange.levelCount = 5;
	barrier.subresourceRange.layerCount = 6;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	barrier.image = brdfLUTImage.image;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);



	VkClearValue clear = { 0.0f, 0.0f, 0.0f, 0.0f };


	for (uint32_t face = 0; face < 6; ++face) {
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = 32;
		viewport.height = 32;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;


		VkRect2D scissor{};
		scissor.offset = { 0,0 };
		scissor.extent = { 32, 32 };

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		std::vector<VkImageView> attachments = { irradianceCubeMapImage.createFaceView(face) };
		irradianceFramebuffers[face].initFrameBuffer(attachments, 32, 32, 1, irradiancePrefilterRenderPass.renderPass);

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = irradiancePrefilterRenderPass.renderPass;
		renderPassBeginInfo.framebuffer = irradianceFramebuffers[face].framebuffer;
		renderPassBeginInfo.renderArea = { {0, 0}, {32, 32} };
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = &clear;

		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, irradiancePipeline.pipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, irradiancePipeline.pipelineLayout, 0, 1, &descriptorManager.bindlessResourceDescriptorSet.descriptorSet, 0, nullptr);
		SkyboxPreprocessPushConstant push{
			.faceIndex = face
		};
		vkCmdPushConstants(commandBuffer, irradiancePipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyboxPreprocessPushConstant), &push);
		vkCmdDraw(commandBuffer, 6, 1, 0, 0);
		vkCmdEndRenderPass(commandBuffer);
	}

	for (uint32_t mip = 0; mip < 5; ++mip) {
		uint32_t mipSize = 128 >> mip;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = mipSize;
		viewport.height = mipSize;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;


		VkRect2D scissor{};
		scissor.offset = { 0,0 };
		scissor.extent = { mipSize, mipSize };

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		for (uint32_t face = 0; face < 6; ++face) {

			std::vector<VkImageView> attachments = { prefilterCubeMapImage.createFaceMipView(face, mip) };
			prefilterFramebuffers[face][mip].initFrameBuffer(attachments, mipSize, mipSize, 1, irradiancePrefilterRenderPass.renderPass);

			VkRenderPassBeginInfo renderPassBeginInfo{};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = irradiancePrefilterRenderPass.renderPass;
			renderPassBeginInfo.framebuffer = prefilterFramebuffers[face][mip].framebuffer;
			renderPassBeginInfo.renderArea = { {0, 0}, {mipSize, mipSize} };
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = &clear;

			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, prefilterPipeline.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, prefilterPipeline.pipelineLayout, 0, 1, &descriptorManager.bindlessResourceDescriptorSet.descriptorSet, 0, nullptr);
			SkyboxPreprocessPushConstant push{
				.faceIndex = face,
				.mipLevel = mip
			};
			vkCmdPushConstants(commandBuffer, prefilterPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyboxPreprocessPushConstant), &push);
			vkCmdDraw(commandBuffer, 6, 1, 0, 0);
			vkCmdEndRenderPass(commandBuffer);

		}
	}

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = 512;
	viewport.height = 512;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;


	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = { 512, 512 };

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	std::vector<VkImageView> attachments = { brdfLUTImage.imageView };
	lutFramebuffer.initFrameBuffer(attachments, 512, 512, 1, lutRenderPass.renderPass);

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = lutRenderPass.renderPass;
	renderPassBeginInfo.framebuffer = lutFramebuffer.framebuffer;
	renderPassBeginInfo.renderArea = { {0, 0}, {512, 512} };
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clear;

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lutPipeline.pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lutPipeline.pipelineLayout, 0, 1, &descriptorManager.bindlessResourceDescriptorSet.descriptorSet, 0, nullptr);
	SkyboxPreprocessPushConstant push{
		.faceIndex = 0,
		.mipLevel = 0
	};
	vkCmdPushConstants(commandBuffer, lutPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyboxPreprocessPushConstant), &push);
	vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);

	VkImageMemoryBarrier barrier2{};
	barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier2.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier2.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier2.image = irradianceCubeMapImage.image;
	barrier2.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6 };

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier2);

	barrier2.image = prefilterCubeMapImage.image;
	barrier2.subresourceRange.levelCount = 5;
	barrier2.subresourceRange.layerCount = 6;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier2);

	barrier2.image = brdfLUTImage.image;
	barrier2.subresourceRange.levelCount = 1;
	barrier2.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier2);


	return true;
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

	initializationCommandBuffer.allocate();
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
	lightStorageBuffers.reserve(maxFramesInFlight);
	for (size_t i = 0; i < maxFramesInFlight; ++i) {
		objectStorageBuffers.emplace_back(vulkanContext.vulkanResources);
		lightStorageBuffers.emplace_back(vulkanContext.vulkanResources);

		objectStorageBuffers[i].initStorageBuffer(sizeof(objectSSBO) * 1000);
		lightStorageBuffers[i].initStorageBuffer(sizeof(lightSSBO) * 10);
	}
	
}

void Renderer::initSampler()
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	/*samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;*/
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
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

	VkSamplerCreateInfo cubemapSamplerInfo{};
	cubemapSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	cubemapSamplerInfo.magFilter = VK_FILTER_LINEAR;
	cubemapSamplerInfo.minFilter = VK_FILTER_LINEAR;
	cubemapSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	cubemapSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	cubemapSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	cubemapSamplerInfo.anisotropyEnable = VK_FALSE;
	cubemapSamplerInfo.maxAnisotropy = 1.0f;
	cubemapSamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	cubemapSamplerInfo.unnormalizedCoordinates = VK_FALSE;
	cubemapSamplerInfo.compareEnable = VK_FALSE;
	cubemapSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	cubemapSamplerInfo.mipLodBias = 0.0f;
	cubemapSamplerInfo.minLod = 0.0f;
	cubemapSamplerInfo.maxLod = 4.0f;

	if (vkCreateSampler(vulkanContext.vulkanResources.device, &cubemapSamplerInfo, nullptr, &cubemapSampler)) {
		throw std::runtime_error("Failed to create cubemap sampler");
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

	//Roughness Metallic Occlusion Emissive
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
	pushConstantRange.size = sizeof(PushConstant);

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

void Renderer::initSkyboxPipeline()
{
	auto vertShader = readFile("skybox.vert.spv");
	auto fragShader = readFile("skybox.frag.spv");

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
	rasterInfo.cullMode = VK_CULL_MODE_NONE;
	rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterInfo.depthBiasEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
	depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilInfo.depthTestEnable = VK_FALSE;
	depthStencilInfo.depthWriteEnable = VK_FALSE;
	depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilInfo.stencilTestEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
	multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingInfo.sampleShadingEnable = VK_FALSE;
	multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	std::array<VkPipelineColorBlendAttachmentState, 1> colorAttachments{};
	for (auto& colorBlendAttachmentState : colorAttachments) {
		colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachmentState.blendEnable = VK_FALSE;
	}

	VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
	colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendInfo.logicOpEnable = VK_FALSE;
	colorBlendInfo.attachmentCount = 1;
	colorBlendInfo.pAttachments = colorAttachments.data();


	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PushConstant);

	std::array<VkDescriptorSetLayout, 3> descriptorSetLayouts = {
		descriptorManager.globalDescriptorSetLayout,
		descriptorManager.bindlessResourceDescriptorSetLayout,
		descriptorManager.targetDescriptorSetLayout
	};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	skyboxPipeline.initPipeline(vertexShaderModule, fragmentShaderModule, pipelineLayoutInfo, rasterInfo, depthStencilInfo, colorBlendInfo, vertexInputInfo, inputAssemblyInfo, viewportState, multisamplingInfo, lightingRenderPass.renderPass);

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
	pushConstantRange.size = sizeof(PushConstant);

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

void Renderer::initPreprocessIBLResources()
{
	irradianceCubeMapImage.initImage(VK_IMAGE_TYPE_2D, VK_FORMAT_R16G16B16A16_SFLOAT, { 32, 32, 1 }, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY, 1, 6, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
	irradianceCubeMapImage.initImageView(VK_IMAGE_VIEW_TYPE_CUBE, VK_FORMAT_R16G16B16A16_SFLOAT, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6 });

	prefilterCubeMapImage.initImage(VK_IMAGE_TYPE_2D, VK_FORMAT_R16G16B16A16_SFLOAT, { 128, 128, 1 }, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY, 5, 6, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
	prefilterCubeMapImage.initImageView(VK_IMAGE_VIEW_TYPE_CUBE, VK_FORMAT_R16G16B16A16_SFLOAT, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 5, 0, 6 });

	brdfLUTImage.initImage(VK_IMAGE_TYPE_2D, VK_FORMAT_R16G16_SFLOAT, {512, 512, 1}, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL);
	brdfLUTImage.initImageView(VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R16G16_SFLOAT, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	irradianceFramebuffers.reserve(6);
	for (size_t i = 0; i < 6; ++i) {
		irradianceFramebuffers.emplace_back(vulkanContext.vulkanResources);
	}

	prefilterFramebuffers.resize(6);
	for (size_t i = 0; i < 6; ++i) {
		prefilterFramebuffers[i].reserve(5);
		for (size_t j = 0; j < 5; ++j) {
			prefilterFramebuffers[i].emplace_back(vulkanContext.vulkanResources);
		}
	}
	
}

void Renderer::initPreprocessIBLPasses()
{
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorReference{};
		colorReference.attachment = 0;
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorReference;

		std::vector<VkAttachmentDescription> attachments = { colorAttachment };
		std::vector<VkSubpassDescription> subpasses = { subpass };
		std::vector<VkSubpassDependency> dependencies = { };

		irradiancePrefilterRenderPass.initRenderPass(attachments, subpasses, dependencies);
	}

	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = VK_FORMAT_R16G16_SFLOAT;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorReference{};
		colorReference.attachment = 0;
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorReference;

		std::vector<VkAttachmentDescription> attachments = { colorAttachment };
		std::vector<VkSubpassDescription> subpasses = { subpass };
		std::vector<VkSubpassDependency> dependencies = { };

		lutRenderPass.initRenderPass(attachments, subpasses, dependencies);
	}
}

void Renderer::initPreprocessIBLPipelines()
{
	{
		auto vertShader = readFile("irradiance.vert.spv");
		auto fragShader = readFile("irradiance.frag.spv");

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
		pushConstantRange.size = sizeof(SkyboxPreprocessPushConstant);

		std::array<VkDescriptorSetLayout, 1> descriptorSetLayouts = {
			descriptorManager.bindlessResourceDescriptorSetLayout
		};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		irradiancePipeline.initPipeline(vertexShaderModule, fragmentShaderModule, pipelineLayoutInfo, rasterInfo, depthStencilInfo, colorBlendInfo, vertexInputInfo, inputAssemblyInfo, viewportState, multisamplingInfo, irradiancePrefilterRenderPass.renderPass);

		vkDestroyShaderModule(vulkanContext.vulkanResources.device, vertexShaderModule, nullptr);
		vkDestroyShaderModule(vulkanContext.vulkanResources.device, fragmentShaderModule, nullptr);
	}

	{
		auto vertShader = readFile("prefilter.vert.spv");
		auto fragShader = readFile("prefilter.frag.spv");

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
		pushConstantRange.size = sizeof(SkyboxPreprocessPushConstant);

		std::array<VkDescriptorSetLayout, 1> descriptorSetLayouts = {
			descriptorManager.bindlessResourceDescriptorSetLayout
		};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		prefilterPipeline.initPipeline(vertexShaderModule, fragmentShaderModule, pipelineLayoutInfo, rasterInfo, depthStencilInfo, colorBlendInfo, vertexInputInfo, inputAssemblyInfo, viewportState, multisamplingInfo, irradiancePrefilterRenderPass.renderPass);

		vkDestroyShaderModule(vulkanContext.vulkanResources.device, vertexShaderModule, nullptr);
		vkDestroyShaderModule(vulkanContext.vulkanResources.device, fragmentShaderModule, nullptr);
	}

	{
		auto vertShader = readFile("brdfLUT.vert.spv");
		auto fragShader = readFile("brdfLUT.frag.spv");

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
		pushConstantRange.size = sizeof(SkyboxPreprocessPushConstant);

		std::array<VkDescriptorSetLayout, 1> descriptorSetLayouts = {
			descriptorManager.bindlessResourceDescriptorSetLayout
		};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		lutPipeline.initPipeline(vertexShaderModule, fragmentShaderModule, pipelineLayoutInfo, rasterInfo, depthStencilInfo, colorBlendInfo, vertexInputInfo, inputAssemblyInfo, viewportState, multisamplingInfo, lutRenderPass.renderPass);

		vkDestroyShaderModule(vulkanContext.vulkanResources.device, vertexShaderModule, nullptr);
		vkDestroyShaderModule(vulkanContext.vulkanResources.device, fragmentShaderModule, nullptr);
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
	pushConstantRange.size = sizeof(PushConstant);

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