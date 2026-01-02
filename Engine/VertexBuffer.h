#pragma once
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <string>
#include <stdexcept>
#include "vk_mem_alloc.h"
#include <array>
#include "Buffer.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include "CommandBuffer.h"


struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec3 normal;
	glm::vec4 tangent;
	glm::vec2 uv;

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && normal == other.normal && tangent == other.tangent && uv == other.uv;
	}
};

using Vertices = std::vector<Vertex>;

class VertexBuffer : protected Buffer
{
public:
	friend class Renderer;

	VertexBuffer(VulkanResources& vulkanResources);
	void initVertexBuffer(std::shared_ptr<Vertices> vertices, VkQueue transferQueue, VkCommandPool commandPool);
	void bind(VkCommandBuffer commandBuffer);
	void destroyVertexBuffer();
	~VertexBuffer();

	uint32_t vertexCount;

public:
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescription() {
		std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, normal);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, tangent);

		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[4].offset = offsetof(Vertex, uv);

		return attributeDescriptions;
	}

private:
	void uploadThroughStaging(void* data, VkDeviceSize size, VkQueue transferQueue, VkCommandPool commandPool);

private:
	VulkanResources& vulkanResources;
};

