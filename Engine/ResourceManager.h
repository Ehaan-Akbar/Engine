#pragma once
#include "Helper.h"
#include "stb_image.h"



class ResourceManager
{
public:

	struct TextureResource {
		std::string path;
		int width, height;
		std::vector<uint8_t> pixels;
	};

	ResourceManager();
	void createTextureImage(std::string path);
	~ResourceManager();


private:
	std::unordered_map<uint32_t, std::shared_ptr<TextureResource>> textures;
};

