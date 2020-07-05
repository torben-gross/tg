#ifndef TG_VULKAN_SPIRV_H
#define TG_VULKAN_SPIRV_H

#include "tg_common.h"

// TODO: these probably correspond to the ones in graphics.h
#define TG_SPIRV_MAX_GLOBALS    16
#define TG_SPIRV_MAX_INPUTS     8
#define TG_SPIRV_MAX_OUTPUTS    TG_SPIRV_MAX_INPUTS
#define TG_SPIRV_MAX_NAME       256



typedef enum tg_spirv_global_resource_type
{
    TG_SPIRV_GLOBAL_RESOURCE_TYPE_INVALID                   = 0,
    TG_SPIRV_GLOBAL_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER    = 1,
    TG_SPIRV_GLOBAL_RESOURCE_TYPE_UNIFORM_BUFFER            = 6,
    TG_SPIRV_GLOBAL_RESOURCE_TYPE_STORAGE_BUFFER            = 7,
    TG_SPIRV_GLOBAL_RESOURCE_TYPE_STORAGE_IMAGE             = 3
} tg_spirv_global_resource_type;

typedef enum tg_spirv_inout_format
{
    TG_SPIRV_INOUT_FORMAT_INVALID                = 0,
    TG_SPIRV_INOUT_FORMAT_R32_SFLOAT             = 100,
    TG_SPIRV_INOUT_FORMAT_R32_SINT               = 99,
    TG_SPIRV_INOUT_FORMAT_R32_UINT               = 98,
    TG_SPIRV_INOUT_FORMAT_R32G32_SFLOAT          = 103,
    TG_SPIRV_INOUT_FORMAT_R32G32_SINT            = 102,
    TG_SPIRV_INOUT_FORMAT_R32G32_UINT            = 101,
    TG_SPIRV_INOUT_FORMAT_R32G32B32_SFLOAT       = 106,
    TG_SPIRV_INOUT_FORMAT_R32G32B32_SINT         = 105,
    TG_SPIRV_INOUT_FORMAT_R32G32B32_UINT         = 104,
    TG_SPIRV_INOUT_FORMAT_R32G32B32A32_SFLOAT    = 109,
    TG_SPIRV_INOUT_FORMAT_R32G32B32A32_SINT      = 108,
    TG_SPIRV_INOUT_FORMAT_R32G32B32A32_UINT      = 107
} tg_spirv_inout_format;

typedef enum tg_spirv_shader_type
{
    TG_SPIRV_SHADER_TYPE_COMPUTE     = 32,
    TG_SPIRV_SHADER_TYPE_FRAGMENT    = 16,
    TG_SPIRV_SHADER_TYPE_VERTEX      = 1
} tg_spirv_shader_type;



typedef struct tg_spirv_global_resource
{
    tg_spirv_global_resource_type    type;
    u32                              descriptor_set;
    u32                              binding;
} tg_spirv_global_resource;

typedef struct tg_spirv_inout_resource
{
    u32                      location;
    tg_spirv_inout_format    format;
    u32                      offset;
} tg_spirv_inout_resource;

typedef struct tg_spirv_layout
{
    tg_spirv_shader_type        shader_type;
    char                        p_entry_point_name[TG_SPIRV_MAX_NAME];
    u8                          input_resource_count;
    u8                          global_resource_count;
    u8                          output_resource_count;
    u32                         vertex_stride;
    tg_spirv_inout_resource     p_input_resources[TG_SPIRV_MAX_INPUTS];
    tg_spirv_global_resource    p_global_resources[TG_SPIRV_MAX_GLOBALS];
    tg_spirv_inout_resource     p_output_resources[TG_SPIRV_MAX_OUTPUTS];
} tg_spirv_layout;



void tg_spirv_fill_layout(u32 word_count, const u32* p_words, tg_spirv_layout* p_spirv_layout);

#endif
