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


class transform : public Component {
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


class mesh : public Component {
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

	uint32_t textureIndex;

	void loadModel(const std::string& file) {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file.c_str())) {
			throw std::runtime_error(warn + err);
		}

		vertices->clear();
		indices->clear();


		std::unordered_map<Vertex, uint32_t> uniqueVertices{};


		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};
				
				if (index.vertex_index >= 0) {
					vertex.pos = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2],
					};

					auto colorIndex = 3 * index.vertex_index + 2;
					if (colorIndex < attrib.colors.size()) {
						vertex.color = {
							attrib.colors[colorIndex - 2],
							attrib.colors[colorIndex - 1],
							attrib.colors[colorIndex - 0],
						};
					}
					else {
						vertex.color = { 1.0f, 1.0f, 1.0f };
					}
				}

				if (index.normal_index >= 0) {
					vertex.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2],
					};
				}

				if (index.texcoord_index >= 0) {
					vertex.uv = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						attrib.texcoords[2 * index.texcoord_index + 1],
					};
				}

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices->size());
					vertices->push_back(vertex);
				}

				indices->push_back(uniqueVertices[vertex]);
			}
		}
	}
};

class material : public Component {
public:
	uint32_t textureIndex;
	uint32_t albedoIndex;
	uint32_t roughnessIndex;
	uint32_t normalIndex;
	uint32_t occlusionIndex;
	uint32_t emissiveIndex;
};

class light : public Component {
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

