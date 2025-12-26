#include "ResourceManager.h"

ResourceManager::ResourceManager()
{

}

std::shared_ptr<ResourceManager::ImageResource> ResourceManager::createImage(std::string&& path, ResourceType type)
{
	auto textureResource = std::make_shared<ImageResource>();
	textureResource->type = type;
	textureResource->path = path;
	textureResource->id = idCounter++;

	images[type].push_back(textureResource);

	return textureResource;
}

void ResourceManager::loadImage(std::shared_ptr<ImageResource> imageResource)
{
	if (imageResource->cpuState == ResourceState::UNLOADED) {
		imageResource->cpuState = ResourceState::LOADING;
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(imageResource->path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		if (!pixels) {
			imageResource->cpuState = ResourceState::FAILED;
			return;
		}

		imageResource->width = texWidth;
		imageResource->height = texHeight;
		imageResource->pixels = pixels;

		//stbi_image_free(pixels);
		imageResource->cpuState = ResourceState::LOADED;
	}

	uploadQueue.push(imageResource);
}

ResourceManager::~ResourceManager()
{

}
