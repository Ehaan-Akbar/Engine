#pragma once
#include <typeindex>
#include <iostream>
#include <fstream>
#include <memory>
#include <unordered_map>


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "tiny_obj_loader.h"

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Helper.h"

namespace std {
	template<>
	struct hash<Vertex> {
		size_t operator()(const Vertex& vertex) const {
			size_t seed = 0;
			hashCombine(seed, vertex.pos, vertex.color, vertex.normal, vertex.uv);
			return seed;
		}
	};
}

class Component
{
public:

	virtual ~Component() = default;
};


class Transform : public Component {
public:
	glm::vec3 position{};
	glm::vec3 scale{ 1.f, 1.f, 1.f };
	glm::vec3 rotation{};

	glm::mat4 transformationMatrix() {
		auto transform = glm::translate(glm::mat4{ 1.f }, position);
		transform = glm::rotate(transform, rotation.y, { 0.0f, 1.0f, 0.0f });
		transform = glm::rotate(transform, rotation.x, { 1.0f, 0.0f, 0.0f });
		transform = glm::rotate(transform, rotation.z, { 0.0f, 0.0f, 1.0f });
		transform = glm::scale(transform, scale);
		return transform;
	}
};


class Mesh : public Component {
public:
	std::shared_ptr<Vertices> vertices = std::make_shared<Vertices>(std::initializer_list<Vertex>{
		// 0
		{ {-0.5f, -0.5f, 0.5f}, { 0.7f, 0.7f, 0.7f }, { -0.577f, -0.577f,  0.577f }, { 0.0f, 0.0f } },
			// 1
		{ { 0.5f, -0.5f,  0.5f}, { 0.7f, 0.7f, 0.7f },  { 0.577f, -0.577f,  0.577f}, {0.0f, 0.0f} },
			// 2
		{ { 0.5f,  0.5f,  0.5f}, { 0.7f, 0.7f, 0.7f },  { 0.577f,  0.577f,  0.577f}, {0.0f, 0.0f} },
			// 3
		{ {-0.5f,  0.5f,  0.5f}, { 0.7f, 0.7f, 0.7f },  {-0.577f,  0.577f,  0.577f}, {0.0f, 0.0f} },
			// 4
		{ {-0.5f, -0.5f, -0.5f}, { 0.7f, 0.7f, 0.7f },  {-0.577f, -0.577f, -0.577f}, {0.0f, 0.0f} },
			// 5
		{ { 0.5f, -0.5f, -0.5f}, { 0.7f, 0.7f, 0.7f },  { 0.577f, -0.577f, -0.577f}, {0.0f, 0.0f} },
			// 6
		{ { 0.5f,  0.5f, -0.5f}, { 0.7f, 0.7f, 0.7f },  { 0.577f,  0.577f, -0.577f}, {0.0f, 0.0f} },
			// 7
		{ {-0.5f,  0.5f, -0.5f}, { 0.7f, 0.7f, 0.7f },  {-0.577f,  0.577f, -0.577f}, {0.0f, 0.0f} },
	});

	std::shared_ptr<Indices> indices = std::make_shared<Indices>(std::initializer_list<uint32_t>{
		// Front face
		0, 1, 2, 2, 3, 0,
			// Right face
			1, 5, 6, 6, 2, 1,
			// Back face
			5, 4, 7, 7, 6, 5,
			// Left face
			4, 0, 3, 3, 7, 4,
			// Top face
			3, 2, 6, 6, 7, 3,
			// Bottom face
			4, 5, 1, 1, 0, 4
	});

};

class Material : public Component {
public:
	uint32_t albedoIndex = 0;
	uint32_t roughnessIndex = 0;
	uint32_t normalIndex = 0;
	uint32_t occlusionIndex = 0;
	uint32_t emissiveIndex = 0;
};

class Light : public Component {
public:
	enum LIGHT_TYPE : uint32_t {
		DIRECTIONAL,
		POINT,
		SPOT,
		AREA
	};

	LIGHT_TYPE type;
	glm::vec4 position;
	glm::vec4 direction;
	glm::vec4 color;
};

