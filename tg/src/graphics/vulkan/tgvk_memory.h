#ifndef TGVK_MEMORY_H
#define TGVK_MEMORY_H

#include "tg_common.h"

#ifdef TG_VULKAN

#include "graphics/vulkan/tgvk_common.h"

#ifdef TG_DEBUG

#define TGVK_MALLOC(alignment, size, memory_type_bits, type) \
    tgvk_memory_allocator_alloc(alignment, size, memory_type_bits, type, __LINE__, __FILE__)

#define TGVK_MALLOC_DEBUG_INFO(alignment, size, memory_type_bits, type, line, p_filename) \
    tgvk_memory_allocator_alloc(alignment, size, memory_type_bits, type, line, p_filename)

#else

#define TGVK_MALLOC(alignment, size, memory_type_bits, type) \
    tgvk_memory_allocator_alloc(alignment, size, memory_type_bits, type)

#define TGVK_MALLOC_DEBUG_INFO(alignment, size, memory_type_bits, type, line, p_filename) \
    tgvk_memory_allocator_alloc(alignment, size, memory_type_bits, type)

#endif

typedef enum tgvk_memory_type
{
    TGVK_MEMORY_DEVICE    = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    TGVK_MEMORY_HOST      = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
} tgvk_memory_type;

typedef struct tgvk_memory_entry tgvk_memory_entry;
typedef struct tgvk_memory_block
{
    tgvk_memory_entry*    p_entry;
    VkDeviceMemory        device_memory;
    VkDeviceSize          offset;
    VkDeviceSize          size;
    void*                 p_mapped_device_memory;
} tgvk_memory_block;

void                 tgvk_memory_allocator_init(VkDevice device, VkPhysicalDevice physical_device);
void                 tgvk_memory_allocator_shutdown(VkDevice device);
VkDeviceSize         tgvk_memory_aligned_size(VkDeviceSize size);
VkDeviceSize         tgvk_memory_page_size(void);
tgvk_memory_block    tgvk_memory_allocator_alloc(VkDeviceSize alignment, VkDeviceSize size, u32 memory_type_bits, tgvk_memory_type type TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename));
void                 tgvk_memory_allocator_free(tgvk_memory_block* p_memory_block);

#endif

#endif
