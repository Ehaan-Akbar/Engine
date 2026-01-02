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
#include "StorageBuffer.h"
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
#include "DescriptorManager.h"
#include "Camera.h"
#include "ResourceManager.h"
#include "StagingBuffer.h"


struct MAX_RESOURCE_COUNT {
	static const uint32_t UNIFORM_BUFFER = 16;
	static const uint32_t COMBINED_IMAGE_SAMPLER_TEXTURE = 16;
	static const uint32_t COMBINED_IMAGE_SAMPLER_ALBEDO = 16;
	static const uint32_t COMBINED_IMAGE_SAMPLER_ROUGHNESS = 16;
	static const uint32_t COMBINED_IMAGE_SAMPLER_NORMAL = 16;
	static const uint32_t COMBINED_IMAGE_SAMPLER_OCCLUSION = 16;
	static const uint32_t COMBINED_IMAGE_SAMPLER_EMISSIVE = 16;

	static int getTotal() {
		return UNIFORM_BUFFER + COMBINED_IMAGE_SAMPLER_TEXTURE * 6;
	}
};

struct PushConstant {
	uint32_t ssboIndex;
	uint32_t skyboxIndex;
	uint32_t padding[2];
};

struct SkyboxPreprocessPushConstant {
	uint32_t faceIndex;
	uint32_t mipLevel;
	uint32_t paddingsigmasobkisibidohio[2];
};

struct globalUBO {
	glm::mat4 view;
	glm::mat4 projection;
	glm::vec4 camPos;
	glm::vec4 dimensions;
	glm::mat4 inverseProjection;
	glm::mat4 inverseView;
	//{Objects, Lights, Billboards?, Something random}
	glm::vec4 numberOfEntities;
};

struct objectSSBO {
	glm::mat4 model;
	uint32_t albedoIndex;
	uint32_t roughnessIndex;
	uint32_t normalIndex;
	uint32_t occlusionIndex;
	uint32_t emissiveIndex;
	uint32_t padding[3];
};

//0 - Directional
//1 - Point
//2 - Spot
//3 - Area
struct lightSSBO {
	glm::vec4 lightType;
	glm::vec4 lightDir;
	glm::vec4 lightPos;
	glm::vec4 lightColor;
};


class Renderer
{

public:

	friend class App;

	Renderer(VulkanContext& vulkanContext);
	void init();
	void update(ECS& ecs, Camera& camera, ResourceManager& resourceManager);
	void destroy();
	~Renderer();

	bool beginFrame();
	void submit(ECS& ecs, Camera& camera);
	void endFrame();

	bool preprocess(ResourceManager& resourceManager);
	bool handleResourcesUpload(ResourceManager& resourceManager, VkCommandBuffer& commandBuffer);
	bool computeSkyBoxMaps(VkCommandBuffer& commandBuffer);


	void recreateSwapchain();
	
	void justFix() {
		//TODO: Resources


		

		//Updating Target Descriptors
		descriptorManager.targetDescriptorSet.update(DescriptorManager::TARGET_BINDING::ALBEDO_IMAGE, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, { textureSampler, gBufferAlbedoImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
		descriptorManager.targetDescriptorSet.update(DescriptorManager::TARGET_BINDING::NORMAL_IMAGE, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, { textureSampler, gBufferNormalImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
		descriptorManager.targetDescriptorSet.update(DescriptorManager::TARGET_BINDING::MATERIAL_IMAGE, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, { textureSampler, gBufferMaterialImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
		descriptorManager.targetDescriptorSet.update(DescriptorManager::TARGET_BINDING::LIGHTING_IMAGE, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, { textureSampler, lightingImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
		descriptorManager.targetDescriptorSet.update(DescriptorManager::TARGET_BINDING::DEPTH_IMAGE, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, { depthSampler, gBufferDepthImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
		descriptorManager.targetDescriptorSet.update(DescriptorManager::TARGET_BINDING::SKYBOX_IRRADIANCE_IMAGE, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, { cubemapSampler, irradianceCubeMapImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
		descriptorManager.targetDescriptorSet.update(DescriptorManager::TARGET_BINDING::SKYBOX_PREFILTER_IMAGE, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, { cubemapSampler, prefilterCubeMapImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
		descriptorManager.targetDescriptorSet.update(DescriptorManager::TARGET_BINDING::SKYBOX_LUT_IMAGE, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, { textureSampler, brdfLUTImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	
	}

public:

	float getAspectRatio() const { return static_cast<float>(swapchain.swapchain.extent.width) / static_cast<float>(swapchain.swapchain.extent.height); }

private:
	void initCommandBuffers();
	void initSyncObjects();
	void initUniformBuffers();
	void initStorageBuffers();


	VulkanContext& vulkanContext;
	Swapchain swapchain{ vulkanContext.vulkanResources };

	
	void initSampler();
	VkSampler textureSampler;
	VkSampler depthSampler;
	VkSampler cubemapSampler;

	//Swapchain / Blit Pass
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	std::vector<Framebuffer> framebuffers;
	RenderPass swapchainRenderPass{ vulkanContext.vulkanResources };
	Pipeline swapchainPipeline{ vulkanContext.vulkanResources };
	void initSwapchainResources();
	void initSwapchainRenderPass();
	void initSwapchainPipeline();
	//Swapchain / Blit Pass

	//G-Buffer
	Image gBufferAlbedoImage{ vulkanContext.vulkanResources };
	Image gBufferNormalImage{ vulkanContext.vulkanResources };
	Image gBufferMaterialImage{ vulkanContext.vulkanResources };
	Image gBufferDepthImage{ vulkanContext.vulkanResources };

	Framebuffer gBufferFramebuffer{ vulkanContext.vulkanResources };
	RenderPass gBufferRenderPass{ vulkanContext.vulkanResources };
	Pipeline gBufferPipeline{ vulkanContext.vulkanResources };

	void initGBufferResources();
	void initGBufferPass();
	void initGBufferPipeline();
	//G-Buffer

	//Skybox
	Pipeline skyboxPipeline{ vulkanContext.vulkanResources };
	void initSkyboxPipeline();
	//Skybox

	//Lighting
	Image lightingImage{ vulkanContext.vulkanResources };
	Image lightingDepthImage{ vulkanContext.vulkanResources };

	Framebuffer lightingFramebuffer{ vulkanContext.vulkanResources };
	RenderPass lightingRenderPass{ vulkanContext.vulkanResources };
	Pipeline lightingPipeline{ vulkanContext.vulkanResources };

	void initLightingResources();
	void initLightingPass();
	void initLightingPipeline();
	//Lighting

	//IBL
	Image irradianceCubeMapImage{ vulkanContext.vulkanResources };
	Image prefilterCubeMapImage{ vulkanContext.vulkanResources };
	Image brdfLUTImage{ vulkanContext.vulkanResources };

	std::vector<Framebuffer> irradianceFramebuffers;
	std::vector<std::vector<Framebuffer>> prefilterFramebuffers;
	Framebuffer lutFramebuffer{ vulkanContext.vulkanResources };
	RenderPass irradiancePrefilterRenderPass{ vulkanContext.vulkanResources };
	RenderPass lutRenderPass{ vulkanContext.vulkanResources };
	Pipeline irradiancePipeline{ vulkanContext.vulkanResources };
	Pipeline prefilterPipeline{ vulkanContext.vulkanResources };
	Pipeline lutPipeline{ vulkanContext.vulkanResources };

	void initPreprocessIBLResources();
	void initPreprocessIBLPasses();
	void initPreprocessIBLPipelines();

	//IBL

	DescriptorManager descriptorManager{ vulkanContext.vulkanResources };

	//Images
	std::vector<Image*> images;
	//Images


	CommandPool graphicsCommandPool{ vulkanContext.vulkanResources };
	std::vector<CommandBuffer> commandBuffers;
	CommandBuffer initializationCommandBuffer{ vulkanContext.vulkanResources, graphicsCommandPool.commandPool };

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	int maxFramesInFlight = 2;
	uint32_t currentFrame = 0;
	uint32_t imageIndex;



	StagingBuffer stagingBuffer{ vulkanContext.vulkanResources };
	


	VertexBuffer mainVertexBuffer{ vulkanContext.vulkanResources };
	IndexBuffer mainIndexBuffer{ vulkanContext.vulkanResources };

	std::vector<UniformBuffer> globalUniformBuffers;
	std::vector<StorageBuffer> objectStorageBuffers;
	std::vector<StorageBuffer> lightStorageBuffers;
	
	

	bool isMainVertexBufferInitialized = false;

	std::vector<Vertex> batchedVertices;
	std::vector<uint32_t> batchedIndices;

	struct drawInfo {
		uint32_t indexCount;
		uint32_t firstIndex;
		uint32_t vertexOffset;
		uint32_t ssboIndex;
		std::shared_ptr<Transform> transform;
		std::shared_ptr<Material> material;
	};

	struct lightInfo {
		Light::LIGHT_TYPE type;
		glm::vec4 direction;
		glm::vec4 position;
		glm::vec4 color;
	};

	std::vector<drawInfo> drawInfos;
	std::vector<lightInfo> lightInfos;

};




