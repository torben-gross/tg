#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "graphics/tg_graphics_image_loader.h"
#include "memory/tg_memory.h"

tg_color_image_h tg_color_image_load(const char* p_filename)
{
    TG_ASSERT(p_filename);

    tg_color_image_h color_image_h = TG_MEMORY_ALLOC(sizeof(*color_image_h));

    u32* p_data = TG_NULL;
    tg_image_load(p_filename, &color_image_h->width, &color_image_h->height, &color_image_h->format, &p_data);
    tg_image_convert_format(p_data, color_image_h->width, color_image_h->height, color_image_h->format, TG_COLOR_IMAGE_FORMAT_R8G8B8A8);
    color_image_h->mip_levels = TG_IMAGE_MAX_MIP_LEVELS(color_image_h->width, color_image_h->height);
    const u64 size = (u64)color_image_h->width * (u64)color_image_h->height * sizeof(*p_data);

    tg_vulkan_buffer staging_buffer = { 0 };
    staging_buffer = tg_vulkan_buffer_create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    memcpy(staging_buffer.p_mapped_device_memory, p_data, (size_t)size);

    tg_vulkan_color_image_create_info vulkan_color_image_create_info = { 0 };
    {
        vulkan_color_image_create_info.width = color_image_h->width;
        vulkan_color_image_create_info.height = color_image_h->height;
        vulkan_color_image_create_info.mip_levels = color_image_h->mip_levels;
        vulkan_color_image_create_info.format = TG_VULKAN_COLOR_IMAGE_FORMAT;
        vulkan_color_image_create_info.min_filter = VK_FILTER_NEAREST; // TODO: optional struct for sampler stuff, otherwise default
        vulkan_color_image_create_info.mag_filter = VK_FILTER_NEAREST;
        vulkan_color_image_create_info.address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vulkan_color_image_create_info.address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vulkan_color_image_create_info.address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
    *color_image_h = tg_vulkan_color_image_create(&vulkan_color_image_create_info);

    VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY); // TODO: create once and reuse for every image
    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, color_image_h, 0, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    tg_vulkan_command_buffer_cmd_copy_buffer_to_color_image(command_buffer, staging_buffer.buffer, color_image_h);
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, color_image_h, 0, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, TG_VULKAN_COLOR_IMAGE_LAYOUT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    tg_vulkan_command_buffer_end_and_submit(command_buffer, &graphics_queue);
    tg_vulkan_command_buffer_free(graphics_command_pool, command_buffer);
    tg_vulkan_buffer_destroy(&staging_buffer);

    tg_image_free(p_data);

    return color_image_h;
}

tg_color_image_h tg_color_image_create(const tg_color_image_create_info* p_color_image_create_info)
{
    TG_ASSERT(p_color_image_create_info);

    tg_color_image_h color_image_h = TG_MEMORY_ALLOC(sizeof(*color_image_h));
    color_image_h->type = TG_HANDLE_TYPE_COLOR_IMAGE;

    tg_vulkan_color_image_create_info vulkan_color_image_create_info = { 0 };
    {
        vulkan_color_image_create_info.width = p_color_image_create_info->width;
        vulkan_color_image_create_info.height = p_color_image_create_info->height;
        vulkan_color_image_create_info.mip_levels = p_color_image_create_info->mip_levels;
        vulkan_color_image_create_info.format = tg_vulkan_color_image_convert_format(p_color_image_create_info->format);
        vulkan_color_image_create_info.min_filter = tg_vulkan_image_convert_filter(p_color_image_create_info->min_filter);
        vulkan_color_image_create_info.mag_filter = tg_vulkan_image_convert_filter(p_color_image_create_info->mag_filter);
        vulkan_color_image_create_info.address_mode_u = tg_vulkan_image_convert_address_mode(p_color_image_create_info->address_mode_u);
        vulkan_color_image_create_info.address_mode_v = tg_vulkan_image_convert_address_mode(p_color_image_create_info->address_mode_v);
        vulkan_color_image_create_info.address_mode_w = tg_vulkan_image_convert_address_mode(p_color_image_create_info->address_mode_w);
    }
    *color_image_h = tg_vulkan_color_image_create(&vulkan_color_image_create_info);

    VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, color_image_h, 0, 0, VK_IMAGE_LAYOUT_UNDEFINED, TG_VULKAN_COLOR_IMAGE_LAYOUT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    tg_vulkan_command_buffer_end_and_submit(command_buffer, &graphics_queue);
    tg_vulkan_command_buffer_free(graphics_command_pool, command_buffer);

    return color_image_h;
}

void tg_color_image_destroy(tg_color_image_h color_image_h)
{
    TG_ASSERT(color_image_h);

    tg_vulkan_color_image_destroy(color_image_h);
    TG_MEMORY_FREE(color_image_h);
}



tg_depth_image_h tg_depth_image_create(const tg_depth_image_create_info* p_depth_image_create_info)
{
    TG_ASSERT(p_depth_image_create_info);

    tg_depth_image_h depth_image_h = TG_MEMORY_ALLOC(sizeof(*depth_image_h));
    depth_image_h->type = TG_HANDLE_TYPE_DEPTH_IMAGE;

    tg_vulkan_depth_image_create_info vulkan_depth_image_create_info = { 0 };
    {
        vulkan_depth_image_create_info.width = p_depth_image_create_info->width;
        vulkan_depth_image_create_info.height = p_depth_image_create_info->height;
        vulkan_depth_image_create_info.format = tg_vulkan_depth_image_convert_format(p_depth_image_create_info->format);
        vulkan_depth_image_create_info.min_filter = tg_vulkan_image_convert_filter(p_depth_image_create_info->min_filter);
        vulkan_depth_image_create_info.mag_filter = tg_vulkan_image_convert_filter(p_depth_image_create_info->mag_filter);
        vulkan_depth_image_create_info.address_mode_u = tg_vulkan_image_convert_address_mode(p_depth_image_create_info->address_mode_u);
        vulkan_depth_image_create_info.address_mode_v = tg_vulkan_image_convert_address_mode(p_depth_image_create_info->address_mode_v);
        vulkan_depth_image_create_info.address_mode_w = tg_vulkan_image_convert_address_mode(p_depth_image_create_info->address_mode_w);
    }
    *depth_image_h = tg_vulkan_depth_image_create(&vulkan_depth_image_create_info);

    VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_transition_depth_image_layout(command_buffer, depth_image_h, 0, 0, VK_IMAGE_LAYOUT_UNDEFINED, TG_VULKAN_COLOR_IMAGE_LAYOUT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    tg_vulkan_command_buffer_end_and_submit(command_buffer, &graphics_queue);
    tg_vulkan_command_buffer_free(graphics_command_pool, command_buffer);

    return depth_image_h;
}

void tg_depth_image_destroy(tg_depth_image_h depth_image_h)
{
    TG_ASSERT(depth_image_h);

    tg_vulkan_depth_image_destroy(depth_image_h);
    TG_MEMORY_FREE(depth_image_h);
}

#endif
