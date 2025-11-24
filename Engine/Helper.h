#pragma once
#include <iostream>
#include <functional>
#include <string>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>
#include "vk_mem_alloc.h"
#include "VkBootstrap.h"


static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

struct VulkanResources {
    VkInstance instance;
    VkSurfaceKHR surface;
    VkDevice device;
    VkPhysicalDevice physicalDevice;

    vkb::Device vkb_device;
    vkb::Instance vkb_instance;

    VmaAllocator allocator;
};

template <typename T, typename... Rest>
void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
    seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
    (hashCombine(seed, rest), ...);
}


