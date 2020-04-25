#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "graphics/tg_graphics_image_loader.h"
#include "memory/tg_memory.h"



tg_image_h tgg_image_create(const char* p_filename)
{
    TG_ASSERT(p_filename);

    tg_image_h image_h = TG_MEMORY_ALLOC(sizeof(*image_h));

    u32* p_data = TG_NULL;
    tg_image_load(p_filename, &image_h->width, &image_h->height, &image_h->format, &p_data);
    tg_image_convert_format(p_data, image_h->width, image_h->height, image_h->format, TG_IMAGE_FORMAT_R8G8B8A8);
    image_h->mip_levels = TG_IMAGE_MAX_MIP_LEVELS(image_h->width, image_h->height);
    const u64 size = (u64)image_h->width * (u64)image_h->height * sizeof(*p_data);

    tg_vulkan_buffer staging_buffer = { 0 };
    staging_buffer = tgg_vulkan_buffer_create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    memcpy(staging_buffer.p_mapped_device_memory, p_data, (size_t)size);

    tg_vulkan_image_create_info vulkan_image_create_info = { 0 };
    {
        vulkan_image_create_info.width = image_h->width;
        vulkan_image_create_info.height = image_h->height;
        vulkan_image_create_info.mip_levels = image_h->mip_levels;
        vulkan_image_create_info.format = TG_VULKAN_IMAGE_FORMAT;
        vulkan_image_create_info.aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
        vulkan_image_create_info.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        vulkan_image_create_info.sample_count = VK_SAMPLE_COUNT_1_BIT;
        vulkan_image_create_info.image_usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        vulkan_image_create_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        vulkan_image_create_info.min_filter = VK_FILTER_NEAREST;
        vulkan_image_create_info.mag_filter = VK_FILTER_NEAREST;
        vulkan_image_create_info.address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vulkan_image_create_info.address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vulkan_image_create_info.address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
    *image_h = tgg_vulkan_image_create(&vulkan_image_create_info);

    VkCommandBuffer command_buffer = tgg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tgg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tgg_vulkan_command_buffer_cmd_copy_buffer_to_image(command_buffer, staging_buffer.buffer, image_h);
    tgg_vulkan_command_buffer_cmd_transition_image_layout(command_buffer, image_h, 0, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    tgg_vulkan_command_buffer_end_and_submit(command_buffer, &graphics_queue);
    tgg_vulkan_command_buffer_free(graphics_command_pool, command_buffer);

    tg_image_free(p_data);

    return image_h;
}

void tgg_image_destroy(tg_image_h image_h)
{
    TG_ASSERT(image_h);

    tgg_vulkan_image_destroy(image_h);
    TG_MEMORY_FREE(image_h);
}

#endif
