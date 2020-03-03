#include "tg_graphics_vulkan.h"

#include "tg_image.h"
#include "tg/platform/tg_allocator.h"

void tg_graphics_image_create(const char* filename, tg_image_h* p_image_h)
{
    tg_image image = { 0 };
    tg_image_load(filename, &image.width, &image.height, &image.format, &image.data);
    tg_image_convert_to_format(image.data, image.width, image.height, image.format, TG_IMAGE_FORMAT_R8G8B8A8);
    const ui32 mip_levels = TG_IMAGE_MAX_MIP_LEVELS(16, 16);
    const VkDeviceSize size = (ui64)image.width * (ui64)image.height * sizeof(*image.data);

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
    memcpy(data, image.data, (size_t)size);
    vkUnmapMemory(device, staging_buffer_memory);

    tg_graphics_vulkan_image_create(
        image.width,
        image.height,
        mip_levels,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &image.image,
        &image.device_memory
    );

    tg_graphics_vulkan_image_transition_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_levels, &image.image);
    tg_graphics_vulkan_buffer_copy_to_image(image.width, image.height, &staging_buffer, &image.image);

    vkDestroyBuffer(device, staging_buffer, NULL);
    vkFreeMemory(device, staging_buffer_memory, NULL);

    tg_graphics_vulkan_image_mipmaps_generate(image.image, image.width, image.height, VK_FORMAT_R8G8B8A8_SRGB, mip_levels);
    tg_graphics_vulkan_image_view_create(image.image, VK_FORMAT_R8G8B8A8_SRGB, mip_levels, VK_IMAGE_ASPECT_COLOR_BIT, &image.image_view);
    tg_graphics_vulkan_sampler_create(image.image, mip_levels, VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, &image.sampler);

    *p_image_h = tg_malloc(sizeof(**p_image_h));
    **p_image_h = image;
}
void tg_graphics_image_destroy(tg_image_h image_h)
{
    vkDestroySampler(device, image_h->sampler, NULL);
    vkDestroyImageView(device, image_h->image_view, NULL);
    vkFreeMemory(device, image_h->device_memory, NULL);
    vkDestroyImage(device, image_h->image, NULL);
    tg_image_free(image_h->data);
    tg_free(image_h);
}
