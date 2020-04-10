#ifndef TG_GRAPHICS_VULKAN_SHADER_H
#define TG_GRAPHICS_VULKAN_SHADER_H

#include "graphics/vulkan/tg_graphics_vulkan.h"

VkShaderModule    tg_graphics_vulkan_fragment_shader_get_shader_module(tg_fragment_shader_h fragment_shader_h);
VkShaderModule    tg_graphics_vulkan_vertex_shader_get_shader_module(tg_vertex_shader_h vertex_shader_h);

#endif
