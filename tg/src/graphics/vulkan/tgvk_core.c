#include "graphics/vulkan/tgvk_core.h"

#ifdef TG_VULKAN

#include "graphics/tg_image_io.h"
#include "graphics/vulkan/tgvk_shader_library.h"
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

#define TGVK_DEVICE_EXTENSION_COUNT      2
#define TGVK_DEVICE_EXTENSION_NAMES      { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_SHADER_ATOMIC_INT64_EXTENSION_NAME }

#ifdef TG_DEBUG
#define TGVK_VALIDATION_LAYER_COUNT      1
#define TGVK_VALIDATION_LAYER_NAMES      { "VK_LAYER_KHRONOS_validation" }
#else
#define TGVK_VALIDATION_LAYER_COUNT      0
#define TGVK_VALIDATION_LAYER_NAMES      TG_NULL
#endif



typedef struct tgvk_simage
{
    tgvk_image_type         type;
    u32                     width;
    u32                     height;
    VkFormat                format;
    VkFilter                sampler_mag_filter;
    VkFilter                sampler_min_filter;
    VkSamplerAddressMode    sampler_address_mode_u;
    VkSamplerAddressMode    sampler_address_mode_v;
    VkSamplerAddressMode    sampler_address_mode_w;
    u8                      p_memory[0];
} tgvk_simage;



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

static tg_read_write_lock          internal_staging_buffer_lock;
static tgvk_buffer                 internal_staging_buffer;

static shaderc_compiler_t          shaderc_compiler;



#ifdef TG_DEBUG
#pragma warning(push)
#pragma warning(disable:4100)
static VKAPI_ATTR VkBool32 VKAPI_CALL tg__debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* user_data)
{
    TG_DEBUG_LOG("%s\n", p_callback_data->pMessage);
    tgp_file_create("tgvk_last_error.txt", (tg_size)tg_strlen_no_nul(p_callback_data->pMessage), p_callback_data->pMessage, TG_TRUE);
    return VK_TRUE;
}
#pragma warning(pop)
#endif



/*------------------------------------------------------------+
| General utilities                                           |
+------------------------------------------------------------*/

#define TGVK_QUEUE_TAKE(type_mask, p_queue)    (p_queue) = &p_queues[type_mask]; TG_MUTEX_LOCK((p_queue)->h_mutex)
#define TGVK_QUEUE_RELEASE(p_queue)       TG_MUTEX_UNLOCK((p_queue)->h_mutex)



void tg__get_transition_info(tgvk_image_type image_type, tgvk_image_layout_type src_type, tgvk_image_layout_type dst_type, TG_OUT VkAccessFlags* p_src_access_mask, TG_OUT VkAccessFlags* p_dst_access_mask, TG_OUT VkImageLayout* p_old_layout, TG_OUT VkImageLayout* p_new_layout, TG_OUT VkPipelineStageFlags* p_src_stage_bits, TG_OUT VkPipelineStageFlags* p_dst_stage_bits)
{
    VkAccessFlags p_access_masks[2] = { 0 };
    VkImageLayout p_layouts[2] = { 0 };
    VkPipelineStageFlags p_stage_bits[2] = { 0 };

    const tgvk_image_layout_type p_types[2] = { src_type, dst_type };
    for (u32 i = 0; i < 2; i++)
    {
        switch (p_types[i])
        {
        case TGVK_LAYOUT_UNDEFINED:
        {
            p_access_masks[i] = 0;
            p_layouts[i] = VK_IMAGE_LAYOUT_UNDEFINED;
            p_stage_bits[i] = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        } break;
        case TGVK_LAYOUT_COLOR_ATTACHMENT_WRITE:
        {
            p_access_masks[i] = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        } break;
        case TGVK_LAYOUT_DEPTH_ATTACHMENT_WRITE:
        {
            p_access_masks[i] = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_READ_C:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_READ_BIT;
            p_layouts[i] = image_type == TGVK_IMAGE_TYPE_STORAGE ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_READ_CF:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_READ_BIT;
            p_layouts[i] = image_type == TGVK_IMAGE_TYPE_STORAGE ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_READ_CFV:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_READ_BIT;
            p_layouts[i] = image_type == TGVK_IMAGE_TYPE_STORAGE ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_READ_CV:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_READ_BIT;
            p_layouts[i] = image_type == TGVK_IMAGE_TYPE_STORAGE ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_READ_F:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_READ_BIT;
            p_layouts[i] = image_type == TGVK_IMAGE_TYPE_STORAGE ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_READ_FV:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_WRITE_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_GENERAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_READ_V:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_WRITE_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_GENERAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_WRITE_C:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_WRITE_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_GENERAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_WRITE_CF:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_WRITE_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_GENERAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_WRITE_CFV:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_WRITE_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_GENERAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_WRITE_CV:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_WRITE_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_GENERAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_WRITE_F:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_WRITE_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_GENERAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_WRITE_FV:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_WRITE_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_GENERAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_WRITE_V:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_WRITE_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_GENERAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_READ_WRITE_C:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_GENERAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_READ_WRITE_CF:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_GENERAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_READ_WRITE_CFV:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_GENERAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_READ_WRITE_CV:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_GENERAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_READ_WRITE_F:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_GENERAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_READ_WRITE_FV:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_GENERAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_SHADER_READ_WRITE_V:
        {
            p_access_masks[i] = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_GENERAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        } break;
        case TGVK_LAYOUT_TRANSFER_READ:
        {
            p_access_masks[i] = VK_ACCESS_TRANSFER_READ_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } break;
        case TGVK_LAYOUT_TRANSFER_WRITE:
        {
            p_access_masks[i] = VK_ACCESS_TRANSFER_WRITE_BIT;
            p_layouts[i] = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            p_stage_bits[i] = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } break;

        default: TG_INVALID_CODEPATH(); break;
        }
    }

    *p_src_access_mask = p_access_masks[0];
    *p_dst_access_mask = p_access_masks[1];
    *p_old_layout = p_layouts[0];
    *p_new_layout = p_layouts[1];
    *p_src_stage_bits = p_stage_bits[0];
    *p_dst_stage_bits = p_stage_bits[1];
}

tgvk_buffer* tg__staging_buffer_take(VkDeviceSize size)
{
    TG_RWL_LOCK_FOR_WRITE(internal_staging_buffer_lock);

    if (internal_staging_buffer.memory.size < size)
    {
        if (internal_staging_buffer.buffer)
        {
            tgvk_buffer_destroy(&internal_staging_buffer);
        }

        VkBufferCreateInfo buffer_create_info = { 0 };
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.pNext = TG_NULL;
        buffer_create_info.flags = 0;
        buffer_create_info.size = tgvk_memory_aligned_size(size);
        buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.queueFamilyIndexCount = 0;
        buffer_create_info.pQueueFamilyIndices = TG_NULL;

        TGVK_CALL(vkCreateBuffer(device, &buffer_create_info, TG_NULL, &internal_staging_buffer.buffer));

        VkMemoryRequirements memory_requirements = { 0 };
        vkGetBufferMemoryRequirements(device, internal_staging_buffer.buffer, &memory_requirements);
        internal_staging_buffer.memory = TGVK_MALLOC(memory_requirements.alignment, memory_requirements.size, memory_requirements.memoryTypeBits, TGVK_MEMORY_HOST);
        TGVK_CALL(vkBindBufferMemory(device, internal_staging_buffer.buffer, internal_staging_buffer.memory.device_memory, internal_staging_buffer.memory.offset));
    }

    return &internal_staging_buffer;
}

void tg__staging_buffer_release(void)
{
#pragma warning(push)
#pragma warning(disable:26110)
    TG_RWL_UNLOCK_FOR_WRITE(internal_staging_buffer_lock);
#pragma warning(pop)
}

static tgvk_pipeline_layout tg__pipeline_layout_create(u32 shader_count, const tgvk_shader* const* pp_shaders)
{
    tgvk_pipeline_layout pipeline_layout = { 0 };
    pipeline_layout.global_resource_count = 0;

    for (u32 i = 0; i < shader_count; i++)
    {
        if (pp_shaders[i] != TG_NULL)
        {
            for (u32 j = 0; j < pp_shaders[i]->spirv_layout.global_resources.count; j++)
            {
                TG_ASSERT(pp_shaders[i]->spirv_layout.global_resources.p_resources[j].descriptor_set == 0);
                b32 found = TG_FALSE;
                for (u32 k = 0; k < pipeline_layout.global_resource_count; k++)
                {
                    const b32 same_set = pipeline_layout.p_global_resources[k].descriptor_set == pp_shaders[i]->spirv_layout.global_resources.p_resources[j].descriptor_set;
                    const b32 same_binding = pipeline_layout.p_global_resources[k].binding == pp_shaders[i]->spirv_layout.global_resources.p_resources[j].binding;
                    if (same_set && same_binding)
                    {
                        TG_ASSERT(pipeline_layout.p_global_resources[k].type == pp_shaders[i]->spirv_layout.global_resources.p_resources[j].type);
                        pipeline_layout.p_shader_stages[k] |= (VkShaderStageFlags)pp_shaders[i]->spirv_layout.shader_type;
                        found = TG_TRUE;
                        break;
                    }
                }
                if (!found)
                {
                    TG_ASSERT(pipeline_layout.global_resource_count + 1 < TG_MAX_SHADER_GLOBAL_RESOURCES);
                    pipeline_layout.p_global_resources[pipeline_layout.global_resource_count] = pp_shaders[i]->spirv_layout.global_resources.p_resources[j];
                    pipeline_layout.p_shader_stages[pipeline_layout.global_resource_count] = (VkShaderStageFlags)pp_shaders[i]->spirv_layout.shader_type;
                    pipeline_layout.global_resource_count++;
                }
            }
        }
    }

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





u32 tg_color_image_format_channels(tg_color_image_format format)
{
    u32 result = 0;

    switch (format)
    {
    case TG_COLOR_IMAGE_FORMAT_A8B8G8R8_UNORM:         result = 4; break;
    case TG_COLOR_IMAGE_FORMAT_B8G8R8A8_UNORM:         result = 4; break;
    case TG_COLOR_IMAGE_FORMAT_R16G16B16A16_SFLOAT:    result = 4; break;
    case TG_COLOR_IMAGE_FORMAT_R32G32B32A32_SFLOAT:    result = 4; break;
    case TG_COLOR_IMAGE_FORMAT_R32_UINT:               result = 1; break;
    case TG_COLOR_IMAGE_FORMAT_R8_UINT:                result = 1; break;
    case TG_COLOR_IMAGE_FORMAT_R8_UNORM:               result = 1; break;
    case TG_COLOR_IMAGE_FORMAT_R8G8_UNORM:             result = 2; break;
    case TG_COLOR_IMAGE_FORMAT_R8G8B8_UNORM:           result = 3; break;
    case TG_COLOR_IMAGE_FORMAT_R8G8B8A8_UNORM:         result = 4; break;

    default: TG_INVALID_CODEPATH(); break;
    }

    return result;
}

tg_size tg_color_image_format_size(tg_color_image_format format)
{
    tg_size result = 0;

    switch (format)
    {
    case TG_COLOR_IMAGE_FORMAT_A8B8G8R8_UNORM:         result = 4; break;
    case TG_COLOR_IMAGE_FORMAT_B8G8R8A8_UNORM:         result = 4; break;
    case TG_COLOR_IMAGE_FORMAT_R16G16B16A16_SFLOAT:    result = 8; break;
    case TG_COLOR_IMAGE_FORMAT_R32G32B32A32_SFLOAT:    result = 16; break;
    case TG_COLOR_IMAGE_FORMAT_R32_UINT:               result = 4; break;
    case TG_COLOR_IMAGE_FORMAT_R8_UINT:                result = 1; break;
    case TG_COLOR_IMAGE_FORMAT_R8_UNORM:               result = 1; break;
    case TG_COLOR_IMAGE_FORMAT_R8G8_UNORM:             result = 2; break;
    case TG_COLOR_IMAGE_FORMAT_R8G8B8_UNORM:           result = 3; break;
    case TG_COLOR_IMAGE_FORMAT_R8G8B8A8_UNORM:         result = 4; break;

    default: TG_INVALID_CODEPATH(); break;
    }

    return result;
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





void tgvk_buffer_copy(VkDeviceSize size, tgvk_buffer* p_src, tgvk_buffer* p_dst)
{
    const u32 thread_id = tgp_get_thread_id();
    tgvk_command_buffer_begin(&p_global_graphics_command_buffers[thread_id], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    tgvk_cmd_copy_buffer(&p_global_graphics_command_buffers[thread_id], size, p_src, p_dst);
    tgvk_command_buffer_end_and_submit(&p_global_graphics_command_buffers[thread_id]);
}

tgvk_buffer tgvk_buffer_create(VkDeviceSize size, VkBufferUsageFlags buffer_usage_flags, tgvk_memory_type type TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename))
{
    tgvk_buffer buffer = { 0 };

    VkBufferUsageFlags usage_flags = buffer_usage_flags;
    if (type == TGVK_MEMORY_DEVICE)
    {
        // Note: This is necessary so this buffer can be cleared with zeroes.
        usage_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    VkBufferCreateInfo buffer_create_info = { 0 };
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext = TG_NULL;
    buffer_create_info.flags = 0;
    buffer_create_info.size = tgvk_memory_aligned_size(size);
    buffer_create_info.usage = usage_flags;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = TG_NULL;

    TGVK_CALL(vkCreateBuffer(device, &buffer_create_info, TG_NULL, &buffer.buffer));

    VkMemoryRequirements memory_requirements = { 0 };
    vkGetBufferMemoryRequirements(device, buffer.buffer, &memory_requirements);
    buffer.memory = TGVK_MALLOC_DEBUG_INFO(memory_requirements.alignment, memory_requirements.size, memory_requirements.memoryTypeBits, type, line, p_filename);
    TGVK_CALL(vkBindBufferMemory(device, buffer.buffer, buffer.memory.device_memory, buffer.memory.offset));

    if (type == TGVK_MEMORY_HOST)
    {
        tg_memory_nullify(buffer.memory.size, buffer.memory.p_mapped_device_memory);
    }
    else
    {
        TG_ASSERT(type == TGVK_MEMORY_DEVICE);
        tgvk_buffer* p_staging_buffer = tg__staging_buffer_take(buffer.memory.size);
        tg_memory_nullify(buffer.memory.size, p_staging_buffer->memory.p_mapped_device_memory);
        tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
        tgvk_cmd_copy_buffer(p_command_buffer, buffer.memory.size, p_staging_buffer, &buffer);
        tgvk_command_buffer_end_and_submit(p_command_buffer);
        tg__staging_buffer_release();
    }

    return buffer;
}

void tgvk_buffer_destroy(tgvk_buffer* p_buffer)
{
    tgvk_memory_allocator_free(&p_buffer->memory);
    vkDestroyBuffer(device, p_buffer->buffer, TG_NULL);
}



void tgvk_cmd_begin_render_pass(tgvk_command_buffer* p_command_buffer, VkRenderPass render_pass, tgvk_framebuffer* p_framebuffer, VkSubpassContents subpass_contents)
{
    VkRenderPassBeginInfo render_pass_begin_info = { 0 };
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = TG_NULL;
    render_pass_begin_info.renderPass = render_pass;
    render_pass_begin_info.framebuffer = p_framebuffer->framebuffer;
    render_pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
    render_pass_begin_info.renderArea.extent = (VkExtent2D){ p_framebuffer->width, p_framebuffer->height };
    render_pass_begin_info.clearValueCount = 0;
    render_pass_begin_info.pClearValues = TG_NULL;

    vkCmdBeginRenderPass(p_command_buffer->buffer, &render_pass_begin_info, subpass_contents);
}

void tgvk_cmd_bind_and_draw_screen_quad(tgvk_command_buffer* p_command_buffer)
{
    vkCmdBindIndexBuffer(p_command_buffer->buffer, screen_quad_ibo.buffer, 0, VK_INDEX_TYPE_UINT16);
    const VkDeviceSize vertex_buffer_offset = 0;
    vkCmdBindVertexBuffers(p_command_buffer->buffer, 0, 1, &screen_quad_positions_vbo.buffer, &vertex_buffer_offset);
    vkCmdBindVertexBuffers(p_command_buffer->buffer, 1, 1, &screen_quad_uvs_vbo.buffer, &vertex_buffer_offset);
    vkCmdDrawIndexed(p_command_buffer->buffer, 6, 1, 0, 0, 0); 
}

void tgvk_cmd_bind_descriptor_set(tgvk_command_buffer* p_command_buffer, tgvk_pipeline* p_pipeline, tgvk_descriptor_set* p_descriptor_set)
{
    vkCmdBindDescriptorSets(
        p_command_buffer->buffer,
        p_pipeline->is_graphics_pipeline ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE,
        p_pipeline->layout.pipeline_layout,
        0,
        1,
        &p_descriptor_set->set,
        0,
        TG_NULL
    );
}

void tgvk_cmd_bind_index_buffer(tgvk_command_buffer* p_command_buffer, tgvk_buffer* p_buffer)
{
    vkCmdBindIndexBuffer(p_command_buffer->buffer, p_buffer->buffer, 0, VK_INDEX_TYPE_UINT16);
}

void tgvk_cmd_bind_pipeline(tgvk_command_buffer* p_command_buffer, tgvk_pipeline* p_pipeline)
{
    vkCmdBindPipeline(
        p_command_buffer->buffer,
        p_pipeline->is_graphics_pipeline ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE,
        p_pipeline->pipeline
    );
}

void tgvk_cmd_bind_vertex_buffer(tgvk_command_buffer* p_command_buffer, u32 binding, const tgvk_buffer* p_buffer)
{
    const VkDeviceSize vertex_buffer_offset = 0;
    vkCmdBindVertexBuffers(p_command_buffer->buffer, binding, 1, &p_buffer->buffer, &vertex_buffer_offset);
}

void tgvk_cmd_blit_image(tgvk_command_buffer* p_command_buffer, tgvk_image* p_source, tgvk_image* p_destination, const VkImageBlit* p_region)
{
    VkImageBlit region = { 0 };
    if (p_region)
    {
        region = *p_region;
    }
    else
    {
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.mipLevel = 0;
        region.srcSubresource.baseArrayLayer = 0;
        region.srcSubresource.layerCount = 1;
        region.srcOffsets[0].x = 0;
        region.srcOffsets[0].y = 0;
        region.srcOffsets[0].z = 0;
        region.srcOffsets[1].x = p_source->width;
        region.srcOffsets[1].y = p_source->height;
        region.srcOffsets[1].z = 1;
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.mipLevel = 0;
        region.dstSubresource.baseArrayLayer = 0;
        region.dstSubresource.layerCount = 1;
        region.dstOffsets[0].x = 0;
        region.dstOffsets[0].y = 0;
        region.dstOffsets[0].z = 0;
        region.dstOffsets[1].x = p_destination->width;
        region.dstOffsets[1].y = p_destination->height;
        region.dstOffsets[1].z = 1;
    }

    vkCmdBlitImage(p_command_buffer->buffer, p_source->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, p_destination->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_NEAREST);
}

void tgvk_cmd_blit_image_3d_slice_to_image(tgvk_command_buffer* p_command_buffer, u32 slice_depth, tgvk_image_3d* p_source, tgvk_image* p_destination, const VkImageBlit* p_region)
{
    VkImageBlit region = { 0 };
    if (p_region)
    {
        region = *p_region;
    }
    else
    {
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.mipLevel = 0;
        region.srcSubresource.baseArrayLayer = 0;
        region.srcSubresource.layerCount = 1;
        region.srcOffsets[0].x = 0;
        region.srcOffsets[0].y = 0;
        region.srcOffsets[0].z = slice_depth;
        region.srcOffsets[1].x = p_source->width;
        region.srcOffsets[1].y = p_source->height;
        region.srcOffsets[1].z = slice_depth + 1;
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.mipLevel = 0;
        region.dstSubresource.baseArrayLayer = 0;
        region.dstSubresource.layerCount = 1;
        region.dstOffsets[0].x = 0;
        region.dstOffsets[0].y = 0;
        region.dstOffsets[0].z = 0;
        region.dstOffsets[1].x = p_destination->width;
        region.dstOffsets[1].y = p_destination->height;
        region.dstOffsets[1].z = 1;
    }

    vkCmdBlitImage(p_command_buffer->buffer, p_source->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, p_destination->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_NEAREST);
}

void tgvk_cmd_blit_layered_image_layer_to_image(tgvk_command_buffer* p_command_buffer, u32 layer, tgvk_layered_image* p_source, tgvk_image* p_destination, const VkImageBlit* p_region)
{
    VkImageBlit region = { 0 };
    if (p_region)
    {
        region = *p_region;
    }
    else
    {
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.mipLevel = 0;
        region.srcSubresource.baseArrayLayer = 0;
        region.srcSubresource.layerCount = 1;
        region.srcOffsets[0].x = 0;
        region.srcOffsets[0].y = 0;
        region.srcOffsets[0].z = layer;
        region.srcOffsets[1].x = p_source->width;
        region.srcOffsets[1].y = p_source->height;
        region.srcOffsets[1].z = layer + 1;
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.mipLevel = 0;
        region.dstSubresource.baseArrayLayer = 0;
        region.dstSubresource.layerCount = 1;
        region.dstOffsets[0].x = 0;
        region.dstOffsets[0].y = 0;
        region.dstOffsets[0].z = 0;
        region.dstOffsets[1].x = p_destination->width;
        region.dstOffsets[1].y = p_destination->height;
        region.dstOffsets[1].z = 1;
    }

    vkCmdBlitImage(p_command_buffer->buffer, p_source->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, p_destination->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_NEAREST);
}

void tgvk_cmd_clear_image(tgvk_command_buffer* p_command_buffer, tgvk_image* p_image)
{
    if (p_image->type & (TGVK_IMAGE_TYPE_COLOR | TGVK_IMAGE_TYPE_STORAGE))
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

        vkCmdClearColorImage(p_command_buffer->buffer, p_image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color_value, 1, &image_subresource_range);
    }
    else if (p_image->type == TGVK_IMAGE_TYPE_DEPTH)
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

        vkCmdClearDepthStencilImage(p_command_buffer->buffer, p_image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_depth_stencil_value, 1, &image_subresource_range);
    }
    else
    {
        TG_INVALID_CODEPATH();
    }
}

void tgvk_cmd_clear_image_3d(tgvk_command_buffer* p_command_buffer, tgvk_image_3d* p_image_3d)
{
    if (p_image_3d->type == TGVK_IMAGE_TYPE_COLOR || p_image_3d->type == TGVK_IMAGE_TYPE_STORAGE)
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

        vkCmdClearColorImage(p_command_buffer->buffer, p_image_3d->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color_value, 1, &image_subresource_range);
    }
    else if (p_image_3d->type == TGVK_IMAGE_TYPE_DEPTH)
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

        vkCmdClearDepthStencilImage(p_command_buffer->buffer, p_image_3d->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_depth_stencil_value, 1, &image_subresource_range);
    }
    else
    {
        TG_INVALID_CODEPATH();
    }
}

void tgvk_cmd_clear_layered_image(tgvk_command_buffer* p_command_buffer, tgvk_layered_image* p_image)
{
    if (p_image->type == TGVK_IMAGE_TYPE_COLOR || p_image->type == TGVK_IMAGE_TYPE_STORAGE)
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

        vkCmdClearColorImage(p_command_buffer->buffer, p_image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color_value, 1, &image_subresource_range);
    }
    else if (p_image->type == TGVK_IMAGE_TYPE_DEPTH)
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

        vkCmdClearDepthStencilImage(p_command_buffer->buffer, p_image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_depth_stencil_value, 1, &image_subresource_range);
    }
    else
    {
        TG_INVALID_CODEPATH();
    }
}

void tgvk_cmd_copy_buffer(tgvk_command_buffer* p_command_buffer, VkDeviceSize size, tgvk_buffer* p_src, tgvk_buffer* p_dst)
{
    VkBufferCopy buffer_copy = { 0 };
    buffer_copy.srcOffset = 0;
    buffer_copy.dstOffset = 0;
    buffer_copy.size = size;

    vkCmdCopyBuffer(p_command_buffer->buffer, p_src->buffer, p_dst->buffer, 1, &buffer_copy);
}

void tgvk_cmd_copy_buffer_to_cube_map(tgvk_command_buffer* p_command_buffer, tgvk_buffer* p_source, tgvk_cube_map* p_destination)
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

    vkCmdCopyBufferToImage(p_command_buffer->buffer, p_source->buffer, p_destination->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
}

void tgvk_cmd_copy_buffer_to_image(tgvk_command_buffer* p_command_buffer, tgvk_buffer* p_source, tgvk_image* p_destination)
{
    VkBufferImageCopy buffer_image_copy = { 0 };
    buffer_image_copy.bufferOffset = 0;
    buffer_image_copy.bufferRowLength = 0;
    buffer_image_copy.bufferImageHeight = 0;
    buffer_image_copy.imageSubresource.aspectMask = p_destination->type == TGVK_IMAGE_TYPE_COLOR ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
    buffer_image_copy.imageSubresource.mipLevel = 0;
    buffer_image_copy.imageSubresource.baseArrayLayer = 0;
    buffer_image_copy.imageSubresource.layerCount = 1;
    buffer_image_copy.imageOffset.x = 0;
    buffer_image_copy.imageOffset.y = 0;
    buffer_image_copy.imageOffset.z = 0;
    buffer_image_copy.imageExtent.width = p_destination->width;
    buffer_image_copy.imageExtent.height = p_destination->height;
    buffer_image_copy.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(p_command_buffer->buffer, p_source->buffer, p_destination->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
}

void tgvk_cmd_copy_buffer_to_image_3d(tgvk_command_buffer* p_command_buffer, tgvk_buffer* p_source, tgvk_image_3d* p_destination)
{
    VkBufferImageCopy buffer_image_copy = { 0 };
    buffer_image_copy.bufferOffset = 0;
    buffer_image_copy.bufferRowLength = 0;
    buffer_image_copy.bufferImageHeight = 0;
    buffer_image_copy.imageSubresource.aspectMask = p_destination->type == TGVK_IMAGE_TYPE_DEPTH ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    buffer_image_copy.imageSubresource.mipLevel = 0;
    buffer_image_copy.imageSubresource.baseArrayLayer = 0;
    buffer_image_copy.imageSubresource.layerCount = 1;
    buffer_image_copy.imageOffset.x = 0;
    buffer_image_copy.imageOffset.y = 0;
    buffer_image_copy.imageOffset.z = 0;
    buffer_image_copy.imageExtent.width = p_destination->width;
    buffer_image_copy.imageExtent.height = p_destination->height;
    buffer_image_copy.imageExtent.depth = p_destination->depth;

    vkCmdCopyBufferToImage(p_command_buffer->buffer, p_source->buffer, p_destination->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
}

void tgvk_cmd_copy_color_image(tgvk_command_buffer* p_command_buffer, tgvk_image* p_source, tgvk_image* p_destination)
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

    vkCmdCopyImage(p_command_buffer->buffer, p_source->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, p_destination->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);
}

void tgvk_cmd_copy_color_image_to_buffer(tgvk_command_buffer* p_command_buffer, tgvk_image* p_source, tgvk_buffer* p_destination)
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

    vkCmdCopyImageToBuffer(p_command_buffer->buffer, p_source->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, p_destination->buffer, 1, &buffer_image_copy);
}

void tgvk_cmd_copy_depth_image_pixel_to_buffer(tgvk_command_buffer* p_command_buffer, tgvk_image* p_source, tgvk_buffer* p_destination, u32 x, u32 y)
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

    vkCmdCopyImageToBuffer(p_command_buffer->buffer, p_source->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, p_destination->buffer, 1, &buffer_image_copy);
}

void tgvk_cmd_copy_image_3d_to_buffer(tgvk_command_buffer* p_command_buffer, tgvk_image_3d* p_source, tgvk_buffer* p_destination)
{
    VkBufferImageCopy buffer_image_copy = { 0 };
    buffer_image_copy.bufferOffset = 0;
    buffer_image_copy.bufferRowLength = 0;
    buffer_image_copy.bufferImageHeight = 0;
    buffer_image_copy.imageSubresource.aspectMask = p_source->type == TGVK_IMAGE_TYPE_DEPTH ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    buffer_image_copy.imageSubresource.mipLevel = 0;
    buffer_image_copy.imageSubresource.baseArrayLayer = 0;
    buffer_image_copy.imageSubresource.layerCount = 1;
    buffer_image_copy.imageOffset.x = 0;
    buffer_image_copy.imageOffset.y = 0;
    buffer_image_copy.imageOffset.z = 0;
    buffer_image_copy.imageExtent.width = p_source->width;
    buffer_image_copy.imageExtent.height = p_source->height;
    buffer_image_copy.imageExtent.depth = p_source->depth;

    vkCmdCopyImageToBuffer(p_command_buffer->buffer, p_source->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, p_destination->buffer, 1, &buffer_image_copy);
}

void tgvk_cmd_draw_indexed(tgvk_command_buffer* p_command_buffer, u32 index_count)
{
    vkCmdDrawIndexed(p_command_buffer->buffer, index_count, 1, 0, 0, 0);
}

void tgvk_cmd_transition_cube_map_layout(tgvk_command_buffer* p_command_buffer, tgvk_cube_map* p_cube_map, tgvk_image_layout_type src_type, tgvk_image_layout_type dst_type)
{
    VkAccessFlags src_access_mask, dst_access_mask;
    VkImageLayout old_layout, new_layout;
    VkPipelineStageFlags src_stage_bits, dst_stage_bits;

    tg__get_transition_info(TGVK_IMAGE_TYPE_COLOR, src_type, dst_type, &src_access_mask, &dst_access_mask, &old_layout, &new_layout, &src_stage_bits, &dst_stage_bits);
    tgvk_cmd_transition_cube_map_layout2(p_command_buffer, p_cube_map, src_access_mask, dst_access_mask, old_layout, new_layout, src_stage_bits, dst_stage_bits);
}

void tgvk_cmd_transition_cube_map_layout2(tgvk_command_buffer* p_command_buffer, tgvk_cube_map* p_cube_map, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits)
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

    vkCmdPipelineBarrier(p_command_buffer->buffer, src_stage_bits, dst_stage_bits, 0, 0, TG_NULL, 0, TG_NULL, 1, &image_memory_barrier);
}

void tgvk_cmd_transition_image_layout(tgvk_command_buffer* p_command_buffer, tgvk_image* p_image, tgvk_image_layout_type src_type, tgvk_image_layout_type dst_type)
{
    VkAccessFlags src_access_mask, dst_access_mask;
    VkImageLayout old_layout, new_layout;
    VkPipelineStageFlags src_stage_bits, dst_stage_bits;

    tg__get_transition_info(p_image->type, src_type, dst_type, &src_access_mask, &dst_access_mask, &old_layout, &new_layout, &src_stage_bits, &dst_stage_bits);
    tgvk_cmd_transition_image_layout2(p_command_buffer, p_image, src_access_mask, dst_access_mask, old_layout, new_layout, src_stage_bits, dst_stage_bits);
}

void tgvk_cmd_transition_image_layout2(tgvk_command_buffer* p_command_buffer, tgvk_image* p_image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits)
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
    image_memory_barrier.subresourceRange.aspectMask = p_image->type == TGVK_IMAGE_TYPE_DEPTH ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(p_command_buffer->buffer, src_stage_bits, dst_stage_bits, 0, 0, TG_NULL, 0, TG_NULL, 1, &image_memory_barrier);
}

void tgvk_cmd_transition_image_3d_layout(tgvk_command_buffer* p_command_buffer, tgvk_image_3d* p_image_3d, tgvk_image_layout_type src_type, tgvk_image_layout_type dst_type)
{
    VkAccessFlags src_access_mask, dst_access_mask;
    VkImageLayout old_layout, new_layout;
    VkPipelineStageFlags src_stage_bits, dst_stage_bits;

    tg__get_transition_info(p_image_3d->type, src_type, dst_type, &src_access_mask, &dst_access_mask, &old_layout, &new_layout, &src_stage_bits, &dst_stage_bits);
    tgvk_cmd_transition_image_3d_layout2(p_command_buffer, p_image_3d, src_access_mask, dst_access_mask, old_layout, new_layout, src_stage_bits, dst_stage_bits);
}

void tgvk_cmd_transition_image_3d_layout2(tgvk_command_buffer* p_command_buffer, tgvk_image_3d* p_image_3d, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits)
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
    image_memory_barrier.subresourceRange.aspectMask = p_image_3d->type == TGVK_IMAGE_TYPE_DEPTH ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(p_command_buffer->buffer, src_stage_bits, dst_stage_bits, 0, 0, TG_NULL, 0, TG_NULL, 1, &image_memory_barrier);
}

void tgvk_cmd_transition_layered_image_layout(tgvk_command_buffer* p_command_buffer, tgvk_layered_image* p_image, tgvk_image_layout_type src_type, tgvk_image_layout_type dst_type)
{
    VkAccessFlags src_access_mask, dst_access_mask;
    VkImageLayout old_layout, new_layout;
    VkPipelineStageFlags src_stage_bits, dst_stage_bits;

    tg__get_transition_info(p_image->type, src_type, dst_type, &src_access_mask, &dst_access_mask, &old_layout, &new_layout, &src_stage_bits, &dst_stage_bits);
    tgvk_cmd_transition_layered_image_layout2(p_command_buffer, p_image, src_access_mask, dst_access_mask, old_layout, new_layout, src_stage_bits, dst_stage_bits);
}

void tgvk_cmd_transition_layered_image_layout2(tgvk_command_buffer* p_command_buffer, tgvk_layered_image* p_image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits)
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
    image_memory_barrier.subresourceRange.aspectMask = p_image->type == TGVK_IMAGE_TYPE_DEPTH ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(p_command_buffer->buffer, src_stage_bits, dst_stage_bits, 0, 0, TG_NULL, 0, TG_NULL, 1, &image_memory_barrier);
}



tgvk_command_buffer* tgvk_command_buffer_get_and_begin_global(tgvk_command_pool_type type)
{
    tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_global(type);
    tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    return p_command_buffer;
}

tgvk_command_buffer* tgvk_command_buffer_get_global(tgvk_command_pool_type type)
{
    tgvk_command_buffer* p_command_buffer = TG_NULL;

    const u32 thread_id = tgp_get_thread_id();
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
    command_buffer.pool_type = type;
    command_buffer.level = level;

    VkCommandBufferAllocateInfo command_buffer_allocate_info = { 0 };
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = TG_NULL;
    command_buffer_allocate_info.level = level;
    command_buffer_allocate_info.commandBufferCount = 1;

    switch (type)
    {
    case TGVK_COMMAND_POOL_TYPE_COMPUTE:
    {
        const u32 thread_id = tgp_get_thread_id();
        command_buffer_allocate_info.commandPool = p_compute_command_pools[thread_id];
    } break;
    case TGVK_COMMAND_POOL_TYPE_GRAPHICS:
    {
        const u32 thread_id = tgp_get_thread_id();
        command_buffer_allocate_info.commandPool = p_graphics_command_pools[thread_id];
    } break;
    case TGVK_COMMAND_POOL_TYPE_PRESENT:
    {
        command_buffer_allocate_info.commandPool = present_command_pool;
    } break;
    default: TG_INVALID_CODEPATH(); break;
    }

    TGVK_CALL(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &command_buffer.buffer));

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

    switch (p_command_buffer->pool_type)
    {
    case TGVK_COMMAND_POOL_TYPE_COMPUTE:
    {
        const u32 thread_id = tgp_get_thread_id();
        command_pool = p_compute_command_pools[thread_id];
    } break;
    case TGVK_COMMAND_POOL_TYPE_GRAPHICS:
    {
        const u32 thread_id = tgp_get_thread_id();
        command_pool = p_graphics_command_pools[thread_id];
    } break;
    case TGVK_COMMAND_POOL_TYPE_PRESENT:
    {
        command_pool = present_command_pool;
    } break;
    default: TG_INVALID_CODEPATH(); break;
    }

    vkFreeCommandBuffers(device, command_pool, 1, &p_command_buffer->buffer);
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

    TGVK_CALL(vkBeginCommandBuffer(p_command_buffer->buffer, &command_buffer_begin_info));
}

void tgvk_command_buffer_begin_secondary(tgvk_command_buffer* p_command_buffer, VkCommandBufferUsageFlags flags, VkRenderPass render_pass, tgvk_framebuffer* p_framebuffer)
{
    VkCommandBufferInheritanceInfo command_buffer_inheritance_info = { 0 };
    command_buffer_inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    command_buffer_inheritance_info.pNext = TG_NULL;
    command_buffer_inheritance_info.renderPass = render_pass;
    command_buffer_inheritance_info.subpass = 0;
    command_buffer_inheritance_info.framebuffer = p_framebuffer->framebuffer;
    command_buffer_inheritance_info.occlusionQueryEnable = TG_FALSE;
    command_buffer_inheritance_info.queryFlags = 0;
    command_buffer_inheritance_info.pipelineStatistics = 0;

    VkCommandBufferBeginInfo command_buffer_begin_info = { 0 };
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = TG_NULL;
    command_buffer_begin_info.flags = flags;
    command_buffer_begin_info.pInheritanceInfo = &command_buffer_inheritance_info;

    TGVK_CALL(vkBeginCommandBuffer(p_command_buffer->buffer, &command_buffer_begin_info));
}

void tgvk_command_buffer_end_and_submit(tgvk_command_buffer* p_command_buffer)
{
    TGVK_CALL(vkEndCommandBuffer(p_command_buffer->buffer));

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = TG_NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = TG_NULL;
    submit_info.pWaitDstStageMask = TG_NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &p_command_buffer->buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = TG_NULL;

    tgvk_queue* p_queue = TG_NULL;
    TGVK_QUEUE_TAKE(p_command_buffer->pool_type, p_queue);

    TGVK_CALL(vkQueueSubmit(p_queue->queue, 1, &submit_info, p_queue->fence));
    TGVK_CALL(vkWaitForFences(device, 1, &p_queue->fence, VK_FALSE, TG_U64_MAX));
    TGVK_CALL(vkResetFences(device, 1, &p_queue->fence));

    TGVK_QUEUE_RELEASE(p_queue);
}



tgvk_cube_map tgvk_cube_map_create(u32 dimension, VkFormat format, const tgvk_sampler_create_info* p_sampler_create_info TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename))
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
    cube_map.memory = TGVK_MALLOC_DEBUG_INFO(memory_requirements.alignment, memory_requirements.size, memory_requirements.memoryTypeBits, TGVK_MEMORY_DEVICE, line, p_filename);
    TGVK_CALL(vkBindImageMemory(device, cube_map.image, cube_map.memory.device_memory, cube_map.memory.offset));

    VkImageViewCreateInfo image_view_create_info = { 0 };
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = TG_NULL;
    image_view_create_info.flags = 0;
    image_view_create_info.image = cube_map.image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    image_view_create_info.format = format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 6;

    TGVK_CALL(vkCreateImageView(device, &image_view_create_info, TG_NULL, &cube_map.image_view));

    if (p_sampler_create_info)
    {
        cube_map.sampler = tgvk_sampler_create2(p_sampler_create_info);
    }
    else
    {
        cube_map.sampler = tgvk_sampler_create();
    }

    return cube_map;
}

void tgvk_cube_map_destroy(tgvk_cube_map* p_cube_map)
{
    tgvk_sampler_destroy(&p_cube_map->sampler);
    vkDestroyImageView(device, p_cube_map->image_view, TG_NULL);
    tgvk_memory_allocator_free(&p_cube_map->memory);
    vkDestroyImage(device, p_cube_map->image, TG_NULL);
}



tgvk_descriptor_set tgvk_descriptor_set_create(const tgvk_pipeline* p_pipeline)
{
    tgvk_descriptor_set descriptor_set = { 0 };

    TG_ASSERT(p_pipeline->layout.global_resource_count <= TG_MAX_SHADER_GLOBAL_RESOURCES);

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

    TGVK_CALL(vkCreateDescriptorPool(device, &descriptor_pool_create_info, TG_NULL, &descriptor_set.pool));

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = { 0 };
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = TG_NULL;
    descriptor_set_allocate_info.descriptorPool = descriptor_set.pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &p_pipeline->layout.descriptor_set_layout;

    TGVK_CALL(vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &descriptor_set.set));

    return descriptor_set;
}

void tgvk_descriptor_set_destroy(tgvk_descriptor_set* p_descriptor_set)
{
    vkDestroyDescriptorPool(device, p_descriptor_set->pool, TG_NULL);
}



void tgvk_descriptor_set_update_cube_map(VkDescriptorSet descriptor_set, tgvk_cube_map* p_cube_map, u32 dst_binding)
{
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    descriptor_image_info.sampler = p_cube_map->sampler.sampler;
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

void tgvk_descriptor_set_update_image(VkDescriptorSet descriptor_set, tgvk_image* p_image, u32 dst_binding)
{
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    descriptor_image_info.sampler = p_image->sampler.sampler;
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

void tgvk_descriptor_set_update_image2(VkDescriptorSet descriptor_set, tgvk_image* p_image, u32 dst_binding, VkDescriptorType descriptor_type)
{
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    descriptor_image_info.sampler = p_image->sampler.sampler;
    descriptor_image_info.imageView = p_image->image_view;
    descriptor_image_info.imageLayout = descriptor_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet write_descriptor_set = { 0 };
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = TG_NULL;
    write_descriptor_set.dstSet = descriptor_set;
    write_descriptor_set.dstBinding = dst_binding;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = descriptor_type;
    write_descriptor_set.pImageInfo = &descriptor_image_info;
    write_descriptor_set.pBufferInfo = TG_NULL;
    write_descriptor_set.pTexelBufferView = TG_NULL;

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
}

void tgvk_descriptor_set_update_image_array(VkDescriptorSet descriptor_set, tgvk_image* p_image, u32 dst_binding, u32 array_index)
{
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    descriptor_image_info.sampler = p_image->sampler.sampler;
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

void tgvk_descriptor_set_update_image_3d(VkDescriptorSet descriptor_set, tgvk_image_3d* p_image_3d, u32 dst_binding)
{
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    descriptor_image_info.sampler = p_image_3d->sampler.sampler;
    descriptor_image_info.imageView = p_image_3d->image_view;

    VkWriteDescriptorSet write_descriptor_set = { 0 };
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = TG_NULL;
    write_descriptor_set.dstSet = descriptor_set;
    write_descriptor_set.dstBinding = dst_binding;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.pImageInfo = &descriptor_image_info;
    write_descriptor_set.pBufferInfo = TG_NULL;
    write_descriptor_set.pTexelBufferView = TG_NULL;

    if (p_image_3d->type == TGVK_IMAGE_TYPE_COLOR || p_image_3d->type == TGVK_IMAGE_TYPE_DEPTH)
    {
        descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    }
    else if (p_image_3d->type == TGVK_IMAGE_TYPE_STORAGE)
    {
        descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    }

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
}

void tgvk_descriptor_set_update_layered_image(VkDescriptorSet descriptor_set, tgvk_layered_image* p_image, u32 dst_binding)
{
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    descriptor_image_info.sampler = p_image->sampler.sampler;
    descriptor_image_info.imageView = p_image->read_image_view;
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

void tgvk_descriptor_set_update_storage_buffer(VkDescriptorSet descriptor_set, tgvk_buffer* p_buffer, u32 dst_binding)
{
    VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
    descriptor_buffer_info.buffer = p_buffer->buffer;
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

void tgvk_descriptor_set_update_storage_buffer_array(VkDescriptorSet descriptor_set, tgvk_buffer* p_buffer, u32 dst_binding, u32 array_index)
{
    VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
    descriptor_buffer_info.buffer = p_buffer->buffer;
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

void tgvk_descriptor_set_update_uniform_buffer(VkDescriptorSet descriptor_set, tgvk_buffer* p_buffer, u32 dst_binding)
{
    VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
    descriptor_buffer_info.buffer = p_buffer->buffer;
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

void tgvk_descriptor_set_update_uniform_buffer_array(VkDescriptorSet descriptor_set, tgvk_buffer* p_buffer, u32 dst_binding, u32 array_index)
{
    VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
    descriptor_buffer_info.buffer = p_buffer->buffer;
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

void tgvk_descriptor_set_update_uniform_buffer_range(VkDescriptorSet descriptor_set, VkDeviceSize offset, VkDeviceSize range, tgvk_buffer* p_buffer, u32 dst_binding)
{
    VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
    descriptor_buffer_info.buffer = p_buffer->buffer;
    descriptor_buffer_info.offset = offset;
    descriptor_buffer_info.range = range;

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
    framebuffer.layers = 1;

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

tgvk_framebuffer tgvk_framebuffer_create_layered(VkRenderPass render_pass, u32 attachment_count, const VkImageView* p_attachments, u32 width, u32 height, u32 layers)
{
    tgvk_framebuffer framebuffer = { 0 };

    framebuffer.width = width;
    framebuffer.height = height;
    framebuffer.layers = layers;

    VkFramebufferCreateInfo framebuffer_create_info = { 0 };
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.pNext = TG_NULL;
    framebuffer_create_info.flags = 0;
    framebuffer_create_info.renderPass = render_pass;
    framebuffer_create_info.attachmentCount = attachment_count;
    framebuffer_create_info.pAttachments = p_attachments;
    framebuffer_create_info.width = width;
    framebuffer_create_info.height = height;
    framebuffer_create_info.layers = layers;

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



b32 tgvk_get_physical_device_image_format_properties(VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, TG_OUT VkImageFormatProperties* p_image_format_properties)
{
    const VkResult vk_result = vkGetPhysicalDeviceImageFormatProperties(physical_device, format, type, tiling, usage, flags, p_image_format_properties);
    TG_ASSERT(vk_result == VK_ERROR_FORMAT_NOT_SUPPORTED || vk_result == VK_SUCCESS);
    const b32 result = vk_result == VK_SUCCESS;
    return result;
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

        VkBufferCreateInfo buffer_create_info = { 0 };
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.pNext = TG_NULL;
        buffer_create_info.flags = 0;
        buffer_create_info.size = tgvk_memory_aligned_size(size);
        buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.queueFamilyIndexCount = 0;
        buffer_create_info.pQueueFamilyIndices = TG_NULL;

        TGVK_CALL(vkCreateBuffer(device, &buffer_create_info, TG_NULL, &global_staging_buffer.buffer));

        VkMemoryRequirements memory_requirements = { 0 };
        vkGetBufferMemoryRequirements(device, global_staging_buffer.buffer, &memory_requirements);
        global_staging_buffer.memory = TGVK_MALLOC(memory_requirements.alignment, memory_requirements.size, memory_requirements.memoryTypeBits, TGVK_MEMORY_HOST);
        TGVK_CALL(vkBindBufferMemory(device, global_staging_buffer.buffer, global_staging_buffer.memory.device_memory, global_staging_buffer.memory.offset));
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



tgvk_image tgvk_image_create(tgvk_image_type_flags type_flags, u32 width, u32 height, VkFormat format, const tgvk_sampler_create_info* p_sampler_create_info TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename))
{
    tgvk_image image = { 0 };

    VkImageUsageFlags usage = 0;
    VkImageAspectFlagBits aspect_mask = 0;
    if (type_flags & TGVK_IMAGE_TYPE_COLOR)
    {
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        aspect_mask |= VK_IMAGE_ASPECT_COLOR_BIT;
    }
    if (type_flags & TGVK_IMAGE_TYPE_DEPTH)
    {
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        aspect_mask |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    if (type_flags & TGVK_IMAGE_TYPE_STORAGE)
    {
        usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        aspect_mask |= VK_IMAGE_ASPECT_COLOR_BIT;
    }

    image.type = type_flags;
    image.width = width;
    image.height = height;
    image.format = format;

    VkImageCreateInfo image_create_info = { 0 };
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = TG_NULL;
    image_create_info.flags = 0;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = format;
    image_create_info.extent.width = width;
    image_create_info.extent.height = height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | usage;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = TG_NULL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    TGVK_CALL(vkCreateImage(device, &image_create_info, TG_NULL, &image.image));

    VkMemoryRequirements memory_requirements = { 0 };
    vkGetImageMemoryRequirements(device, image.image, &memory_requirements);
    image.memory = TGVK_MALLOC_DEBUG_INFO(memory_requirements.alignment, memory_requirements.size, memory_requirements.memoryTypeBits, TGVK_MEMORY_DEVICE, line, p_filename);
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

    VkImageViewCreateInfo image_view_create_info = { 0 };
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = TG_NULL;
    image_view_create_info.flags = 0;
    image_view_create_info.image = image.image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = aspect_mask;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;

    TGVK_CALL(vkCreateImageView(device, &image_view_create_info, TG_NULL, &image.image_view));

    if (p_sampler_create_info)
    {
        image.sampler = tgvk_sampler_create2(p_sampler_create_info);
    }
    else
    {
        image.sampler = tgvk_sampler_create();
    }

    return image;
}

tgvk_image tgvk_image_create2(tgvk_image_type type, const char* p_filename, const tgvk_sampler_create_info* p_sampler_create_info TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename2))
{
    u32 w, h;
    tg_color_image_format f;
    u32* p_data = TG_NULL;
    tg_image_load(p_filename, &w, &h, &f, &p_data);
    //h_color_image->mip_levels = TG_IMAGE_MAX_MIP_LEVELS(h_color_image->width, h_color_image->height);// TODO: mipmapping
    const tg_size size = (tg_size)w * (tg_size)h * sizeof(*p_data);

    tgvk_buffer* p_staging_buffer = tg__staging_buffer_take((VkDeviceSize)size);
    tg_memcpy(size, p_data, p_staging_buffer->memory.p_mapped_device_memory);

    tgvk_image image = tgvk_image_create(type, w, h, (VkFormat)f, p_sampler_create_info
#ifdef TG_DEBUG
        , line, p_filename2
#endif
    );

    tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
    tgvk_cmd_transition_image_layout(p_command_buffer, &image, TGVK_LAYOUT_UNDEFINED, TGVK_LAYOUT_TRANSFER_WRITE);
    tgvk_cmd_copy_buffer_to_image(p_command_buffer, p_staging_buffer, &image);
    tgvk_cmd_transition_image_layout(p_command_buffer, &image, TGVK_LAYOUT_TRANSFER_WRITE, TGVK_LAYOUT_SHADER_READ_CFV);
    tgvk_command_buffer_end_and_submit(p_command_buffer);
    tg__staging_buffer_release();

    tg_image_free(p_data);

    return image;
}

void tgvk_image_destroy(tgvk_image* p_image)
{
    if (p_image->sampler.sampler)
    {
        tgvk_sampler_destroy(&p_image->sampler);
    }
    if (p_image->image_view)
    {
        vkDestroyImageView(device, p_image->image_view, TG_NULL);
    }
    tgvk_memory_allocator_free(&p_image->memory);
    vkDestroyImage(device, p_image->image, TG_NULL);
}

b32 tgvk_image_serialize(tgvk_image* p_image, const char* p_filename)
{
    const tg_size staging_buffer_size = (tg_size)p_image->width * (tg_size)p_image->height * tg_color_image_format_size((tg_color_image_format)p_image->format);
    const tg_size size = sizeof(tgvk_simage) + staging_buffer_size;
    tgvk_simage* p_simage = (tgvk_simage*)TG_MALLOC_STACK(size);

    p_simage->type = p_image->type;
    p_simage->width = p_image->width;
    p_simage->height = p_image->height;
    p_simage->format = p_image->format;
    p_simage->sampler_mag_filter = p_image->sampler.mag_filter;
    p_simage->sampler_min_filter = p_image->sampler.min_filter;
    p_simage->sampler_address_mode_u = p_image->sampler.address_mode_u;
    p_simage->sampler_address_mode_v = p_image->sampler.address_mode_v;
    p_simage->sampler_address_mode_w = p_image->sampler.address_mode_w;

    tgvk_buffer* p_staging_buffer = tg__staging_buffer_take(staging_buffer_size);
    tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
    tgvk_cmd_copy_color_image_to_buffer(p_command_buffer, p_image, p_staging_buffer);
    tgvk_command_buffer_end_and_submit(p_command_buffer);
    tg_memcpy(staging_buffer_size, p_staging_buffer->memory.p_mapped_device_memory, p_simage->p_memory);
    tg__staging_buffer_release();

    const b32 result = tgp_file_create(p_filename, size, p_simage, TG_TRUE);
    TG_FREE_STACK(size);

    return result;
}

b32 tgvk_image_deserialize(const char* p_filename, TG_OUT tgvk_image* p_image TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename2))
{
    b32 result = TG_FALSE;

    tg_file_properties file_properties = { 0 };
    const b32 get_file_properties_result = tgp_file_get_properties(p_filename, &file_properties);
    if (get_file_properties_result)
    {
        u8* p_buffer = (u8*)TG_MALLOC_STACK(file_properties.size);
        const b32 load_file_result = tgp_file_load(p_filename, file_properties.size, p_buffer);
        if (load_file_result)
        {
            result = TG_TRUE;

            const tgvk_simage* p_simage = (tgvk_simage*)p_buffer;

            tgvk_sampler_create_info sampler_create_info = { 0 };
            sampler_create_info.mag_filter = p_simage->sampler_mag_filter;
            sampler_create_info.min_filter = p_simage->sampler_min_filter;
            sampler_create_info.address_mode_u = p_simage->sampler_address_mode_u;
            sampler_create_info.address_mode_v = p_simage->sampler_address_mode_v;
            sampler_create_info.address_mode_w = p_simage->sampler_address_mode_w;

            *p_image = tgvk_image_create(p_simage->type, p_simage->width, p_simage->height, p_simage->format, &sampler_create_info
#ifdef TG_DEBUG
                , line, p_filename2
#endif
            );

            const VkDeviceSize staging_buffer_size = file_properties.size - sizeof(tgvk_simage);
            tgvk_buffer* p_staging_buffer = tg__staging_buffer_take(staging_buffer_size);
            tg_memcpy(staging_buffer_size, p_simage->p_memory, p_staging_buffer->memory.p_mapped_device_memory);
            tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
            tgvk_cmd_transition_image_layout(p_command_buffer, p_image, TGVK_LAYOUT_UNDEFINED, TGVK_LAYOUT_TRANSFER_WRITE);
            tgvk_cmd_copy_buffer_to_image(p_command_buffer, p_staging_buffer, p_image);
            tgvk_cmd_transition_image_layout(p_command_buffer, p_image, TGVK_LAYOUT_TRANSFER_WRITE, TGVK_LAYOUT_SHADER_READ_CFV);
            tgvk_command_buffer_end_and_submit(p_command_buffer);
            tg__staging_buffer_release();
        }
        TG_FREE_STACK(file_properties.size);
    }

    return result;
}

b32 tgvk_image_store_to_disc(tgvk_image* p_image, const char* p_filename, b32 force_alpha_one, b32 replace_existing)
{
    const u32 width = p_image->width;
    const u32 height = p_image->height;

    tgvk_image blit_image = TGVK_IMAGE_CREATE(TGVK_IMAGE_TYPE_COLOR, width, height, (VkFormat)TG_COLOR_IMAGE_FORMAT_B8G8R8A8_UNORM, TG_NULL);

    const VkDeviceSize staging_buffer_size = (VkDeviceSize)width * (VkDeviceSize)height * tg_color_image_format_size((tg_color_image_format)blit_image.format);
    tgvk_buffer* p_staging_buffer = tg__staging_buffer_take(staging_buffer_size);

    tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
    tgvk_cmd_transition_image_layout(p_command_buffer, &blit_image, TGVK_LAYOUT_UNDEFINED, TGVK_LAYOUT_TRANSFER_WRITE);
    tgvk_cmd_blit_image(p_command_buffer, p_image, &blit_image, TG_NULL);
    tgvk_cmd_transition_image_layout(p_command_buffer, &blit_image, TGVK_LAYOUT_TRANSFER_WRITE, TGVK_LAYOUT_TRANSFER_READ);
    tgvk_cmd_copy_color_image_to_buffer(p_command_buffer, &blit_image, p_staging_buffer);
    tgvk_command_buffer_end_and_submit(p_command_buffer);
    const b32 result = tg_image_store_to_disc(p_filename, width, height, (tg_color_image_format)blit_image.format, p_staging_buffer->memory.p_mapped_device_memory, force_alpha_one, replace_existing);

    tg__staging_buffer_release();
    tgvk_image_destroy(&blit_image);

    return result;
}



tgvk_image_3d tgvk_image_3d_create(tgvk_image_type type, u32 width, u32 height, u32 depth, VkFormat format, const tgvk_sampler_create_info* p_sampler_create_info TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename))
{
    tgvk_image_3d image_3d = { 0 };

    VkImageUsageFlags usage = 0;
    VkImageAspectFlagBits aspect_mask = 0;
    if (type == TGVK_IMAGE_TYPE_COLOR)
    {
        usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    else if (type == TGVK_IMAGE_TYPE_DEPTH)
    {
        usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else if (type == TGVK_IMAGE_TYPE_STORAGE)
    {
        usage = VK_IMAGE_USAGE_STORAGE_BIT;
        aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    else
    {
        TG_INVALID_CODEPATH();
    }

    image_3d.type = type;
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
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | usage;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = TG_NULL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    TGVK_CALL(vkCreateImage(device, &image_create_info, TG_NULL, &image_3d.image));

    VkMemoryRequirements memory_requirements = { 0 };
    vkGetImageMemoryRequirements(device, image_3d.image, &memory_requirements);
    image_3d.memory = TGVK_MALLOC_DEBUG_INFO(memory_requirements.alignment, memory_requirements.size, memory_requirements.memoryTypeBits, TGVK_MEMORY_DEVICE, line, p_filename);
    TGVK_CALL(vkBindImageMemory(device, image_3d.image, image_3d.memory.device_memory, image_3d.memory.offset));

    VkImageViewCreateInfo image_view_create_info = { 0 };
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = TG_NULL;
    image_view_create_info.flags = 0;
    image_view_create_info.image = image_3d.image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_3D;
    image_view_create_info.format = format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = aspect_mask;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;

    TGVK_CALL(vkCreateImageView(device, &image_view_create_info, TG_NULL, &image_3d.image_view));

    if (p_sampler_create_info)
    {
        // TODO: mip levels, also above and below, once respectively
        image_3d.sampler = tgvk_sampler_create2(p_sampler_create_info);
    }
    else
    {
        image_3d.sampler = tgvk_sampler_create();
    }

    return image_3d;
}

void tgvk_image_3d_destroy(tgvk_image_3d* p_image_3d)
{
    tgvk_sampler_destroy(&p_image_3d->sampler);
    vkDestroyImageView(device, p_image_3d->image_view, TG_NULL);
    tgvk_memory_allocator_free(&p_image_3d->memory);
    vkDestroyImage(device, p_image_3d->image, TG_NULL);
}

b32 tgvk_image_3d_store_slice_to_disc(tgvk_image_3d* p_image_3d, u32 slice_depth, const char* p_filename, b32 force_alpha_one, b32 replace_existing)
{
    const u32 width = p_image_3d->width;
    const u32 height = p_image_3d->height;

    tgvk_image blit_image = TGVK_IMAGE_CREATE(TGVK_IMAGE_TYPE_COLOR, width, height, (VkFormat)TG_COLOR_IMAGE_FORMAT_B8G8R8A8_UNORM, TG_NULL);

    const VkDeviceSize staging_buffer_size = (VkDeviceSize)width * (VkDeviceSize)height * tg_color_image_format_size((tg_color_image_format)blit_image.format);
    tgvk_buffer* p_staging_buffer = tg__staging_buffer_take(staging_buffer_size);

    tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
    tgvk_cmd_transition_image_layout(p_command_buffer, &blit_image, TGVK_LAYOUT_UNDEFINED, TGVK_LAYOUT_TRANSFER_WRITE);
    tgvk_cmd_blit_image_3d_slice_to_image(p_command_buffer, slice_depth, p_image_3d, &blit_image, TG_NULL);
    tgvk_cmd_transition_image_layout(p_command_buffer, &blit_image, TGVK_LAYOUT_TRANSFER_WRITE, TGVK_LAYOUT_TRANSFER_READ);
    tgvk_cmd_copy_color_image_to_buffer(p_command_buffer, &blit_image, p_staging_buffer);
    tgvk_command_buffer_end_and_submit(p_command_buffer);
    const b32 result = tg_image_store_to_disc(p_filename, width, height, (tg_color_image_format)blit_image.format, p_staging_buffer->memory.p_mapped_device_memory, force_alpha_one, replace_existing);

    tg__staging_buffer_release();
    tgvk_image_destroy(&blit_image);

    return result;
}



tgvk_layered_image tgvk_layered_image_create(tgvk_image_type type, u32 width, u32 height, u32 layers, VkFormat format, const tgvk_sampler_create_info* p_sampler_create_info TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename))
{
    tgvk_layered_image image = { 0 };

    VkImageUsageFlags usage = 0;
    VkImageAspectFlagBits aspect_mask = 0;
    if (type == TGVK_IMAGE_TYPE_COLOR)
    {
        usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    else if (type == TGVK_IMAGE_TYPE_DEPTH)
    {
        usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else if (type == TGVK_IMAGE_TYPE_STORAGE)
    {
        usage = VK_IMAGE_USAGE_STORAGE_BIT;
        aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    else
    {
        TG_INVALID_CODEPATH();
    }

    image.type = type;
    image.width = width;
    image.height = height;
    image.layers = layers;
    image.format = format;

    VkImageCreateInfo image_create_info = { 0 };
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = TG_NULL;
    image_create_info.flags = VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
    image_create_info.imageType = VK_IMAGE_TYPE_3D;
    image_create_info.format = format;
    image_create_info.extent.width = width;
    image_create_info.extent.height = height;
    image_create_info.extent.depth = layers;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | usage;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = TG_NULL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    TGVK_CALL(vkCreateImage(device, &image_create_info, TG_NULL, &image.image));

    VkMemoryRequirements memory_requirements = { 0 };
    vkGetImageMemoryRequirements(device, image.image, &memory_requirements);
    image.memory = TGVK_MALLOC_DEBUG_INFO(memory_requirements.alignment, memory_requirements.size, memory_requirements.memoryTypeBits, TGVK_MEMORY_DEVICE, line, p_filename);
    TGVK_CALL(vkBindImageMemory(device, image.image, image.memory.device_memory, image.memory.offset));

    // TODO: mipmapping

    VkImageViewCreateInfo read_image_view_create_info = { 0 };
    read_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    read_image_view_create_info.pNext = TG_NULL;
    read_image_view_create_info.flags = 0;
    read_image_view_create_info.image = image.image;
    read_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_3D;
    read_image_view_create_info.format = format;
    read_image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    read_image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    read_image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    read_image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    read_image_view_create_info.subresourceRange.aspectMask = aspect_mask;
    read_image_view_create_info.subresourceRange.baseMipLevel = 0;
    read_image_view_create_info.subresourceRange.levelCount = 1;
    read_image_view_create_info.subresourceRange.baseArrayLayer = 0;
    read_image_view_create_info.subresourceRange.layerCount = 1;

    TGVK_CALL(vkCreateImageView(device, &read_image_view_create_info, TG_NULL, &image.read_image_view));

    VkImageViewCreateInfo write_image_view_create_info = { 0 };
    write_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    write_image_view_create_info.pNext = TG_NULL;
    write_image_view_create_info.flags = 0;
    write_image_view_create_info.image = image.image;
    write_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    write_image_view_create_info.format = format;
    write_image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    write_image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    write_image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    write_image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    write_image_view_create_info.subresourceRange.aspectMask = aspect_mask;
    write_image_view_create_info.subresourceRange.baseMipLevel = 0;
    write_image_view_create_info.subresourceRange.levelCount = 1;
    write_image_view_create_info.subresourceRange.baseArrayLayer = 0;
    write_image_view_create_info.subresourceRange.layerCount = layers;

    TGVK_CALL(vkCreateImageView(device, &write_image_view_create_info, TG_NULL, &image.write_image_view));

    if (p_sampler_create_info)
    {
        image.sampler = tgvk_sampler_create2(p_sampler_create_info);
    }
    else
    {
        image.sampler = tgvk_sampler_create();
    }

    return image;
}

void tgvk_layered_image_destroy(tgvk_layered_image* p_image)
{
    if (p_image->sampler.sampler)
    {
        tgvk_sampler_destroy(&p_image->sampler);
    }
    if (p_image->read_image_view)
    {
        vkDestroyImageView(device, p_image->read_image_view, TG_NULL);
    }
    if (p_image->write_image_view)
    {
        vkDestroyImageView(device, p_image->write_image_view, TG_NULL);
    }
    TG_ASSERT(p_image->memory.device_memory);
    tgvk_memory_allocator_free(&p_image->memory);
    TG_ASSERT(p_image->image);
    vkDestroyImage(device, p_image->image, TG_NULL);
}

b32 tgvk_layered_image_store_layer_to_disc(tgvk_layered_image* p_image, u32 layer, const char* p_filename, b32 force_alpha_one, b32 replace_existing)
{
    const u32 width = p_image->width;
    const u32 height = p_image->height;

    tgvk_image blit_image = TGVK_IMAGE_CREATE(TGVK_IMAGE_TYPE_COLOR, width, height, (VkFormat)TG_COLOR_IMAGE_FORMAT_B8G8R8A8_UNORM, TG_NULL);

    const VkDeviceSize staging_buffer_size = (VkDeviceSize)width * (VkDeviceSize)height * tg_color_image_format_size((tg_color_image_format)blit_image.format);
    tgvk_buffer* p_staging_buffer = tg__staging_buffer_take(staging_buffer_size);

    tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
    tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    tgvk_cmd_transition_image_layout(p_command_buffer, &blit_image, TGVK_LAYOUT_UNDEFINED, TGVK_LAYOUT_TRANSFER_WRITE);
    tgvk_cmd_blit_layered_image_layer_to_image(p_command_buffer, layer, p_image, &blit_image, TG_NULL);
    tgvk_cmd_transition_image_layout(p_command_buffer, &blit_image, TGVK_LAYOUT_TRANSFER_WRITE, TGVK_LAYOUT_TRANSFER_READ);
    tgvk_cmd_copy_color_image_to_buffer(p_command_buffer, &blit_image, p_staging_buffer);
    tgvk_command_buffer_end_and_submit(p_command_buffer);
    const b32 result = tg_image_store_to_disc(p_filename, width, height, (tg_color_image_format)blit_image.format, p_staging_buffer->memory.p_mapped_device_memory, force_alpha_one, replace_existing);

    tg__staging_buffer_release();
    tgvk_image_destroy(&blit_image);

    return result;
}



tgvk_pipeline tgvk_pipeline_create_compute(const tgvk_shader* p_compute_shader)
{
    tgvk_pipeline compute_pipeline = { 0 };

    compute_pipeline.is_graphics_pipeline = TG_FALSE;
    compute_pipeline.layout = tg__pipeline_layout_create(1, &p_compute_shader);

    const char* p_name = "main";

    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info = { 0 };
    pipeline_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_shader_stage_create_info.pNext = TG_NULL;
    pipeline_shader_stage_create_info.flags = 0;
    pipeline_shader_stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_shader_stage_create_info.module = p_compute_shader->shader_module;
    pipeline_shader_stage_create_info.pName = p_name;
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

    graphics_pipeline.is_graphics_pipeline = TG_TRUE;
    graphics_pipeline.layout = tg__pipeline_layout_create(3, &p_create_info->p_vertex_shader);

    VkPipelineShaderStageCreateInfo p_pipeline_shader_stage_create_infos[3] = { 0 };

    u32 pipeline_shader_stage_count = 0;

    const char* p_name = "main";

    p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].pNext = TG_NULL;
    p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].flags = 0;
    p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].stage = VK_SHADER_STAGE_VERTEX_BIT;
    p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].module = p_create_info->p_vertex_shader->shader_module;
    p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].pName = p_name;
    p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].pSpecializationInfo = TG_NULL;
    pipeline_shader_stage_count++;

    if (p_create_info->p_geometry_shader)
    {
        p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].pNext = TG_NULL;
        p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].flags = 0;
        p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
        p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].module = p_create_info->p_geometry_shader->shader_module;
        p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].pName = p_name;
        p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].pSpecializationInfo = TG_NULL;
        pipeline_shader_stage_count++;
    }

    p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].pNext = TG_NULL;
    p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].flags = 0;
    p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].module = p_create_info->p_fragment_shader->shader_module;
    p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].pName = p_name;
    p_pipeline_shader_stage_create_infos[pipeline_shader_stage_count].pSpecializationInfo = TG_NULL;
    pipeline_shader_stage_count++;

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
    pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipeline_multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    pipeline_multisample_state_create_info.minSampleShading = 0.0f;
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

    TG_ASSERT(p_create_info->p_fragment_shader->spirv_layout.fragment_shader_output.count <= TG_MAX_SHADER_ATTACHMENTS);

    VkPipelineColorBlendAttachmentState p_pipeline_color_blend_attachment_states[TG_MAX_SHADER_ATTACHMENTS] = { 0 };
    for (u32 i = 0; i < p_create_info->p_fragment_shader->spirv_layout.fragment_shader_output.count; i++)
    {
        if (!p_create_info->p_blend_modes || p_create_info->p_blend_modes[i] == TG_BLEND_MODE_NONE)
        {
            p_pipeline_color_blend_attachment_states[i].blendEnable = VK_FALSE;
            p_pipeline_color_blend_attachment_states[i].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            p_pipeline_color_blend_attachment_states[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            p_pipeline_color_blend_attachment_states[i].colorBlendOp = VK_BLEND_OP_ADD;
            p_pipeline_color_blend_attachment_states[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            p_pipeline_color_blend_attachment_states[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            p_pipeline_color_blend_attachment_states[i].alphaBlendOp = VK_BLEND_OP_ADD;
            p_pipeline_color_blend_attachment_states[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        }
        else if (p_create_info->p_blend_modes[i] == TG_BLEND_MODE_ADD)
        {
            p_pipeline_color_blend_attachment_states[i].blendEnable = VK_TRUE;
            p_pipeline_color_blend_attachment_states[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            p_pipeline_color_blend_attachment_states[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            p_pipeline_color_blend_attachment_states[i].colorBlendOp = VK_BLEND_OP_ADD;
            p_pipeline_color_blend_attachment_states[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            p_pipeline_color_blend_attachment_states[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            p_pipeline_color_blend_attachment_states[i].alphaBlendOp = VK_BLEND_OP_ADD;
            p_pipeline_color_blend_attachment_states[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        }
        else if (p_create_info->p_blend_modes[i] == TG_BLEND_MODE_BLEND)
        {
            p_pipeline_color_blend_attachment_states[i].blendEnable = VK_TRUE;
            p_pipeline_color_blend_attachment_states[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            p_pipeline_color_blend_attachment_states[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            p_pipeline_color_blend_attachment_states[i].colorBlendOp = VK_BLEND_OP_ADD;
            p_pipeline_color_blend_attachment_states[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            p_pipeline_color_blend_attachment_states[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            p_pipeline_color_blend_attachment_states[i].alphaBlendOp = VK_BLEND_OP_ADD;
            p_pipeline_color_blend_attachment_states[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        }
        else
        {
            TG_INVALID_CODEPATH();
        }
    }

    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = { 0 };
    pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipeline_color_blend_state_create_info.pNext = TG_NULL;
    pipeline_color_blend_state_create_info.flags = 0;
    pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
    pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    pipeline_color_blend_state_create_info.attachmentCount = p_create_info->p_fragment_shader->spirv_layout.fragment_shader_output.count;
    pipeline_color_blend_state_create_info.pAttachments = p_pipeline_color_blend_attachment_states;
    pipeline_color_blend_state_create_info.blendConstants[0] = 0.0f;
    pipeline_color_blend_state_create_info.blendConstants[1] = 0.0f;
    pipeline_color_blend_state_create_info.blendConstants[2] = 0.0f;
    pipeline_color_blend_state_create_info.blendConstants[3] = 0.0f;

    VkVertexInputBindingDescription p_vertex_input_binding_descriptions[TG_MAX_SHADER_INPUTS] = { 0 };
    VkVertexInputAttributeDescription p_vertex_input_attribute_descriptions[TG_MAX_SHADER_INPUTS] = { 0 };
    for (u8 i = 0; i < p_create_info->p_vertex_shader->spirv_layout.vertex_shader_input.count; i++)
    {
        p_vertex_input_binding_descriptions[i].binding = i;
        p_vertex_input_binding_descriptions[i].stride = tg_vertex_input_attribute_format_get_size(p_create_info->p_vertex_shader->spirv_layout.vertex_shader_input.p_resources[i].format);
        p_vertex_input_binding_descriptions[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        p_vertex_input_attribute_descriptions[i].binding = i;
        p_vertex_input_attribute_descriptions[i].location = p_create_info->p_vertex_shader->spirv_layout.vertex_shader_input.p_resources[i].location;
        p_vertex_input_attribute_descriptions[i].format = p_create_info->p_vertex_shader->spirv_layout.vertex_shader_input.p_resources[i].format;
        p_vertex_input_attribute_descriptions[i].offset = 0;
    }

    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = { 0 };
    pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipeline_vertex_input_state_create_info.pNext = TG_NULL;
    pipeline_vertex_input_state_create_info.flags = 0;
    pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = p_create_info->p_vertex_shader->spirv_layout.vertex_shader_input.count;
    pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = p_vertex_input_binding_descriptions;
    pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = p_create_info->p_vertex_shader->spirv_layout.vertex_shader_input.count;
    pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = p_vertex_input_attribute_descriptions;

    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_create_info.pNext = TG_NULL;
    graphics_pipeline_create_info.flags = 0;
    graphics_pipeline_create_info.stageCount = pipeline_shader_stage_count;
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



VkRenderPass tgvk_render_pass_create(const VkAttachmentDescription* p_attachment_description, const VkSubpassDescription* p_subpass_description)
{
    TG_ASSERT(!p_subpass_description->inputAttachmentCount);
    TG_ASSERT(!p_subpass_description->pInputAttachments);
    TG_ASSERT(!p_subpass_description->pResolveAttachments);
    TG_ASSERT(!p_subpass_description->preserveAttachmentCount);
    TG_ASSERT(!p_subpass_description->pPreserveAttachments);

    VkRenderPass render_pass = VK_NULL_HANDLE;

    VkRenderPassCreateInfo render_pass_create_info = { 0 };
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = TG_NULL;
    render_pass_create_info.flags = 0;
    render_pass_create_info.attachmentCount = p_subpass_description->colorAttachmentCount + (u32)(p_subpass_description->pDepthStencilAttachment != TG_NULL);
    render_pass_create_info.pAttachments = p_attachment_description;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = p_subpass_description;
    render_pass_create_info.dependencyCount = 0;
    render_pass_create_info.pDependencies = TG_NULL;

    TGVK_CALL(vkCreateRenderPass(device, &render_pass_create_info, TG_NULL, &render_pass));

    return render_pass;
}

void tgvk_render_pass_destroy(VkRenderPass render_pass)
{
    vkDestroyRenderPass(device, render_pass, TG_NULL);
}



tgvk_sampler tgvk_sampler_create(void)
{
    tgvk_sampler sampler = { 0 };

    tgvk_sampler_create_info sampler_create_info = { 0 };
    sampler_create_info.min_filter = VK_FILTER_LINEAR;
    sampler_create_info.mag_filter = VK_FILTER_LINEAR;
    sampler_create_info.address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    sampler = tgvk_sampler_create2(&sampler_create_info);

    return sampler;
}

tgvk_sampler tgvk_sampler_create2(const tgvk_sampler_create_info* p_sampler_create_info)
{
    tgvk_sampler sampler = { 0 };
    sampler.mag_filter = (VkFilter)p_sampler_create_info->mag_filter;
    sampler.min_filter = (VkFilter)p_sampler_create_info->min_filter;
    sampler.address_mode_u = (VkSamplerAddressMode)p_sampler_create_info->address_mode_u;
    sampler.address_mode_v = (VkSamplerAddressMode)p_sampler_create_info->address_mode_v;
    sampler.address_mode_w = (VkSamplerAddressMode)p_sampler_create_info->address_mode_w;

    VkSamplerCreateInfo sampler_create_info = { 0 };
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.pNext = TG_NULL;
    sampler_create_info.flags = 0;
    sampler_create_info.magFilter = p_sampler_create_info->mag_filter;
    sampler_create_info.minFilter = p_sampler_create_info->min_filter;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.addressModeU = sampler.address_mode_u;
    sampler_create_info.addressModeV = sampler.address_mode_v;
    sampler_create_info.addressModeW = sampler.address_mode_w;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.anisotropyEnable = VK_TRUE;
    sampler_create_info.maxAnisotropy = 16.0f;
    sampler_create_info.compareEnable = VK_FALSE;
    sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = 1.0f;
    sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;

    TGVK_CALL(vkCreateSampler(device, &sampler_create_info, TG_NULL, &sampler.sampler));

    return sampler;
}

void tgvk_sampler_destroy(tgvk_sampler* p_sampler)
{
    vkDestroySampler(device, p_sampler->sampler, TG_NULL);
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



#ifndef TG_DISTRIBUTE
void tg__shader_requires_recompilation(const char* p_glsl_filename, const char* p_spirv_filename, const char* p_glsl_source, TG_OUT b32* p_regenerate_and_recompile_glsl, TG_OUT tg_size* p_max_generated_glsl_size)
{
    *p_regenerate_and_recompile_glsl = TG_FALSE;
    *p_max_generated_glsl_size = 0;

    tg_file_properties glsl_properties = { 0 };
    const b32 get_glsl_properties_result = tgp_file_get_properties(p_glsl_filename, &glsl_properties);
    TG_ASSERT2(get_glsl_properties_result, "The shader with given filename does not exist");

    tg_file_properties properties_spirv = { 0 };
    const b32 get_spirv_properties_result = tgp_file_get_properties(p_spirv_filename, &properties_spirv);
    if (!get_spirv_properties_result || tgp_system_time_compare(&properties_spirv.last_write_time, &glsl_properties.last_write_time) == -1)
    {
        *p_regenerate_and_recompile_glsl = TG_TRUE;
    }

    *p_max_generated_glsl_size = glsl_properties.size;

    const char* p_it = p_glsl_source;
    while (*p_it != '\0')
    {
        if (tg_string_starts_with(p_it, "#include"))
        {
            char p_inc_filename[TG_MAX_PATH] = { 0 };
            p_it += sizeof("#include") - 1;
            p_it = tg_string_skip_whitespace(p_it);
            TG_ASSERT2(*p_it == '\"', "The included filename must be surrounded by quotation marks");
            p_it++;

            u32 inc_filename_char_idx = 0;
            while (*p_it != '\"')
            {
                p_inc_filename[inc_filename_char_idx++] = *p_it++;
            }

            tg_file_properties inc_properties = { 0 };
            const b32 get_inc_properties_result = tgp_file_get_properties(p_inc_filename, &inc_properties);
            TG_ASSERT2(get_inc_properties_result, "The included file does not exist");

            if (tgp_system_time_compare(&properties_spirv.last_write_time, &inc_properties.last_write_time) == -1)
            {
                *p_regenerate_and_recompile_glsl = TG_TRUE;
            }

            *p_max_generated_glsl_size += inc_properties.size;
        }
        p_it = tg_string_next_line(p_it);
    }
}

tg_size tg__shader_generate_glsl(const char* p_glsl_source, tg_size generated_glsl_buffer_size, TG_OUT char* p_glsl_generated_buffer, TG_OUT u32* p_instanced_location_count, TG_OUT b32* p_instanced_locations)
{
    TG_ASSERT(*p_instanced_locations == 0);

    char p_inc_filename_buffer[TG_MAX_PATH] = { 0 };

    const char* p_it = p_glsl_source;
    char* p_generated_it = p_glsl_generated_buffer;

    while (*p_it != '\0')
    {
        if (tg_string_starts_with(p_it, "#include"))
        {
            p_it = tg_string_skip_whitespace(&p_it[sizeof("#include") - 1]);
            TG_ASSERT2(*p_it == '\"', "The included filename must be surrounded by quotation marks");
            p_it++;

            u32 idx = 0;
            while (*p_it != '\"')
            {
                p_inc_filename_buffer[idx++] = *p_it++;
            }
            p_inc_filename_buffer[idx] = '\0';

            tg_file_properties inc_properties = { 0 };
            const b32 get_inc_properties_result = tgp_file_get_properties(p_inc_filename_buffer, &inc_properties);
            TG_ASSERT2(get_inc_properties_result, "The included file doest not exist");

            const b32 load_file_result = tgp_file_load(p_inc_filename_buffer, inc_properties.size, p_generated_it);
            TG_ASSERT2(load_file_result, "The included file could not be loaded");

            p_generated_it += inc_properties.size;
            p_it = tg_string_next_line(p_it);
        }
        else if (tg_string_starts_with(p_it, "//"))
        {
            p_it = tg_string_next_line(p_it);
        }
        else
        {
            u32 location = TG_U32_MAX;
            do
            {
                if (tg_string_starts_with(p_it, "location"))
                {
                    // Determine location
                    const char* p_location_it = p_it;
                    p_location_it += sizeof("location") - 1;
                    p_location_it = tg_string_skip_whitespace(p_location_it);
                    TG_ASSERT(*p_location_it == '=');
                    p_location_it++;
                    p_location_it = tg_string_skip_whitespace(p_location_it);
                    location = tg_string_to_u32(p_location_it);
                }
                // Cut instance annotation from source
                else if (tg_string_starts_with(p_it, "#instance"))
                {
                    TG_ASSERT(location != TG_U32_MAX);
                    TG_ASSERT(location < TG_MAX_SHADER_INPUTS);
                    (*p_instanced_location_count)++;
                    p_instanced_locations[location] = TG_TRUE;
                    // Remove whitespace, simply for visual perfection!
                    while (*(p_generated_it - 1) == ' ')
                    {
                        p_generated_it--;
                    }
                    *p_generated_it++ = '\r';
                    *p_generated_it++ = '\n';
                    p_it = tg_string_next_line(p_it);
                }

                *p_generated_it++ = *p_it;
            } while (*p_it++ != '\n');
        }
    }
    const tg_size generated_glsl_size = (tg_size)(p_generated_it - p_glsl_generated_buffer);
    TG_ASSERT(generated_glsl_size <= generated_glsl_buffer_size);
    return generated_glsl_size;
}
#endif

tgvk_shader tgvk_shader_create(const char* p_filename)
{
    char p_spirv_filename_buffer[TG_MAX_PATH] = { 0 };
    tg_stringf(sizeof(p_spirv_filename_buffer), p_spirv_filename_buffer, "%s.spv", p_filename);

#ifndef TG_DISTRIBUTE

    tg_file_properties properties = { 0 };
    const b32 get_uncompiled_properties_result = tgp_file_get_properties(p_filename, &properties);
    TG_ASSERT2(get_uncompiled_properties_result, "The shader with given filename does not exist");

    char* p_glsl_source = TG_MALLOC_STACK(properties.size + 1);
    tgp_file_load(p_filename, properties.size + 1, p_glsl_source);
    p_glsl_source[properties.size] = '\0';

    b32 regenerate_and_recompile_glsl;
    tg_size generated_glsl_buffer_size;
    tg__shader_requires_recompilation(p_filename, p_spirv_filename_buffer, p_glsl_source, &regenerate_and_recompile_glsl, &generated_glsl_buffer_size);

    if (regenerate_and_recompile_glsl)
    {
        char* p_glsl_generated_buffer = TG_MALLOC_STACK(generated_glsl_buffer_size);
        u32 instanced_location_count = 0;
        b32 p_instanced_locations[TG_MAX_SHADER_INPUTS] = { 0 };
        const tg_size generated_glsl_size = tg__shader_generate_glsl(p_glsl_source, generated_glsl_buffer_size, p_glsl_generated_buffer, &instanced_location_count, p_instanced_locations);

        TG_ASSERT(shaderc_compiler);

        shaderc_shader_kind shader_kind = 0;
        if (tg_string_equal(properties.p_extension, "vert"))
        {
            shader_kind = shaderc_vertex_shader;
        }
        else if (tg_string_equal(properties.p_extension, "geom"))
        {
            shader_kind = shaderc_geometry_shader;
        }
        else if (tg_string_equal(properties.p_extension, "frag"))
        {
            shader_kind = shaderc_fragment_shader;
        }
        else if (tg_string_equal(properties.p_extension, "comp"))
        {
            shader_kind = shaderc_compute_shader;
        }
        else
        {
            TG_INVALID_CODEPATH();
        }

        const shaderc_compilation_result_t result = shaderc_compile_into_spv(shaderc_compiler, p_glsl_generated_buffer, generated_glsl_size, shader_kind, "", "main", TG_NULL);
        TG_ASSERT(result);

#ifdef TG_DEBUG
        if (shaderc_result_get_num_warnings(result) || shaderc_result_get_num_errors(result))
        {
            const char* p_error_message = shaderc_result_get_error_message(result);
            char p_buffer[4096] = { 0 };
            const u32 line = tg_string_to_u32(p_error_message + 1);
            char p_line_buffer[1024] = { 0 };
            const char* p_line = p_glsl_generated_buffer;
            for (u32 i = 1; i < line; i++)
            {
                p_line = tg_string_next_line(p_line);
            }
            p_line = tg_string_skip_whitespace(p_line);
            tg_strcpy_line(sizeof(p_line_buffer), p_line_buffer, p_line);
            tg_stringf(sizeof(p_buffer), p_buffer, "Error compiling \"%s\":\n%sCode at %u: %s", p_filename, p_error_message, line, p_line_buffer);
            TG_DEBUG_LOG(p_buffer);
            TG_INVALID_CODEPATH();
        }
#endif

        const tg_size spirv_size = (tg_size)shaderc_result_get_length(result);
        const char* p_spirv_data = shaderc_result_get_bytes(result);

        // TODO: Insert OpString and read that in tg_spirv.c
        //const tg_size spirv_buffer_size = spirv_size + instanced_location_count * (3 + );
        //char* p_spirv_buffer

        tgp_file_create(p_spirv_filename_buffer, spirv_size, p_spirv_data, TG_TRUE);

        shaderc_result_release(result);
        TG_FREE_STACK(generated_glsl_buffer_size);
    }

    TG_FREE_STACK(properties.size + 1);
#endif

    tg_file_properties file_properties = { 0 };
    const b32 get_properties_result = tgp_file_get_properties(p_spirv_filename_buffer, &file_properties);
    TG_ASSERT(get_properties_result);

    char* p_content = TG_MALLOC_STACK(file_properties.size);
    tgp_file_load(p_spirv_filename_buffer, file_properties.size, p_content);

    const tgvk_shader shader = tgvk_shader_create_from_spirv((u32)file_properties.size, p_content);

    TG_FREE_STACK(file_properties.size);

    return shader;
}

tgvk_shader tgvk_shader_create_from_glsl(tg_shader_type type, const char* p_source)
{
    TG_ASSERT(shaderc_compiler);

    TG_INVALID_CODEPATH(); // TODO: we need to abstract to the functions that 'tgvk_shader_create' uses!

    const char* p_it = p_source;
    tg_size generated_size_upper_bound = 0;
    b32 contains_includes = TG_FALSE;
    while (*p_it != '\0')
    {
        if (tg_string_starts_with(p_it, "#include"))
        {
            contains_includes = TG_TRUE;

            char p_inc_filename[TG_MAX_PATH] = { 0 };
            p_it += sizeof("#include") - 1;
            p_it = tg_string_skip_whitespace(p_it);
            TG_ASSERT2(*p_it == '\"', "The included filename must be surrounded by quotation marks");
            p_it++;

            u32 idx = 0;
            while (*p_it != '\"')
            {
                p_inc_filename[idx++] = *p_it++;
            }

            tg_file_properties inc_properties = { 0 };
            const b32 get_inc_properties_result = tgp_file_get_properties(p_inc_filename, &inc_properties);
            TG_ASSERT2(get_inc_properties_result, "The included file does not exist");

            generated_size_upper_bound += inc_properties.size;
            p_it = tg_string_next_line(p_it);
        }
        else
        {
            do
            {
                generated_size_upper_bound++;
            } while (*p_it++ != '\n');
        }
    }

    const char* p_generated_src = p_source;
    tg_size generated_size = generated_size_upper_bound;
    TG_ASSERT(contains_includes || tg_strlen_no_nul(p_source) == generated_size_upper_bound);

    if (contains_includes)
    {
        p_it = p_source;
        char* p_generated_it = TG_MALLOC_STACK(generated_size_upper_bound);
        p_generated_src = p_generated_it;

        while (*p_it != '\0')
        {
            if (tg_string_starts_with(p_it, "#include"))
            {
                TG_INVALID_CODEPATH(); // TODO: untested and replicated
                
                char p_inc_filename[TG_MAX_PATH] = { 0 };
                p_it += sizeof("#include") - 1;
                p_it = tg_string_skip_whitespace(p_it);
                TG_ASSERT2(*p_it == '\"', "The included filename must be surrounded by quotation marks");
                p_it++;

                u32 idx = 0;
                while (*p_it != '\"')
                {
                    p_inc_filename[idx++] = *p_it++;
                }

                tg_file_properties inc_properties = { 0 };
                const b32 get_inc_properties_result = tgp_file_get_properties(p_inc_filename, &inc_properties);
                TG_ASSERT2(get_inc_properties_result, "The included file doest not exist");

                const b32 load_file_result = tgp_file_load(p_inc_filename, inc_properties.size, p_generated_it);
                TG_ASSERT2(load_file_result, "The included file could not be loaded");
                p_generated_it += inc_properties.size;

                p_it = tg_string_next_line(p_it);
            }
            else
            {
                do
                {
                    *p_generated_it++ = *p_it;
                } while (*p_it++ != '\n');
            }
        }
        generated_size = (tg_size)(p_generated_it - p_generated_src);
    }

    shaderc_shader_kind shader_kind = 0;
    switch (type)
    {
    case TG_SHADER_TYPE_VERTEX: shader_kind = shaderc_vertex_shader; break;
    case TG_SHADER_TYPE_GEOMETRY: shader_kind = shaderc_geometry_shader; break;
    case TG_SHADER_TYPE_FRAGMENT: shader_kind = shaderc_fragment_shader; break;
    case TG_SHADER_TYPE_COMPUTE: shader_kind = shaderc_compute_shader; break;

    default: TG_INVALID_CODEPATH(); break;
    }
    const shaderc_compilation_result_t result = shaderc_compile_into_spv(shaderc_compiler, p_generated_src, generated_size, shader_kind, "", "main", TG_NULL);
    TG_ASSERT(result);

#ifdef TG_DEBUG
    if (shaderc_result_get_num_warnings(result) || shaderc_result_get_num_errors(result))
    {
        const char* p_error_message = shaderc_result_get_error_message(result);
        TG_DEBUG_LOG(p_error_message);
        TG_INVALID_CODEPATH();
    }
#endif

    const size_t length = shaderc_result_get_length(result);
    const char* p_bytes = shaderc_result_get_bytes(result);

    const tgvk_shader shader = tgvk_shader_create_from_spirv((u32)length, p_bytes);

    shaderc_result_release(result);

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



tgvk_buffer tgvk_storage_buffer_create(VkDeviceSize size, b32 visible TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename))
{
    tgvk_buffer buffer = tgvk_buffer_create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, visible ? TGVK_MEMORY_HOST : TGVK_MEMORY_DEVICE TG_DEBUG_PARAM(line) TG_DEBUG_PARAM(p_filename));
    return buffer;
}



tgvk_buffer tgvk_uniform_buffer_create(VkDeviceSize size TG_DEBUG_PARAM(u32 line) TG_DEBUG_PARAM(const char* p_filename))
{
    tgvk_buffer buffer = tgvk_buffer_create(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, TGVK_MEMORY_HOST TG_DEBUG_PARAM(line) TG_DEBUG_PARAM(p_filename));
    return buffer;
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
    u32 queue_family_property_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &queue_family_property_count, TG_NULL);
    if (queue_family_property_count == 0)
    {
        return TG_FALSE;
    }

    const tg_size queue_family_properties_size = (tg_size)queue_family_property_count * sizeof(VkQueueFamilyProperties);
    VkQueueFamilyProperties* p_queue_family_properties = TG_MALLOC_STACK(queue_family_properties_size);
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

    TG_FREE_STACK(queue_family_properties_size);
    const b32 result = supports_graphics_family && supports_present_family && supports_compute_family;
    return result;
}

static void tg__physical_device_find_queue_family_indices(void)
{
    TG_ASSERT(tg__physical_device_supports_required_queue_families(physical_device));

    p_queues[TGVK_QUEUE_TYPE_COMPUTE].queue_family_index = TG_U32_MAX;
    p_queues[TGVK_QUEUE_TYPE_GRAPHICS].queue_family_index = TG_U32_MAX;
    p_queues[TGVK_QUEUE_TYPE_PRESENT].queue_family_index = TG_U32_MAX;
    p_queues[TGVK_QUEUE_TYPE_COMPUTE_LOW_PRIORITY].queue_family_index = TG_U32_MAX;
    p_queues[TGVK_QUEUE_TYPE_GRAPHICS_LOW_PRIORITY].queue_family_index = TG_U32_MAX;

    u32 queue_family_property_count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_property_count, TG_NULL);
    TG_ASSERT(queue_family_property_count);
    const tg_size queue_family_properties_size = (tg_size)queue_family_property_count * sizeof(VkQueueFamilyProperties);
    VkQueueFamilyProperties* p_queue_family_properties = TG_MALLOC_STACK(queue_family_properties_size);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_property_count, p_queue_family_properties);

    b32 resolved = TG_FALSE;
    u32 best_idx = TG_U32_MAX;
    u32 best_count = 0;
    for (u32 i = 0; i < queue_family_property_count; i++)
    {
        const b32 supports_graphics_family = (p_queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
        VkBool32 spf = VK_FALSE;
        TGVK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface.surface, &spf));
        const b32 supports_present_family = spf != 0;
        const b32 supports_compute_family = (p_queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
    
        if (supports_graphics_family && supports_present_family && supports_compute_family)
        {
            if (p_queue_family_properties[i].queueCount >= TGVK_QUEUE_TYPE_COUNT)
            {
                p_queues[TGVK_QUEUE_TYPE_COMPUTE].queue_family_index = i;
                p_queues[TGVK_QUEUE_TYPE_GRAPHICS].queue_family_index = i;
                p_queues[TGVK_QUEUE_TYPE_PRESENT].queue_family_index = i;
                p_queues[TGVK_QUEUE_TYPE_COMPUTE_LOW_PRIORITY].queue_family_index = i;
                p_queues[TGVK_QUEUE_TYPE_GRAPHICS_LOW_PRIORITY].queue_family_index = i;

                resolved = TG_TRUE;
                break;
            }
            else if (p_queue_family_properties[i].queueCount > best_count)
            {
                best_idx = i;
                best_count = p_queue_family_properties[i].queueCount;
            }
        }
    }

    if (!resolved)
    {
        if (best_idx != TG_U32_MAX)
        {
            resolved = TG_TRUE;

            p_queues[TGVK_QUEUE_TYPE_COMPUTE].queue_family_index = best_idx;
            p_queues[TGVK_QUEUE_TYPE_GRAPHICS].queue_family_index = best_idx;
            p_queues[TGVK_QUEUE_TYPE_PRESENT].queue_family_index = best_idx;
            p_queues[TGVK_QUEUE_TYPE_COMPUTE_LOW_PRIORITY].queue_family_index = best_idx;
            p_queues[TGVK_QUEUE_TYPE_GRAPHICS_LOW_PRIORITY].queue_family_index = best_idx;
        }
        else
        {
            b32 supports_compute_family = TG_FALSE;
            b32 supports_graphics_family = TG_FALSE;
            b32 supports_present_family = TG_FALSE;

            u32 best_compute_idx = TG_U32_MAX;
            u32 best_graphics_idx = TG_U32_MAX;

            u32 best_compute_count = 0;
            u32 best_graphics_count = 0;

            for (u32 i = 0; i < queue_family_property_count; i++)
            {
                if (!supports_compute_family && (p_queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0)
                {
                    if (p_queue_family_properties[i].queueCount >= 2)
                    {
                        supports_compute_family = TG_TRUE;
                        p_queues[TGVK_QUEUE_TYPE_COMPUTE].queue_family_index = i;
                        p_queues[TGVK_QUEUE_TYPE_COMPUTE_LOW_PRIORITY].queue_family_index = i;
                    }
                    else
                    {
                        best_compute_idx = i;
                        best_compute_count = p_queue_family_properties[i].queueCount;
                    }
                }
                if (!supports_graphics_family && (p_queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
                {
                    if (p_queue_family_properties[i].queueCount >= 2)
                    {
                        supports_graphics_family = TG_TRUE;
                        p_queues[TGVK_QUEUE_TYPE_GRAPHICS].queue_family_index = i;
                        p_queues[TGVK_QUEUE_TYPE_GRAPHICS_LOW_PRIORITY].queue_family_index = i;
                    }
                    else
                    {
                        best_graphics_idx = i;
                        best_graphics_count = p_queue_family_properties[i].queueCount;
                    }
                }
                if (!supports_present_family)
                {
                    VkBool32 spf = VK_FALSE;
                    TGVK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface.surface, &spf));
                    if (spf == VK_TRUE)
                    {
                        supports_present_family = TG_TRUE;
                        p_queues[TGVK_QUEUE_TYPE_PRESENT].queue_family_index = i;
                    }
                }

                if (supports_graphics_family && supports_present_family && supports_compute_family)
                {
                    resolved = TG_TRUE;
                    break;
                }
            }

            if (!resolved)
            {
                if (!supports_compute_family)
                {
                    TG_ASSERT(best_compute_idx != TG_U32_MAX);
                    p_queues[TGVK_QUEUE_TYPE_COMPUTE].queue_family_index = best_compute_idx;
                    p_queues[TGVK_QUEUE_TYPE_COMPUTE_LOW_PRIORITY].queue_family_index = best_compute_idx;
                }
                if (!supports_graphics_family)
                {
                    TG_ASSERT(best_graphics_idx != TG_U32_MAX);
                    p_queues[TGVK_QUEUE_TYPE_GRAPHICS].queue_family_index = best_graphics_idx;
                    p_queues[TGVK_QUEUE_TYPE_GRAPHICS_LOW_PRIORITY].queue_family_index = best_graphics_idx;
                }
                TG_ASSERT(supports_present_family);
            }
        }
    }

    TG_ASSERT(resolved);
    TG_FREE_STACK(queue_family_properties_size);
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
    const tg_size device_extension_properties_size = (tg_size)device_extension_property_count * sizeof(VkExtensionProperties);
    VkExtensionProperties* device_extension_properties = TG_MALLOC_STACK(device_extension_properties_size);
    TGVK_CALL(vkEnumerateDeviceExtensionProperties(pd, TG_NULL, &device_extension_property_count, device_extension_properties));

    b32 supports_extensions = TG_TRUE;
    const char* pp_device_extension_names[] = TGVK_DEVICE_EXTENSION_NAMES;
    for (u32 i = 0; i < TGVK_DEVICE_EXTENSION_COUNT; i++)
    {
        b32 supports_extension = TG_FALSE;
        for (u32 j = 0; j < device_extension_property_count; j++)
        {
            if (tg_string_equal(pp_device_extension_names[i], device_extension_properties[j].extensionName))
            {
                supports_extension = TG_TRUE;
                break;
            }
        }
        supports_extensions &= supports_extension;
    }

    TG_FREE_STACK(device_extension_properties_size);
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

    VkPhysicalDeviceFeatures2 physical_device_features_2 = { 0 };
    physical_device_features_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    vkGetPhysicalDeviceFeatures2(pd, &physical_device_features_2);
    b32 supports_shader_buffer_int64_atomics = physical_device_features_2.features.shaderInt64 == VK_TRUE;
    const VkPhysicalDeviceFeatures2* p_features_it = &physical_device_features_2;
    while (p_features_it)
    {
        if (p_features_it->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES)
        {
            const VkPhysicalDeviceShaderAtomicInt64Features* p_shader_atomic_int64_features = (const VkPhysicalDeviceShaderAtomicInt64Features*) p_features_it;
            supports_shader_buffer_int64_atomics &= p_shader_atomic_int64_features->shaderBufferInt64Atomics == VK_TRUE;
            break;
        }
        p_features_it = p_features_it->pNext;
    }

    const b32 supports_1024_compute_work_groups = physical_device_properties.limits.maxComputeWorkGroupInvocations >= 1024;

    if (!tg__physical_device_supports_extension(pd) ||
        !tg__physical_device_supports_required_queue_families(pd) ||
        !physical_device_surface_format_count ||
        !physical_device_present_mode_count ||
        !supports_shader_buffer_int64_atomics ||
        !supports_1024_compute_work_groups)
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
        const tg_size layer_properties_size = (tg_size)layer_property_count * sizeof(VkLayerProperties);
        VkLayerProperties* p_layer_properties = TG_MALLOC_STACK(layer_properties_size);
        vkEnumerateInstanceLayerProperties(&layer_property_count, p_layer_properties);

        const char* p_validation_layer_names[] = TGVK_VALIDATION_LAYER_NAMES;
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
        TG_FREE_STACK(layer_properties_size);
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
    const char* pp_enabled_extension_names[] = TGVK_INSTANCE_EXTENSION_NAMES;
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
    win32_surface_create_info.hwnd = tgp_get_window_handle();

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
    const tg_size physical_devices_size = (tg_size)physical_device_count * sizeof(VkPhysicalDevice);
    VkPhysicalDevice* p_physical_devices = TG_MALLOC_STACK(physical_devices_size);
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

    TG_FREE_STACK(physical_devices_size);

    return pd;
}

static VkDevice tg__device_create(void)
{
    VkDevice d = VK_NULL_HANDLE;

    const f32 p_queue_priorities[TGVK_QUEUE_TYPE_COUNT] = { 1.0f, 1.0f, 1.0f, 0.5f, 0.5f };
    f32 pp_queue_priorities[TGVK_QUEUE_TYPE_COUNT][TGVK_QUEUE_TYPE_COUNT] = { 0 };
    VkDeviceQueueCreateInfo p_device_queue_create_infos[TGVK_QUEUE_TYPE_COUNT] = { 0 };

    tg__physical_device_find_queue_family_indices();

    for (u8 i = 0; i < TGVK_QUEUE_TYPE_COUNT; i++)
    {
        p_device_queue_create_infos[i].queueFamilyIndex = TG_U32_MAX;
    }

    // TODO: inline 'tg__physical_device_find_queue_family_indices' to remove redundant code below
    u32 queue_family_property_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_property_count, TG_NULL);
    TG_ASSERT(queue_family_property_count);
    const tg_size queue_family_properties_size = (tg_size)queue_family_property_count * sizeof(VkQueueFamilyProperties);
    VkQueueFamilyProperties* p_queue_family_properties = TG_MALLOC_STACK(queue_family_properties_size);

    u32 device_queue_create_info_count = 0;
    for (u8 i = 0; i < TGVK_QUEUE_TYPE_COUNT; i++)
    {
        p_queues[i].priority = p_queue_priorities[i];

        b32 queue_submitted = TG_FALSE;
        for (u8 j = 0; j < device_queue_create_info_count; j++)
        {
            VkDeviceQueueCreateInfo* p_info = &p_device_queue_create_infos[j];

            if (p_info->queueFamilyIndex == p_queues[i].queue_family_index)
            {
                queue_submitted = TG_TRUE;
                b32 priority_submitted = TG_FALSE;
                for (u32 k = 0; k < TGVK_QUEUE_TYPE_COUNT; k++)
                {
                    if (p_info->pQueuePriorities[k] == p_queue_priorities[i])
                    {
                        priority_submitted = TG_TRUE;
                        p_queues[i].queue_index = k;
                        break;
                    }
                }
                if (!priority_submitted)
                {
                    if (p_info->queueCount < p_queue_family_properties[p_info->queueFamilyIndex].queueCount)
                    {
                        pp_queue_priorities[j][p_info->queueCount] = p_queue_priorities[i];
                        p_queues[i].queue_index = p_info->queueCount++;
                    }
                    else
                    {
                        p_queues[i].queue_index = p_info->queueCount - 1;
                        p_queues[i].priority = pp_queue_priorities[j][p_queues[i].queue_index];
                    }
                }
                break;
            }
        }

        if (!queue_submitted)
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

    TG_FREE_STACK(queue_family_properties_size);

    VkPhysicalDeviceFeatures physical_device_features = { 0 };
    physical_device_features.robustBufferAccess = VK_FALSE;
    physical_device_features.fullDrawIndexUint32 = VK_FALSE;
    physical_device_features.imageCubeArray = VK_FALSE;
    physical_device_features.independentBlend = VK_TRUE;
    physical_device_features.geometryShader = VK_TRUE;
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
    physical_device_features.shaderInt64 = VK_TRUE;
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
    const char* pp_enabled_layer_names[] = TGVK_VALIDATION_LAYER_NAMES;
#else
    const char** pp_enabled_layer_names = TG_NULL;
#endif
#if TGVK_DEVICE_EXTENSION_COUNT
    const char* pp_enabled_extension_names[] = TGVK_DEVICE_EXTENSION_NAMES;
#else
    const char** pp_enabled_extension_names = TG_NULL;
#endif

    VkPhysicalDeviceShaderAtomicInt64Features physical_device_shader_atomic_int64_features = { 0 };
    physical_device_shader_atomic_int64_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES;
    physical_device_shader_atomic_int64_features.pNext = TG_NULL;
    physical_device_shader_atomic_int64_features.shaderBufferInt64Atomics = VK_TRUE;
    physical_device_shader_atomic_int64_features.shaderSharedInt64Atomics = VK_FALSE;

    VkDeviceCreateInfo device_create_info = { 0 };
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = &physical_device_shader_atomic_int64_features;
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
    const tg_size surface_formats_size = (tg_size)surface_format_count * sizeof(VkSurfaceFormatKHR);
    VkSurfaceFormatKHR* p_surface_formats = TG_MALLOC_STACK(surface_formats_size);
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
    TG_FREE_STACK(surface_formats_size);

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

    tgp_get_window_size(&swapchain_extent.width, &swapchain_extent.height);
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
        VkImageViewCreateInfo image_view_create_info = { 0 };
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.pNext = TG_NULL;
        image_view_create_info.flags = 0;
        image_view_create_info.image = p_swapchain_images[i];
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        image_view_create_info.format = surface.format.format;
        image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;

        TGVK_CALL(vkCreateImageView(device, &image_view_create_info, TG_NULL, &p_swapchain_image_views[i]));
    }
}

static void tg__screen_quad_create(void)
{
    const u16 p_indices[6] = { 0, 1, 2, 2, 3, 0 };

    v2 p_positions[4] = { 0 };
    p_positions[0] = (v2){ -1.0f,  1.0f };
    p_positions[1] = (v2){ 1.0f,  1.0f };
    p_positions[2] = (v2){ 1.0f, -1.0f };
    p_positions[3] = (v2){ -1.0f, -1.0f };

    v2 p_uvs[4] = { 0 };
    p_uvs[0] = (v2){ 0.0f,  1.0f };
    p_uvs[1] = (v2){ 1.0f,  1.0f };
    p_uvs[2] = (v2){ 1.0f,  0.0f };
    p_uvs[3] = (v2){ 0.0f,  0.0f };

    screen_quad_ibo = TGVK_BUFFER_CREATE(sizeof(p_indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, TGVK_MEMORY_DEVICE);
    screen_quad_positions_vbo = TGVK_BUFFER_CREATE(sizeof(p_positions), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, TGVK_MEMORY_DEVICE);
    screen_quad_uvs_vbo = TGVK_BUFFER_CREATE(sizeof(p_uvs), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, TGVK_MEMORY_DEVICE);

    const tg_size staging_buffer_size = TG_MAX3(sizeof(p_indices), sizeof(p_positions), sizeof(p_uvs));
    tgvk_buffer* p_staging_buffer = tg__staging_buffer_take(staging_buffer_size);

    tg_memcpy(sizeof(p_indices), p_indices, p_staging_buffer->memory.p_mapped_device_memory);
    tgvk_buffer_copy(sizeof(p_indices), p_staging_buffer, &screen_quad_ibo);

    tg_memcpy(sizeof(p_positions), p_positions, p_staging_buffer->memory.p_mapped_device_memory);
    tgvk_buffer_copy(sizeof(p_positions), p_staging_buffer, &screen_quad_positions_vbo);

    tg_memcpy(sizeof(p_uvs), p_uvs, p_staging_buffer->memory.p_mapped_device_memory);
    tgvk_buffer_copy(sizeof(p_uvs), p_staging_buffer, &screen_quad_uvs_vbo);

    tg__staging_buffer_release();
}

static void tg__screen_quad_destroy(void)
{
    tgvk_buffer_destroy(&screen_quad_uvs_vbo);
    tgvk_buffer_destroy(&screen_quad_positions_vbo);
    tgvk_buffer_destroy(&screen_quad_ibo);
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
        TGVK_CALL(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &p_global_compute_command_buffers[i].buffer));

        command_buffer_allocate_info.commandPool = p_graphics_command_pools[i];
        TGVK_CALL(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &p_global_graphics_command_buffers[i].buffer));
    }

    tg__swapchain_create();

    tgvk_memory_allocator_init(device, physical_device);
    global_staging_buffer_lock = TG_RWL_CREATE();
    internal_staging_buffer_lock = TG_RWL_CREATE();

    shaderc_compiler = shaderc_compiler_initialize();
    tgvk_shader_library_init();
    tg__screen_quad_create();
}

void tg_graphics_wait_idle(void)
{
    TGVK_CALL(vkDeviceWaitIdle(device));
}

void tg_graphics_shutdown(void)
{
    tg__screen_quad_destroy();
    tgvk_shader_library_shutdown();
    shaderc_compiler_release(shaderc_compiler);

    tgvk_buffer_destroy(&internal_staging_buffer);
    tgvk_buffer_destroy(&global_staging_buffer);
    tgvk_memory_allocator_shutdown(device);

    for (u32 i = 0; i < TG_MAX_SWAPCHAIN_IMAGES; i++)
    {
        vkDestroyImageView(device, p_swapchain_image_views[i], TG_NULL);
    }
    vkDestroySwapchainKHR(device, swapchain, TG_NULL);

    for (u32 i = 0; i < TG_MAX_THREADS; i++)
    {
        vkFreeCommandBuffers(device, p_graphics_command_pools[i], 1, &p_global_graphics_command_buffers[i].buffer);
        vkFreeCommandBuffers(device, p_compute_command_pools[i], 1, &p_global_compute_command_buffers[i].buffer);
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
    TG_ASSERT(false);
    vkDeviceWaitIdle(device);

    for (u32 i = 0; i < TG_MAX_SWAPCHAIN_IMAGES; i++)
    {
        vkDestroyImageView(device, p_swapchain_image_views[i], TG_NULL);
    }
    vkDestroySwapchainKHR(device, swapchain, TG_NULL);
    tg__swapchain_create();
}

#endif
