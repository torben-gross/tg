#ifndef TG_SPIRV_H
#define TG_SPIRV_H

#include "graphics/tg_graphics.h"
#include "tg_common.h"



typedef enum tg_spirv_global_resource_type
{
    TG_SPIRV_GLOBAL_RESOURCE_TYPE_INVALID                   = 0,
    TG_SPIRV_GLOBAL_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER    = 1,
    TG_SPIRV_GLOBAL_RESOURCE_TYPE_UNIFORM_BUFFER            = 6,
    TG_SPIRV_GLOBAL_RESOURCE_TYPE_STORAGE_BUFFER            = 7,
    TG_SPIRV_GLOBAL_RESOURCE_TYPE_STORAGE_IMAGE             = 3
} tg_spirv_global_resource_type;

// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkShaderStageFlagBits.html
typedef enum tg_spirv_shader_type
{
    TG_SPIRV_SHADER_TYPE_COMPUTE     = 32,
    TG_SPIRV_SHADER_TYPE_FRAGMENT    = 16,
    TG_SPIRV_SHADER_TYPE_GEOMETRY    = 8,
    TG_SPIRV_SHADER_TYPE_VERTEX      = 1
} tg_spirv_shader_type;



typedef struct tg_spirv_global_resource
{
    tg_spirv_global_resource_type    type;
    u32                              array_element_count; // 0 if this is not an array
    u32                              descriptor_set;
    u32                              binding;
} tg_spirv_global_resource;

typedef struct tg_spirv_inout_resource
{
    u32                                 location;
    tg_vertex_input_attribute_format    format;
} tg_spirv_inout_resource;

typedef struct tg_spirv_layout
{
    tg_spirv_shader_type               shader_type;
    union
    {
        struct
        {
            u32                        count;
            tg_spirv_inout_resource    p_resources[TG_MAX_SHADER_INPUTS];
        } vertex_shader_input;
        struct
        {
            u32                        count;
            tg_spirv_inout_resource    p_resources[TG_MAX_SHADER_INPUTS];
        } fragment_shader_output;
    };
    struct
    {
        u32                            count;
        tg_spirv_global_resource       p_resources[TG_MAX_SHADER_GLOBAL_RESOURCES];
    } global_resources;
} tg_spirv_layout;



void tg_spirv_fill_layout(u32 word_count, const u32* p_words, tg_spirv_layout* p_spirv_layout);

#endif
