#pragma once
#include "Helper.h"
#include "stb_image.h"
#include <unordered_map>
#include <vector>
#include <queue>
#include <array>


class ResourceManager
{
public:

	enum ResourceType {
		TEXTURES = 0,
		ALBEDOS = 1,
		ROUGHNESS = 2,
		NORMALS = 3,
		OCCLUSIONS = 4,
		EMISSIVES = 5
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
			if (pixels) {
				stbi_image_free(pixels);
			}
		}
	};

public:

	ResourceManager();
	std::shared_ptr<ImageResource> createImage(std::string&& path, ResourceType type);
	void loadImage(std::shared_ptr<ImageResource> imageResource);
	~ResourceManager();

	std::queue<std::shared_ptr<ImageResource>> uploadQueue;

private:

	std::array<std::vector<std::shared_ptr<ImageResource>>, 6> images;

	static uint32_t idCounter;
};

inline uint32_t ResourceManager::idCounter = 0;