#include "graphics/vulkan/tg_vulkan_memory_allocator.h"

#ifdef TG_VULKAN

#include "math/tg_math.h"
#include "memory/tg_memory.h"
#include "platform/tg_platform.h"



#ifdef TG_DEBUG
#define VK_CALL(x)    TG_ASSERT(x == VK_SUCCESS) // TODO: how can i make this not be a duplicate?
#else
#define VK_CALL(x)    x
#endif

#define TG_ROUND(size) ((((size) + vulkan_memory.page_size - 1) / vulkan_memory.page_size) * vulkan_memory.page_size)



typedef struct tg_vulkan_memory_entry
{
    b32      reserved;
    u32      page_count;
    u64      offset;
} tg_vulkan_memory_entry;

typedef struct tg_vulkan_memory_pool
{
    VkMemoryPropertyFlags      memory_property_flags;
    VkDeviceMemory             device_memory;
    void*                      p_mapped_device_memory;
    u32                        total_page_count;
    u32                        reserved_page_count;
    u32                        entry_count;
    tg_vulkan_memory_entry*    p_entries;
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
            const VkDeviceSize aligned_size_per_allocation = TG_ROUND(size_per_allocation);
            p_allocation_size_per_memory_heap[i] = aligned_size_per_allocation;
        }
    }

    vulkan_memory.pool_count = physical_device_memory_properties.memoryTypeCount;
    for (u32 i = 0; i < vulkan_memory.pool_count; i++)
    {
        vulkan_memory.p_pools[i].memory_property_flags = physical_device_memory_properties.memoryTypes[i].propertyFlags;

        const VkDeviceSize allocation_size = p_allocation_size_per_memory_heap[physical_device_memory_properties.memoryTypes[i].heapIndex];
        TG_ASSERT(allocation_size / vulkan_memory.page_size <= TG_U32_MAX);
        const u32 total_page_count = (u32)(allocation_size / vulkan_memory.page_size);
        
        vulkan_memory.p_pools[i].total_page_count = total_page_count;
        vulkan_memory.p_pools[i].entry_count = 1;
        vulkan_memory.p_pools[i].p_entries = TG_MEMORY_ALLOC(total_page_count * sizeof(*vulkan_memory.p_pools[i].p_entries));
        vulkan_memory.p_pools[i].p_entries[0].reserved = TG_FALSE;
        vulkan_memory.p_pools[i].p_entries[0].page_count = total_page_count;
        vulkan_memory.p_pools[i].p_entries[0].offset = 0;

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
        if (vulkan_memory.p_pools[i].reserved_page_count != 0)
        {
            TG_DEBUG_LOG("%u unfreed pages in pool #%u in vulkan allocator!\n", vulkan_memory.p_pools[i].reserved_page_count, i);
        }
        vkFreeMemory(device, vulkan_memory.p_pools[i].device_memory, TG_NULL);
        TG_MEMORY_FREE(vulkan_memory.p_pools[i].p_entries);
    }
}

tg_vulkan_memory_block tg_vulkan_memory_allocator_alloc(VkDeviceSize alignment, VkDeviceSize size, u32 memory_type_bits, VkMemoryPropertyFlags memory_property_flags)
{
    TG_ASSERT(memory_type_bits && memory_property_flags);

    tg_vulkan_memory_block memory = { 0 };

    const VkDeviceSize aligned_size = TG_ROUND(size);
    const u32 required_page_count = (u32)(aligned_size / vulkan_memory.page_size);

    for (u32 i = 0; i < vulkan_memory.pool_count; i++)
    {
        const b32 is_required_memory_type = (memory_type_bits & (1 << i)) == (1 << i);
        const b32 has_required_memory_properties = (vulkan_memory.p_pools[i].memory_property_flags & memory_property_flags) == memory_property_flags;

        if (is_required_memory_type && has_required_memory_properties && vulkan_memory.p_pools[i].reserved_page_count + required_page_count <= vulkan_memory.p_pools[i].total_page_count)
        {
            tg_vulkan_memory_pool* p_pool = &vulkan_memory.p_pools[i];
            for (i32 j = p_pool->entry_count - 1; j >= 0; j--)
            {
                tg_vulkan_memory_entry* p_entry = &p_pool->p_entries[j];
                if (!p_entry->reserved && p_entry->page_count >= required_page_count)
                {
                    p_entry->reserved = TG_TRUE;
                    if (p_entry->page_count > required_page_count)
                    {
                        for (i32 k = p_pool->entry_count - 1; k > j; k--)
                        {
                            p_pool->p_entries[k + 1] = p_pool->p_entries[k];
                        }
                        p_pool->entry_count++;

                        tg_vulkan_memory_entry* p_new_entry = &p_pool->p_entries[j + 1];
                        p_new_entry->reserved = TG_FALSE;
                        p_new_entry->page_count = p_entry->page_count - required_page_count;
                        p_entry->page_count = required_page_count;
                        p_new_entry->offset = p_entry->offset + (p_entry->page_count * vulkan_memory.page_size);
                    }

                    memory.pool_index = i;
                    memory.device_memory = vulkan_memory.p_pools[i].device_memory;
                    memory.offset = p_entry->offset;
                    memory.size = aligned_size;
                    memory.p_mapped_device_memory = p_pool->p_mapped_device_memory ? (u8*)p_pool->p_mapped_device_memory + p_entry->offset : TG_NULL;

                    p_pool->reserved_page_count += required_page_count;
                    goto end;
                }
            }
        }
    }

    end:
    TG_ASSERT(memory.device_memory);

    return memory;
}

void tg_vulkan_memory_allocator_free(tg_vulkan_memory_block* p_memory)
{
    TG_ASSERT(p_memory);

    const u32 first_page = (u32)(p_memory->offset / (VkDeviceSize)vulkan_memory.page_size);
    const u32 page_count = (u32)(p_memory->size / vulkan_memory.page_size);
    vulkan_memory.p_pools[p_memory->pool_index].reserved_page_count -= page_count;

    tg_vulkan_memory_pool* p_pool = &vulkan_memory.p_pools[p_memory->pool_index];
    for (i32 i = p_pool->entry_count - 1; i >= 0; i--) // TODO: hashmap for faster deallocation?
    {
        tg_vulkan_memory_entry* p_entry = &p_pool->p_entries[i];
        if (p_entry->offset == p_memory->offset)
        {
            p_entry->reserved = TG_FALSE;
            u32 left_merge_count = 0;
            u32 right_merge_count = 0;

            for (i32 j = i - 1; j >= 0; j--)
            {
                if (!p_pool->p_entries[j].reserved)
                {
                    left_merge_count++;
                    p_entry->page_count += p_pool->p_entries[j].page_count;
                }
                else break;
            }
            for (u32 j = i + 1; j < p_pool->entry_count; j++)
            {
                if (!p_pool->p_entries[j].reserved)
                {
                    right_merge_count++;
                    p_entry->page_count += p_pool->p_entries[j].page_count;
				}
				else break;
			}

            if (left_merge_count != 0)
            {
                p_entry->offset = p_pool->p_entries[i - left_merge_count].offset;
                p_pool->p_entries[i - left_merge_count] = *p_entry;
            }
            for (u32 j = i + 1 + right_merge_count; j < p_pool->entry_count; j++)
            {
                p_pool->p_entries[j - left_merge_count - right_merge_count] = p_pool->p_entries[j];
            }
            p_pool->entry_count -= left_merge_count + right_merge_count;
        }
    }
}

#endif
