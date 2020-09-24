#ifndef TG_GRAPHICS_VULKAN_MEMORY_ALLOCATOR_H
#define TG_GRAPHICS_VULKAN_MEMORY_ALLOCATOR_H

#include "tg_common.h"

#ifdef TG_VULKAN

#pragma warning(push, 3)
#include <vulkan/vulkan.h>
#pragma warning(pop)

typedef struct tgvk_memory_block
{
    u32               pool_index;
    VkDeviceMemory    device_memory;
    VkDeviceSize      offset;
    VkDeviceSize      size;
    void*             p_mapped_device_memory;
} tgvk_memory_block;

void                 tgvk_memory_allocator_init(VkDevice device, VkPhysicalDevice physical_device);
void                 tgvk_memory_allocator_shutdown(VkDevice device);
VkDeviceSize         tgvk_memory_aligned_size(VkDeviceSize size);
tgvk_memory_block    tgvk_memory_allocator_alloc(VkDeviceSize alignment, VkDeviceSize size, u32 memory_type_bits, VkMemoryPropertyFlags memory_property_flags);
void                 tgvk_memory_allocator_free(tgvk_memory_block* p_memory_block);

#endif

#endif
