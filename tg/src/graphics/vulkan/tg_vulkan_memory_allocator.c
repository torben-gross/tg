#include "graphics/vulkan/tg_vulkan_memory_allocator.h"

#ifdef TG_VULKAN

#ifdef TG_DEBUG
#define VK_CALL(x)    TG_ASSERT(x == VK_SUCCESS) // TODO: how can i make this not be a duplicate?
#else
#define VK_CALL(x)    x
#endif

#include "math/tg_math.h"
#include "memory/tg_memory.h"



typedef struct tg_vulkan_memory_pool
{
    VkMemoryPropertyFlags    memory_property_flags;
    VkDeviceMemory           device_memory;
    void*                    p_mapped_device_memory;
    u32                      page_count;
    u32                      reserved_page_count;
    b32*                     p_pages_reserved;
} tg_vulkan_memory_pool;

typedef struct tg_vulkan_memory
{
    VkDeviceSize             page_size;
    u32                      pool_count;
    tg_vulkan_memory_pool    p_pools[VK_MAX_MEMORY_TYPES];
} tg_vulkan_memory;



tg_vulkan_memory    vulkan_memory;

#ifdef TG_DEBUG
b32                 initialized = TG_FALSE;
#endif



VkDeviceSize tg_vulkan_memory_allocator_internal_round(VkDeviceSize size)
{
    const VkDeviceSize result = ((size + vulkan_memory.page_size - 1) / vulkan_memory.page_size) * vulkan_memory.page_size;
    return result;
}



tg_vulkan_memory_block tg_vulkan_memory_allocator_alloc(VkDeviceSize size, u32 memory_type_bits, VkMemoryPropertyFlags memory_property_flags)
{
    TG_ASSERT(memory_type_bits && memory_property_flags);

    const VkDeviceSize aligned_size = tg_vulkan_memory_allocator_internal_round(size);
    const u32 required_page_count = (u32)(aligned_size / vulkan_memory.page_size);

    for (u32 i = 0; i < vulkan_memory.pool_count; i++)
    {
        const b32 is_required_memory_type = (memory_type_bits & (1 << i)) == (1 << i);
        const b32 has_required_memory_properties = (vulkan_memory.p_pools[i].memory_property_flags & memory_property_flags) == memory_property_flags;

        i32 first_available_page = -1;
        u32 available_page_count = 0;
        if (is_required_memory_type && has_required_memory_properties && vulkan_memory.p_pools[i].reserved_page_count + required_page_count <= vulkan_memory.p_pools[i].page_count)
        {
            for (u32 j = 0; j < vulkan_memory.p_pools[i].page_count; j++)
            {
                if (!vulkan_memory.p_pools[i].p_pages_reserved[j])
                {
                    if (first_available_page == -1)
                    {
                        first_available_page = j;
                    }
                    available_page_count++;
                    if (available_page_count == required_page_count)
                    {
                        vulkan_memory.p_pools[i].reserved_page_count += required_page_count;
                        for (u32 k = 0; k < required_page_count; k++)
                        {
                            vulkan_memory.p_pools[i].p_pages_reserved[(u32)first_available_page + k] = VK_TRUE;
                        }

                        tg_vulkan_memory_block memory = { 0 };
                        memory.pool_index = i;
                        memory.device_memory = vulkan_memory.p_pools[i].device_memory;
                        memory.offset = (VkDeviceSize)first_available_page * vulkan_memory.page_size;
                        memory.size = aligned_size;
                        memory.p_mapped_device_memory = (void*)&((u8*)vulkan_memory.p_pools[i].p_mapped_device_memory)[memory.offset];

                        return memory;
                    }
                }
                else
                {
                    first_available_page = -1;
                    available_page_count = 0;
                }
            }
        }
    }

    TG_INVALID_CODEPATH();
    return (tg_vulkan_memory_block){ 0 };
}

void tg_vulkan_memory_allocator_free(tg_vulkan_memory_block* p_memory)
{
    TG_ASSERT(p_memory);

    const u32 first_page = (u32)(p_memory->offset / (VkDeviceSize)vulkan_memory.page_size);
    const u32 page_count = (u32)(p_memory->size / vulkan_memory.page_size);
    vulkan_memory.p_pools[p_memory->pool_index].reserved_page_count -= page_count;
    for (u32 i = 0; i < page_count; i++)
    {
        vulkan_memory.p_pools[p_memory->pool_index].p_pages_reserved[first_page + i] = TG_FALSE;
    }
}

void tg_vulkan_memory_allocator_init(VkDevice device, VkPhysicalDevice physical_device)
{
    TG_ASSERT(!initialized);

    VkPhysicalDeviceProperties physical_device_properties = { 0 };
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

    VkPhysicalDeviceMemoryProperties physical_device_memory_properties = { 0 };
    vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_memory_properties);
    
    VkDeviceSize p_memory_types_per_memory_heap[VK_MAX_MEMORY_HEAPS] = { 0 };
    for (u32 i = 0; i < physical_device_memory_properties.memoryTypeCount; i++)
    {
        p_memory_types_per_memory_heap[physical_device_memory_properties.memoryTypes[i].heapIndex]++;
    }
    VkDeviceSize p_allocation_size_per_memory_heap[VK_MAX_MEMORY_HEAPS] = { 0 };

    vulkan_memory.page_size = tgm_u64_max(1024, physical_device_properties.limits.bufferImageGranularity);

    const VkDeviceSize heap_size_fraction_denominator = 4;
    for (u32 i = 0; i < physical_device_memory_properties.memoryHeapCount; i++)
    {
        if (p_memory_types_per_memory_heap[i])
        {
            const VkDeviceSize size_per_allocation = (physical_device_memory_properties.memoryHeaps[i].size / heap_size_fraction_denominator) / p_memory_types_per_memory_heap[i];
            const VkDeviceSize aligned_size_per_allocation = tg_vulkan_memory_allocator_internal_round(size_per_allocation);
            p_allocation_size_per_memory_heap[i] = aligned_size_per_allocation;
        }
    }

    vulkan_memory.pool_count = physical_device_memory_properties.memoryTypeCount;
    for (u32 i = 0; i < vulkan_memory.pool_count; i++)
    {
        vulkan_memory.p_pools[i].memory_property_flags = physical_device_memory_properties.memoryTypes[i].propertyFlags;

        const VkDeviceSize allocation_size = p_allocation_size_per_memory_heap[physical_device_memory_properties.memoryTypes[i].heapIndex];
        TG_ASSERT(allocation_size / vulkan_memory.page_size <= TG_U32_MAX);
        const u32 page_count = (u32)(allocation_size / vulkan_memory.page_size);
        
        vulkan_memory.p_pools[i].page_count = page_count;
        vulkan_memory.p_pools[i].p_pages_reserved = TG_MEMORY_ALLOC(page_count * sizeof(*vulkan_memory.p_pools[i].p_pages_reserved));

        VkMemoryAllocateInfo memory_allocate_info = { 0 };
        memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_allocate_info.pNext = TG_NULL;
        memory_allocate_info.allocationSize = allocation_size;
        memory_allocate_info.memoryTypeIndex = i;

        VK_CALL(vkAllocateMemory(device, &memory_allocate_info, TG_NULL, &vulkan_memory.p_pools[i].device_memory));
        if (physical_device_memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            VK_CALL(vkMapMemory(device, vulkan_memory.p_pools[i].device_memory, 0, VK_WHOLE_SIZE, 0, &vulkan_memory.p_pools[i].p_mapped_device_memory));
        }
        else
        {
            vulkan_memory.p_pools[i].p_mapped_device_memory = TG_NULL;
        }
    }

#ifdef TG_DEBUG
    initialized = TG_TRUE;
#endif
}

void tg_vulkan_memory_allocator_shutdown(VkDevice device)
{
    TG_ASSERT(initialized);

    for (u32 i = 0; i < vulkan_memory.pool_count; i++)
    {
        TG_ASSERT(vulkan_memory.p_pools[i].reserved_page_count == 0);
        vkFreeMemory(device, vulkan_memory.p_pools[i].device_memory, TG_NULL);
        TG_MEMORY_FREE(vulkan_memory.p_pools[i].p_pages_reserved);
    }
}

#endif
