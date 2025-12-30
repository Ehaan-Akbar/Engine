#pragma once
#include "Helper.h"
#include "stb_image.h"
#include "tiny_obj_loader.h"
#include "tiny_gltf.h"
#include <unordered_map>
#include <vector>
#include <queue>
#include <array>
#include "Component.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include <cstring>


class ResourceManager
{
public:

	struct MeshResource {
		std::shared_ptr<Vertices> vertices = std::make_shared<Vertices>();
		std::shared_ptr<Indices> indices = std::make_shared<Indices>();
		
		uint32_t textureIndex;
		uint32_t albedoIndex;
		uint32_t roughnessIndex;
		uint32_t normalIndex;
		uint32_t occlusionIndex;
		uint32_t emissiveIndex;
	};

	enum ResourceType {
		TEXTURES = 0
	};

	enum ResourceState {
		UNLOADED = 0,
		LOADING = 1,
		LOADED = 2,
		FAILED = 3
	};

private:
	struct ImageResource {
		ResourceType type;
		ResourceState cpuState = ResourceState::UNLOADED;
		ResourceState gpuState = ResourceState::UNLOADED;

		std::string path;
		int width, height;
		stbi_uc* pixels;

		uint32_t refCount = 0;
		uint32_t id;

		void setGPUState(ResourceState state) {
			gpuState = state;
		}

		uint32_t getID() {
			return id;
		}

		~ImageResource() {
			free();
		}

		void free() {
			if (pixels) {
				stbi_image_free(pixels);
			}
		}
	};

public:

	ResourceManager();
	std::shared_ptr<ImageResource> createImage(std::string&& path, ResourceType type);
	std::shared_ptr<MeshResource> loadOBJ(const std::string&& file);
	std::vector<std::shared_ptr<MeshResource>> loadGLTF(const std::string&& file);
	void loadImage(std::shared_ptr<ImageResource> imageResource);
	void loadImage(std::shared_ptr<ImageResource> imageResource, stbi_uc* pixels, int width, int height);
	~ResourceManager();

	

	std::queue<std::shared_ptr<ImageResource>> uploadQueue;

private:

	std::array<std::vector<std::shared_ptr<ImageResource>>, 1> images;

	static uint32_t idCounter;
};

inline uint32_t ResourceManager::idCounter = 0;