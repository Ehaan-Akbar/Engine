#include "ResourceManager.h"

ResourceManager::ResourceManager()
{
	loadImage(createImage("default.png", TEXTURES));
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

std::shared_ptr<ResourceManager::MeshResource> ResourceManager::loadOBJ(const std::string&& file)
{
	std::shared_ptr<MeshResource> mesh = std::make_shared<MeshResource>();
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file.c_str())) {
		throw std::runtime_error(warn + err);
	}


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
				uniqueVertices[vertex] = static_cast<uint32_t>(mesh->vertices->size());
				mesh->vertices->push_back(vertex);
			}

			mesh->indices->push_back(uniqueVertices[vertex]);
		}
	}
	
	return mesh;
}

std::vector<std::shared_ptr<ResourceManager::MeshResource>> ResourceManager::loadGLTF(const std::string&& file)
{
	std::vector<std::shared_ptr<MeshResource>> meshes;
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err, warn;

	if (!loader.LoadASCIIFromFile(&model, &err, &warn, file.c_str())) {
		throw std::runtime_error(warn + err);
	}

	std::vector<uint32_t> gltfToImageResourceID(model.images.size());
	for (size_t i = 0; i < model.images.size(); ++i) {
		const tinygltf::Image& image = model.images[i];

		if (image.component != 4) {
			throw std::runtime_error("Only RGBA rn");
		}

		size_t pixelCount = static_cast<size_t>(image.width) * image.height * 4;
		/*stbi_uc* pixels = new stbi_uc[pixelCount];
		if (image.image.size() == pixelCount) {
			std::memcpy(pixels, image.image.data(), pixelCount);
		}*/
		stbi_uc* pixels = static_cast<stbi_uc*>(malloc(pixelCount));
		if (!pixels) {
			throw std::runtime_error("Out of memory alocateing gltf image lmfao");
		}
		if (image.image.size() == pixelCount) {
			std::memcpy(pixels, image.image.data(), pixelCount);
		}
		else {
			throw std::runtime_error("fa");
		}

		auto imageRes = createImage("Path", TEXTURES);
		loadImage(imageRes, pixels, image.width, image.height);

		gltfToImageResourceID[i] = imageRes->getID();
	}

	for (const auto& mesh : model.meshes) {
		for (const auto& primitive : mesh.primitives) {
			std::shared_ptr<MeshResource> mesh = std::make_shared<MeshResource>();

			const tinygltf::Accessor& posAcc = model.accessors.at(primitive.attributes.at("POSITION"));
			const tinygltf::BufferView& posView = model.bufferViews.at(posAcc.bufferView);
			const tinygltf::Buffer posBuf = model.buffers.at(posView.buffer);

			const float* positions = reinterpret_cast<const float*>(&posBuf.data[posView.byteOffset + posAcc.byteOffset]);

			const float* normals = nullptr;
			if (primitive.attributes.count("NORMAL")) {
				const auto& acc = model.accessors.at(primitive.attributes.at("NORMAL"));
				const auto& view = model.bufferViews.at(acc.bufferView);
				const auto& buf = model.buffers.at(view.buffer);

				normals = reinterpret_cast<const float*>(&buf.data[view.byteOffset + acc.byteOffset]);
			}

			const float* uvs = nullptr;
			if (primitive.attributes.count("TEXCOORD_0")) {
				const auto& acc = model.accessors.at(primitive.attributes.at("TEXCOORD_0"));
				const auto& view = model.bufferViews.at(acc.bufferView);
				const auto& buf = model.buffers.at(view.buffer);

				uvs = reinterpret_cast<const float*>(&buf.data[view.byteOffset + acc.byteOffset]);
			}

			const size_t vertexCount = posAcc.count;
			for (size_t i = 0; i < vertexCount; ++i) {
				Vertex vertex{};

				vertex.pos = {
					positions[i * 3 + 0],
					positions[i * 3 + 1],
					positions[i * 3 + 2]
				};

				if (normals) {
					vertex.normal = {
						normals[i * 3 + 0],
						normals[i * 3 + 1],
						normals[i * 3 + 2]
					};
				}

				if (uvs) {
					vertex.uv = {
						uvs[i * 2 + 0],
						uvs[i * 2 + 1]
					};
				}

				vertex.color = { 1.0f, 1.0f, 1.0f };
				mesh->vertices->push_back(vertex);
			}

			if (primitive.indices >= 0) {
				const tinygltf::Accessor& indexAcc = model.accessors.at(primitive.indices);
				const tinygltf::BufferView indexView = model.bufferViews.at(indexAcc.bufferView);
				const tinygltf::Buffer indexBuf = model.buffers.at(indexView.buffer);

				const void* data = &indexBuf.data[indexView.byteOffset + indexAcc.byteOffset];

				const size_t indexCount = indexAcc.count;
				for (size_t i = 0; i < indexCount; ++i) {
					uint32_t index = 0;
					
					switch (indexAcc.componentType) {
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
						index = reinterpret_cast<const uint8_t*>(data)[i];
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
						index = reinterpret_cast<const uint16_t*>(data)[i];
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
						index = reinterpret_cast<const uint32_t*>(data)[i];
						break;

					default:
						throw std::runtime_error("Not supported type index");
					}

					mesh->indices->push_back(index);
				}
			}

			if (primitive.material >= 0) {
				const auto& mat = model.materials[primitive.material];

				auto resolveTexture = [&](int texIndex) -> int {
					if (texIndex < 0) return -1;
					const auto& tex = model.textures[texIndex];
					return gltfToImageResourceID[tex.source];
					};

				mesh->albedoIndex = resolveTexture(mat.pbrMetallicRoughness.baseColorTexture.index);
				mesh->roughnessIndex = resolveTexture(mat.pbrMetallicRoughness.metallicRoughnessTexture.index);
				mesh->normalIndex = resolveTexture(mat.normalTexture.index);
				mesh->occlusionIndex = resolveTexture(mat.occlusionTexture.index);
				mesh->emissiveIndex = resolveTexture(mat.emissiveTexture.index);
			}

			meshes.push_back(mesh);
		}
	}

	return meshes;
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

void ResourceManager::loadImage(std::shared_ptr<ImageResource> imageResource, stbi_uc* pixels, int width, int height)
{
	if (imageResource->cpuState == ResourceState::UNLOADED) {
		imageResource->cpuState = ResourceState::LOADING;
		if (!pixels) {
			imageResource->cpuState = ResourceState::FAILED;
			return;
		}
		imageResource->width = width;
		imageResource->height = height;
		imageResource->pixels = pixels;

		imageResource->cpuState = ResourceState::LOADED;
	}

	uploadQueue.push(imageResource);
}

ResourceManager::~ResourceManager()
{

}
