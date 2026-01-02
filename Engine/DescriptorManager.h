#pragma once
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

class DescriptorManager
{
public:
	friend class Renderer;


	enum SET : uint32_t {
		GLOBAL = 0,
		BINDLESS_RESOURCES = 1,
		TARGET = 2
	};

	enum GLOBAL_BINDING : uint32_t {
		GLOBAL_UBO = 0,
		OBJECT_SSBO = 1,
		LIGHTING_SSBO = 2
	};

	enum TARGET_BINDING : uint32_t {
		ALBEDO_IMAGE = 0,
		NORMAL_IMAGE = 1,
		MATERIAL_IMAGE = 2,
		LIGHTING_IMAGE = 3,
		DEPTH_IMAGE = 4,
		SKYBOX_IRRADIANCE_IMAGE = 5,
		SKYBOX_PREFILTER_IMAGE = 6,
		SKYBOX_LUT_IMAGE = 7
	};

	enum RESOURCE_BINDING : uint32_t {
		TEXTURES = 0
	};

	DescriptorManager(VulkanResources& vulkanResources);
	void initDescriptorManager();
	void destroy();
	~DescriptorManager();


private:
	void initDescriptorPool();
	void initGlobalDescriptorSet();
	void initBindlessResourceDescriptorSet();
	void initTargetDescriptorSet();

	VulkanResources& vulkanResources;

	DescriptorPool descriptorPool{ vulkanResources };

	VkDescriptorSetLayout globalDescriptorSetLayout;
	DescriptorSet globalDescriptorSet{ vulkanResources };

	VkDescriptorSetLayout bindlessResourceDescriptorSetLayout;
	DescriptorSet bindlessResourceDescriptorSet{ vulkanResources };

	VkDescriptorSetLayout targetDescriptorSetLayout;
	DescriptorSet targetDescriptorSet{ vulkanResources };




};

