#include "tg/graphics/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "tg/graphics/tg_graphics_image_loader.h"
#include "tg/platform/tg_allocator.h"

void tg_graphics_image_create(const char* filename, tg_image_h* p_image_h)
{
    TG_ASSERT(filename && p_image_h);

    *p_image_h = tg_allocator_allocate(sizeof(**p_image_h));
    tg_image_load(filename, &(**p_image_h).width, &(**p_image_h).height, &(**p_image_h).format, &(**p_image_h).data);
    tg_image_convert_to_format((**p_image_h).data, (**p_image_h).width, (**p_image_h).height, (**p_image_h).format, TG_IMAGE_FORMAT_R8G8B8A8);
    const ui32 mip_levels = TG_IMAGE_MAX_MIP_LEVELS(16, 16);
    const VkDeviceSize size = (ui64)(**p_image_h).width * (ui64)(**p_image_h).height * sizeof(*(**p_image_h).data);

    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;
    tg_graphics_vulkan_buffer_create(
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &staging_buffer,
        &staging_buffer_memory
    );

    void* data = NULL;
    VK_CALL(vkMapMemory(device, staging_buffer_memory, 0, size, 0, &data));
    memcpy(data, (**p_image_h).data, (size_t)size);
    vkUnmapMemory(device, staging_buffer_memory);

    tg_graphics_vulkan_image_create(
        (**p_image_h).width,
        (**p_image_h).height,
        mip_levels,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &(**p_image_h).image,
        &(**p_image_h).device_memory
    );

    tg_graphics_vulkan_image_transition_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_levels, &(**p_image_h).image);
    tg_graphics_vulkan_buffer_copy_to_image((**p_image_h).width, (**p_image_h).height, &staging_buffer, &(**p_image_h).image);

    vkDestroyBuffer(device, staging_buffer, NULL);
    vkFreeMemory(device, staging_buffer_memory, NULL);

    tg_graphics_vulkan_image_mipmaps_generate((**p_image_h).image, (**p_image_h).width, (**p_image_h).height, VK_FORMAT_R8G8B8A8_SRGB, mip_levels);
    tg_graphics_vulkan_image_view_create((**p_image_h).image, VK_FORMAT_R8G8B8A8_SRGB, mip_levels, VK_IMAGE_ASPECT_COLOR_BIT, &(**p_image_h).image_view);
    tg_graphics_vulkan_sampler_create((**p_image_h).image, mip_levels, VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, &(**p_image_h).sampler);
}

void tg_graphics_image_destroy(tg_image_h image_h)
{
    vkDestroySampler(device, image_h->sampler, NULL);
    vkDestroyImageView(device, image_h->image_view, NULL);
    vkFreeMemory(device, image_h->device_memory, NULL);
    vkDestroyImage(device, image_h->image, NULL);
    tg_image_free(image_h->data);
    tg_allocator_free(image_h);
}

#endif
