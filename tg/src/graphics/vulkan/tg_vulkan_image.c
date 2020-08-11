#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "graphics/tg_image_loader.h"
#include "memory/tg_memory.h"

tg_color_image_h tg_color_image_create(const char* p_filename)
{
    TG_ASSERT(p_filename);

    tg_color_image_h h_color_image = TG_MEMORY_ALLOC(sizeof(*h_color_image));

    u32* p_data = TG_NULL;
    tg_image_load(p_filename, &h_color_image->width, &h_color_image->height, &h_color_image->format, &p_data);
    tg_image_convert_format(p_data, h_color_image->width, h_color_image->height, h_color_image->format, TG_COLOR_IMAGE_FORMAT_R8G8B8A8);
    h_color_image->mip_levels = TG_IMAGE_MAX_MIP_LEVELS(h_color_image->width, h_color_image->height);
    const u64 size = (u64)h_color_image->width * (u64)h_color_image->height * sizeof(*p_data);

    tg_vulkan_buffer staging_buffer = { 0 };
    staging_buffer = tg_vulkan_buffer_create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    tg_memory_copy(size, p_data, staging_buffer.memory.p_mapped_device_memory);

    tg_vulkan_color_image_create_info vulkan_color_image_create_info = { 0 };
    vulkan_color_image_create_info.width = h_color_image->width;
    vulkan_color_image_create_info.height = h_color_image->height;
    vulkan_color_image_create_info.mip_levels = h_color_image->mip_levels;
    vulkan_color_image_create_info.format = TG_VULKAN_COLOR_IMAGE_FORMAT;
    vulkan_color_image_create_info.p_vulkan_sampler_create_info = TG_NULL;

    *h_color_image = tg_vulkan_color_image_create(&vulkan_color_image_create_info);

    VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY); // TODO: create once and reuse for every image
    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, h_color_image, 0, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    tg_vulkan_command_buffer_cmd_copy_buffer_to_color_image(command_buffer, staging_buffer.buffer, h_color_image);
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, h_color_image, 0, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, TG_VULKAN_COLOR_IMAGE_LAYOUT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    tg_vulkan_command_buffer_end_and_submit(command_buffer, TG_VULKAN_QUEUE_TYPE_GRAPHICS);
    tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, command_buffer);
    tg_vulkan_buffer_destroy(&staging_buffer);

    tg_image_free(p_data);

    return h_color_image;
}

tg_color_image_h tg_color_image_create_empty(const tg_color_image_create_info* p_color_image_create_info)
{
    TG_ASSERT(p_color_image_create_info);

    tg_color_image_h h_color_image = TG_MEMORY_ALLOC(sizeof(*h_color_image));
    h_color_image->type = TG_HANDLE_TYPE_COLOR_IMAGE;

    tg_vulkan_sampler_create_info vulkan_sampler_create_info = { 0 };
    vulkan_sampler_create_info.min_filter = tg_vulkan_image_convert_filter(p_color_image_create_info->min_filter);
    vulkan_sampler_create_info.mag_filter = tg_vulkan_image_convert_filter(p_color_image_create_info->mag_filter);
    vulkan_sampler_create_info.address_mode_u = tg_vulkan_image_convert_address_mode(p_color_image_create_info->address_mode_u);
    vulkan_sampler_create_info.address_mode_v = tg_vulkan_image_convert_address_mode(p_color_image_create_info->address_mode_v);
    vulkan_sampler_create_info.address_mode_w = tg_vulkan_image_convert_address_mode(p_color_image_create_info->address_mode_w);

    tg_vulkan_color_image_create_info vulkan_color_image_create_info = { 0 };
    vulkan_color_image_create_info.width = p_color_image_create_info->width;
    vulkan_color_image_create_info.height = p_color_image_create_info->height;
    vulkan_color_image_create_info.mip_levels = p_color_image_create_info->mip_levels;
    vulkan_color_image_create_info.format = tg_vulkan_color_image_convert_format(p_color_image_create_info->format);
    vulkan_color_image_create_info.p_vulkan_sampler_create_info = &vulkan_sampler_create_info;

    *h_color_image = tg_vulkan_color_image_create(&vulkan_color_image_create_info);

    VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_transition_color_image_layout(command_buffer, h_color_image, 0, 0, VK_IMAGE_LAYOUT_UNDEFINED, TG_VULKAN_COLOR_IMAGE_LAYOUT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    tg_vulkan_command_buffer_end_and_submit(command_buffer, TG_VULKAN_QUEUE_TYPE_GRAPHICS);
    tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, command_buffer);

    return h_color_image;
}

void tg_color_image_destroy(tg_color_image_h h_color_image)
{
    TG_ASSERT(h_color_image);

    tg_vulkan_color_image_destroy(h_color_image);
    TG_MEMORY_FREE(h_color_image);
}



tg_cube_map_h tg_cube_map_create(u32 width, u32 height, u32 depth, tg_color_image_format format)
{
    tg_cube_map_h h_cube_map = TG_NULL;
    TG_VULKAN_TAKE_HANDLE(p_cube_maps, h_cube_map);
    h_cube_map->type = TG_HANDLE_TYPE_CUBE_MAP;

    h_cube_map->cube_map = tg_vulkan_cube_map_create(width, height, depth, tg_vulkan_color_image_convert_format(format), TG_NULL);

    return h_cube_map;
}



tg_depth_image_h tg_depth_image_create(const tg_depth_image_create_info* p_depth_image_create_info)
{
    TG_ASSERT(p_depth_image_create_info);

    tg_depth_image_h h_depth_image = TG_MEMORY_ALLOC(sizeof(*h_depth_image));
    h_depth_image->type = TG_HANDLE_TYPE_DEPTH_IMAGE;

    tg_vulkan_sampler_create_info vulkan_sampler_create_info = { 0 };
    vulkan_sampler_create_info.min_filter = tg_vulkan_image_convert_filter(p_depth_image_create_info->min_filter);
    vulkan_sampler_create_info.mag_filter = tg_vulkan_image_convert_filter(p_depth_image_create_info->mag_filter);
    vulkan_sampler_create_info.address_mode_u = tg_vulkan_image_convert_address_mode(p_depth_image_create_info->address_mode_u);
    vulkan_sampler_create_info.address_mode_v = tg_vulkan_image_convert_address_mode(p_depth_image_create_info->address_mode_v);
    vulkan_sampler_create_info.address_mode_w = tg_vulkan_image_convert_address_mode(p_depth_image_create_info->address_mode_w);

    tg_vulkan_depth_image_create_info vulkan_depth_image_create_info = { 0 };
    vulkan_depth_image_create_info.width = p_depth_image_create_info->width;
    vulkan_depth_image_create_info.height = p_depth_image_create_info->height;
    vulkan_depth_image_create_info.format = tg_vulkan_depth_image_convert_format(p_depth_image_create_info->format);
    vulkan_depth_image_create_info.p_vulkan_sampler_create_info = &vulkan_sampler_create_info;

    *h_depth_image = tg_vulkan_depth_image_create(&vulkan_depth_image_create_info);

    VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_transition_depth_image_layout(command_buffer, h_depth_image, 0, 0, VK_IMAGE_LAYOUT_UNDEFINED, TG_VULKAN_DEPTH_IMAGE_LAYOUT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    tg_vulkan_command_buffer_end_and_submit(command_buffer, TG_VULKAN_QUEUE_TYPE_GRAPHICS);
    tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, command_buffer);

    return h_depth_image;
}

void tg_depth_image_destroy(tg_depth_image_h h_depth_image)
{
    TG_ASSERT(h_depth_image);

    tg_vulkan_depth_image_destroy(h_depth_image);
    TG_MEMORY_FREE(h_depth_image);
}



void tg_storage_image_3d_copy_to_storage_buffer(tg_storage_image_3d_h h_storage_image_3d, tg_storage_buffer_h h_storage_buffer)
{
    TG_ASSERT(h_storage_image_3d && h_storage_buffer);

    VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY); // TODO: global
    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_transition_storage_image_3d_layout(command_buffer, h_storage_image_3d, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    tg_vulkan_command_buffer_cmd_copy_storage_image_3d_to_buffer(command_buffer, h_storage_image_3d, h_storage_buffer->buffer.buffer);
    tg_vulkan_command_buffer_cmd_transition_storage_image_3d_layout(command_buffer, h_storage_image_3d, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    tg_vulkan_command_buffer_end_and_submit(command_buffer, TG_VULKAN_QUEUE_TYPE_GRAPHICS);
    tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, command_buffer);
}

void tg_storage_image_3d_clear(tg_storage_image_3d_h h_storage_image_3d)
{
    TG_ASSERT(h_storage_image_3d);

    VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, VK_COMMAND_BUFFER_LEVEL_PRIMARY); // TODO: global
    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    tg_vulkan_command_buffer_cmd_transition_storage_image_3d_layout(command_buffer, h_storage_image_3d, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    tg_vulkan_command_buffer_cmd_clear_storage_image_3d(command_buffer, h_storage_image_3d);
    tg_vulkan_command_buffer_cmd_transition_storage_image_3d_layout(command_buffer, h_storage_image_3d, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    tg_vulkan_command_buffer_end_and_submit(command_buffer, TG_VULKAN_QUEUE_TYPE_GRAPHICS);
    tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_GRAPHICS, command_buffer);
}

tg_storage_image_3d_h tg_storage_image_3d_create(u32 width, u32 height, u32 depth, tg_storage_image_format format)
{
    TG_ASSERT(width && height && depth);

    tg_storage_image_3d_h h_storage_image_3d = TG_MEMORY_ALLOC(sizeof(*h_storage_image_3d));
    *h_storage_image_3d = tg_vulkan_storage_image_3d_create(width, height, depth, tg_vulkan_storage_image_convert_format(format));
    return h_storage_image_3d;
}

void tg_storage_image_3d_destroy(tg_storage_image_3d_h h_storage_image_3d)
{
    TG_ASSERT(h_storage_image_3d);

    tg_vulkan_storage_image_3d_destroy(h_storage_image_3d);
    TG_MEMORY_FREE(h_storage_image_3d);
}

#endif
