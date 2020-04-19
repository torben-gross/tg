#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "graphics/tg_graphics_image_loader.h"
#include "memory/tg_memory_allocator.h"



typedef struct tg_image
{
    u32                width;
    u32                height;
    tg_image_format    format;
    u32*               data;
    VkImage            image;
    VkDeviceMemory     device_memory;
    VkImageView        image_view;
    VkSampler          sampler;
} tg_image;



tg_image_h tg_graphics_image_create(const char* p_filename)
{
    TG_ASSERT(p_filename);

    tg_image_h image_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(*image_h));
    tg_image_load(p_filename, &image_h->width, &image_h->height, &image_h->format, &image_h->data);
    tg_image_convert_format(image_h->data, image_h->width, image_h->height, image_h->format, TG_IMAGE_FORMAT_R8G8B8A8);
    const u32 mip_levels = TG_IMAGE_MAX_MIP_LEVELS(image_h->width, image_h->height);
    const VkDeviceSize size = (u64)image_h->width * (u64)image_h->height * sizeof(*image_h->data);

    tg_vulkan_buffer staging_buffer = { 0 };
    staging_buffer = tg_graphics_vulkan_buffer_create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    memcpy(staging_buffer.p_mapped_device_memory, image_h->data, (size_t)size);

    tg_graphics_vulkan_image_create(image_h->width, image_h->height, mip_levels, VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &image_h->image, &image_h->device_memory);
    tg_graphics_vulkan_image_transition_layout(image_h->image, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, mip_levels, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    tg_graphics_vulkan_buffer_copy_to_image(image_h->width, image_h->height, staging_buffer.buffer, image_h->image);
    tg_graphics_vulkan_buffer_destroy(&staging_buffer);
    tg_graphics_vulkan_image_mipmaps_generate(image_h->image, image_h->width, image_h->height, VK_FORMAT_R8G8B8A8_SRGB, mip_levels);
    tg_graphics_vulkan_image_view_create(image_h->image, VK_FORMAT_R8G8B8A8_SRGB, mip_levels, VK_IMAGE_ASPECT_COLOR_BIT, &image_h->image_view);
    image_h->sampler = tg_graphics_vulkan_sampler_create(mip_levels, VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);

    return image_h;
}

void tg_graphics_image_destroy(tg_image_h image_h)
{
    vkDeviceWaitIdle(device); // TODO: for all destroys

    tg_graphics_vulkan_sampler_destroy(image_h->sampler);
    tg_graphics_vulkan_image_view_destroy(image_h->image_view);
    tg_graphics_vulkan_image_destroy(image_h->image, image_h->device_memory);
    tg_image_free(image_h->data);
    TG_MEMORY_ALLOCATOR_FREE(image_h);
}

#endif
