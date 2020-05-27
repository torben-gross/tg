#ifndef TG_GRAPHICS_VULKAN_MEMORY_ALLOCATOR_H
#define TG_GRAPHICS_VULKAN_MEMORY_ALLOCATOR_H

#include "tg_common.h"

#ifdef TG_VULKAN

#include <vulkan/vulkan.h>

typedef struct tg_vulkan_memory_block
{
    u32               pool_index;
    VkDeviceMemory    device_memory;
    VkDeviceSize      offset;
    VkDeviceSize      size;
    void*             p_mapped_device_memory;
} tg_vulkan_memory_block;

tg_vulkan_memory_block    tg_vulkan_memory_allocator_alloc(VkDeviceSize size, u32 memory_type_bits, VkMemoryPropertyFlags memory_property_flags);
void                      tg_vulkan_memory_allocator_free(tg_vulkan_memory_block* p_memory);
void                      tg_vulkan_memory_allocator_init(VkDevice device, VkPhysicalDevice physical_device);
void                      tg_vulkan_memory_allocator_shutdown();

#endif

#endif
