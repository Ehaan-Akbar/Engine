#define VK_ENABLE_BETA_EXTENSIONS
#define VKB_VK_HEADER <Volk/volk.h>

#define VK_NO_PROTOTYPES
#define VOLK_IMPLEMENTATION

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION

#include <Volk/volk.h>
#include "vk_mem_alloc.h"
#include "VkBootstrap.h"