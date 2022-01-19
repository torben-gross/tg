#ifndef TGVK_RENDER_TARGET_H
#define TGVK_RENDER_TARGET_H

#include "graphics/vulkan/tgvk_core.h"

#ifdef TG_VULKAN

#ifdef TG_DEBUG
#define TGVK_RENDER_TARGET_CREATE(color_width, color_height, color_format, p_color_sampler_create_info, depth_width, depth_height, depth_format, p_depth_sampler_create_info, fence_create_flags) \
    tgvk_render_target_create(color_width, color_height, color_format, p_color_sampler_create_info, depth_width, depth_height, depth_format, p_depth_sampler_create_info, fence_create_flags, __LINE__, __FILE__)
#else
#define TGVK_RENDER_TARGET_CREATE(color_width, color_height, color_format, p_color_sampler_create_info, depth_width, depth_height, depth_format, p_depth_sampler_create_info, fence_create_flags) \
    tgvk_render_target_create(color_width, color_height, color_format, p_color_sampler_create_info, depth_width, depth_height, depth_format, p_depth_sampler_create_info, fence_create_flags)
#endif

typedef struct tg_render_target
{
    tgvk_image    color_attachment;
    tgvk_image    depth_attachment;
    tgvk_image    color_attachment_copy;
    tgvk_image    depth_attachment_copy;
    VkFence       fence; // TODO: use 'timeline' semaphore?
} tg_render_target;

u32                      tg_render_target_get_width(tg_render_target* p_render_target);
u32                      tg_render_target_get_height(tg_render_target* p_render_target);
tg_color_image_format    tg_render_target_get_color_format(tg_render_target* p_render_target);
void                     tg_render_target_get_color_data_copy(tg_render_target* p_render_target, TG_INOUT u32* p_buffer_size, TG_OUT void* p_buffer);

tg_render_target         tgvk_render_target_create(u32 color_width, u32 color_height, VkFormat color_format, const tgvk_sampler_create_info* p_color_sampler_create_info, u32 depth_width, u32 depth_height, VkFormat depth_format, const tgvk_sampler_create_info* p_depth_sampler_create_info, VkFenceCreateFlags fence_create_flags TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename));
void                     tgvk_render_target_destroy(tg_render_target* p_render_target);

void                     tgvk_descriptor_set_update_render_target(VkDescriptorSet descriptor_set, tg_render_target* p_render_target, u32 dst_binding);

#endif

#endif
