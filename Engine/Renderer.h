#pragma once
#include "Instance.h"
#include "Surface.h"
#include "Device.h"
#include "Swapchain.h"
#include "RenderPass.h"
#include "Pipeline.h"

#include "Framebuffer.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "Buffer.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "UniformBuffer.h"
#include "DescriptorPool.h"
#include "DescriptorSet.h"
#include "Image.h"

#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <iostream>

#include "VulkanContext.h"

#include "ECS.h"
#include "Camera.h"

struct MAX_RESOURCE_COUNT {
	static const uint32_t UNIFORM_BUFFER = 16;
	static const uint32_t COMBINED_IMAGE_SAMPLER = 16;
};

struct PushConstantMVP {
	alignas(4) uint32_t uboIndex;
	alignas(4) uint32_t textureIndex;
	alignas(8) uint32_t padding[2];
};

struct UBO {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
	glm::vec4 lightPos;
	glm::vec4 lightDir;
	glm::vec4 camPos;
};


class Renderer
{

public:

	friend class App;

	Renderer(VulkanContext& vulkanContext);
	void init();
	void destroy();
	~Renderer();

	bool beginFrame();
	void submit(ECS& ecs, Camera& camera);
	void endFrame();


	void recreateSwapchain();


public:

	float getAspectRatio() const { return static_cast<float>(swapchain.swapchain.extent.width) / static_cast<float>(swapchain.swapchain.extent.height); }

private:
	void initCommandBuffers();
	void initSyncObjects();
	void initUniformBuffers();


	VulkanContext& vulkanContext;
	Swapchain swapchain{ vulkanContext.vulkanResources };

	
	void initSampler();
	VkSampler textureSampler;
	//
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	std::vector<Framebuffer> framebuffers;
	RenderPass swapchainRenderPass{ vulkanContext.vulkanResources };
	Pipeline swapchainPipeline{ vulkanContext.vulkanResources };
	void initSwapchainResources();
	void initSwapchainRenderPass();
	void initSwapchainPipeline();

	Image renderedImage{ vulkanContext.vulkanResources };
	Image renderedDepthImage{ vulkanContext.vulkanResources };
	Framebuffer renderedFramebuffer{ vulkanContext.vulkanResources };
	RenderPass renderedRenderPass{ vulkanContext.vulkanResources };
	Pipeline renderedPipeline{ vulkanContext.vulkanResources };
	void initRenderingResources();
	void initRenderingPass();
	void initRenderingPipeline();

	DescriptorPool sharedDescriptorPool{ vulkanContext.vulkanResources };
	void initDescriptorPools();

	VkDescriptorSetLayout uboDescriptorSetLayout;
	VkDescriptorSetLayout samplerDescriptorSetLayout;
	std::vector<DescriptorSet> uboDescriptorSets;
	DescriptorSet samplerDescriptorSet{ vulkanContext.vulkanResources };
	void initDescriptorSets();
	//

	//Bindless descriptorl
	VkDescriptorSetLayout bindlessDescriptorSetLayout;
	DescriptorPool bindlessDescriptorPool{ vulkanContext.vulkanResources };
	DescriptorSet bindlessDescriptorSet{ vulkanContext.vulkanResources };
	void initBindlessDescriptors();
	//


	CommandPool graphicsCommandPool{ vulkanContext.vulkanResources };
	std::vector<CommandBuffer> commandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	int maxFramesInFlight = 2;
	uint32_t currentFrame = 0;
	uint32_t imageIndex;






	VertexBuffer mainVertexBuffer{ vulkanContext.vulkanResources };
	IndexBuffer mainIndexBuffer{ vulkanContext.vulkanResources };
	std::vector<UniformBuffer> uniformBuffers;
	

	bool isMainVertexBufferInitialized = false;

	std::vector<Vertex> batchedVertices;
	std::vector<uint32_t> batchedIndices;

	std::vector<glm::mat4> modelTransforms;

};




