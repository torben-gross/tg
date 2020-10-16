#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "graphics/tg_image_loader.h"
#include "graphics/tg_shader_library.h"
#include "util/tg_string.h"
#include <shaderc/shaderc.h>



#ifdef TG_WIN32

#ifdef TG_DEBUG
#define TGVK_INSTANCE_EXTENSION_COUNT    4
#define TGVK_INSTANCE_EXTENSION_NAMES    { VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME }
#else
#define TGVK_INSTANCE_EXTENSION_COUNT    3
#define TGVK_INSTANCE_EXTENSION_NAMES    { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME }
#endif

#else

#define TGVK_INSTANCE_EXTENSION_COUNT    0
#define TGVK_INSTANCE_EXTENSION_NAMES    TG_NULL

#endif

#define TGVK_DEVICE_EXTENSION_COUNT      1
#define TGVK_DEVICE_EXTENSION_NAMES      { VK_KHR_SWAPCHAIN_EXTENSION_NAME }

#ifdef TG_DEBUG
#define TGVK_VALIDATION_LAYER_COUNT      1
#define TGVK_VALIDATION_LAYER_NAMES      { "VK_LAYER_KHRONOS_validation" }
#else
#define TGVK_VALIDATION_LAYER_COUNT      0
#define TGVK_VALIDATION_LAYER_NAMES      TG_NULL
#endif



#ifdef TG_DEBUG
static VkDebugUtilsMessengerEXT    debug_utils_messenger = VK_NULL_HANDLE;
#endif

static tgvk_queue                  p_queues[TGVK_QUEUE_TYPE_COUNT];

static VkCommandPool               p_compute_command_pools[TG_MAX_THREADS];
static VkCommandPool               p_graphics_command_pools[TG_MAX_THREADS];
static VkCommandPool               present_command_pool;

static tgvk_command_buffer         p_global_compute_command_buffers[TG_MAX_THREADS];
static tgvk_command_buffer         p_global_graphics_command_buffers[TG_MAX_THREADS];

static tg_read_write_lock          global_staging_buffer_lock;
static tgvk_buffer                 global_staging_buffer;

static tg_read_write_lock          handle_lock;

#define TGVK_STRUCTURE_NAME(name)                    tg_##name
#define TGVK_STRUCTURE_SIZE_NAME(name)               name##_size
#define TGVK_STRUCTURE_BUFFER_COUNT_NAME(name)       name##_buffer_count
#define TGVK_STRUCTURE_BUFFER_NAME(name)             p_##name##_buffer
#define TGVK_STRUCTURE_FREE_LIST_COUNT_NAME(name)    name##_free_list_count
#define TGVK_STRUCTURE_FREE_LIST_NAME(name)          p_##name##_free_list

#define TGVK_DEFINE_STRUCTURE_BUFFER(name, count)                                  \
    static u64 TGVK_STRUCTURE_SIZE_NAME(name) = sizeof(TGVK_STRUCTURE_NAME(name)); \
    static u16 TGVK_STRUCTURE_BUFFER_COUNT_NAME(name);                             \
    static TGVK_STRUCTURE_NAME(name) TGVK_STRUCTURE_BUFFER_NAME(name)[count];      \
    static u16 TGVK_STRUCTURE_FREE_LIST_COUNT_NAME(name);                          \
    static u16 TGVK_STRUCTURE_FREE_LIST_NAME(name)[count]

TGVK_DEFINE_STRUCTURE_BUFFER(color_image, TG_MAX_COLOR_IMAGES);
TGVK_DEFINE_STRUCTURE_BUFFER(color_image_3d, TG_MAX_COLOR_IMAGES_3D);
TGVK_DEFINE_STRUCTURE_BUFFER(compute_shader, TG_MAX_COMPUTE_SHADERS);
TGVK_DEFINE_STRUCTURE_BUFFER(cube_map, TG_MAX_CUBE_MAPS);
TGVK_DEFINE_STRUCTURE_BUFFER(depth_image, TG_MAX_DEPTH_IMAGES);
TGVK_DEFINE_STRUCTURE_BUFFER(fragment_shader, TG_MAX_FRAGMENT_SHADERS);
TGVK_DEFINE_STRUCTURE_BUFFER(material, TG_MAX_MATERIALS);
TGVK_DEFINE_STRUCTURE_BUFFER(mesh, TG_MAX_MESHES);
TGVK_DEFINE_STRUCTURE_BUFFER(render_command, TG_MAX_RENDER_COMMANDS);
TGVK_DEFINE_STRUCTURE_BUFFER(renderer, TG_MAX_RENDERERS);
TGVK_DEFINE_STRUCTURE_BUFFER(storage_buffer, TG_MAX_STORAGE_BUFFERS);
TGVK_DEFINE_STRUCTURE_BUFFER(uniform_buffer, TG_MAX_UNIFORM_BUFFERS);
TGVK_DEFINE_STRUCTURE_BUFFER(vertex_shader, TG_MAX_VERTEX_SHADERS);

#define TGVK_STRUCTURE_TAKE(name, p_handle)                                                                                               \
    if (TGVK_STRUCTURE_FREE_LIST_COUNT_NAME(name) > 0)                                                                                    \
        (p_handle) = &TGVK_STRUCTURE_BUFFER_NAME(name)[TGVK_STRUCTURE_FREE_LIST_NAME(name)[--TGVK_STRUCTURE_FREE_LIST_COUNT_NAME(name)]]; \
    else                                                                                                                                  \
        (p_handle) = &TGVK_STRUCTURE_BUFFER_NAME(name)[TGVK_STRUCTURE_BUFFER_COUNT_NAME(name)++]

#define TGVK_STRUCTURE_RELEASE(name, p_handle)                                                                                   \
    if (&TGVK_STRUCTURE_BUFFER_NAME(name)[TGVK_STRUCTURE_BUFFER_COUNT_NAME(name) - 1] == (TGVK_STRUCTURE_NAME(name)*)(p_handle)) \
        TGVK_STRUCTURE_BUFFER_COUNT_NAME(name)--;                                                                                \
    else                                                                                                                         \
        TGVK_STRUCTURE_FREE_LIST_NAME(name)[TGVK_STRUCTURE_FREE_LIST_COUNT_NAME(name)++] =                                       \
            (u16)((TGVK_STRUCTURE_NAME(name)*)(p_handle) - TGVK_STRUCTURE_BUFFER_NAME(name));                                    \
    tg_memory_nullify(TGVK_STRUCTURE_SIZE_NAME(name), p_handle)



#ifdef TG_DEBUG
#pragma warning(push)
#pragma warning(disable:4100)
static VKAPI_ATTR VkBool32 VKAPI_CALL tg__debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* user_data)
{
    TG_DEBUG_LOG("%s\n", p_callback_data->pMessage);
    return VK_TRUE;
}
#pragma warning(pop)
#endif



/*------------------------------------------------------------+
| General utilities                                           |
+------------------------------------------------------------*/

#define TGVK_QUEUE_TAKE(type, p_queue)    (p_queue) = &p_queues[type]; TG_MUTEX_LOCK((p_queue)->h_mutex)
#define TGVK_QUEUE_RELEASE(p_queue)       TG_MUTEX_UNLOCK((p_queue)->h_mutex)



static VkImageView tg__image_view_create(VkImage image, VkImageViewType view_type, VkFormat format, VkImageAspectFlagBits aspect_mask, u32 mip_levels)
{
    VkImageView image_view = VK_NULL_HANDLE;

    VkImageViewCreateInfo image_view_create_info = { 0 };
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = TG_NULL;
    image_view_create_info.flags = 0;
    image_view_create_info.image = image;
    image_view_create_info.viewType = view_type;
    image_view_create_info.format = format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = aspect_mask;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = mip_levels;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = view_type == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;

    TGVK_CALL(vkCreateImageView(device, &image_view_create_info, TG_NULL, &image_view));

    return image_view;
}



static tgvk_pipeline_layout tg__pipeline_layout_create(u32 shader_count, const tgvk_shader* const* pp_shaders)
{
    tgvk_pipeline_layout pipeline_layout = { 0 };
    pipeline_layout.global_resource_count = 0;

    for (u32 i = 0; i < shader_count; i++)
    {
        for (u32 j = 0; j < pp_shaders[i]->spirv_layout.global_resource_count; j++)
        {
            TG_ASSERT(pp_shaders[i]->spirv_layout.p_global_resources[j].descriptor_set == 0);
            b32 found = TG_FALSE;
            for (u32 k = 0; k < pipeline_layout.global_resource_count; k++)
            {
                const b32 same_set = pipeline_layout.p_global_resources[k].descriptor_set == pp_shaders[i]->spirv_layout.p_global_resources[j].descriptor_set;
                const b32 same_binding = pipeline_layout.p_global_resources[k].binding == pp_shaders[i]->spirv_layout.p_global_resources[j].binding;
                if (same_set && same_binding)
                {
                    TG_ASSERT(pipeline_layout.p_global_resources[k].type == pp_shaders[i]->spirv_layout.p_global_resources[j].type);
                    pipeline_layout.p_shader_stages[k] |= (VkShaderStageFlags)pp_shaders[i]->spirv_layout.shader_type;
                    found = TG_TRUE;
                    break;
                }
            }
            if (!found)
            {
                TG_ASSERT(pipeline_layout.global_resource_count + 1 < TG_MAX_SHADER_GLOBAL_RESOURCES);
                pipeline_layout.p_global_resources[pipeline_layout.global_resource_count] = pp_shaders[i]->spirv_layout.p_global_resources[j];
                pipeline_layout.p_shader_stages[pipeline_layout.global_resource_count] = (VkShaderStageFlags)pp_shaders[i]->spirv_layout.shader_type;
                pipeline_layout.global_resource_count++;
            }
        }
    }

    TG_ASSERT(pipeline_layout.global_resource_count);

    for (u32 i = 0; i < pipeline_layout.global_resource_count; i++)
    {
        pipeline_layout.p_descriptor_set_layout_bindings[i].binding = pipeline_layout.p_global_resources[i].binding;
        pipeline_layout.p_descriptor_set_layout_bindings[i].descriptorType = (VkDescriptorType)pipeline_layout.p_global_resources[i].type;
        pipeline_layout.p_descriptor_set_layout_bindings[i].descriptorCount = tgm_u32_max(1, pipeline_layout.p_global_resources[i].array_element_count);
        pipeline_layout.p_descriptor_set_layout_bindings[i].stageFlags = pipeline_layout.p_shader_stages[i];
        pipeline_layout.p_descriptor_set_layout_bindings[i].pImmutableSamplers = TG_NULL;
    }

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = { 0 };
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.pNext = TG_NULL;
    descriptor_set_layout_create_info.flags = 0;
    descriptor_set_layout_create_info.bindingCount = pipeline_layout.global_resource_count;
    descriptor_set_layout_create_info.pBindings = pipeline_layout.p_descriptor_set_layout_bindings;

    TGVK_CALL(vkCreateDescriptorSetLayout(device, &descriptor_set_layout_create_info, TG_NULL, &pipeline_layout.descriptor_set_layout));

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = { 0 };
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pNext = TG_NULL;
    pipeline_layout_create_info.flags = 0;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &pipeline_layout.descriptor_set_layout;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = TG_NULL;

    TGVK_CALL(vkCreatePipelineLayout(device, &pipeline_layout_create_info, TG_NULL, &pipeline_layout.pipeline_layout));

    return pipeline_layout;
}



static VkSampler tg__sampler_create_custom(u32 mip_levels, VkFilter min_filter, VkFilter mag_filter, VkSamplerAddressMode address_mode_u, VkSamplerAddressMode address_mode_v, VkSamplerAddressMode address_mode_w)
{
    VkSampler sampler = VK_NULL_HANDLE;

    VkSamplerCreateInfo sampler_create_info = { 0 };
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.pNext = TG_NULL;
    sampler_create_info.flags = 0;
    sampler_create_info.magFilter = mag_filter;
    sampler_create_info.minFilter = min_filter;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.addressModeU = address_mode_u;
    sampler_create_info.addressModeV = address_mode_v;
    sampler_create_info.addressModeW = address_mode_w;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.anisotropyEnable = VK_TRUE;
    sampler_create_info.maxAnisotropy = 16;
    sampler_create_info.compareEnable = VK_FALSE;
    sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = (f32)mip_levels;
    sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;

    TGVK_CALL(vkCreateSampler(device, &sampler_create_info, TG_NULL, &sampler));

    return sampler;
}

static VkSampler tg__sampler_create(u32 mip_levels)
{
    return tg__sampler_create_custom(mip_levels, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);
}

static void tg__sampler_destroy(VkSampler sampler)
{
    vkDestroySampler(device, sampler, TG_NULL);
}





u32 tg_vertex_input_attribute_format_get_alignment(tg_vertex_input_attribute_format format)
{
    u32 result = 0;

    switch (format)
    {
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_SFLOAT:          result = 4; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_SINT:            result = 4; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_UINT:            result = 4; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_SFLOAT:       result = 4; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_SINT:         result = 4; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_UINT:         result = 4; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT:    result = 4; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_SINT:      result = 4; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_UINT:      result = 4; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT: result = 4; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32A32_SINT:   result = 4; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32A32_UINT:   result = 4; break;

    default: TG_INVALID_CODEPATH(); break;
    }

    return result;
}

u32 tg_vertex_input_attribute_format_get_size(tg_vertex_input_attribute_format format)
{
    u32 result = 0;

    switch (format)
    {
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_SFLOAT:          result = 4; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_SINT:            result = 4; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_UINT:            result = 4; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_SFLOAT:       result = 8; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_SINT:         result = 8; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_UINT:         result = 8; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT:    result = 12; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_SINT:      result = 12; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_UINT:      result = 12; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT: result = 16; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32A32_SINT:   result = 16; break;
    case TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32A32_UINT:   result = 16; break;

    default: TG_INVALID_CODEPATH(); break;
    }

    return result;
}


void* tgvk_handle_array(tg_structure_type type)
{
    void* p_array = TG_NULL;

    switch (type)
    {
    case TG_STRUCTURE_TYPE_COLOR_IMAGE:     p_array = TGVK_STRUCTURE_BUFFER_NAME(color_image);     break;
    case TG_STRUCTURE_TYPE_COLOR_IMAGE_3D:  p_array = TGVK_STRUCTURE_BUFFER_NAME(color_image_3d);  break;
    case TG_STRUCTURE_TYPE_COMPUTE_SHADER:  p_array = TGVK_STRUCTURE_BUFFER_NAME(compute_shader);  break;
    case TG_STRUCTURE_TYPE_CUBE_MAP:        p_array = TGVK_STRUCTURE_BUFFER_NAME(cube_map);        break;
    case TG_STRUCTURE_TYPE_DEPTH_IMAGE:     p_array = TGVK_STRUCTURE_BUFFER_NAME(depth_image);     break;
    case TG_STRUCTURE_TYPE_FRAGMENT_SHADER: p_array = TGVK_STRUCTURE_BUFFER_NAME(fragment_shader); break;
    case TG_STRUCTURE_TYPE_MATERIAL:        p_array = TGVK_STRUCTURE_BUFFER_NAME(material);        break;
    case TG_STRUCTURE_TYPE_MESH:            p_array = TGVK_STRUCTURE_BUFFER_NAME(mesh);            break;
    case TG_STRUCTURE_TYPE_RENDER_COMMAND:  p_array = TGVK_STRUCTURE_BUFFER_NAME(render_command);  break;
    case TG_STRUCTURE_TYPE_RENDERER:        p_array = TGVK_STRUCTURE_BUFFER_NAME(renderer);        break;
    case TG_STRUCTURE_TYPE_STORAGE_BUFFER:  p_array = TGVK_STRUCTURE_BUFFER_NAME(storage_buffer);  break;
    case TG_STRUCTURE_TYPE_UNIFORM_BUFFER:  p_array = TGVK_STRUCTURE_BUFFER_NAME(uniform_buffer);  break;
    case TG_STRUCTURE_TYPE_VERTEX_SHADER:   p_array = TGVK_STRUCTURE_BUFFER_NAME(vertex_shader);   break;

    default: TG_INVALID_CODEPATH(); break;
    }

    return p_array;
}

void* tgvk_handle_take(tg_structure_type type)
{
    void* p_handle = TG_NULL;

    TG_RWL_LOCK_FOR_WRITE(handle_lock);
    switch (type)
    {
    case TG_STRUCTURE_TYPE_COLOR_IMAGE:     { TGVK_STRUCTURE_TAKE(color_image, p_handle);     } break;
    case TG_STRUCTURE_TYPE_COLOR_IMAGE_3D:  { TGVK_STRUCTURE_TAKE(color_image_3d, p_handle);  } break;
    case TG_STRUCTURE_TYPE_COMPUTE_SHADER:  { TGVK_STRUCTURE_TAKE(compute_shader, p_handle);  } break;
    case TG_STRUCTURE_TYPE_CUBE_MAP:        { TGVK_STRUCTURE_TAKE(cube_map, p_handle);        } break;
    case TG_STRUCTURE_TYPE_DEPTH_IMAGE:     { TGVK_STRUCTURE_TAKE(depth_image, p_handle);     } break;
    case TG_STRUCTURE_TYPE_FRAGMENT_SHADER: { TGVK_STRUCTURE_TAKE(fragment_shader, p_handle); } break;
    case TG_STRUCTURE_TYPE_MATERIAL:        { TGVK_STRUCTURE_TAKE(material, p_handle);        } break;
    case TG_STRUCTURE_TYPE_MESH:            { TGVK_STRUCTURE_TAKE(mesh, p_handle);           } break;
    case TG_STRUCTURE_TYPE_RENDER_COMMAND:  { TGVK_STRUCTURE_TAKE(render_command, p_handle);  } break;
    case TG_STRUCTURE_TYPE_RENDERER:        { TGVK_STRUCTURE_TAKE(renderer, p_handle);        } break;
    case TG_STRUCTURE_TYPE_STORAGE_BUFFER:  { TGVK_STRUCTURE_TAKE(storage_buffer, p_handle);  } break;
    case TG_STRUCTURE_TYPE_UNIFORM_BUFFER:  { TGVK_STRUCTURE_TAKE(uniform_buffer, p_handle);  } break;
    case TG_STRUCTURE_TYPE_VERTEX_SHADER:   { TGVK_STRUCTURE_TAKE(vertex_shader, p_handle);   } break;

    default: TG_INVALID_CODEPATH(); break;
    }
    *(tg_structure_type*)p_handle = type;
    TG_RWL_UNLOCK_FOR_WRITE(handle_lock);

    return p_handle;
}

void tgvk_handle_release(void* p_handle)
{
    TG_ASSERT(p_handle && *(tg_structure_type*)p_handle != TG_STRUCTURE_TYPE_INVALID);
    
    const tg_structure_type type = *(tg_structure_type*)p_handle;
    TG_RWL_LOCK_FOR_WRITE(handle_lock);
    switch (type)
    {
    case TG_STRUCTURE_TYPE_COLOR_IMAGE:     { TGVK_STRUCTURE_RELEASE(color_image, p_handle);     } break;
    case TG_STRUCTURE_TYPE_COLOR_IMAGE_3D:  { TGVK_STRUCTURE_RELEASE(color_image_3d, p_handle);  } break;
    case TG_STRUCTURE_TYPE_COMPUTE_SHADER:  { TGVK_STRUCTURE_RELEASE(compute_shader, p_handle);  } break;
    case TG_STRUCTURE_TYPE_CUBE_MAP:        { TGVK_STRUCTURE_RELEASE(cube_map, p_handle);        } break;
    case TG_STRUCTURE_TYPE_DEPTH_IMAGE:     { TGVK_STRUCTURE_RELEASE(depth_image, p_handle);     } break;
    case TG_STRUCTURE_TYPE_FRAGMENT_SHADER: { TGVK_STRUCTURE_RELEASE(fragment_shader, p_handle); } break;
    case TG_STRUCTURE_TYPE_MATERIAL:        { TGVK_STRUCTURE_RELEASE(material, p_handle);        } break;
    case TG_STRUCTURE_TYPE_MESH:            { TGVK_STRUCTURE_RELEASE(mesh, p_handle);           } break;
    case TG_STRUCTURE_TYPE_RENDER_COMMAND:  { TGVK_STRUCTURE_RELEASE(render_command, p_handle);  } break;
    case TG_STRUCTURE_TYPE_RENDERER:        { TGVK_STRUCTURE_RELEASE(renderer, p_handle);        } break;
    case TG_STRUCTURE_TYPE_STORAGE_BUFFER:  { TGVK_STRUCTURE_RELEASE(storage_buffer, p_handle);  } break;
    case TG_STRUCTURE_TYPE_UNIFORM_BUFFER:  { TGVK_STRUCTURE_RELEASE(uniform_buffer, p_handle);  } break;
    case TG_STRUCTURE_TYPE_VERTEX_SHADER:   { TGVK_STRUCTURE_RELEASE(vertex_shader, p_handle);   } break;
    
    default: TG_INVALID_CODEPATH(); break;
    }
    TG_RWL_UNLOCK_FOR_WRITE(handle_lock);
}



void tgvk_buffer_copy(VkDeviceSize size, tgvk_buffer* p_src, tgvk_buffer* p_dst)
{
    const u32 thread_id = tg_platform_get_thread_id();
    tgvk_command_buffer_begin(&p_global_graphics_command_buffers[thread_id], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    tgvk_command_buffer_cmd_copy_buffer(&p_global_graphics_command_buffers[thread_id], size, p_src, p_dst);
    tgvk_command_buffer_end_and_submit(&p_global_graphics_command_buffers[thread_id]);
}

tgvk_buffer tgvk_buffer_create(VkDeviceSize size, VkBufferUsageFlags buffer_usage_flags, VkMemoryPropertyFlags memory_property_flags)
{
    tgvk_buffer buffer = { 0 };

    VkBufferCreateInfo buffer_create_info = { 0 };
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext = TG_NULL;
    buffer_create_info.flags = 0;
    buffer_create_info.size = tgvk_memory_aligned_size(size);
    buffer_create_info.usage = buffer_usage_flags;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = TG_NULL;

    TGVK_CALL(vkCreateBuffer(device, &buffer_create_info, TG_NULL, &buffer.buffer));

    VkMemoryRequirements memory_requirements = { 0 };
    vkGetBufferMemoryRequirements(device, buffer.buffer, &memory_requirements);
    buffer.memory = tgvk_memory_allocator_alloc(memory_requirements.alignment, memory_requirements.size, memory_requirements.memoryTypeBits, memory_property_flags);
    TGVK_CALL(vkBindBufferMemory(device, buffer.buffer, buffer.memory.device_memory, buffer.memory.offset));

    return buffer;
}

void tgvk_buffer_destroy(tgvk_buffer* p_buffer)
{
    tgvk_memory_allocator_free(&p_buffer->memory);
    vkDestroyBuffer(device, p_buffer->buffer, TG_NULL);
}

void tgvk_buffer_flush_device_to_host(tgvk_buffer* p_buffer)
{
    VkMappedMemoryRange mapped_memory_range = { 0 };
    mapped_memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mapped_memory_range.pNext = TG_NULL;
    mapped_memory_range.memory = p_buffer->memory.device_memory;
    mapped_memory_range.offset = p_buffer->memory.offset;
    mapped_memory_range.size = p_buffer->memory.size;

    TGVK_CALL(vkInvalidateMappedMemoryRanges(device, 1, &mapped_memory_range));
}

void tgvk_buffer_flush_device_to_host_range(tgvk_buffer* p_buffer, VkDeviceSize offset, VkDeviceSize size)
{
    TG_ASSERT(size <= p_buffer->memory.size);

    VkPhysicalDeviceProperties physical_device_properties = { 0 };
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

    VkMappedMemoryRange mapped_memory_range = { 0 };
    mapped_memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mapped_memory_range.pNext = TG_NULL;
    mapped_memory_range.memory = p_buffer->memory.device_memory;
    mapped_memory_range.offset = p_buffer->memory.offset + offset;
    mapped_memory_range.size = TG_CEIL_TO_MULTIPLE(size, physical_device_properties.limits.nonCoherentAtomSize);

    TGVK_CALL(vkInvalidateMappedMemoryRanges(device, 1, &mapped_memory_range));
}

void tgvk_buffer_flush_host_to_device(tgvk_buffer* p_buffer)
{
    VkMappedMemoryRange mapped_memory_range = { 0 };
    mapped_memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mapped_memory_range.pNext = TG_NULL;
    mapped_memory_range.memory = p_buffer->memory.device_memory;
    mapped_memory_range.offset = p_buffer->memory.offset;
    mapped_memory_range.size = p_buffer->memory.size;

    TGVK_CALL(vkFlushMappedMemoryRanges(device, 1, &mapped_memory_range));
}

void tgvk_buffer_flush_host_to_device_range(tgvk_buffer* p_buffer, VkDeviceSize offset, VkDeviceSize size)
{
    TG_ASSERT(size <= p_buffer->memory.size);

    VkPhysicalDeviceProperties physical_device_properties = { 0 };
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

    VkMappedMemoryRange mapped_memory_range = { 0 };
    mapped_memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mapped_memory_range.pNext = TG_NULL;
    mapped_memory_range.memory = p_buffer->memory.device_memory;
    mapped_memory_range.offset = p_buffer->memory.offset + offset;
    mapped_memory_range.size = TG_CEIL_TO_MULTIPLE(size, physical_device_properties.limits.nonCoherentAtomSize);

    TGVK_CALL(vkFlushMappedMemoryRanges(device, 1, &mapped_memory_range));
}



tgvk_image tgvk_color_image_create(u32 width, u32 height, VkFormat format, const tg_sampler_create_info* p_sampler_create_info)
{
    tgvk_image image = { 0 };
    image.width = width;
    image.height = height;
    image.format = format;

    VkImageCreateInfo image_create_info = { 0 };
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = TG_NULL;
    image_create_info.flags = 0;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = image.format;
    image_create_info.extent.width = image.width;
    image_create_info.extent.height = image.height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = TG_NULL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    TGVK_CALL(vkCreateImage(device, &image_create_info, TG_NULL, &image.image));

    VkMemoryRequirements memory_requirements = { 0 };
    vkGetImageMemoryRequirements(device, image.image, &memory_requirements);
    image.memory = tgvk_memory_allocator_alloc(memory_requirements.alignment, memory_requirements.size, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    TGVK_CALL(vkBindImageMemory(device, image.image, image.memory.device_memory, image.memory.offset));

    // TODO: mipmapping
    //VkCommandBuffer command_buffer = tgvk_command_buffer_create(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    //tgvk_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    //
    //VkImageMemoryBarrier mipmap_image_memory_barrier = { 0 };
    //{
    //    mipmap_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    //    mipmap_image_memory_barrier.pNext = TG_NULL;
    //    mipmap_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    //    mipmap_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    //    mipmap_image_memory_barrier.image = color_image.image;
    //    mipmap_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //    mipmap_image_memory_barrier.subresourceRange.baseMipLevel = 0;
    //    mipmap_image_memory_barrier.subresourceRange.levelCount = 1;
    //    mipmap_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    //    mipmap_image_memory_barrier.subresourceRange.layerCount = 1;
    //}
    //u32 mip_width = color_image.width;
    //u32 mip_height = color_image.height;
    //for (u32 i = 1; i < color_image.mip_levels; i++)
    //{
    //    const u32 next_mip_width = tgm_u32_max(1, mip_width / 2);
    //    const u32 next_mip_height = tgm_u32_max(1, mip_height / 2);
    //
    //    mipmap_image_memory_barrier.subresourceRange.baseMipLevel = i - 1;
    //    mipmap_image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    //    mipmap_image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    //    mipmap_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    //    mipmap_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    //
    //    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, TG_NULL, 0, TG_NULL, 1, &mipmap_image_memory_barrier);
    //
    //    VkImageBlit image_blit = { 0 };
    //    {
    //        image_blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //        image_blit.srcSubresource.mipLevel = i - 1;
    //        image_blit.srcSubresource.baseArrayLayer = 0;
    //        image_blit.srcSubresource.layerCount = 1;
    //        image_blit.srcOffsets[0].x = 0;
    //        image_blit.srcOffsets[0].y = 0;
    //        image_blit.srcOffsets[0].z = 0;
    //        image_blit.srcOffsets[1].x = mip_width;
    //        image_blit.srcOffsets[1].y = mip_height;
    //        image_blit.srcOffsets[1].z = 1;
    //        image_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //        image_blit.dstSubresource.mipLevel = i;
    //        image_blit.dstSubresource.baseArrayLayer = 0;
    //        image_blit.dstSubresource.layerCount = 1;
    //        image_blit.dstOffsets[0].x = 0;
    //        image_blit.dstOffsets[0].y = 0;
    //        image_blit.dstOffsets[0].z = 0;
    //        image_blit.dstOffsets[1].x = next_mip_width;
    //        image_blit.dstOffsets[1].y = next_mip_height;
    //        image_blit.dstOffsets[1].z = 1;
    //    }
    //    vkCmdBlitImage(command_buffer, color_image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, color_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_blit, VK_FILTER_LINEAR);
    //
    //    mipmap_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    //    mipmap_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //    mipmap_image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    //    mipmap_image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    //
    //    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, TG_NULL, 0, TG_NULL, 1, &mipmap_image_memory_barrier);
    //
    //    mip_width = next_mip_width;
    //    mip_height = next_mip_height;
    //}
    //
    //mipmap_image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    //mipmap_image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    //mipmap_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    //mipmap_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //mipmap_image_memory_barrier.subresourceRange.baseMipLevel = color_image.mip_levels - 1;
    //
    //vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, TG_NULL, 0, TG_NULL, 1, &image_memory_barrier);
    //tgvk_command_buffer_end_and_submit(command_buffer, &graphics_queue);
    //tgvk_command_buffer_destroy(graphics_command_pool, command_buffer);
    
    image.image_view = tg__image_view_create(image.image, VK_IMAGE_VIEW_TYPE_2D, image.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    if (p_sampler_create_info)
    {
        image.sampler = tg__sampler_create_custom(
            1,
            p_sampler_create_info->min_filter,
            p_sampler_create_info->mag_filter,
            p_sampler_create_info->address_mode_u,
            p_sampler_create_info->address_mode_v,
            p_sampler_create_info->address_mode_w
        );
    }
    else
    {
        image.sampler = tg__sampler_create(1);
    }

    return image;
}

tgvk_image tgvk_color_image_create2(const char* p_filename, const tg_sampler_create_info* p_sampler_create_info)
{
    u32 w, h;
    tg_color_image_format f;
    u32* p_data = TG_NULL;
    tg_image_load(p_filename, &w, &h, &f, &p_data);
    //h_color_image->mip_levels = TG_IMAGE_MAX_MIP_LEVELS(h_color_image->width, h_color_image->height);// TODO: mipmapping
    const u64 size = (u64)w * (u64)h * sizeof(*p_data);

    tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(size);
    tg_memory_copy(size, p_data, p_staging_buffer->memory.p_mapped_device_memory);

    tgvk_image image = tgvk_color_image_create(w, h, (VkFormat)f, p_sampler_create_info);

    tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
    tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    tgvk_command_buffer_cmd_transition_color_image_layout(p_command_buffer, &image, 0, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    tgvk_command_buffer_cmd_copy_buffer_to_color_image(p_command_buffer, p_staging_buffer->buffer, &image);
    tgvk_command_buffer_cmd_transition_color_image_layout(p_command_buffer, &image, 0, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    tgvk_command_buffer_end_and_submit(p_command_buffer);
    tgvk_global_staging_buffer_release();

    tg_image_free(p_data);

    return image;
}



tgvk_image_3d tgvk_color_image_3d_create(u32 width, u32 height, u32 depth, VkFormat format, const tg_sampler_create_info* p_sampler_create_info)
{
    tgvk_image_3d image_3d = { 0 };
    image_3d.width = width;
    image_3d.height = height;
    image_3d.depth = depth;
    image_3d.format = format;

    VkImageCreateInfo image_create_info = { 0 };
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = TG_NULL;
    image_create_info.flags = 0;
    image_create_info.imageType = VK_IMAGE_TYPE_3D;
    image_create_info.format = format;
    image_create_info.extent.width = width;
    image_create_info.extent.height = height;
    image_create_info.extent.depth = depth;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = TG_NULL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    TGVK_CALL(vkCreateImage(device, &image_create_info, TG_NULL, &image_3d.image));

    VkMemoryRequirements memory_requirements = { 0 };
    vkGetImageMemoryRequirements(device, image_3d.image, &memory_requirements);
    image_3d.memory = tgvk_memory_allocator_alloc(memory_requirements.alignment, memory_requirements.size, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    TGVK_CALL(vkBindImageMemory(device, image_3d.image, image_3d.memory.device_memory, image_3d.memory.offset));

    image_3d.image_view = tg__image_view_create(image_3d.image, VK_IMAGE_VIEW_TYPE_3D, image_3d.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    if (p_sampler_create_info)
    {
        // TODO: mip levels, also above and below, once respectively
        image_3d.sampler = tg__sampler_create_custom(
            1,
            p_sampler_create_info->min_filter,
            p_sampler_create_info->mag_filter,
            p_sampler_create_info->address_mode_u,
            p_sampler_create_info->address_mode_v,
            p_sampler_create_info->address_mode_w
        );
    }
    else
    {
        image_3d.sampler = tg__sampler_create(1);
    }

    return image_3d;
}

void tgvk_color_image_3d_destroy(tgvk_image_3d* p_image_3d)
{
    vkDestroySampler(device, p_image_3d->sampler, TG_NULL);
    vkDestroyImageView(device, p_image_3d->image_view, TG_NULL);
    tgvk_memory_allocator_free(&p_image_3d->memory);
    vkDestroyImage(device, p_image_3d->image, TG_NULL);
}



tgvk_command_buffer* tgvk_command_buffer_get_global(tgvk_command_pool_type type)
{
    tgvk_command_buffer* p_command_buffer = TG_NULL;

    const u32 thread_id = tg_platform_get_thread_id();
    switch (type)
    {
    case TGVK_COMMAND_POOL_TYPE_COMPUTE:
    {
        p_command_buffer = &p_global_compute_command_buffers[thread_id];
    } break;
    case TGVK_COMMAND_POOL_TYPE_GRAPHICS:
    {
        p_command_buffer = &p_global_graphics_command_buffers[thread_id];
    } break;
    case TGVK_COMMAND_POOL_TYPE_PRESENT:

    default: TG_INVALID_CODEPATH(); break;
    }

    return p_command_buffer;
}

tgvk_command_buffer tgvk_command_buffer_create(tgvk_command_pool_type type, VkCommandBufferLevel level)
{
    tgvk_command_buffer command_buffer = { 0 };
    command_buffer.command_pool_type = type;
    command_buffer.command_buffer_level = level;

    VkCommandBufferAllocateInfo command_buffer_allocate_info = { 0 };
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = TG_NULL;
    command_buffer_allocate_info.level = level;
    command_buffer_allocate_info.commandBufferCount = 1;

    switch (type)
    {
    case TGVK_COMMAND_POOL_TYPE_COMPUTE:
    {
        const u32 thread_id = tg_platform_get_thread_id();
        command_buffer_allocate_info.commandPool = p_compute_command_pools[thread_id];
    } break;
    case TGVK_COMMAND_POOL_TYPE_GRAPHICS:
    {
        const u32 thread_id = tg_platform_get_thread_id();
        command_buffer_allocate_info.commandPool = p_graphics_command_pools[thread_id];
    } break;
    case TGVK_COMMAND_POOL_TYPE_PRESENT:
    {
        command_buffer_allocate_info.commandPool = present_command_pool;
    } break;
    default: TG_INVALID_CODEPATH(); break;
    }

    TGVK_CALL(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &command_buffer.command_buffer));

    return command_buffer;
}

void tgvk_command_buffers_create(tgvk_command_pool_type type, VkCommandBufferLevel level, u32 count, tgvk_command_buffer* p_command_buffers)
{
    for (u32 i = 0; i < count; i++)
    {
        p_command_buffers[i] = tgvk_command_buffer_create(type, level);
    }
}

void tgvk_command_buffer_destroy(tgvk_command_buffer* p_command_buffer)
{
    VkCommandPool command_pool = VK_NULL_HANDLE;

    switch (p_command_buffer->command_pool_type)
    {
    case TGVK_COMMAND_POOL_TYPE_COMPUTE:
    {
        const u32 thread_id = tg_platform_get_thread_id();
        command_pool = p_compute_command_pools[thread_id];
    } break;
    case TGVK_COMMAND_POOL_TYPE_GRAPHICS:
    {
        const u32 thread_id = tg_platform_get_thread_id();
        command_pool = p_graphics_command_pools[thread_id];
    } break;
    case TGVK_COMMAND_POOL_TYPE_PRESENT:
    {
        command_pool = present_command_pool;
    } break;
    default: TG_INVALID_CODEPATH(); break;
    }

    vkFreeCommandBuffers(device, command_pool, 1, &p_command_buffer->command_buffer);
}

void tgvk_command_buffers_destroy(u32 count, tgvk_command_buffer* p_command_buffers)
{
    for (u32 i = 0; i < count; i++)
    {
        tgvk_command_buffer_destroy(&p_command_buffers[i]);
    }
}

void tgvk_command_buffer_begin(tgvk_command_buffer* p_command_buffer, VkCommandBufferUsageFlags flags)
{
    VkCommandBufferBeginInfo command_buffer_begin_info = { 0 };
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = TG_NULL;
    command_buffer_begin_info.flags = flags;
    command_buffer_begin_info.pInheritanceInfo = TG_NULL;

    TGVK_CALL(vkBeginCommandBuffer(p_command_buffer->command_buffer, &command_buffer_begin_info));
}

void tgvk_command_buffer_begin_secondary(tgvk_command_buffer* p_command_buffer, VkCommandBufferUsageFlags flags, VkRenderPass render_pass, VkFramebuffer framebuffer)
{
    VkCommandBufferInheritanceInfo command_buffer_inheritance_info = { 0 };
    command_buffer_inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    command_buffer_inheritance_info.pNext = TG_NULL;
    command_buffer_inheritance_info.renderPass = render_pass;
    command_buffer_inheritance_info.subpass = 0;
    command_buffer_inheritance_info.framebuffer = framebuffer;
    command_buffer_inheritance_info.occlusionQueryEnable = TG_FALSE;
    command_buffer_inheritance_info.queryFlags = 0;
    command_buffer_inheritance_info.pipelineStatistics = 0;

    VkCommandBufferBeginInfo command_buffer_begin_info = { 0 };
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = TG_NULL;
    command_buffer_begin_info.flags = flags;
    command_buffer_begin_info.pInheritanceInfo = &command_buffer_inheritance_info;

    TGVK_CALL(vkBeginCommandBuffer(p_command_buffer->command_buffer, &command_buffer_begin_info));
}

void tgvk_command_buffer_cmd_begin_render_pass(tgvk_command_buffer* p_command_buffer, VkRenderPass render_pass, tgvk_framebuffer* p_framebuffer, VkSubpassContents subpass_contents)
{
    VkRenderPassBeginInfo render_pass_begin_info = { 0 };
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = TG_NULL;
    render_pass_begin_info.renderPass = render_pass;
    render_pass_begin_info.framebuffer = p_framebuffer->framebuffer;
    render_pass_begin_info.renderArea.offset = (VkOffset2D) { 0, 0 };
    render_pass_begin_info.renderArea.extent = (VkExtent2D) { p_framebuffer->width, p_framebuffer->height };
    render_pass_begin_info.clearValueCount = 0;
    render_pass_begin_info.pClearValues = TG_NULL;

    vkCmdBeginRenderPass(p_command_buffer->command_buffer, &render_pass_begin_info, subpass_contents);
}

void tgvk_command_buffer_cmd_blit_image(tgvk_command_buffer* p_command_buffer, tgvk_image* p_source, tgvk_image* p_destination, const VkImageBlit* p_region)
{
    vkCmdBlitImage(p_command_buffer->command_buffer, p_source->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, p_destination->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, p_region, VK_FILTER_NEAREST);
}

void tgvk_command_buffer_cmd_clear_color_image(tgvk_command_buffer* p_command_buffer, tgvk_image* p_color_image)
{
    VkImageSubresourceRange image_subresource_range = { 0 };
    image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_subresource_range.baseMipLevel = 0;
    image_subresource_range.levelCount = 1;
    image_subresource_range.baseArrayLayer = 0;
    image_subresource_range.layerCount = 1;

    VkClearColorValue clear_color_value = { 0 };
    clear_color_value.float32[0] = 0.0f;
    clear_color_value.float32[1] = 0.0f;
    clear_color_value.float32[2] = 0.0f;
    clear_color_value.float32[3] = 0.0f;

    vkCmdClearColorImage(p_command_buffer->command_buffer, p_color_image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color_value, 1, &image_subresource_range);
}

void tgvk_command_buffer_cmd_clear_color_image_3d(tgvk_command_buffer* p_command_buffer, tgvk_image_3d* p_color_image_3d)
{
    VkImageSubresourceRange image_subresource_range = { 0 };
    image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_subresource_range.baseMipLevel = 0;
    image_subresource_range.levelCount = 1;
    image_subresource_range.baseArrayLayer = 0;
    image_subresource_range.layerCount = 1;

    VkClearColorValue clear_color_value = { 0 };
    clear_color_value.float32[0] = 0.0f;
    clear_color_value.float32[1] = 0.0f;
    clear_color_value.float32[2] = 0.0f;
    clear_color_value.float32[3] = 0.0f;

    vkCmdClearColorImage(p_command_buffer->command_buffer, p_color_image_3d->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color_value, 1, &image_subresource_range);
}

void tgvk_command_buffer_cmd_clear_depth_image(tgvk_command_buffer* p_command_buffer, tgvk_image* p_depth_image)
{
    VkImageSubresourceRange image_subresource_range = { 0 };
    image_subresource_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    image_subresource_range.baseMipLevel = 0;
    image_subresource_range.levelCount = 1;
    image_subresource_range.baseArrayLayer = 0;
    image_subresource_range.layerCount = 1;

    VkClearDepthStencilValue clear_depth_stencil_value = { 0 };
    clear_depth_stencil_value.depth = 1.0f;
    clear_depth_stencil_value.stencil = 0;

    vkCmdClearDepthStencilImage(p_command_buffer->command_buffer, p_depth_image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_depth_stencil_value, 1, &image_subresource_range);
}

void tgvk_command_buffer_cmd_copy_buffer(tgvk_command_buffer* p_command_buffer, VkDeviceSize size, tgvk_buffer* p_src, tgvk_buffer* p_dst)
{
    VkBufferCopy buffer_copy = { 0 };
    buffer_copy.srcOffset = 0;
    buffer_copy.dstOffset = 0;
    buffer_copy.size = size;

    vkCmdCopyBuffer(p_command_buffer->command_buffer, p_src->buffer, p_dst->buffer, 1, &buffer_copy);
}

void tgvk_command_buffer_cmd_copy_buffer_to_color_image(tgvk_command_buffer* p_command_buffer, VkBuffer source, tgvk_image* p_destination)
{
    VkBufferImageCopy buffer_image_copy = { 0 };
    buffer_image_copy.bufferOffset = 0;
    buffer_image_copy.bufferRowLength = 0;
    buffer_image_copy.bufferImageHeight = 0;
    buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    buffer_image_copy.imageSubresource.mipLevel = 0;
    buffer_image_copy.imageSubresource.baseArrayLayer = 0;
    buffer_image_copy.imageSubresource.layerCount = 1;
    buffer_image_copy.imageOffset.x = 0;
    buffer_image_copy.imageOffset.y = 0;
    buffer_image_copy.imageOffset.z = 0;
    buffer_image_copy.imageExtent.width = p_destination->width;
    buffer_image_copy.imageExtent.height = p_destination->height;
    buffer_image_copy.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(p_command_buffer->command_buffer, source, p_destination->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
}

void tgvk_command_buffer_cmd_copy_buffer_to_color_image_3d(tgvk_command_buffer* p_command_buffer, VkBuffer source, tgvk_image_3d* p_destination)
{
    VkBufferImageCopy buffer_image_copy = { 0 };
    buffer_image_copy.bufferOffset = 0;
    buffer_image_copy.bufferRowLength = 0;
    buffer_image_copy.bufferImageHeight = 0;
    buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    buffer_image_copy.imageSubresource.mipLevel = 0;
    buffer_image_copy.imageSubresource.baseArrayLayer = 0;
    buffer_image_copy.imageSubresource.layerCount = 1;
    buffer_image_copy.imageOffset.x = 0;
    buffer_image_copy.imageOffset.y = 0;
    buffer_image_copy.imageOffset.z = 0;
    buffer_image_copy.imageExtent.width = p_destination->width;
    buffer_image_copy.imageExtent.height = p_destination->height;
    buffer_image_copy.imageExtent.depth = p_destination->depth;

    vkCmdCopyBufferToImage(p_command_buffer->command_buffer, source, p_destination->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
}

void tgvk_command_buffer_cmd_copy_buffer_to_cube_map(tgvk_command_buffer* p_command_buffer, VkBuffer source, tgvk_cube_map* p_destination)
{
    VkBufferImageCopy buffer_image_copy = { 0 };
    buffer_image_copy.bufferOffset = 0;
    buffer_image_copy.bufferRowLength = 0;
    buffer_image_copy.bufferImageHeight = 0;
    buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    buffer_image_copy.imageSubresource.mipLevel = 0;
    buffer_image_copy.imageSubresource.baseArrayLayer = 0;
    buffer_image_copy.imageSubresource.layerCount = 6;
    buffer_image_copy.imageOffset.x = 0;
    buffer_image_copy.imageOffset.y = 0;
    buffer_image_copy.imageOffset.z = 0;
    buffer_image_copy.imageExtent.width = p_destination->dimension;
    buffer_image_copy.imageExtent.height = p_destination->dimension;
    buffer_image_copy.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(p_command_buffer->command_buffer, source, p_destination->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
}

void tgvk_command_buffer_cmd_copy_buffer_to_depth_image(tgvk_command_buffer* p_command_buffer, VkBuffer source, tgvk_image* p_destination)
{
    VkBufferImageCopy buffer_image_copy = { 0 };
    buffer_image_copy.bufferOffset = 0;
    buffer_image_copy.bufferRowLength = 0;
    buffer_image_copy.bufferImageHeight = 0;
    buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    buffer_image_copy.imageSubresource.mipLevel = 0;
    buffer_image_copy.imageSubresource.baseArrayLayer = 0;
    buffer_image_copy.imageSubresource.layerCount = 1;
    buffer_image_copy.imageOffset.x = 0;
    buffer_image_copy.imageOffset.y = 0;
    buffer_image_copy.imageOffset.z = 0;
    buffer_image_copy.imageExtent.width = p_destination->width;
    buffer_image_copy.imageExtent.height = p_destination->height;
    buffer_image_copy.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(p_command_buffer->command_buffer, source, p_destination->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
}

void tgvk_command_buffer_cmd_copy_color_image(tgvk_command_buffer* p_command_buffer, tgvk_image* p_source, tgvk_image* p_destination)
{
    VkImageCopy image_copy = { 0 };
    image_copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_copy.srcSubresource.mipLevel = 0;
    image_copy.srcSubresource.baseArrayLayer = 0;
    image_copy.srcSubresource.layerCount = 1;
    image_copy.srcOffset.x = 0;
    image_copy.srcOffset.y = 0;
    image_copy.srcOffset.z = 0;
    image_copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_copy.dstSubresource.mipLevel = 0;
    image_copy.dstSubresource.baseArrayLayer = 0;
    image_copy.dstSubresource.layerCount = 1;
    image_copy.dstOffset.x = 0;
    image_copy.dstOffset.y = 0;
    image_copy.dstOffset.z = 0;
    image_copy.extent.width = p_source->width;
    image_copy.extent.height = p_source->height;
    image_copy.extent.depth = 1;

    vkCmdCopyImage(p_command_buffer->command_buffer, p_source->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, p_destination->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);
}

void tgvk_command_buffer_cmd_copy_color_image_to_buffer(tgvk_command_buffer* p_command_buffer, tgvk_image* p_source, VkBuffer destination)
{
    VkBufferImageCopy buffer_image_copy = { 0 };
    buffer_image_copy.bufferOffset = 0;
    buffer_image_copy.bufferRowLength = 0;
    buffer_image_copy.bufferImageHeight = 0;
    buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    buffer_image_copy.imageSubresource.mipLevel = 0;
    buffer_image_copy.imageSubresource.baseArrayLayer = 0;
    buffer_image_copy.imageSubresource.layerCount = 1;
    buffer_image_copy.imageOffset.x = 0;
    buffer_image_copy.imageOffset.y = 0;
    buffer_image_copy.imageOffset.z = 0;
    buffer_image_copy.imageExtent.width = p_source->width;
    buffer_image_copy.imageExtent.height = p_source->height;
    buffer_image_copy.imageExtent.depth = 1;

    vkCmdCopyImageToBuffer(p_command_buffer->command_buffer, p_source->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination, 1, &buffer_image_copy);
}

void tgvk_command_buffer_cmd_copy_color_image_3d_to_buffer(tgvk_command_buffer* p_command_buffer, tgvk_image_3d* p_source, VkBuffer destination)
{
    VkBufferImageCopy buffer_image_copy = { 0 };
    buffer_image_copy.bufferOffset = 0;
    buffer_image_copy.bufferRowLength = 0;
    buffer_image_copy.bufferImageHeight = 0;
    buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    buffer_image_copy.imageSubresource.mipLevel = 0;
    buffer_image_copy.imageSubresource.baseArrayLayer = 0;
    buffer_image_copy.imageSubresource.layerCount = 1;
    buffer_image_copy.imageOffset.x = 0;
    buffer_image_copy.imageOffset.y = 0;
    buffer_image_copy.imageOffset.z = 0;
    buffer_image_copy.imageExtent.width = p_source->width;
    buffer_image_copy.imageExtent.height = p_source->height;
    buffer_image_copy.imageExtent.depth = p_source->depth;

    vkCmdCopyImageToBuffer(p_command_buffer->command_buffer, p_source->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination, 1, &buffer_image_copy);
}

void tgvk_command_buffer_cmd_copy_depth_image_pixel_to_buffer(tgvk_command_buffer* p_command_buffer, tgvk_image* p_source, VkBuffer destination, u32 x, u32 y)
{
    VkBufferImageCopy buffer_image_copy = { 0 };
    buffer_image_copy.bufferOffset = 0;
    buffer_image_copy.bufferRowLength = 0;
    buffer_image_copy.bufferImageHeight = 0;
    buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    buffer_image_copy.imageSubresource.mipLevel = 0;
    buffer_image_copy.imageSubresource.baseArrayLayer = 0;
    buffer_image_copy.imageSubresource.layerCount = 1;
    buffer_image_copy.imageOffset.x = x;
    buffer_image_copy.imageOffset.y = y;
    buffer_image_copy.imageOffset.z = 0;
    buffer_image_copy.imageExtent.width = 1;
    buffer_image_copy.imageExtent.height = 1;
    buffer_image_copy.imageExtent.depth = 1;

    vkCmdCopyImageToBuffer(p_command_buffer->command_buffer, p_source->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination, 1, &buffer_image_copy);
}

void tgvk_command_buffer_cmd_transition_color_image_layout(tgvk_command_buffer* p_command_buffer, tgvk_image* p_image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits)
{
    VkImageMemoryBarrier image_memory_barrier = { 0 };
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = TG_NULL;
    image_memory_barrier.srcAccessMask = src_access_mask;
    image_memory_barrier.dstAccessMask = dst_access_mask;
    image_memory_barrier.oldLayout = old_layout;
    image_memory_barrier.newLayout = new_layout;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.image = p_image->image;
    image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(p_command_buffer->command_buffer, src_stage_bits, dst_stage_bits, 0, 0, TG_NULL, 0, TG_NULL, 1, &image_memory_barrier);
}

void tgvk_command_buffer_cmd_transition_color_image_3d_layout(tgvk_command_buffer* p_command_buffer, tgvk_image_3d* p_image_3d, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits)
{
    VkImageMemoryBarrier image_memory_barrier = { 0 };
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = TG_NULL;
    image_memory_barrier.srcAccessMask = src_access_mask;
    image_memory_barrier.dstAccessMask = dst_access_mask;
    image_memory_barrier.oldLayout = old_layout;
    image_memory_barrier.newLayout = new_layout;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.image = p_image_3d->image;
    image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(p_command_buffer->command_buffer, src_stage_bits, dst_stage_bits, 0, 0, TG_NULL, 0, TG_NULL, 1, &image_memory_barrier);
}

void tgvk_command_buffer_cmd_transition_cube_map_layout(tgvk_command_buffer* p_command_buffer, tgvk_cube_map* p_cube_map, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits)
{
    VkImageMemoryBarrier image_memory_barrier = { 0 };
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = TG_NULL;
    image_memory_barrier.srcAccessMask = src_access_mask;
    image_memory_barrier.dstAccessMask = dst_access_mask;
    image_memory_barrier.oldLayout = old_layout;
    image_memory_barrier.newLayout = new_layout;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.image = p_cube_map->image;
    image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 6;

    vkCmdPipelineBarrier(p_command_buffer->command_buffer, src_stage_bits, dst_stage_bits, 0, 0, TG_NULL, 0, TG_NULL, 1, &image_memory_barrier);
}

void tgvk_command_buffer_cmd_transition_depth_image_layout(tgvk_command_buffer* p_command_buffer, tgvk_image* p_image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits)
{
    VkImageMemoryBarrier image_memory_barrier = { 0 };
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = TG_NULL;
    image_memory_barrier.srcAccessMask = src_access_mask;
    image_memory_barrier.dstAccessMask = dst_access_mask;
    image_memory_barrier.oldLayout = old_layout;
    image_memory_barrier.newLayout = new_layout;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.image = p_image->image;
    image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(p_command_buffer->command_buffer, src_stage_bits, dst_stage_bits, 0, 0, TG_NULL, 0, TG_NULL, 1, &image_memory_barrier);
}

void tgvk_command_buffer_end_and_submit(tgvk_command_buffer* p_command_buffer)
{
    TGVK_CALL(vkEndCommandBuffer(p_command_buffer->command_buffer));

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = TG_NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = TG_NULL;
    submit_info.pWaitDstStageMask = TG_NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &p_command_buffer->command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = TG_NULL;

    tgvk_queue* p_queue = TG_NULL;
    TGVK_QUEUE_TAKE(p_command_buffer->command_pool_type, p_queue);

    TGVK_CALL(vkQueueSubmit(p_queue->queue, 1, &submit_info, p_queue->fence));
    TGVK_CALL(vkWaitForFences(device, 1, &p_queue->fence, VK_FALSE, TG_U64_MAX));
    TGVK_CALL(vkResetFences(device, 1, &p_queue->fence));

    TGVK_QUEUE_RELEASE(p_queue);
}



tgvk_cube_map tgvk_cube_map_create(u32 dimension, VkFormat format, const tg_sampler_create_info* p_sampler_create_info)
{
    tgvk_cube_map cube_map = { 0 };
    cube_map.dimension = dimension;
    cube_map.format = format;

    VkImageCreateInfo image_create_info = { 0 };
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = TG_NULL;
    image_create_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = cube_map.format;
    image_create_info.extent.width = cube_map.dimension;
    image_create_info.extent.height = cube_map.dimension;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 6;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = TG_NULL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    TGVK_CALL(vkCreateImage(device, &image_create_info, TG_NULL, &cube_map.image));

    VkMemoryRequirements memory_requirements = { 0 };
    vkGetImageMemoryRequirements(device, cube_map.image, &memory_requirements);
    cube_map.memory = tgvk_memory_allocator_alloc(memory_requirements.alignment, memory_requirements.size, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    TGVK_CALL(vkBindImageMemory(device, cube_map.image, cube_map.memory.device_memory, cube_map.memory.offset));

    cube_map.image_view = tg__image_view_create(cube_map.image, VK_IMAGE_VIEW_TYPE_CUBE, cube_map.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    if (p_sampler_create_info)
    {
        cube_map.sampler = tg__sampler_create_custom(
            1,
            p_sampler_create_info->min_filter,
            p_sampler_create_info->mag_filter,
            p_sampler_create_info->address_mode_u,
            p_sampler_create_info->address_mode_v,
            p_sampler_create_info->address_mode_w
        );
    }
    else
    {
        cube_map.sampler = tg__sampler_create(1);
    }

    return cube_map;
}

void tgvk_cube_map_destroy(tgvk_cube_map* p_cube_map)
{
    vkDestroySampler(device, p_cube_map->sampler, TG_NULL);
    vkDestroyImageView(device, p_cube_map->image_view, TG_NULL);
    tgvk_memory_allocator_free(&p_cube_map->memory);
    vkDestroyImage(device, p_cube_map->image, TG_NULL);
}


tgvk_image tgvk_depth_image_create(u32 width, u32 height, VkFormat format, const tg_sampler_create_info* p_sampler_create_info)
{
    tgvk_image depth_image = { 0 };
    depth_image.width = width;
    depth_image.height = height;
    depth_image.format = format;

    VkImageCreateInfo image_create_info = { 0 };
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = TG_NULL;
    image_create_info.flags = 0;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = depth_image.format;
    image_create_info.extent.width = depth_image.width;
    image_create_info.extent.height = depth_image.height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = TG_NULL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    TGVK_CALL(vkCreateImage(device, &image_create_info, TG_NULL, &depth_image.image));

    VkMemoryRequirements memory_requirements = { 0 };
    vkGetImageMemoryRequirements(device, depth_image.image, &memory_requirements);
    depth_image.memory = tgvk_memory_allocator_alloc(memory_requirements.alignment, memory_requirements.size, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    TGVK_CALL(vkBindImageMemory(device, depth_image.image, depth_image.memory.device_memory, depth_image.memory.offset));

    depth_image.image_view = tg__image_view_create(depth_image.image, VK_IMAGE_VIEW_TYPE_2D, depth_image.format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    if (p_sampler_create_info)
    {
        depth_image.sampler = tg__sampler_create_custom(
            1,
            p_sampler_create_info->min_filter,
            p_sampler_create_info->mag_filter,
            p_sampler_create_info->address_mode_u,
            p_sampler_create_info->address_mode_v,
            p_sampler_create_info->address_mode_w
        );
    }
    else
    {
        depth_image.sampler = tg__sampler_create(1);
    }

    return depth_image;
}



tgvk_descriptor_set tgvk_descriptor_set_create(const tgvk_pipeline* p_pipeline)
{
    tgvk_descriptor_set descriptor_set = { 0 };

    VkDescriptorPoolSize p_descriptor_pool_sizes[TG_MAX_SHADER_GLOBAL_RESOURCES] = { 0 };
    for (u32 i = 0; i < p_pipeline->layout.global_resource_count; i++)
    {
        p_descriptor_pool_sizes[i].type = p_pipeline->layout.p_descriptor_set_layout_bindings[i].descriptorType;
        p_descriptor_pool_sizes[i].descriptorCount = p_pipeline->layout.p_descriptor_set_layout_bindings[i].descriptorCount;
    }

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = { 0 };
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext = TG_NULL;
    descriptor_pool_create_info.flags = 0;
    descriptor_pool_create_info.maxSets = 1;
    descriptor_pool_create_info.poolSizeCount = p_pipeline->layout.global_resource_count;
    descriptor_pool_create_info.pPoolSizes = p_descriptor_pool_sizes;

    TGVK_CALL(vkCreateDescriptorPool(device, &descriptor_pool_create_info, TG_NULL, &descriptor_set.descriptor_pool));

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = { 0 };
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = TG_NULL;
    descriptor_set_allocate_info.descriptorPool = descriptor_set.descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &p_pipeline->layout.descriptor_set_layout;

    TGVK_CALL(vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &descriptor_set.descriptor_set));

    return descriptor_set;
}

void tgvk_descriptor_set_destroy(tgvk_descriptor_set* p_descriptor_set)
{
    vkDestroyDescriptorPool(device, p_descriptor_set->descriptor_pool, TG_NULL);
}



void tgvk_descriptor_set_update(VkDescriptorSet descriptor_set, tg_handle handle, u32 dst_binding)
{
    const tg_structure_type type = *(tg_structure_type*)handle;

    switch (type) // TODO: arrays not supported: see e.g. tgvk_descriptor_set_update_storage_buffer_array
    {
    case TG_STRUCTURE_TYPE_COLOR_IMAGE:
    {
        tg_color_image_h h_color_image = (tg_color_image_h)handle;
        tgvk_descriptor_set_update_image(descriptor_set, &h_color_image->image, dst_binding);
    } break;
    case TG_STRUCTURE_TYPE_COLOR_IMAGE_3D:
    {
        tg_color_image_3d_h h_color_image_3d = (tg_color_image_3d_h)handle;
        tgvk_descriptor_set_update_image_3d(descriptor_set, &h_color_image_3d->image_3d, dst_binding);
    } break;
    case TG_STRUCTURE_TYPE_CUBE_MAP:
    {
        tg_cube_map_h h_cube_map = (tg_cube_map_h)handle;
        tgvk_descriptor_set_update_cube_map(descriptor_set, &h_cube_map->cube_map, dst_binding);
    } break;
    case TG_STRUCTURE_TYPE_DEPTH_IMAGE:
    {
        tg_depth_image* h_depth_image = (tg_depth_image_h)handle;
        tgvk_descriptor_set_update_image(descriptor_set, &h_depth_image->image, dst_binding);
    } break;
    case TG_STRUCTURE_TYPE_RENDER_TARGET:
    {
        tg_render_target_h h_render_target = (tg_render_target_h)handle;
        tgvk_descriptor_set_update_render_target(descriptor_set, h_render_target, dst_binding);
    } break;
    case TG_STRUCTURE_TYPE_STORAGE_BUFFER:
    {
        tg_storage_buffer_h h_storage_buffer = (tg_storage_buffer_h)handle;
        tgvk_descriptor_set_update_storage_buffer(descriptor_set, h_storage_buffer->buffer.buffer, dst_binding);
    } break;
    case TG_STRUCTURE_TYPE_UNIFORM_BUFFER:
    {
        tg_uniform_buffer_h h_uniform_buffer = (tg_uniform_buffer_h)handle;
        tgvk_descriptor_set_update_uniform_buffer(descriptor_set, h_uniform_buffer->buffer.buffer, dst_binding);
    } break;
    default: TG_INVALID_CODEPATH(); break;
    }
}

void tgvk_descriptor_set_update_image(VkDescriptorSet descriptor_set, tgvk_image* p_image, u32 dst_binding)
{
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    descriptor_image_info.sampler = p_image->sampler;
    descriptor_image_info.imageView = p_image->image_view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write_descriptor_set = { 0 };
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = TG_NULL;
    write_descriptor_set.dstSet = descriptor_set;
    write_descriptor_set.dstBinding = dst_binding;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_set.pImageInfo = &descriptor_image_info;
    write_descriptor_set.pBufferInfo = TG_NULL;
    write_descriptor_set.pTexelBufferView = TG_NULL;

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
}

void tgvk_descriptor_set_update_image_3d(VkDescriptorSet descriptor_set, tgvk_image_3d* p_image_3d, u32 dst_binding)
{
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    descriptor_image_info.sampler = p_image_3d->sampler;
    descriptor_image_info.imageView = p_image_3d->image_view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet write_descriptor_set = { 0 };
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = TG_NULL;
    write_descriptor_set.dstSet = descriptor_set;
    write_descriptor_set.dstBinding = dst_binding;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write_descriptor_set.pImageInfo = &descriptor_image_info;
    write_descriptor_set.pBufferInfo = TG_NULL;
    write_descriptor_set.pTexelBufferView = TG_NULL;

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
}

void tgvk_descriptor_set_update_cube_map(VkDescriptorSet descriptor_set, tgvk_cube_map* p_cube_map, u32 dst_binding)
{
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    descriptor_image_info.sampler = p_cube_map->sampler;
    descriptor_image_info.imageView = p_cube_map->image_view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write_descriptor_set = { 0 };
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = TG_NULL;
    write_descriptor_set.dstSet = descriptor_set;
    write_descriptor_set.dstBinding = dst_binding;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_set.pImageInfo = &descriptor_image_info;
    write_descriptor_set.pBufferInfo = TG_NULL;
    write_descriptor_set.pTexelBufferView = TG_NULL;

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
}

void tgvk_descriptor_set_update_image_array(VkDescriptorSet descriptor_set, tgvk_image* p_image, u32 dst_binding, u32 array_index)
{
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    descriptor_image_info.sampler = p_image->sampler;
    descriptor_image_info.imageView = p_image->image_view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write_descriptor_set = { 0 };
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = TG_NULL;
    write_descriptor_set.dstSet = descriptor_set;
    write_descriptor_set.dstBinding = dst_binding;
    write_descriptor_set.dstArrayElement = array_index;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_set.pImageInfo = &descriptor_image_info;
    write_descriptor_set.pBufferInfo = TG_NULL;
    write_descriptor_set.pTexelBufferView = TG_NULL;

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
}

void tgvk_descriptor_set_update_render_target(VkDescriptorSet descriptor_set, tg_render_target* p_render_target, u32 dst_binding)
{
    tgvk_descriptor_set_update_image(descriptor_set, &p_render_target->color_attachment_copy, dst_binding);
    // TODO: select color or depth somewhere
}

void tgvk_descriptor_set_update_storage_buffer(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding)
{
    VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
    descriptor_buffer_info.buffer = buffer;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet write_descriptor_set = { 0 };
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = TG_NULL;
    write_descriptor_set.dstSet = descriptor_set;
    write_descriptor_set.dstBinding = dst_binding;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write_descriptor_set.pImageInfo = TG_NULL;
    write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
    write_descriptor_set.pTexelBufferView = TG_NULL;

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
}

void tgvk_descriptor_set_update_storage_buffer_array(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding, u32 array_index)
{
    VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
    descriptor_buffer_info.buffer = buffer;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet write_descriptor_set = { 0 };
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = TG_NULL;
    write_descriptor_set.dstSet = descriptor_set;
    write_descriptor_set.dstBinding = dst_binding;
    write_descriptor_set.dstArrayElement = array_index;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write_descriptor_set.pImageInfo = TG_NULL;
    write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
    write_descriptor_set.pTexelBufferView = TG_NULL;

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
}

void tgvk_descriptor_set_update_uniform_buffer(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding)
{
    VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
    descriptor_buffer_info.buffer = buffer;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet write_descriptor_set = { 0 };
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = TG_NULL;
    write_descriptor_set.dstSet = descriptor_set;
    write_descriptor_set.dstBinding = dst_binding;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set.pImageInfo = TG_NULL;
    write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
    write_descriptor_set.pTexelBufferView = TG_NULL;

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
}

void tgvk_descriptor_set_update_uniform_buffer_array(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding, u32 array_index)
{
    VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
    descriptor_buffer_info.buffer = buffer;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet write_descriptor_set = { 0 };
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = TG_NULL;
    write_descriptor_set.dstSet = descriptor_set;
    write_descriptor_set.dstBinding = dst_binding;
    write_descriptor_set.dstArrayElement = array_index;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set.pImageInfo = TG_NULL;
    write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
    write_descriptor_set.pTexelBufferView = TG_NULL;

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
}

void tgvk_descriptor_sets_update(u32 write_descriptor_set_count, const VkWriteDescriptorSet* p_write_descriptor_sets)
{
    vkUpdateDescriptorSets(device, write_descriptor_set_count, p_write_descriptor_sets, 0, TG_NULL);
}



VkFence tgvk_fence_create(VkFenceCreateFlags fence_create_flags)
{
    VkFence fence = VK_NULL_HANDLE;

    VkFenceCreateInfo fence_create_info = { 0 };
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = TG_NULL;
    fence_create_info.flags = fence_create_flags;

    TGVK_CALL(vkCreateFence(device, &fence_create_info, TG_NULL, &fence));

    return fence;
}

void tgvk_fence_destroy(VkFence fence)
{
    vkDestroyFence(device, fence, TG_NULL);
}

void tgvk_fence_reset(VkFence fence)
{
    TGVK_CALL(vkResetFences(device, 1, &fence));
}

void tgvk_fence_wait(VkFence fence)
{
    TGVK_CALL(vkWaitForFences(device, 1, &fence, VK_FALSE, UINT64_MAX));
}



tgvk_framebuffer tgvk_framebuffer_create(VkRenderPass render_pass, u32 attachment_count, const VkImageView* p_attachments, u32 width, u32 height)
{
    tgvk_framebuffer framebuffer = { 0 };

    framebuffer.width = width;
    framebuffer.height = height;

    VkFramebufferCreateInfo framebuffer_create_info = { 0 };
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.pNext = TG_NULL;
    framebuffer_create_info.flags = 0;
    framebuffer_create_info.renderPass = render_pass;
    framebuffer_create_info.attachmentCount = attachment_count;
    framebuffer_create_info.pAttachments = p_attachments;
    framebuffer_create_info.width = width;
    framebuffer_create_info.height = height;
    framebuffer_create_info.layers = 1;

    TGVK_CALL(vkCreateFramebuffer(device, &framebuffer_create_info, TG_NULL, &framebuffer.framebuffer));

    return framebuffer;
}

void tgvk_framebuffer_destroy(tgvk_framebuffer* p_framebuffer)
{
    vkDestroyFramebuffer(device, p_framebuffer->framebuffer, TG_NULL);
}

void tgvk_framebuffers_destroy(u32 count, tgvk_framebuffer* p_framebuffers)
{
    for (u32 i = 0; i < count; i++)
    {
        tgvk_framebuffer_destroy(&p_framebuffers[i]);
    }
}



void tgvk_image_destroy(tgvk_image* p_image)
{
    vkDestroySampler(device, p_image->sampler, TG_NULL);
    vkDestroyImageView(device, p_image->image_view, TG_NULL);
    tgvk_memory_allocator_free(&p_image->memory);
    vkDestroyImage(device, p_image->image, TG_NULL);
}



VkPhysicalDeviceProperties tgvk_physical_device_get_properties(void)
{
    VkPhysicalDeviceProperties physical_device_properties = { 0 };
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
    return physical_device_properties;
}



void tgvk_pipeline_shader_stage_create_infos_create(const tgvk_shader* p_vertex_shader, const tgvk_shader* p_fragment_shader, VkPipelineShaderStageCreateInfo* p_pipeline_shader_stage_create_infos)
{
    p_pipeline_shader_stage_create_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    p_pipeline_shader_stage_create_infos[0].pNext = TG_NULL;
    p_pipeline_shader_stage_create_infos[0].flags = 0;
    p_pipeline_shader_stage_create_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    p_pipeline_shader_stage_create_infos[0].module = p_vertex_shader->shader_module;
    p_pipeline_shader_stage_create_infos[0].pName = p_vertex_shader->spirv_layout.p_entry_point_name;
    p_pipeline_shader_stage_create_infos[0].pSpecializationInfo = TG_NULL;

    p_pipeline_shader_stage_create_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    p_pipeline_shader_stage_create_infos[1].pNext = TG_NULL;
    p_pipeline_shader_stage_create_infos[1].flags = 0;
    p_pipeline_shader_stage_create_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    p_pipeline_shader_stage_create_infos[1].module = p_fragment_shader->shader_module;
    p_pipeline_shader_stage_create_infos[1].pName = p_fragment_shader->spirv_layout.p_entry_point_name;
    p_pipeline_shader_stage_create_infos[1].pSpecializationInfo = TG_NULL;
}

VkPipelineInputAssemblyStateCreateInfo tgvk_pipeline_input_assembly_state_create_info_create(void)
{
    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info = { 0 };
    pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipeline_input_assembly_state_create_info.pNext = TG_NULL;
    pipeline_input_assembly_state_create_info.flags = 0;
    pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    return pipeline_input_assembly_state_create_info;
}

VkViewport tgvk_viewport_create(f32 width, f32 height)
{
    VkViewport viewport = { 0 };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    return viewport;
}

VkPipelineViewportStateCreateInfo tgvk_pipeline_viewport_state_create_info_create(const VkViewport* p_viewport, const VkRect2D* p_scissors)
{
    VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = { 0 };
    pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipeline_viewport_state_create_info.pNext = TG_NULL;
    pipeline_viewport_state_create_info.flags = 0;
    pipeline_viewport_state_create_info.viewportCount = 1;
    pipeline_viewport_state_create_info.pViewports = p_viewport;
    pipeline_viewport_state_create_info.scissorCount = 1;
    pipeline_viewport_state_create_info.pScissors = p_scissors;

    return pipeline_viewport_state_create_info;
}

VkPipelineRasterizationStateCreateInfo tgvk_pipeline_rasterization_state_create_info_create(VkPipelineRasterizationStateCreateInfo* p_next, VkBool32 depth_clamp_enable, VkPolygonMode polygon_mode, VkCullModeFlags cull_mode)
{
    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info = { 0 };
    pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipeline_rasterization_state_create_info.pNext = p_next;
    pipeline_rasterization_state_create_info.flags = 0;
    pipeline_rasterization_state_create_info.depthClampEnable = depth_clamp_enable;
    pipeline_rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    pipeline_rasterization_state_create_info.polygonMode = polygon_mode;
    pipeline_rasterization_state_create_info.cullMode = cull_mode;
    pipeline_rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipeline_rasterization_state_create_info.depthBiasEnable = VK_FALSE;
    pipeline_rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
    pipeline_rasterization_state_create_info.depthBiasClamp = 0.0f;
    pipeline_rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;
    pipeline_rasterization_state_create_info.lineWidth = 1.0f;

    return pipeline_rasterization_state_create_info;
}

VkPipelineMultisampleStateCreateInfo tgvk_pipeline_multisample_state_create_info_create(u32 rasterization_samples, VkBool32 sample_shading_enable, f32 min_sample_shading)
{
    VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = { 0 };
    pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipeline_multisample_state_create_info.pNext = TG_NULL;
    pipeline_multisample_state_create_info.flags = 0;
    pipeline_multisample_state_create_info.rasterizationSamples = rasterization_samples;
    pipeline_multisample_state_create_info.sampleShadingEnable = sample_shading_enable;
    pipeline_multisample_state_create_info.minSampleShading = min_sample_shading;
    pipeline_multisample_state_create_info.pSampleMask = TG_NULL;
    pipeline_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    pipeline_multisample_state_create_info.alphaToOneEnable = VK_FALSE;

    return pipeline_multisample_state_create_info;
}

VkPipelineDepthStencilStateCreateInfo tgvk_pipeline_depth_stencil_state_create_info_create(VkBool32 depth_test_enable, VkBool32 depth_write_enable)
{
    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = { 0 };
    pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipeline_depth_stencil_state_create_info.pNext = TG_NULL;
    pipeline_depth_stencil_state_create_info.flags = 0;
    pipeline_depth_stencil_state_create_info.depthTestEnable = depth_test_enable;
    pipeline_depth_stencil_state_create_info.depthWriteEnable = depth_write_enable;
    pipeline_depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
    pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    pipeline_depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    pipeline_depth_stencil_state_create_info.front.failOp = VK_STENCIL_OP_KEEP;
    pipeline_depth_stencil_state_create_info.front.passOp = VK_STENCIL_OP_KEEP;
    pipeline_depth_stencil_state_create_info.front.depthFailOp = VK_STENCIL_OP_KEEP;
    pipeline_depth_stencil_state_create_info.front.compareOp = VK_COMPARE_OP_NEVER;
    pipeline_depth_stencil_state_create_info.front.compareMask = 0;
    pipeline_depth_stencil_state_create_info.front.writeMask = 0;
    pipeline_depth_stencil_state_create_info.front.reference = 0;
    pipeline_depth_stencil_state_create_info.back.failOp = VK_STENCIL_OP_KEEP;
    pipeline_depth_stencil_state_create_info.back.passOp = VK_STENCIL_OP_KEEP;
    pipeline_depth_stencil_state_create_info.back.depthFailOp = VK_STENCIL_OP_KEEP;
    pipeline_depth_stencil_state_create_info.back.compareOp = VK_COMPARE_OP_NEVER;
    pipeline_depth_stencil_state_create_info.back.compareMask = 0;
    pipeline_depth_stencil_state_create_info.back.writeMask = 0;
    pipeline_depth_stencil_state_create_info.back.reference = 0;
    pipeline_depth_stencil_state_create_info.minDepthBounds = 0.0f;
    pipeline_depth_stencil_state_create_info.maxDepthBounds = 0.0f;

    return pipeline_depth_stencil_state_create_info;
}

void tgvk_pipeline_color_blend_attachmen_states_create(VkBool32 blend_enable, u32 count, VkPipelineColorBlendAttachmentState* p_pipeline_color_blend_attachment_states)
{
    for (u32 i = 0; i < count; i++)
    {
        p_pipeline_color_blend_attachment_states[i].blendEnable = blend_enable;
        p_pipeline_color_blend_attachment_states[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        p_pipeline_color_blend_attachment_states[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        p_pipeline_color_blend_attachment_states[i].colorBlendOp = VK_BLEND_OP_ADD;
        p_pipeline_color_blend_attachment_states[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        p_pipeline_color_blend_attachment_states[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        p_pipeline_color_blend_attachment_states[i].alphaBlendOp = VK_BLEND_OP_ADD;
        p_pipeline_color_blend_attachment_states[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }
}

VkPipelineColorBlendStateCreateInfo tgvk_pipeline_color_blend_state_create_info_create(u32 attachment_count, VkPipelineColorBlendAttachmentState* p_pipeline_color_blend_attachmen_states)
{
    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = { 0 };
    pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipeline_color_blend_state_create_info.pNext = TG_NULL;
    pipeline_color_blend_state_create_info.flags = 0;
    pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
    pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    pipeline_color_blend_state_create_info.attachmentCount = attachment_count;
    pipeline_color_blend_state_create_info.pAttachments = p_pipeline_color_blend_attachmen_states;
    pipeline_color_blend_state_create_info.blendConstants[0] = 0.0f;
    pipeline_color_blend_state_create_info.blendConstants[1] = 0.0f;
    pipeline_color_blend_state_create_info.blendConstants[2] = 0.0f;
    pipeline_color_blend_state_create_info.blendConstants[3] = 0.0f;

    return pipeline_color_blend_state_create_info;
}

VkPipelineVertexInputStateCreateInfo tgvk_pipeline_vertex_input_state_create_info_create(u32 vertex_binding_description_count, VkVertexInputBindingDescription* p_vertex_binding_descriptions, u32 vertex_attribute_description_count, VkVertexInputAttributeDescription* p_vertex_attribute_descriptions)
{
    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = { 0 };
    pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipeline_vertex_input_state_create_info.pNext = TG_NULL;
    pipeline_vertex_input_state_create_info.flags = 0;
    pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = vertex_binding_description_count;
    pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = p_vertex_binding_descriptions;
    pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = vertex_attribute_description_count;
    pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = p_vertex_attribute_descriptions;

    return pipeline_vertex_input_state_create_info;
}



tgvk_pipeline tgvk_pipeline_create_compute(const tgvk_shader* p_compute_shader)
{
    tgvk_pipeline compute_pipeline = { 0 };

    compute_pipeline.layout = tg__pipeline_layout_create(1, &p_compute_shader);

    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info = { 0 };
    pipeline_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_shader_stage_create_info.pNext = TG_NULL;
    pipeline_shader_stage_create_info.flags = 0;
    pipeline_shader_stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_shader_stage_create_info.module = p_compute_shader->shader_module;
    pipeline_shader_stage_create_info.pName = p_compute_shader->spirv_layout.p_entry_point_name;
    pipeline_shader_stage_create_info.pSpecializationInfo = TG_NULL;

    VkComputePipelineCreateInfo compute_pipeline_create_info = { 0 };
    compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    compute_pipeline_create_info.pNext = TG_NULL;
    compute_pipeline_create_info.flags = 0;
    compute_pipeline_create_info.stage = pipeline_shader_stage_create_info;
    compute_pipeline_create_info.layout = compute_pipeline.layout.pipeline_layout;
    compute_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    compute_pipeline_create_info.basePipelineIndex = -1;

    TGVK_CALL(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &compute_pipeline_create_info, TG_NULL, &compute_pipeline.pipeline));

    return compute_pipeline;
}

tgvk_pipeline tgvk_pipeline_create_graphics(const tgvk_graphics_pipeline_create_info* p_create_info)
{
    tgvk_pipeline graphics_pipeline = { 0 };

    VkVertexInputBindingDescription p_vertex_input_binding_descriptions[TG_MAX_SHADER_INPUTS] = { 0 };
    VkVertexInputAttributeDescription p_vertex_input_attribute_descriptions[TG_MAX_SHADER_INPUTS] = { 0 };
    for (u8 i = 0; i < p_create_info->p_vertex_shader->spirv_layout.input_resource_count; i++)
    {
        p_vertex_input_binding_descriptions[i].binding = i;
        p_vertex_input_binding_descriptions[i].stride = tg_vertex_input_attribute_format_get_size(p_create_info->p_vertex_shader->spirv_layout.p_input_resources[i].format);
        p_vertex_input_binding_descriptions[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        p_vertex_input_attribute_descriptions[i].binding = i;
        p_vertex_input_attribute_descriptions[i].location = p_create_info->p_vertex_shader->spirv_layout.p_input_resources[i].location;
        p_vertex_input_attribute_descriptions[i].format = p_create_info->p_vertex_shader->spirv_layout.p_input_resources[i].format;
        p_vertex_input_attribute_descriptions[i].offset = 0;
    }
    graphics_pipeline = tgvk_pipeline_create_graphics2(p_create_info, p_create_info->p_vertex_shader->spirv_layout.input_resource_count, p_vertex_input_binding_descriptions, p_vertex_input_attribute_descriptions);

    return graphics_pipeline;
}

tgvk_pipeline tgvk_pipeline_create_graphics2(const tgvk_graphics_pipeline_create_info* p_create_info, u32 vertex_attrib_count, VkVertexInputBindingDescription* p_vertex_bindings, VkVertexInputAttributeDescription* p_vertex_attribs)
{
    tgvk_pipeline graphics_pipeline = { 0 };

    graphics_pipeline.layout = tg__pipeline_layout_create(2, &p_create_info->p_vertex_shader);

    VkPipelineShaderStageCreateInfo p_pipeline_shader_stage_create_infos[2] = { 0 };

    p_pipeline_shader_stage_create_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    p_pipeline_shader_stage_create_infos[0].pNext = TG_NULL;
    p_pipeline_shader_stage_create_infos[0].flags = 0;
    p_pipeline_shader_stage_create_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    p_pipeline_shader_stage_create_infos[0].module = p_create_info->p_vertex_shader->shader_module;
    p_pipeline_shader_stage_create_infos[0].pName = p_create_info->p_vertex_shader->spirv_layout.p_entry_point_name;
    p_pipeline_shader_stage_create_infos[0].pSpecializationInfo = TG_NULL;

    p_pipeline_shader_stage_create_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    p_pipeline_shader_stage_create_infos[1].pNext = TG_NULL;
    p_pipeline_shader_stage_create_infos[1].flags = 0;
    p_pipeline_shader_stage_create_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    p_pipeline_shader_stage_create_infos[1].module = p_create_info->p_fragment_shader->shader_module;
    p_pipeline_shader_stage_create_infos[1].pName = p_create_info->p_fragment_shader->spirv_layout.p_entry_point_name;
    p_pipeline_shader_stage_create_infos[1].pSpecializationInfo = TG_NULL;

    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info = { 0 };
    pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipeline_input_assembly_state_create_info.pNext = TG_NULL;
    pipeline_input_assembly_state_create_info.flags = 0;
    pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = { 0 };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = p_create_info->viewport_size.x;
    viewport.height = p_create_info->viewport_size.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissors = { 0 };
    scissors.offset = (VkOffset2D) { 0, 0 };
    scissors.extent = swapchain_extent; // TODO: what happens for e.g. SSAO, when the viewport is smaller than the extent? just use the viewport size here?

    VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = { 0 };
    pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipeline_viewport_state_create_info.pNext = TG_NULL;
    pipeline_viewport_state_create_info.flags = 0;
    pipeline_viewport_state_create_info.viewportCount = 1;
    pipeline_viewport_state_create_info.pViewports = &viewport;
    pipeline_viewport_state_create_info.scissorCount = 1;
    pipeline_viewport_state_create_info.pScissors = &scissors;

    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info = { 0 };
    pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipeline_rasterization_state_create_info.pNext = TG_NULL;
    pipeline_rasterization_state_create_info.flags = 0;
    pipeline_rasterization_state_create_info.depthClampEnable = VK_TRUE; // TODO: make this optional
    pipeline_rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    pipeline_rasterization_state_create_info.polygonMode = p_create_info->polygon_mode;
    pipeline_rasterization_state_create_info.cullMode = p_create_info->cull_mode;
    pipeline_rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipeline_rasterization_state_create_info.depthBiasEnable = VK_FALSE;
    pipeline_rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
    pipeline_rasterization_state_create_info.depthBiasClamp = 0.0f;
    pipeline_rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;
    pipeline_rasterization_state_create_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = { 0 };
    pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipeline_multisample_state_create_info.pNext = TG_NULL;
    pipeline_multisample_state_create_info.flags = 0;
    pipeline_multisample_state_create_info.rasterizationSamples = p_create_info->sample_count;
    pipeline_multisample_state_create_info.sampleShadingEnable = p_create_info->sample_count == VK_SAMPLE_COUNT_1_BIT ? VK_FALSE : VK_TRUE;
    pipeline_multisample_state_create_info.minSampleShading = p_create_info->sample_count == VK_SAMPLE_COUNT_1_BIT ? 0.0f : 1.0f;
    pipeline_multisample_state_create_info.pSampleMask = TG_NULL;
    pipeline_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    pipeline_multisample_state_create_info.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = { 0 };
    pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipeline_depth_stencil_state_create_info.pNext = TG_NULL;
    pipeline_depth_stencil_state_create_info.flags = 0;
    pipeline_depth_stencil_state_create_info.depthTestEnable = p_create_info->depth_test_enable;
    pipeline_depth_stencil_state_create_info.depthWriteEnable = p_create_info->depth_write_enable;
    pipeline_depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
    pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    pipeline_depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    pipeline_depth_stencil_state_create_info.front.failOp = VK_STENCIL_OP_KEEP;
    pipeline_depth_stencil_state_create_info.front.passOp = VK_STENCIL_OP_KEEP;
    pipeline_depth_stencil_state_create_info.front.depthFailOp = VK_STENCIL_OP_KEEP;
    pipeline_depth_stencil_state_create_info.front.compareOp = VK_COMPARE_OP_NEVER;
    pipeline_depth_stencil_state_create_info.front.compareMask = 0;
    pipeline_depth_stencil_state_create_info.front.writeMask = 0;
    pipeline_depth_stencil_state_create_info.front.reference = 0;
    pipeline_depth_stencil_state_create_info.back.failOp = VK_STENCIL_OP_KEEP;
    pipeline_depth_stencil_state_create_info.back.passOp = VK_STENCIL_OP_KEEP;
    pipeline_depth_stencil_state_create_info.back.depthFailOp = VK_STENCIL_OP_KEEP;
    pipeline_depth_stencil_state_create_info.back.compareOp = VK_COMPARE_OP_NEVER;
    pipeline_depth_stencil_state_create_info.back.compareMask = 0;
    pipeline_depth_stencil_state_create_info.back.writeMask = 0;
    pipeline_depth_stencil_state_create_info.back.reference = 0;
    pipeline_depth_stencil_state_create_info.minDepthBounds = 0.0f;
    pipeline_depth_stencil_state_create_info.maxDepthBounds = 0.0f;

    TG_ASSERT(p_create_info->p_fragment_shader->spirv_layout.output_resource_count <= TG_MAX_SHADER_ATTACHMENTS);

    VkPipelineColorBlendAttachmentState p_pipeline_color_blend_attachment_states[TG_MAX_SHADER_ATTACHMENTS] = { 0 };
    for (u32 i = 0; i < p_create_info->p_fragment_shader->spirv_layout.output_resource_count; i++)
    {
        p_pipeline_color_blend_attachment_states[i].blendEnable = p_create_info->blend_enable;
        p_pipeline_color_blend_attachment_states[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        p_pipeline_color_blend_attachment_states[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        p_pipeline_color_blend_attachment_states[i].colorBlendOp = VK_BLEND_OP_ADD;
        p_pipeline_color_blend_attachment_states[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        p_pipeline_color_blend_attachment_states[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        p_pipeline_color_blend_attachment_states[i].alphaBlendOp = VK_BLEND_OP_ADD;
        p_pipeline_color_blend_attachment_states[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }

    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = { 0 };
    pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipeline_color_blend_state_create_info.pNext = TG_NULL;
    pipeline_color_blend_state_create_info.flags = 0;
    pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
    pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    pipeline_color_blend_state_create_info.attachmentCount = p_create_info->p_fragment_shader->spirv_layout.output_resource_count;
    pipeline_color_blend_state_create_info.pAttachments = p_pipeline_color_blend_attachment_states;
    pipeline_color_blend_state_create_info.blendConstants[0] = 0.0f;
    pipeline_color_blend_state_create_info.blendConstants[1] = 0.0f;
    pipeline_color_blend_state_create_info.blendConstants[2] = 0.0f;
    pipeline_color_blend_state_create_info.blendConstants[3] = 0.0f;

    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = { 0 };
    pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipeline_vertex_input_state_create_info.pNext = TG_NULL;
    pipeline_vertex_input_state_create_info.flags = 0;
    pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = vertex_attrib_count;
    pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = p_vertex_bindings;
    pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = vertex_attrib_count;
    pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = p_vertex_attribs;

    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_create_info.pNext = TG_NULL;
    graphics_pipeline_create_info.flags = 0;
    graphics_pipeline_create_info.stageCount = 2;
    graphics_pipeline_create_info.pStages = p_pipeline_shader_stage_create_infos;
    graphics_pipeline_create_info.pVertexInputState = &pipeline_vertex_input_state_create_info;
    graphics_pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
    graphics_pipeline_create_info.pTessellationState = TG_NULL;
    graphics_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
    graphics_pipeline_create_info.pRasterizationState = &pipeline_rasterization_state_create_info;
    graphics_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
    graphics_pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;
    graphics_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
    graphics_pipeline_create_info.pDynamicState = TG_NULL;
    graphics_pipeline_create_info.layout = graphics_pipeline.layout.pipeline_layout;
    graphics_pipeline_create_info.renderPass = p_create_info->render_pass;
    graphics_pipeline_create_info.subpass = 0;
    graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    graphics_pipeline_create_info.basePipelineIndex = -1;

    TGVK_CALL(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, TG_NULL, &graphics_pipeline.pipeline));

    return graphics_pipeline;
}

void tgvk_pipeline_destroy(tgvk_pipeline* p_pipeline)
{
    vkDestroyPipeline(device, p_pipeline->pipeline, TG_NULL);
    vkDestroyPipelineLayout(device, p_pipeline->layout.pipeline_layout, TG_NULL);
    vkDestroyDescriptorSetLayout(device, p_pipeline->layout.descriptor_set_layout, TG_NULL);
}



void tgvk_queue_present(VkPresentInfoKHR* p_present_info)
{
    tgvk_queue* p_queue = TG_NULL;
    TGVK_QUEUE_TAKE(TGVK_QUEUE_TYPE_PRESENT, p_queue);
    TGVK_CALL(vkQueuePresentKHR(p_queue->queue, p_present_info));
    TGVK_QUEUE_RELEASE(p_queue);
}

void tgvk_queue_submit(tgvk_queue_type type, u32 submit_count, VkSubmitInfo* p_submit_infos, VkFence fence)
{
    tgvk_queue* p_queue = TG_NULL;
    TGVK_QUEUE_TAKE(type, p_queue);
    TGVK_CALL(vkQueueSubmit(p_queue->queue, submit_count, p_submit_infos, fence));
    TGVK_QUEUE_RELEASE(p_queue);
}

void tgvk_queue_wait_idle(tgvk_queue_type type)
{
    tgvk_queue* p_queue = TG_NULL;
    TGVK_QUEUE_TAKE(type, p_queue);
    TGVK_CALL(vkQueueWaitIdle(p_queue->queue));
    TGVK_QUEUE_RELEASE(p_queue);
}



VkRenderPass tgvk_render_pass_create(u32 attachment_count, const VkAttachmentDescription* p_attachments, u32 subpass_count, const VkSubpassDescription* p_subpasses, u32 dependency_count, const VkSubpassDependency* p_dependencies)
{
    VkRenderPass render_pass = VK_NULL_HANDLE;

    VkRenderPassCreateInfo render_pass_create_info = { 0 };
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = TG_NULL;
    render_pass_create_info.flags = 0;
    render_pass_create_info.attachmentCount = attachment_count;
    render_pass_create_info.pAttachments = p_attachments;
    render_pass_create_info.subpassCount = subpass_count;
    render_pass_create_info.pSubpasses = p_subpasses;
    render_pass_create_info.dependencyCount = dependency_count;
    render_pass_create_info.pDependencies = p_dependencies;

    TGVK_CALL(vkCreateRenderPass(device, &render_pass_create_info, TG_NULL, &render_pass));

    return render_pass;
}

void tgvk_render_pass_destroy(VkRenderPass render_pass)
{
    vkDestroyRenderPass(device, render_pass, TG_NULL);
}



tg_render_target tgvk_render_target_create(u32 color_width, u32 color_height, VkFormat color_format, const tg_sampler_create_info* p_color_sampler_create_info, u32 depth_width, u32 depth_height, VkFormat depth_format, const tg_sampler_create_info* p_depth_sampler_create_info, VkFenceCreateFlags fence_create_flags)
{
    tg_render_target render_target = { 0 };
    render_target.type = TG_STRUCTURE_TYPE_RENDER_TARGET;

    render_target.color_attachment = tgvk_color_image_create(color_width, color_height, color_format, p_color_sampler_create_info);
    render_target.color_attachment_copy = tgvk_color_image_create(color_width, color_height, color_format, p_color_sampler_create_info);
    render_target.depth_attachment = tgvk_depth_image_create(depth_width, depth_height, depth_format, p_depth_sampler_create_info);
    render_target.depth_attachment_copy = tgvk_depth_image_create(depth_width, depth_height, depth_format, p_depth_sampler_create_info);

    const u32 thread_id = tg_platform_get_thread_id();
    tgvk_command_buffer_begin(&p_global_graphics_command_buffers[thread_id], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    tgvk_command_buffer_cmd_transition_color_image_layout(&p_global_graphics_command_buffers[thread_id], &render_target.color_attachment, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    tgvk_command_buffer_cmd_transition_color_image_layout(&p_global_graphics_command_buffers[thread_id], &render_target.color_attachment_copy, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    tgvk_command_buffer_cmd_transition_depth_image_layout(&p_global_graphics_command_buffers[thread_id], &render_target.depth_attachment, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
    tgvk_command_buffer_cmd_transition_depth_image_layout(&p_global_graphics_command_buffers[thread_id], &render_target.depth_attachment_copy, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    tgvk_command_buffer_end_and_submit(&p_global_graphics_command_buffers[thread_id]);

    render_target.fence = tgvk_fence_create(fence_create_flags);

    return render_target;
}

void tgvk_render_target_destroy(tg_render_target* p_render_target)
{
    tgvk_fence_destroy(p_render_target->fence);
    tgvk_image_destroy(&p_render_target->depth_attachment_copy);
    tgvk_image_destroy(&p_render_target->color_attachment_copy);
    tgvk_image_destroy(&p_render_target->depth_attachment);
    tgvk_image_destroy(&p_render_target->color_attachment);
}



VkSemaphore tgvk_semaphore_create(void)
{
    VkSemaphore semaphore = VK_NULL_HANDLE;

    VkSemaphoreCreateInfo semaphore_create_info = { 0 };
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = TG_NULL;
    semaphore_create_info.flags = 0;

    TGVK_CALL(vkCreateSemaphore(device, &semaphore_create_info, TG_NULL, &semaphore));

    return semaphore;
}

void tgvk_semaphore_destroy(VkSemaphore semaphore)
{
    vkDestroySemaphore(device, semaphore, TG_NULL);
}



tgvk_shader tgvk_shader_create(const char* p_filename)
{
    char p_filename_buffer[TG_MAX_PATH] = { 0 };
    tg_string_format(sizeof(p_filename_buffer), p_filename_buffer, "%s.spv", p_filename);

    // TODO: shader recompilation here

    tg_file_properties file_properties = { 0 };
    const b32 result = tg_platform_file_get_properties(p_filename_buffer, &file_properties);
    TG_ASSERT(result);
    char* p_content = TG_MEMORY_STACK_ALLOC(file_properties.size);
    tg_platform_file_read(p_filename_buffer, file_properties.size, p_content);

    const tgvk_shader shader = tgvk_shader_create_from_spirv((u32)file_properties.size, p_content);

    TG_MEMORY_STACK_FREE(file_properties.size);

    return shader;
}

tgvk_shader tgvk_shader_create_from_glsl(tg_shader_type type, const char* p_source)
{
    TG_ASSERT(p_source[tg_strlen_no_nul(p_source)] == '\0');

    shaderc_compiler_t compiler = shaderc_compiler_initialize();
    TG_ASSERT(compiler);

    shaderc_shader_kind shader_kind = 0;
    switch (type)
    {
    case TG_SHADER_TYPE_VERTEX: shader_kind = shaderc_vertex_shader; break;
    case TG_SHADER_TYPE_GEOMETRY: shader_kind = shaderc_geometry_shader; break;
    case TG_SHADER_TYPE_FRAGMENT: shader_kind = shaderc_fragment_shader; break;
    case TG_SHADER_TYPE_COMPUTE: shader_kind = shaderc_compute_shader; break;

    default: TG_INVALID_CODEPATH(); break;
    }
    const shaderc_compilation_result_t result = shaderc_compile_into_spv(compiler, p_source, tg_strlen_no_nul(p_source), shader_kind, "", "main", TG_NULL);
    TG_ASSERT(result);

    TG_DEBUG_EXEC(
        if (shaderc_result_get_num_warnings(result) || shaderc_result_get_num_errors(result))
        {
            const char* p_error_message = shaderc_result_get_error_message(result);
            TG_DEBUG_LOG(p_error_message);
            TG_INVALID_CODEPATH();
        }
    );

    const size_t length = shaderc_result_get_length(result);
    const char* p_bytes = shaderc_result_get_bytes(result);

    const tgvk_shader shader = tgvk_shader_create_from_spirv((u32)length, p_bytes);

    shaderc_result_release(result);
    shaderc_compiler_release(compiler);

    return shader;
}

tgvk_shader tgvk_shader_create_from_spirv(u32 size, const char* p_source)
{
    tgvk_shader shader = { 0 };

    const u32 word_count = size / 4;
    const u32* p_words = (u32*)p_source;

    tg_spirv_fill_layout(word_count, p_words, &shader.spirv_layout);

    VkShaderModuleCreateInfo shader_module_create_info = { 0 };
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.pNext = TG_NULL;
    shader_module_create_info.flags = 0;
    shader_module_create_info.codeSize = (size_t)size;
    shader_module_create_info.pCode = (const u32*)p_source;

    TGVK_CALL(vkCreateShaderModule(device, &shader_module_create_info, TG_NULL, &shader.shader_module));

    return shader;
}

void tgvk_shader_destroy(tgvk_shader* p_shader)
{
    vkDestroyShaderModule(device, p_shader->shader_module, TG_NULL);
}



tgvk_buffer* tgvk_global_staging_buffer_take(VkDeviceSize size)
{
    TG_RWL_LOCK_FOR_WRITE(global_staging_buffer_lock);

    if (global_staging_buffer.memory.size < size)
    {
        if (global_staging_buffer.buffer)
        {
            tgvk_buffer_destroy(&global_staging_buffer);
        }
        global_staging_buffer = tgvk_buffer_create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }

    return &global_staging_buffer;
}

void tgvk_global_staging_buffer_release(void)
{
#pragma warning(push)
#pragma warning(disable:26110)
    TG_RWL_UNLOCK_FOR_WRITE(global_staging_buffer_lock);
#pragma warning(pop)
}



VkDescriptorType tgvk_structure_type_convert_to_descriptor_type(tg_structure_type type)
{
    VkDescriptorType descriptor_type = -1;

    switch (type)
    {
    case TG_STRUCTURE_TYPE_COLOR_IMAGE:       descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; break;
    case TG_STRUCTURE_TYPE_DEPTH_IMAGE:       descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; break;
    case TG_STRUCTURE_TYPE_RENDER_TARGET:     descriptor_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; break;
    case TG_STRUCTURE_TYPE_STORAGE_BUFFER:    descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;         break;
    case TG_STRUCTURE_TYPE_UNIFORM_BUFFER:    descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;         break;

    default: TG_INVALID_CODEPATH(); break;
    }

    return descriptor_type;
}

void tgvk_vkresult_convert_to_string(char* p_buffer, VkResult result)
{
    switch (result)
    {
    case VK_SUCCESS:                                            { const char p_string[] = "VK_SUCCESS";                                            tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_NOT_READY:                                          { const char p_string[] = "VK_NOT_READY";                                          tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_TIMEOUT:                                            { const char p_string[] = "VK_TIMEOUT";                                            tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_EVENT_SET:                                          { const char p_string[] = "VK_EVENT_SET";                                          tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_EVENT_RESET:                                        { const char p_string[] = "VK_EVENT_RESET";                                        tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_INCOMPLETE:                                         { const char p_string[] = "VK_INCOMPLETE";                                         tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_OUT_OF_HOST_MEMORY:                           { const char p_string[] = "VK_ERROR_OUT_OF_HOST_MEMORY";                           tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:                         { const char p_string[] = "VK_ERROR_OUT_OF_DEVICE_MEMORY";                         tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_INITIALIZATION_FAILED:                        { const char p_string[] = "VK_ERROR_INITIALIZATION_FAILED";                        tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_DEVICE_LOST:                                  { const char p_string[] = "VK_ERROR_DEVICE_LOST";                                  tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_MEMORY_MAP_FAILED:                            { const char p_string[] = "VK_ERROR_MEMORY_MAP_FAILED";                            tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_LAYER_NOT_PRESENT:                            { const char p_string[] = "VK_ERROR_LAYER_NOT_PRESENT";                            tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_EXTENSION_NOT_PRESENT:                        { const char p_string[] = "VK_ERROR_EXTENSION_NOT_PRESENT";                        tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_FEATURE_NOT_PRESENT:                          { const char p_string[] = "VK_ERROR_FEATURE_NOT_PRESENT";                          tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_INCOMPATIBLE_DRIVER:                          { const char p_string[] = "VK_ERROR_INCOMPATIBLE_DRIVER";                          tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_TOO_MANY_OBJECTS:                             { const char p_string[] = "VK_ERROR_TOO_MANY_OBJECTS";                             tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_FORMAT_NOT_SUPPORTED:                         { const char p_string[] = "VK_ERROR_FORMAT_NOT_SUPPORTED";                         tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_FRAGMENTED_POOL:                              { const char p_string[] = "VK_ERROR_FRAGMENTED_POOL";                              tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_UNKNOWN:                                      { const char p_string[] = "VK_ERROR_UNKNOWN";                                      tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_OUT_OF_POOL_MEMORY:                           { const char p_string[] = "VK_ERROR_OUT_OF_POOL_MEMORY";                           tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:                      { const char p_string[] = "VK_ERROR_INVALID_EXTERNAL_HANDLE";                      tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_FRAGMENTATION:                                { const char p_string[] = "VK_ERROR_FRAGMENTATION";                                tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:               { const char p_string[] = "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";               tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_SURFACE_LOST_KHR:                             { const char p_string[] = "VK_ERROR_SURFACE_LOST_KHR";                             tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:                     { const char p_string[] = "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";                     tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_SUBOPTIMAL_KHR:                                     { const char p_string[] = "VK_SUBOPTIMAL_KHR";                                     tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_OUT_OF_DATE_KHR:                              { const char p_string[] = "VK_ERROR_OUT_OF_DATE_KHR";                              tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:                     { const char p_string[] = "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";                     tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_VALIDATION_FAILED_EXT:                        { const char p_string[] = "VK_ERROR_VALIDATION_FAILED_EXT";                        tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_INVALID_SHADER_NV:                            { const char p_string[] = "VK_ERROR_INVALID_SHADER_NV";                            tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_INCOMPATIBLE_VERSION_KHR:                     { const char p_string[] = "VK_ERROR_INCOMPATIBLE_VERSION_KHR";                     tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: { const char p_string[] = "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT"; tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_NOT_PERMITTED_EXT:                            { const char p_string[] = "VK_ERROR_NOT_PERMITTED_EXT";                            tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:          { const char p_string[] = "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";          tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_THREAD_IDLE_KHR:                                    { const char p_string[] = "VK_THREAD_IDLE_KHR";                                    tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_THREAD_DONE_KHR:                                    { const char p_string[] = "VK_THREAD_DONE_KHR";                                    tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_OPERATION_DEFERRED_KHR:                             { const char p_string[] = "VK_OPERATION_DEFERRED_KHR";                             tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_OPERATION_NOT_DEFERRED_KHR:                         { const char p_string[] = "VK_OPERATION_NOT_DEFERRED_KHR";                         tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_PIPELINE_COMPILE_REQUIRED_EXT:                      { const char p_string[] = "VK_PIPELINE_COMPILE_REQUIRED_EXT";                      tg_memory_copy(sizeof(p_string), p_string, p_buffer); } break;
    default: TG_INVALID_CODEPATH(); break;
    }
}





static VkCommandPool tg__command_pool_create(VkCommandPoolCreateFlags command_pool_create_flags, tgvk_queue_type type)
{
    VkCommandPool command_pool = { 0 };

    VkCommandPoolCreateInfo command_pool_create_info = { 0 };
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext = TG_NULL;
    command_pool_create_info.flags = command_pool_create_flags;
    command_pool_create_info.queueFamilyIndex = p_queues[type].queue_family_index;

    TGVK_CALL(vkCreateCommandPool(device, &command_pool_create_info, TG_NULL, &command_pool));

    return command_pool;
}

static void tg__command_pool_destroy(VkCommandPool command_pool)
{
    vkDestroyCommandPool(device, command_pool, TG_NULL);
}

static b32 tg__physical_device_supports_required_queue_families(VkPhysicalDevice pd)
{
    u32 queue_family_property_count;
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &queue_family_property_count, TG_NULL);
    if (queue_family_property_count == 0)
    {
        return TG_FALSE;
    }

    const u64 queue_family_properties_size = queue_family_property_count * sizeof(VkQueueFamilyProperties);
    VkQueueFamilyProperties* p_queue_family_properties = TG_MEMORY_STACK_ALLOC(queue_family_properties_size);
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &queue_family_property_count, p_queue_family_properties);

    b32 supports_compute_family = TG_FALSE;
    b32 supports_graphics_family = TG_FALSE;
    VkBool32 supports_present_family = VK_FALSE;
    for (u32 i = 0; i < queue_family_property_count; i++)
    {
        supports_compute_family |= (p_queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
        supports_graphics_family |= (p_queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
        VkBool32 spf = VK_FALSE;
        TGVK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, surface.surface, &spf));
        supports_present_family |= spf != 0;
    }

    TG_MEMORY_STACK_FREE(queue_family_properties_size);
    const b32 result = supports_graphics_family && supports_present_family && supports_compute_family;
    return result;
}

static void tg__physical_device_find_queue_family_indices(VkPhysicalDevice pd, u32* p_compute_queue_index, u32* p_graphics_queue_index, u32* p_present_queue_index, u32* p_compute_queue_low_prio_index, u32* p_graphics_queue_low_prio_index)
{
    TG_ASSERT(tg__physical_device_supports_required_queue_families(pd));

    u32 queue_family_property_count;
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &queue_family_property_count, TG_NULL);
    TG_ASSERT(queue_family_property_count);
    const u64 queue_family_properties_size = queue_family_property_count * sizeof(VkQueueFamilyProperties);
    VkQueueFamilyProperties* p_queue_family_properties = TG_MEMORY_STACK_ALLOC(queue_family_properties_size);
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &queue_family_property_count, p_queue_family_properties);

    b32 resolved = TG_FALSE;
    for (u32 i = 0; i < queue_family_property_count; i++)
    {
        const b32 supports_graphics_family = (p_queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
        VkBool32 spf = VK_FALSE;
        TGVK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, surface.surface, &spf));
        const b32 supports_present_family = spf != 0;
        const b32 supports_compute_family = (p_queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
    
        if (supports_graphics_family && supports_present_family && supports_compute_family && p_queue_family_properties[i].queueCount >= TGVK_QUEUE_TYPE_COUNT)
        {
            *p_compute_queue_index = i;
            *p_graphics_queue_index = i;
            *p_present_queue_index = i;
            *p_compute_queue_low_prio_index = i;
            *p_graphics_queue_low_prio_index = i;
            resolved = TG_TRUE;
            break;
        }
    }

    if (!resolved)
    {
        b32 supports_graphics_family = TG_FALSE;
        b32 supports_present_family = TG_FALSE;
        b32 supports_compute_family = TG_FALSE;

        for (u32 i = 0; i < queue_family_property_count; i++)
        {
            if (!supports_graphics_family)
            {
                supports_graphics_family |= (p_queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
                if (supports_graphics_family)
                {
                    *p_graphics_queue_index = i;
                }
            }
            if (!supports_present_family)
            {
                VkBool32 spf = VK_FALSE;
                TGVK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, surface.surface, &spf));
                supports_present_family |= spf != 0;
                if (supports_present_family)
                {
                    *p_present_queue_index = i;
                }
            }
            if (!supports_compute_family)
            {
                supports_compute_family |= (p_queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
                if (supports_compute_family)
                {
                    *p_compute_queue_index = i;
                }
            }

            if (supports_graphics_family && supports_present_family && supports_compute_family)
            {
                resolved = TG_TRUE;
                break;
            }
        }
    }

    TG_ASSERT(resolved);
    TG_MEMORY_STACK_FREE(queue_family_properties_size);
}

static VkSampleCountFlagBits tg__physical_device_find_max_sample_count(VkPhysicalDevice pd)
{
    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(pd, &physical_device_properties);

    const VkSampleCountFlags sample_count_flags = physical_device_properties.limits.framebufferColorSampleCounts & physical_device_properties.limits.framebufferDepthSampleCounts;

    if (sample_count_flags & VK_SAMPLE_COUNT_64_BIT)
    {
        return VK_SAMPLE_COUNT_64_BIT;
    }
    else if (sample_count_flags & VK_SAMPLE_COUNT_32_BIT)
    {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    else if (sample_count_flags & VK_SAMPLE_COUNT_16_BIT)
    {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    else if (sample_count_flags & VK_SAMPLE_COUNT_8_BIT)
    {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    else if (sample_count_flags & VK_SAMPLE_COUNT_4_BIT)
    {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    else if (sample_count_flags & VK_SAMPLE_COUNT_2_BIT)
    {
        return VK_SAMPLE_COUNT_2_BIT;
    }
    else
    {
        return VK_SAMPLE_COUNT_1_BIT;
    }
}

static b32 tg__physical_device_supports_extension(VkPhysicalDevice pd)
{
    u32 device_extension_property_count;
    TGVK_CALL(vkEnumerateDeviceExtensionProperties(pd, TG_NULL, &device_extension_property_count, TG_NULL));
    const u64 device_extension_properties_size = device_extension_property_count * sizeof(VkExtensionProperties);
    VkExtensionProperties* device_extension_properties = TG_MEMORY_STACK_ALLOC(device_extension_properties_size);
    TGVK_CALL(vkEnumerateDeviceExtensionProperties(pd, TG_NULL, &device_extension_property_count, device_extension_properties));

    b32 supports_extensions = TG_TRUE;
    for (u32 i = 0; i < TGVK_DEVICE_EXTENSION_COUNT; i++)
    {
        b32 supports_extension = TG_FALSE;
        for (u32 j = 0; j < device_extension_property_count; j++)
        {
            if (tg_string_equal(((char*[TGVK_DEVICE_EXTENSION_COUNT])TGVK_DEVICE_EXTENSION_NAMES)[i], device_extension_properties[j].extensionName) == 0)
            {
                supports_extension = TG_TRUE;
                break;
            }
        }
        supports_extensions &= supports_extension;
    }

    TG_MEMORY_STACK_FREE(device_extension_properties_size);
    return supports_extensions;
}

static u32 tg__physical_device_rate(VkPhysicalDevice pd)
{
    u32 rating = 0;

    VkPhysicalDeviceProperties physical_device_properties = { 0 };
    vkGetPhysicalDeviceProperties(pd, &physical_device_properties);

    VkPhysicalDeviceFeatures physical_device_features = { 0 };
    vkGetPhysicalDeviceFeatures(pd, &physical_device_features);

    u32 physical_device_surface_format_count;
    TGVK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface.surface, &physical_device_surface_format_count, TG_NULL));

    u32 physical_device_present_mode_count;
    TGVK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface.surface, &physical_device_present_mode_count, TG_NULL));

    if (!tg__physical_device_supports_extension(pd) ||
        !tg__physical_device_supports_required_queue_families(pd) ||
        !physical_device_surface_format_count ||
        !physical_device_present_mode_count)
    {
        return 0;
    }
    if (physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        rating += 1024;
    }
    if (physical_device_features.geometryShader)
    {
        rating += 64;
    }
    if (physical_device_features.samplerAnisotropy)
    {
        rating += 64;
    }
    rating += (u32)tg__physical_device_find_max_sample_count(pd);

    return rating;
}





static VkInstance tg__instance_create(void)
{
    VkInstance inst = VK_NULL_HANDLE;

#ifdef TG_DEBUG
    if (TGVK_VALIDATION_LAYER_COUNT)
    {
        u32 layer_property_count;
        vkEnumerateInstanceLayerProperties(&layer_property_count, TG_NULL);
        const u64 layer_properties_size = layer_property_count * sizeof(VkLayerProperties);
        VkLayerProperties* p_layer_properties = TG_MEMORY_STACK_ALLOC(layer_properties_size);
        vkEnumerateInstanceLayerProperties(&layer_property_count, p_layer_properties);

        const char* p_validation_layer_names[TGVK_VALIDATION_LAYER_COUNT] = TGVK_VALIDATION_LAYER_NAMES;
        for (u32 i = 0; i < TGVK_VALIDATION_LAYER_COUNT; i++)
        {
            b32 layer_found = TG_FALSE;
            for (u32 j = 0; j < layer_property_count; j++)
            {
                if (tg_string_equal(p_validation_layer_names[i], p_layer_properties[j].layerName))
                {
                    layer_found = TG_TRUE;
                    break;
                }
            }
            TG_ASSERT(layer_found);
        }
        TG_MEMORY_STACK_FREE(layer_properties_size);
    }
#endif

    VkApplicationInfo application_info = { 0 };
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pNext = TG_NULL;
    application_info.pApplicationName = TG_NULL;
    application_info.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
    application_info.pEngineName = TG_NULL;
    application_info.engineVersion = VK_MAKE_VERSION(0, 0, 0);
    application_info.apiVersion = VK_API_VERSION_1_2;

#if TGVK_VALIDATION_LAYER_COUNT
    const char* pp_enabled_layer_names[TGVK_VALIDATION_LAYER_COUNT] = TGVK_VALIDATION_LAYER_NAMES;
#else
    const char** pp_enabled_layer_names = TG_NULL;
#endif

#if TGVK_INSTANCE_EXTENSION_COUNT
    const char* pp_enabled_extension_names[TGVK_INSTANCE_EXTENSION_COUNT] = TGVK_INSTANCE_EXTENSION_NAMES;
#else
    const char** pp_enabled_extension_names = TG_NULL;
#endif

    VkInstanceCreateInfo instance_create_info = { 0 };
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = TG_NULL;
    instance_create_info.flags = 0;
    instance_create_info.pApplicationInfo = &application_info;
    instance_create_info.enabledLayerCount = TGVK_VALIDATION_LAYER_COUNT;
    instance_create_info.ppEnabledLayerNames = pp_enabled_layer_names;
    instance_create_info.enabledExtensionCount = TGVK_INSTANCE_EXTENSION_COUNT;
    instance_create_info.ppEnabledExtensionNames = pp_enabled_extension_names;

    TGVK_CALL(vkCreateInstance(&instance_create_info, TG_NULL, &inst));

    return inst;
}

#ifdef TG_DEBUG
static VkDebugUtilsMessengerEXT tg__debug_utils_manager_create(void)
{
    VkDebugUtilsMessengerEXT dum = VK_NULL_HANDLE;

    VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info = { 0 };
    debug_utils_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_utils_messenger_create_info.pNext = TG_NULL;
    debug_utils_messenger_create_info.flags = 0;
    debug_utils_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_utils_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_utils_messenger_create_info.pfnUserCallback = tg__debug_callback;
    debug_utils_messenger_create_info.pUserData = TG_NULL;

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    TG_ASSERT(vkCreateDebugUtilsMessengerEXT);
    TGVK_CALL(vkCreateDebugUtilsMessengerEXT(instance, &debug_utils_messenger_create_info, TG_NULL, &dum));

    return dum;
}
#endif

#ifdef TG_WIN32
static tgvk_surface tg__surface_create(void)
{
    tgvk_surface s = { 0 };

    VkWin32SurfaceCreateInfoKHR win32_surface_create_info = { 0 };
    win32_surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    win32_surface_create_info.pNext = TG_NULL;
    win32_surface_create_info.flags = 0;
    win32_surface_create_info.hinstance = GetModuleHandle(TG_NULL);
    win32_surface_create_info.hwnd = tg_platform_get_window_handle();

    TGVK_CALL(vkCreateWin32SurfaceKHR(instance, &win32_surface_create_info, TG_NULL, &s.surface));

    return s;
}
#endif

static VkPhysicalDevice tg__physical_device_create(void)
{
    VkPhysicalDevice pd = VK_NULL_HANDLE;

    u32 physical_device_count;
    TGVK_CALL(vkEnumeratePhysicalDevices(instance, &physical_device_count, TG_NULL));
    TG_ASSERT(physical_device_count);
    const u64 physical_devices_size = physical_device_count * sizeof(VkPhysicalDevice);
    VkPhysicalDevice* p_physical_devices = TG_MEMORY_STACK_ALLOC(physical_devices_size);
    TGVK_CALL(vkEnumeratePhysicalDevices(instance, &physical_device_count, p_physical_devices));

    u32 best_physical_device_index = 0;
    u32 best_physical_device_rating = 0;
    for (u32 i = 0; i < physical_device_count; i++)
    {
        const u32 rating = tg__physical_device_rate(p_physical_devices[i]);
        if (rating > best_physical_device_rating)
        {
            best_physical_device_index = i;
            best_physical_device_rating = rating;
        }
    }
    TG_ASSERT(best_physical_device_rating != 0);

    pd = p_physical_devices[best_physical_device_index];
    TG_ASSERT(pd != VK_NULL_HANDLE);

    TG_MEMORY_STACK_FREE(physical_devices_size);
    tg__physical_device_find_queue_family_indices(
        pd,
        &p_queues[0].queue_family_index,
        &p_queues[1].queue_family_index,
        &p_queues[2].queue_family_index,
        &p_queues[3].queue_family_index,
        &p_queues[4].queue_family_index
    );

    return pd;
}

static VkDevice tg__device_create(void)
{
    VkDevice d = VK_NULL_HANDLE;

    const f32 p_queue_priorities[TGVK_QUEUE_TYPE_COUNT] = { 1.0f, 1.0f, 1.0f, 0.5f, 0.5f };
    f32 pp_queue_priorities[TGVK_QUEUE_TYPE_COUNT][TGVK_QUEUE_TYPE_COUNT] = { 0 };
    VkDeviceQueueCreateInfo p_device_queue_create_infos[TGVK_QUEUE_TYPE_COUNT] = { 0 };

    for (u8 i = 0; i < TGVK_QUEUE_TYPE_COUNT; i++)
    {
        p_device_queue_create_infos[i].queueFamilyIndex = TG_U32_MAX;
    }

    u32 device_queue_create_info_count = 0;
    for (u8 i = 0; i < TGVK_QUEUE_TYPE_COUNT; i++)
    {
        p_queues[i].priority = p_queue_priorities[i];

        b32 found = TG_FALSE;
        for (u8 j = 0; j < device_queue_create_info_count; j++)
        {
            if (p_device_queue_create_infos[j].queueFamilyIndex == p_queues[i].queue_family_index)
            {
                found = TG_TRUE;
                b32 priority_submitted = TG_FALSE;
                for (u32 k = 0; k < TGVK_QUEUE_TYPE_COUNT; k++)
                {
                    if (p_device_queue_create_infos[j].pQueuePriorities[k] == p_queue_priorities[i])
                    {
                        priority_submitted = TG_TRUE;
                        p_queues[i].queue_index = k;
                        break;
                    }
                }
                if (!priority_submitted)
                {
                    pp_queue_priorities[j][p_device_queue_create_infos[j].queueCount] = p_queue_priorities[i];
                    p_queues[i].queue_index = p_device_queue_create_infos[j].queueCount++;
                }
                break;
            }
        }

        if (!found)
        {
            p_device_queue_create_infos[device_queue_create_info_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            p_device_queue_create_infos[device_queue_create_info_count].pNext = TG_NULL;
            p_device_queue_create_infos[device_queue_create_info_count].flags = 0;
            p_device_queue_create_infos[device_queue_create_info_count].queueFamilyIndex = p_queues[i].queue_family_index;
            p_device_queue_create_infos[device_queue_create_info_count].queueCount = 1;
            p_device_queue_create_infos[device_queue_create_info_count].pQueuePriorities = pp_queue_priorities[i];
            pp_queue_priorities[i][0] = p_queue_priorities[i];

            p_queues[i].queue_index = 0;
            device_queue_create_info_count++;
        }
    }

    VkPhysicalDeviceFeatures physical_device_features = { 0 };
    physical_device_features.robustBufferAccess = VK_FALSE;
    physical_device_features.fullDrawIndexUint32 = VK_FALSE;
    physical_device_features.imageCubeArray = VK_FALSE;
    physical_device_features.independentBlend = VK_FALSE;
    physical_device_features.geometryShader = VK_FALSE;
    physical_device_features.tessellationShader = VK_FALSE;
    physical_device_features.sampleRateShading = VK_FALSE;
    physical_device_features.dualSrcBlend = VK_FALSE;
    physical_device_features.logicOp = VK_FALSE;
    physical_device_features.multiDrawIndirect = VK_FALSE;
    physical_device_features.drawIndirectFirstInstance = VK_FALSE;
    physical_device_features.depthClamp = VK_TRUE;
    physical_device_features.depthBiasClamp = VK_FALSE;
    physical_device_features.fillModeNonSolid = VK_TRUE;
    physical_device_features.depthBounds = VK_FALSE;
    physical_device_features.wideLines = VK_FALSE;
    physical_device_features.largePoints = VK_FALSE;
    physical_device_features.alphaToOne = VK_FALSE;
    physical_device_features.multiViewport = VK_FALSE;
    physical_device_features.samplerAnisotropy = VK_TRUE;
    physical_device_features.textureCompressionETC2 = VK_FALSE;
    physical_device_features.textureCompressionASTC_LDR = VK_FALSE;
    physical_device_features.textureCompressionBC = VK_FALSE;
    physical_device_features.occlusionQueryPrecise = VK_FALSE;
    physical_device_features.pipelineStatisticsQuery = VK_FALSE;
    physical_device_features.vertexPipelineStoresAndAtomics = VK_TRUE;
    physical_device_features.fragmentStoresAndAtomics = VK_TRUE;
    physical_device_features.shaderTessellationAndGeometryPointSize = VK_FALSE;
    physical_device_features.shaderImageGatherExtended = VK_FALSE;
    physical_device_features.shaderStorageImageExtendedFormats = VK_FALSE;
    physical_device_features.shaderStorageImageMultisample = VK_FALSE;
    physical_device_features.shaderStorageImageReadWithoutFormat = VK_FALSE;
    physical_device_features.shaderStorageImageWriteWithoutFormat = VK_FALSE;
    physical_device_features.shaderUniformBufferArrayDynamicIndexing = VK_FALSE;
    physical_device_features.shaderSampledImageArrayDynamicIndexing = VK_FALSE;
    physical_device_features.shaderStorageBufferArrayDynamicIndexing = VK_FALSE;
    physical_device_features.shaderStorageImageArrayDynamicIndexing = VK_FALSE;
    physical_device_features.shaderClipDistance = VK_FALSE;
    physical_device_features.shaderCullDistance = VK_FALSE;
    physical_device_features.shaderFloat64 = VK_FALSE;
    physical_device_features.shaderInt64 = VK_FALSE;
    physical_device_features.shaderInt16 = VK_FALSE;
    physical_device_features.shaderResourceResidency = VK_FALSE;
    physical_device_features.shaderResourceMinLod = VK_FALSE;
    physical_device_features.sparseBinding = VK_FALSE;
    physical_device_features.sparseResidencyBuffer = VK_FALSE;
    physical_device_features.sparseResidencyImage2D = VK_FALSE;
    physical_device_features.sparseResidencyImage3D = VK_FALSE;
    physical_device_features.sparseResidency2Samples = VK_FALSE;
    physical_device_features.sparseResidency4Samples = VK_FALSE;
    physical_device_features.sparseResidency8Samples = VK_FALSE;
    physical_device_features.sparseResidency16Samples = VK_FALSE;
    physical_device_features.sparseResidencyAliased = VK_FALSE;
    physical_device_features.variableMultisampleRate = VK_FALSE;

#if TGVK_VALIDATION_LAYER_COUNT
    const char* pp_enabled_layer_names[TGVK_VALIDATION_LAYER_COUNT] = TGVK_VALIDATION_LAYER_NAMES;
#else
    const char** pp_enabled_layer_names = TG_NULL;
#endif
#if TGVK_DEVICE_EXTENSION_COUNT
    const char* pp_enabled_extension_names[TGVK_DEVICE_EXTENSION_COUNT] = TGVK_DEVICE_EXTENSION_NAMES;
#else
    const char** pp_enabled_extension_names = TG_NULL;
#endif

    VkDeviceCreateInfo device_create_info = { 0 };
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = TG_NULL;
    device_create_info.flags = 0;
    device_create_info.queueCreateInfoCount = device_queue_create_info_count;
    device_create_info.pQueueCreateInfos = p_device_queue_create_infos;
    device_create_info.enabledLayerCount = TGVK_VALIDATION_LAYER_COUNT;
    device_create_info.ppEnabledLayerNames = pp_enabled_layer_names;
    device_create_info.enabledExtensionCount = TGVK_DEVICE_EXTENSION_COUNT;
    device_create_info.ppEnabledExtensionNames = pp_enabled_extension_names;
    device_create_info.pEnabledFeatures = &physical_device_features;

    TGVK_CALL(vkCreateDevice(physical_device, &device_create_info, TG_NULL, &d));

    return d;
}

static void tg__queues_create(u32 count)
{
    for (u32 i = 0; i < count; i++)
    {
        vkGetDeviceQueue(device, p_queues[i].queue_family_index, p_queues[i].queue_index, &p_queues[i].queue);

        b32 found = TG_FALSE;
        for (u32 j = 0; j < i; j++)
        {
            if (p_queues[j].queue == p_queues[i].queue)
            {
                found = TG_TRUE;
                p_queues[i].h_mutex = p_queues[j].h_mutex;
                break;
            }
        }
        if (!found)
        {
            p_queues[i].h_mutex = TG_MUTEX_CREATE();
        }
        p_queues[i].fence = tgvk_fence_create(0);
    }
}

static void tg__swapchain_create(void)
{
    VkSurfaceCapabilitiesKHR surface_capabilities;
    TGVK_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface.surface, &surface_capabilities));

    u32 surface_format_count;
    TGVK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface.surface, &surface_format_count, TG_NULL));
    const u64 surface_formats_size = surface_format_count * sizeof(VkSurfaceFormatKHR);
    VkSurfaceFormatKHR* p_surface_formats = TG_MEMORY_STACK_ALLOC(surface_formats_size);
    TGVK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface.surface, &surface_format_count, p_surface_formats));

    surface.format = p_surface_formats[0];
    for (u32 i = 0; i < surface_format_count; i++)
    {
        if (p_surface_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && p_surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            surface.format = p_surface_formats[i];
            break;
        }
    }
    TG_MEMORY_STACK_FREE(surface_formats_size);

    VkPresentModeKHR p_present_modes[9] = { 0 };
    u32 present_mode_count = 0;
    TGVK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface.surface, &present_mode_count, TG_NULL));
    TGVK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface.surface, &present_mode_count, p_present_modes));

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i < present_mode_count; i++)
    {
        if (p_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            present_mode = p_present_modes[i];
            break;
        }
    }

    tg_platform_get_window_size(&swapchain_extent.width, &swapchain_extent.height);
    swapchain_extent.width = tgm_u32_clamp(swapchain_extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
    swapchain_extent.height = tgm_u32_clamp(swapchain_extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);

    TG_ASSERT(tgm_u32_clamp(TG_MAX_SWAPCHAIN_IMAGES, surface_capabilities.minImageCount, surface_capabilities.maxImageCount) == TG_MAX_SWAPCHAIN_IMAGES);

    VkSwapchainCreateInfoKHR swapchain_create_info = { 0 };
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = TG_NULL;
    swapchain_create_info.flags = 0;
    swapchain_create_info.surface = surface.surface;
    swapchain_create_info.minImageCount = TG_MAX_SWAPCHAIN_IMAGES;
    swapchain_create_info.imageFormat = surface.format.format;
    swapchain_create_info.imageColorSpace = surface.format.colorSpace;
    swapchain_create_info.imageExtent = swapchain_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    TG_ASSERT(p_queues[0].queue_family_index == p_queues[1].queue_family_index && p_queues[1].queue_family_index == p_queues[2].queue_family_index);
    // TODO: this may be relevant to implement as well...
    //const u32 p_queue_family_indices[] = { graphics_queue.index, present_queue.index };
    //if (graphics_queue.index != present_queue.index)
    //{
    //    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    //    swapchain_create_info.queueFamilyIndexCount = 2;
    //    swapchain_create_info.pQueueFamilyIndices = p_queue_family_indices;
    //}
    {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = TG_NULL;
    }

    swapchain_create_info.preTransform = surface_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

    TGVK_CALL(vkCreateSwapchainKHR(device, &swapchain_create_info, TG_NULL, &swapchain));

    u32 surface_image_count = TG_MAX_SWAPCHAIN_IMAGES;
#ifdef TG_DEBUG
    TGVK_CALL(vkGetSwapchainImagesKHR(device, swapchain, &surface_image_count, TG_NULL));
    TG_ASSERT(surface_image_count == TG_MAX_SWAPCHAIN_IMAGES);
#endif
    TGVK_CALL(vkGetSwapchainImagesKHR(device, swapchain, &surface_image_count, p_swapchain_images));

    for (u32 i = 0; i < TG_MAX_SWAPCHAIN_IMAGES; i++)
    {
        p_swapchain_image_views[i] = tg__image_view_create(p_swapchain_images[i], VK_IMAGE_VIEW_TYPE_2D, surface.format.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}





void tg_graphics_init(void)
{
    instance = tg__instance_create();
#ifdef TG_DEBUG
    debug_utils_messenger = tg__debug_utils_manager_create();
#endif
    surface = tg__surface_create();
    physical_device = tg__physical_device_create();
    device = tg__device_create();
    tg__queues_create(TGVK_QUEUE_TYPE_COUNT);

    p_compute_command_pools[0] = tg__command_pool_create(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, TGVK_QUEUE_TYPE_COMPUTE);
    p_graphics_command_pools[0] = tg__command_pool_create(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, TGVK_QUEUE_TYPE_GRAPHICS);
    present_command_pool = tg__command_pool_create(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, TGVK_QUEUE_TYPE_PRESENT);

    for (u32 i = 1; i < TG_MAX_THREADS; i++)
    {
        p_compute_command_pools[i] = tg__command_pool_create(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, TGVK_QUEUE_TYPE_COMPUTE_LOW_PRIORITY);
        p_graphics_command_pools[i] = tg__command_pool_create(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, TGVK_QUEUE_TYPE_GRAPHICS_LOW_PRIORITY);
    }

    VkCommandBufferAllocateInfo command_buffer_allocate_info = { 0 };
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = TG_NULL;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1;

    for (u32 i = 0; i < TG_MAX_THREADS; i++)
    {
        command_buffer_allocate_info.commandPool = p_compute_command_pools[i];
        TGVK_CALL(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &p_global_compute_command_buffers[i].command_buffer));

        command_buffer_allocate_info.commandPool = p_graphics_command_pools[i];
        TGVK_CALL(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &p_global_graphics_command_buffers[i].command_buffer));
    }

    tg__swapchain_create();

    tgvk_memory_allocator_init(device, physical_device);
    handle_lock = TG_RWL_CREATE();
    global_staging_buffer_lock = TG_RWL_CREATE();
    tg_shader_library_init();
    tg_renderer_init_shared_resources();
}

void tg_graphics_wait_idle(void)
{
    TGVK_CALL(vkDeviceWaitIdle(device));
}

void tg_graphics_shutdown(void)
{
    tg_renderer_shutdown_shared_resources();
    tg_shader_library_shutdown();
    if (global_staging_buffer.buffer)
    {
        tgvk_buffer_destroy(&global_staging_buffer);
    }
    tgvk_memory_allocator_shutdown(device);

    for (u32 i = 0; i < TG_MAX_SWAPCHAIN_IMAGES; i++)
    {
        vkDestroyImageView(device, p_swapchain_image_views[i], TG_NULL);
    }
    vkDestroySwapchainKHR(device, swapchain, TG_NULL);

    for (u32 i = 0; i < TG_MAX_THREADS; i++)
    {
        vkFreeCommandBuffers(device, p_graphics_command_pools[i], 1, &p_global_graphics_command_buffers[i].command_buffer);
        vkFreeCommandBuffers(device, p_compute_command_pools[i], 1, &p_global_compute_command_buffers[i].command_buffer);
        vkDestroyCommandPool(device, p_graphics_command_pools[i], TG_NULL);
        vkDestroyCommandPool(device, p_compute_command_pools[i], TG_NULL);
    }
    vkDestroyCommandPool(device, present_command_pool, TG_NULL);

    for (u32 i = 0; i < TGVK_QUEUE_TYPE_COUNT; i++)
    {
        tgvk_fence_destroy(p_queues[i].fence);
    }

    vkDestroyDevice(device, TG_NULL);
#ifdef TG_DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    TG_ASSERT(vkDestroyDebugUtilsMessengerEXT);
    vkDestroyDebugUtilsMessengerEXT(instance, debug_utils_messenger, TG_NULL);
#endif
    vkDestroySurfaceKHR(instance, surface.surface, TG_NULL);
    vkDestroyInstance(instance, TG_NULL);
}

void tg_graphics_on_window_resize(u32 width, u32 height)
{
    vkDeviceWaitIdle(device);

    for (u32 i = 0; i < TG_MAX_SWAPCHAIN_IMAGES; i++)
    {
        vkDestroyImageView(device, p_swapchain_image_views[i], TG_NULL);
    }
    vkDestroySwapchainKHR(device, swapchain, TG_NULL);
    tg__swapchain_create();
    for (u32 i = 0; i < TG_MAX_RENDERERS; i++)
    {
        if (TGVK_STRUCTURE_BUFFER_NAME(renderer)[i].type != TG_STRUCTURE_TYPE_INVALID)
        {
            tgvk_renderer_on_window_resize(&TGVK_STRUCTURE_BUFFER_NAME(renderer)[i], width, height);
        }
    }
}

#endif
