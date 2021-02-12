#include "graphics/vulkan/tgvk_memory_allocator.h"

#ifdef TG_VULKAN

#include "math/tg_math.h"
#include "memory/tg_memory.h"
#include "platform/tg_platform.h"



#define TGVK_MAX_DEVICE_LOCAL_MEMORY_SIZE      (1LL << 32LL)
#define TGVK_MAX_HOST_VISIBLE_COHERENT_SIZE    (1LL << 25LL)

#ifdef TG_DEBUG
#define TG_LINK(p_prev_entry, p_next_entry)                                                                   \
    if ((p_prev_entry) && (p_next_entry))                                                                     \
    {                                                                                                         \
        TG_ASSERT((p_prev_entry)->offset_pages + (p_prev_entry)->page_count == (p_next_entry)->offset_pages); \
    }                                                                                                         \
    if (p_prev_entry) (p_prev_entry)->p_next = (p_next_entry);                                                \
    if (p_next_entry) (p_next_entry)->p_prev = (p_prev_entry)
#else
#define TG_LINK(p_prev_entry, p_next_entry)                                                                   \
    if (p_prev_entry) (p_prev_entry)->p_next = (p_next_entry);                                                \
    if (p_next_entry) (p_next_entry)->p_prev = (p_prev_entry)
#endif

#define TG_LINK_FREE(p_prev_free_entry, p_next_free_entry)                         \
    if (p_prev_free_entry) (p_prev_free_entry)->p_next_free = (p_next_free_entry); \
    if (p_next_free_entry) (p_next_free_entry)->p_prev_free = (p_prev_free_entry)

#define TG_CUT(p_entry)                                \
    {                                                  \
        TG_ASSERT(p_entry);                            \
        TG_LINK((p_entry)->p_prev, (p_entry)->p_next); \
        (p_entry)->p_prev = TG_NULL;                   \
        (p_entry)->p_next = TG_NULL;                   \
    }

#define TG_CUT_FREE(p_pool, p_entry)                                  \
    {                                                                 \
        TG_ASSERT(p_pool);                                            \
        TG_ASSERT(p_entry);                                           \
        TG_LINK_FREE((p_entry)->p_prev_free, (p_entry)->p_next_free); \
        if ((p_pool)->p_first_free_entry == (p_entry))                \
        {                                                             \
            (p_pool)->p_first_free_entry = (p_entry)->p_next_free;    \
        }                                                             \
        (p_entry)->p_prev_free = TG_NULL;                             \
        (p_entry)->p_next_free = TG_NULL;                             \
    }

#define TG_INSERT_FREE(p_pool, p_entry) \
    {                                                                                                                   \
        TG_ASSERT(p_pool);                                                                                              \
        TG_ASSERT(p_entry);                                                                                             \
        if ((p_pool)->p_first_free_entry == TG_NULL)                                                                    \
        {                                                                                                               \
            (p_pool)->p_first_free_entry = p_entry;                                                                     \
            (p_entry)->p_prev_free = TG_NULL;                                                                           \
            (p_entry)->p_next_free = TG_NULL;                                                                           \
        }                                                                                                               \
        else if ((p_pool)->p_first_free_entry->page_count > (p_entry)->page_count)                                      \
        {                                                                                                               \
            (p_entry)->p_prev_free = TG_NULL;                                                                           \
            TG_LINK_FREE(p_entry, (p_pool)->p_first_free_entry);                                                        \
            (p_pool)->p_first_free_entry = p_entry;                                                                     \
        }                                                                                                               \
        else                                                                                                            \
        {                                                                                                               \
            tgvk_memory_entry* p_prev_free = (p_pool)->p_first_free_entry;                                              \
            while (p_prev_free->p_next_free != TG_NULL && p_prev_free->p_next_free->page_count < (p_entry)->page_count) \
            {                                                                                                           \
                p_prev_free = p_prev_free->p_next_free;                                                                 \
            }                                                                                                           \
            tgvk_memory_entry* p_next_free = p_prev_free->p_next_free;                                                  \
            TG_LINK_FREE(p_prev_free, p_entry);                                                                         \
            TG_LINK_FREE(p_entry, p_next_free);                                                                         \
        }                                                                                                               \
    }

#define TG_BUCKET_ENTRY_COUNT    (TG_U8_MAX + 1)

#ifdef TG_DEBUG
#define TG_INIT_BUCKET(p_bucket)                       \
    for (u16 i = 0; i < TG_BUCKET_ENTRY_COUNT; i++)    \
        (p_bucket)->p_memory[i].occupied = -1;         \
    for (u8 i = 0; i < TG_BUCKET_ENTRY_COUNT - 1; i++) \
        (p_bucket)->p_next_free_indices[i] = i + 1
#else
#define TG_INIT_BUCKET(p_bucket)                       \
    for (u8 i = 0; i < TG_BUCKET_ENTRY_COUNT - 1; i++) \
        (p_bucket)->p_next_free_indices[i] = i + 1
#endif




typedef struct tgvk_memory_entry
{
    b32                          occupied;
    u32                          pool_index;
    u32                          offset_pages;
    u32                          page_count;
    struct tgvk_memory_entry*    p_prev;
    struct tgvk_memory_entry*    p_next;
    struct tgvk_memory_entry*    p_prev_free;
    struct tgvk_memory_entry*    p_next_free;
} tgvk_memory_entry;

typedef struct tg_memory_bucket_entry
{
    u8    idx;
    u8    next_idx;
} tg_memory_bucket_entry;

typedef struct tgvk_memory_pool
{
    VkMemoryPropertyFlags    memory_property_flags;
    VkDeviceMemory           device_memory;
    void*                    p_mapped_device_memory;
    u32                      memory_type_bit;
    u32                      page_count;
    tgvk_memory_entry*       p_first_free_entry;

#ifdef TG_DEBUG
    u32                      occupied_entry_count;
    u32                      occupied_page_count;
    u32                      free_entry_count;
    u32                      max_occupied_entry_count;
    u32                      max_occupied_page_count;
    u32                      max_free_entry_count;
#endif
} tgvk_memory_pool;

typedef struct tgvk_memory
{
    tg_read_write_lock    read_write_lock;
    VkDeviceSize          page_size;
    u32                   pool_count;
    tgvk_memory_pool      p_pools[VK_MAX_MEMORY_TYPES];
} tgvk_memory;



typedef struct tgvk_memory_entry_bucket tgvk_memory_entry_bucket;
typedef struct tgvk_memory_entry_bucket
{
    tgvk_memory_entry            p_memory[TG_BUCKET_ENTRY_COUNT];
    u8                           first_free_index;
    u8                           p_next_free_indices[TG_BUCKET_ENTRY_COUNT - 1];
    tgvk_memory_entry_bucket*    p_next;
} tgvk_memory_entry_bucket;



static tgvk_memory                 memory = { 0 };
static tgvk_memory_entry_bucket    entry_buffer = { 0 };

#ifdef TG_DEBUG
static b32            initialized = TG_FALSE;
#endif



tgvk_memory_entry* tg__alloc_entry(void)
{
    tgvk_memory_entry_bucket* p_bucket = &entry_buffer;

    while (p_bucket->first_free_index == 255)
    {
        if (p_bucket->p_next)
        {
            p_bucket = p_bucket->p_next;
        }
        else
        {
            p_bucket->p_next = TG_MALLOC(sizeof(tgvk_memory_entry_bucket));
            p_bucket = p_bucket->p_next;
            TG_INIT_BUCKET(p_bucket);
            break;
        }
    }

    const u8 i = p_bucket->first_free_index;
    p_bucket->first_free_index = p_bucket->p_next_free_indices[i];
    p_bucket->p_next_free_indices[i] = 0;

    tgvk_memory_entry* p_entry = &p_bucket->p_memory[i];
    TG_ASSERT(p_entry->occupied == -1);
    return p_entry;
}

void tg__free_entry(tgvk_memory_entry* p_entry)
{
    tgvk_memory_entry_bucket* p_bucket = &entry_buffer;

    while (p_entry < p_bucket->p_memory || p_entry >= p_bucket->p_memory + TG_BUCKET_ENTRY_COUNT)
    {
        p_bucket = p_bucket->p_next;
        TG_ASSERT(p_bucket);
    }

    TG_ASSERT(p_entry - p_bucket->p_memory < TG_BUCKET_ENTRY_COUNT);
    const u8 i = (u8)(p_entry - p_bucket->p_memory);
    TG_ASSERT(p_bucket->p_next_free_indices[i] == 0);
    p_bucket->p_next_free_indices[i] = p_bucket->first_free_index;
    p_bucket->first_free_index = i;
#ifdef TG_DEBUG
    p_entry->occupied = -1;
#endif
}



void tgvk_memory_allocator_init(VkDevice device, VkPhysicalDevice physical_device)
{
    TG_ASSERT(!initialized);

    TG_INIT_BUCKET(&entry_buffer);

    memory.read_write_lock = TG_RWL_CREATE();

    VkPhysicalDeviceProperties physical_device_properties = { 0 };
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
    
    memory.page_size = TG_MAX(1024, physical_device_properties.limits.bufferImageGranularity);

    VkPhysicalDeviceMemoryProperties physical_device_memory_properties = { 0 };
    vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_memory_properties);

    memory.pool_count = 0;
    for (u32 i = 0; i < physical_device_memory_properties.memoryTypeCount; i++)
    {
        const b32 device_local = physical_device_memory_properties.memoryTypes[i].propertyFlags == TGVK_MEMORY_DEVICE;
        const b32 host_visible_coherent = physical_device_memory_properties.memoryTypes[i].propertyFlags == TGVK_MEMORY_HOST;

        if (device_local || host_visible_coherent)
        {
            tgvk_memory_pool* p_pool = &memory.p_pools[memory.pool_count];

            VkDeviceSize allocation_size = 0;
            switch (physical_device_memory_properties.memoryTypes[i].propertyFlags)
            {
            case TGVK_MEMORY_DEVICE: allocation_size = TGVK_MAX_DEVICE_LOCAL_MEMORY_SIZE;   break;
            case TGVK_MEMORY_HOST:   allocation_size = TGVK_MAX_HOST_VISIBLE_COHERENT_SIZE; break;

            default: TG_INVALID_CODEPATH(); break;
            }
            allocation_size = TG_CEIL_TO_MULTIPLE(allocation_size, memory.page_size);

            p_pool->memory_property_flags = physical_device_memory_properties.memoryTypes[i].propertyFlags;

            TG_ASSERT(allocation_size / memory.page_size <= TG_U32_MAX);
            const u32 page_count = (u32)(allocation_size / memory.page_size);

            p_pool->memory_type_bit = i;
            p_pool->page_count = page_count;
            p_pool->p_first_free_entry = tg__alloc_entry();
            p_pool->p_first_free_entry->occupied = TG_FALSE;
            p_pool->p_first_free_entry->pool_index = memory.pool_count;
            p_pool->p_first_free_entry->offset_pages = 0;
            p_pool->p_first_free_entry->page_count = page_count;
            p_pool->p_first_free_entry->p_prev = TG_NULL;
            p_pool->p_first_free_entry->p_next = TG_NULL;
            p_pool->p_first_free_entry->p_prev_free = TG_NULL;
            p_pool->p_first_free_entry->p_next_free = TG_NULL;

#ifdef TG_DEBUG
            p_pool->occupied_entry_count = 0;
            p_pool->occupied_page_count = 0;
            p_pool->free_entry_count = 1;
            p_pool->max_occupied_entry_count = 0;
            p_pool->max_occupied_page_count = 0;
            p_pool->max_free_entry_count = 1;
#endif

            VkMemoryAllocateInfo memory_allocate_info = { 0 };
            memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memory_allocate_info.pNext = TG_NULL;
            memory_allocate_info.allocationSize = allocation_size;
            memory_allocate_info.memoryTypeIndex = i;

            TGVK_CALL(vkAllocateMemory(device, &memory_allocate_info, TG_NULL, &p_pool->device_memory));
            if (physical_device_memory_properties.memoryTypes[i].propertyFlags == TGVK_MEMORY_HOST)
            {
                TGVK_CALL(vkMapMemory(device, p_pool->device_memory, 0, VK_WHOLE_SIZE, 0, &p_pool->p_mapped_device_memory));
            }
            else
            {
                p_pool->p_mapped_device_memory = TG_NULL;
            }

            memory.pool_count++;
        }
    }

#ifdef TG_DEBUG
    initialized = TG_TRUE;
#endif
}

void tgvk_memory_allocator_shutdown(VkDevice device)
{
    TG_ASSERT(initialized);

    for (u32 i = 0; i < memory.pool_count; i++)
    {
#ifdef TG_DEBUG
        if (memory.p_pools[i].occupied_entry_count != 0)
        {
            TG_DEBUG_LOG("%u unfreed entries with a total number of %u pages in pool #%u in vulkan allocator!\n", memory.p_pools[i].occupied_entry_count, memory.p_pools[i].occupied_page_count, i);
        }
#endif
        vkFreeMemory(device, memory.p_pools[i].device_memory, TG_NULL);
        // TODO: free
        //TG_FREE(memory.p_pools[i].p_entries);
    }
    TG_RWL_DESTROY(memory.read_write_lock);
}

VkDeviceSize tgvk_memory_aligned_size(VkDeviceSize size)
{
    const VkDeviceSize result = TG_CEIL_TO_MULTIPLE(size, memory.page_size);
    return result;
}

tgvk_memory_block tgvk_memory_allocator_alloc(VkDeviceSize alignment, VkDeviceSize size, u32 memory_type_bits, tgvk_memory_type type)
{
    TG_ASSERT(memory_type_bits);
    TG_ASSERT(alignment <= memory.page_size); // TODO: consider alignment!

    tgvk_memory_block memory_block = { 0 };

    const VkDeviceSize aligned_size = TG_CEIL_TO_MULTIPLE(size, memory.page_size);
    const u32 required_page_count = (u32)(aligned_size / memory.page_size);

    for (u32 i = 0; i < memory.pool_count; i++)
    {
        tgvk_memory_pool* p_pool = &memory.p_pools[i];

        const b32 is_required_memory_type = (b32)(memory_type_bits & (1 << p_pool->memory_type_bit)) == (1 << p_pool->memory_type_bit);
        const b32 has_required_memory_properties = (p_pool->memory_property_flags & type) == (VkMemoryPropertyFlags)type;

        if (is_required_memory_type && has_required_memory_properties)
        {
            TG_RWL_LOCK_FOR_WRITE(memory.read_write_lock);

            tgvk_memory_entry* p_entry = p_pool->p_first_free_entry;
            while (p_entry != TG_NULL)
            {
                TG_ASSERT(!p_entry->occupied);
                if (p_entry->page_count >= required_page_count)
                {
                    if (p_entry->page_count > required_page_count)
                    {
                        tgvk_memory_entry* p_new_entry = tg__alloc_entry();

                        p_new_entry->occupied = TG_TRUE;
                        p_new_entry->pool_index = i;
                        p_new_entry->offset_pages = p_entry->offset_pages;
                        p_new_entry->page_count = required_page_count;
                        p_new_entry->p_prev_free = TG_NULL;
                        p_new_entry->p_next_free = TG_NULL;

                        p_entry->offset_pages += required_page_count;
                        p_entry->page_count -= required_page_count;

                        TG_LINK(p_entry->p_prev, p_new_entry);
                        TG_LINK(p_new_entry, p_entry);

                        TG_CUT_FREE(p_pool, p_entry);
                        TG_INSERT_FREE(p_pool, p_entry);

                        p_entry = p_new_entry;
                    }
                    else
                    {
                        TG_ASSERT(p_entry->page_count == required_page_count);

                        p_entry->occupied = TG_TRUE;

                        TG_CUT_FREE(p_pool, p_entry);

#ifdef TG_DEBUG
                        p_pool->free_entry_count--;
#endif
                    }
#ifdef TG_DEBUG
                    p_pool->occupied_entry_count++;
                    p_pool->occupied_page_count += required_page_count;
                    p_pool->max_occupied_entry_count = TG_MAX(p_pool->max_occupied_entry_count, p_pool->occupied_entry_count);
                    p_pool->max_occupied_page_count = TG_MAX(p_pool->max_occupied_page_count, p_pool->occupied_page_count);
#endif

                    memory_block.p_entry = p_entry;
                    memory_block.device_memory = memory.p_pools[i].device_memory;
                    memory_block.offset = (tg_size)p_entry->offset_pages * memory.page_size;
                    memory_block.size = aligned_size;
                    memory_block.p_mapped_device_memory = p_pool->p_mapped_device_memory ? (u8*)p_pool->p_mapped_device_memory + memory_block.offset : TG_NULL;

                    TG_ASSERT(memory_block.offset + memory_block.size < (tg_size)p_pool->page_count * memory.page_size);

                    TG_RWL_UNLOCK_FOR_WRITE(memory.read_write_lock);

                    break;
                }

                TG_ASSERT(p_entry != p_entry->p_next_free);
                p_entry = p_entry->p_next_free;
            }
        }
    }

    TG_ASSERT(memory_block.device_memory);

    return memory_block;
}

void tgvk_memory_allocator_free(tgvk_memory_block* p_memory_block)
{
    TG_ASSERT(p_memory_block);

    tgvk_memory_entry* p_entry = p_memory_block->p_entry;
    TG_ASSERT(p_entry->occupied);
    TG_ASSERT(p_entry->p_prev_free == TG_NULL);
    TG_ASSERT(p_entry->p_next_free == TG_NULL);
    p_entry->occupied = TG_FALSE;

    tgvk_memory_pool* p_pool = &memory.p_pools[p_entry->pool_index];

    TG_RWL_LOCK_FOR_WRITE(memory.read_write_lock);

#ifdef TG_DEBUG
    p_pool->occupied_entry_count--;
    p_pool->occupied_page_count -= p_entry->page_count;
#endif

    if (p_entry->p_prev != TG_NULL && !p_entry->p_prev->occupied)
    {
        TG_ASSERT(p_pool->p_first_free_entry != p_entry);
        tgvk_memory_entry* p_prev = p_entry->p_prev;
        
        p_prev->page_count += p_entry->page_count;
        
        TG_CUT(p_entry);
        
        tg__free_entry(p_entry);
        p_entry = p_prev;

#ifdef TG_DEBUG
        p_pool->free_entry_count--;
#endif
    }

    if (p_entry->p_next != TG_NULL && !p_entry->p_next->occupied)
    {
        tgvk_memory_entry* p_next = p_entry->p_next;
        
        p_next->offset_pages -= p_entry->page_count;
        TG_ASSERT(p_next->offset_pages == p_entry->offset_pages);
        p_next->page_count += p_entry->page_count;

        TG_CUT(p_entry);
        TG_CUT_FREE(p_pool, p_entry);

        tg__free_entry(p_entry);
        p_entry = p_next;

#ifdef TG_DEBUG
        p_pool->free_entry_count--;
#endif
    }

    TG_CUT_FREE(p_pool, p_entry);
    TG_INSERT_FREE(p_pool, p_entry);

#ifdef TG_DEBUG
    p_pool->free_entry_count++;
    p_pool->max_free_entry_count = TG_MAX(p_pool->max_free_entry_count, p_pool->free_entry_count);
#endif

    TG_RWL_UNLOCK_FOR_WRITE(memory.read_write_lock);
}

#endif
