#include "ResourceManager.h"

ResourceManager::ResourceManager()
{

}

void ResourceManager::createTextureImage(std::string path)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels) {
		throw std::runtime_error("Failed to load texture image!");
	}

	auto textureResource = std::make_shared<TextureResource>();
	textureResource->path = path;
	textureResource->width = texWidth;
	textureResource->height = texHeight;
	textureResource->pixels.resize(texWidth * texHeight * 4);
	std::memcpy(textureResource->pixels.data(), pixels, texWidth * texHeight * 4);
}

ResourceManager::~ResourceManager()
{

}
