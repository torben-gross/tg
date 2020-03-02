#include "tg_vulkan.h"

#define _CRT_SECURE_NO_WARNINGS

#ifdef TG_WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "tg_image.h"
#include "tg_vertex.h"
#include "tg/math/tg_math.h"
#include "tg/platform/tg_allocator.h"
#include "tg/platform/tg_platform.h"
#include "tg/util/tg_file_io.h"
#include <stdbool.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

#ifdef TG_WIN32

#ifdef TG_DEBUG
#define INSTANCE_EXTENSION_COUNT 3
#define INSTANCE_EXTENSION_NAMES (char*[]){ VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME }
#else
#define INSTANCE_EXTENSION_COUNT 2
#define INSTANCE_EXTENSION_NAMES (char*[]){ VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME }
#endif

#else

#define INSTANCE_EXTENSION_COUNT 0
#define INSTANCE_EXTENSION_NAMES NULL

#endif

#define DEVICE_EXTENSION_COUNT 1
#define DEVICE_EXTENSION_NAMES (char*[]){ VK_KHR_SWAPCHAIN_EXTENSION_NAME }

#ifdef TG_DEBUG
#define VALIDATION_LAYER_COUNT 1
#define VALIDATION_LAYER_NAMES (char*[]){ "VK_LAYER_KHRONOS_validation" }
#else
#define VALIDATION_LAYER_COUNT 0
#define VALIDATION_LAYER_NAMES NULL
#endif

#ifdef TG_DEBUG
#define VK_CALL(x) TG_ASSERT(x == VK_SUCCESS)
#else
#define VK_CALL(x) x
#endif

#define MAX_FRAMES_IN_FLIGHT 2

#define QUEUE_INDEX_COUNT 2
typedef struct tgvk_queue_families
{
    ui32 graphics;
    ui32 present;
} tgvk_queue_families;

#define MAX_PRESENT_MODE_COUNT 9
#define SURFACE_IMAGE_COUNT 3

typedef struct tgvk_ubo
{
    tgm_mat4f model;
    tgm_mat4f view;
    tgm_mat4f projection;
} tgvk_ubo;

VkInstance instance = VK_NULL_HANDLE;
#ifdef TG_DEBUG
VkDebugUtilsMessengerEXT debug_utils_messenger = VK_NULL_HANDLE;
#endif
VkSurfaceKHR surface = VK_NULL_HANDLE;
VkPhysicalDevice physical_device = VK_NULL_HANDLE;
tgvk_queue_families queue_family_indices = { 0 };
VkDevice device = VK_NULL_HANDLE;
VkQueue graphics_queue = VK_NULL_HANDLE;
VkQueue present_queue = VK_NULL_HANDLE;
VkCommandPool command_pool = VK_NULL_HANDLE;
VkSemaphore image_available_semaphores[MAX_FRAMES_IN_FLIGHT] = { 0 };
VkSemaphore rendering_finished_semaphores[MAX_FRAMES_IN_FLIGHT] = { 0 };
VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT] = { 0 };
VkFence images_in_flight[SURFACE_IMAGE_COUNT] = { 0 };

VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
VkBuffer uniform_buffers[SURFACE_IMAGE_COUNT] = { 0 };
VkDeviceMemory uniform_buffer_memories[SURFACE_IMAGE_COUNT] = { 0 };
ui32 texture_mip_levels = 0;
VkImage texture_image = VK_NULL_HANDLE;
VkDeviceMemory texture_image_memory = VK_NULL_HANDLE;
VkImageView texture_image_view = VK_NULL_HANDLE;
VkSampler texture_sampler = VK_NULL_HANDLE;
VkBuffer vertex_buffer = VK_NULL_HANDLE;
VkDeviceMemory vertex_buffer_memory = VK_NULL_HANDLE;
VkBuffer index_buffer = VK_NULL_HANDLE;
VkDeviceMemory index_buffer_memory = VK_NULL_HANDLE;

VkSwapchainKHR swapchain = VK_NULL_HANDLE;
VkExtent2D swapchain_extent = { 0 };
VkImage images[SURFACE_IMAGE_COUNT] = { 0 };
VkImageView image_views[SURFACE_IMAGE_COUNT] = { 0 };
VkRenderPass render_pass = VK_NULL_HANDLE;
VkFramebuffer framebuffers[SURFACE_IMAGE_COUNT] = { 0 };
VkImage depth_image = VK_NULL_HANDLE;
VkDeviceMemory depth_image_memory = VK_NULL_HANDLE;
VkImageView depth_image_view = VK_NULL_HANDLE;
VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
VkPipeline graphics_pipeline = VK_NULL_HANDLE;
VkDescriptorSet descriptor_sets[SURFACE_IMAGE_COUNT] = { 0 };
VkCommandBuffer command_buffers[SURFACE_IMAGE_COUNT] = { 0 };

ui32 current_frame = 0;
f32 fff = 0.0f;
tg_vertex vertices[] = {
    { { -0.5f, -0.5f,  0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
    { {  0.5f, -0.5f,  0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
    { {  0.5f,  0.5f,  0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
    { { -0.5f,  0.5f,  0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
    { { -0.5f, -0.5f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
    { {  0.5f, -0.5f, -1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
    { {  0.5f,  0.5f, -1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
    { { -0.5f,  0.5f, -1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } }
};
ui16 indices[] = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};

#ifdef TG_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL tgvk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{
    TG_PRINT(callback_data->pMessage);
    return VK_TRUE;
}
#endif

// utility
void tgvk_find_physical_device_queue_families(VkPhysicalDevice pd, tgvk_queue_families* p_qfi, bool* p_complete)
{
    ui32 queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &queue_family_count, NULL);
    TG_ASSERT(queue_family_count);
    VkQueueFamilyProperties* queue_family_properties = tg_malloc(queue_family_count * sizeof(*queue_family_properties));
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &queue_family_count, queue_family_properties);

    bool supports_graphics_family = false;
    VkBool32 supports_present_family = 0;
    for (ui32 i = 0; i < queue_family_count; i++)
    {
        if (!supports_graphics_family)
        {
            if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                p_qfi->graphics = i;
                supports_graphics_family = true;
                continue;
            }
        }
        if (!supports_present_family)
        {
            VK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, surface, &supports_present_family));
            if (supports_present_family)
            {
                p_qfi->present = i;
            }
        }
        if (supports_graphics_family && supports_present_family)
        {
            break;
        }
    }
    *p_complete = supports_graphics_family && supports_present_family;

    tg_free(queue_family_properties);
}
void tgvk_check_physical_device_extension_support(VkPhysicalDevice pd, bool* result)
{
    ui32 device_extension_property_count;
    VK_CALL(vkEnumerateDeviceExtensionProperties(pd, NULL, &device_extension_property_count, NULL));
    VkExtensionProperties* device_extension_properties = tg_malloc(device_extension_property_count * sizeof(*device_extension_properties));
    VK_CALL(vkEnumerateDeviceExtensionProperties(pd, NULL, &device_extension_property_count, device_extension_properties));

    bool supports_extensions = true;
    for (ui32 i = 0; i < DEVICE_EXTENSION_COUNT; i++)
    {
        bool supports_extension = false;
        for (ui32 j = 0; j < device_extension_property_count; j++)
        {
            if (strcmp(DEVICE_EXTENSION_NAMES[i], device_extension_properties[j].extensionName) == 0)
            {
                supports_extension = true;
                break;
            }
        }
        supports_extensions &= supports_extension;
    }

    tg_free(device_extension_properties);
    *result = supports_extensions;
}
void tgvk_find_depth_format(VkFormat* p_format)
{
    const VkFormat formats[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
    const size_t format_count = sizeof(formats) / sizeof(*formats);
    for (int i = 0; i < format_count; i++)
    {
        VkFormatProperties format_properties;
        vkGetPhysicalDeviceFormatProperties(physical_device, formats[i], &format_properties);
        if ((format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            *p_format = formats[i];
            return;
        }
    }
    TG_ASSERT(false);
}
void tgvk_begin_command_buffer(VkCommandBuffer* p_command_buffer)
{
    VkCommandBufferAllocateInfo command_buffer_allocate_info = { 0 };
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = NULL;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1;

    VK_CALL(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, p_command_buffer));

    VkCommandBufferBeginInfo command_buffer_begin_info = { 0 };
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = NULL;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    command_buffer_begin_info.pInheritanceInfo = NULL;

    VK_CALL(vkBeginCommandBuffer(*p_command_buffer, &command_buffer_begin_info));
}
void tgvk_end_command_buffer(VkCommandBuffer command_buffer)
{
    VK_CALL(vkEndCommandBuffer(command_buffer));

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    VK_CALL(vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE));
    VK_CALL(vkQueueWaitIdle(graphics_queue));
    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}
void tgvk_find_memory_type(ui32 memory_type_bits, VkMemoryPropertyFlags memory_property_flags, ui32* p_memory_type)
{
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_memory_properties);

    for (ui32 i = 0; i < physical_device_memory_properties.memoryTypeCount; i++)
    {
        const bool is_memory_type_suitable = memory_type_bits & (1 << i);
        const bool are_property_flags_suitable = (physical_device_memory_properties.memoryTypes[i].propertyFlags & memory_property_flags) == memory_property_flags;
        if (is_memory_type_suitable && are_property_flags_suitable)
        {
            *p_memory_type = i;
            return;
        }
    }

    TG_ASSERT(0);
}
void tgvk_create_buffer(VkDeviceSize size, VkBufferUsageFlags buffer_usage_flags, VkMemoryPropertyFlags memory_property_flags, VkBuffer* p_buffer, VkDeviceMemory* p_device_memory)
{
    VkBufferCreateInfo buffer_create_info = { 0 };
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext = NULL;
    buffer_create_info.flags = 0;
    buffer_create_info.size = size;
    buffer_create_info.usage = buffer_usage_flags;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = NULL;

    VK_CALL(vkCreateBuffer(device, &buffer_create_info, NULL, p_buffer));

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, *p_buffer, &memory_requirements);

    VkMemoryAllocateInfo memory_allocate_info = { 0 };
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.pNext = NULL;
    memory_allocate_info.allocationSize = memory_requirements.size;
    tgvk_find_memory_type(memory_requirements.memoryTypeBits, memory_property_flags, &memory_allocate_info.memoryTypeIndex);

    VK_CALL(vkAllocateMemory(device, &memory_allocate_info, NULL, p_device_memory));
    VK_CALL(vkBindBufferMemory(device, *p_buffer, *p_device_memory, 0));
}
void tgvk_create_image(ui32 width, ui32 height, ui32 mip_levels, VkFormat format, VkImageTiling image_tiling, VkImageUsageFlags image_usage_flags, VkMemoryPropertyFlags memory_property_flags, VkImage* p_image, VkDeviceMemory* p_device_memory)
{
    VkImageCreateInfo image_create_info = { 0 };
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.flags = 0;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = format;
    image_create_info.extent.width = width;
    image_create_info.extent.height = height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = mip_levels;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = image_tiling;
    image_create_info.usage = image_usage_flags;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = 0;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_CALL(vkCreateImage(device, &image_create_info, NULL, p_image));

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device, *p_image, &memory_requirements);

    VkMemoryAllocateInfo memory_allocate_info = { 0 };
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.pNext = NULL;
    memory_allocate_info.allocationSize = memory_requirements.size;
    tgvk_find_memory_type(memory_requirements.memoryTypeBits, memory_property_flags, &memory_allocate_info.memoryTypeIndex);

    VK_CALL(vkAllocateMemory(device, &memory_allocate_info, NULL, p_device_memory));
    VK_CALL(vkBindImageMemory(device, *p_image, *p_device_memory, 0));
}
void tgvk_create_image_view(VkImage image, VkFormat format, ui32 mip_levels, VkImageAspectFlags image_aspect_flags, VkImageView* p_image_view)
{
    VkImageViewCreateInfo image_view_create_info = { 0 };
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = NULL;
    image_view_create_info.flags = 0;
    image_view_create_info.image = image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = image_aspect_flags;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = mip_levels;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;

    VK_CALL(vkCreateImageView(device, &image_view_create_info, NULL, p_image_view));
}
void tgvk_create_sampler(VkImage image, ui32 mip_levels, VkFilter min_filter, VkFilter mag_filter, VkSamplerAddressMode address_mode_u, VkSamplerAddressMode address_mode_v, VkSamplerAddressMode address_mode_w, VkSampler* p_sampler)
{
    VkSamplerCreateInfo sampler_create_info = { 0 };
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.pNext = NULL;
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
    sampler_create_info.minLod = 0;
    sampler_create_info.maxLod = mip_levels;
    sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;

    VK_CALL(vkCreateSampler(device, &sampler_create_info, NULL, p_sampler));
}
void tgvk_generate_mipmaps(VkImage image, ui32 width, ui32 height, VkFormat format, ui32 mip_levels)
{
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(physical_device, format, &format_properties);
    TG_ASSERT(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

    VkCommandBuffer command_buffer;
    tgvk_begin_command_buffer(&command_buffer);

    VkImageMemoryBarrier image_memory_barrier = { 0 };
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = NULL;
    image_memory_barrier.srcAccessMask = 0;
    image_memory_barrier.dstAccessMask = 0;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.image = image;
    image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;

    ui32 mip_width = width;
    ui32 mip_height = height;

    for (ui32 i = 1; i < mip_levels; i++)
    {
        ui32 next_mip_width = max(1, mip_width / 2);
        ui32 next_mip_height = max(1, mip_height / 2);

        image_memory_barrier.subresourceRange.baseMipLevel = i - 1;
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);

        VkImageBlit image_blit = { 0 };
        image_blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_blit.srcSubresource.mipLevel = i - 1;
        image_blit.srcSubresource.baseArrayLayer = 0;
        image_blit.srcSubresource.layerCount = 1;
        image_blit.srcOffsets[0].x = 0;
        image_blit.srcOffsets[0].y = 0;
        image_blit.srcOffsets[0].z = 0;
        image_blit.srcOffsets[1].x = mip_width;
        image_blit.srcOffsets[1].y = mip_height;
        image_blit.srcOffsets[1].z = 1;
        image_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_blit.dstSubresource.mipLevel = i;
        image_blit.dstSubresource.baseArrayLayer = 0;
        image_blit.dstSubresource.layerCount = 1;
        image_blit.dstOffsets[0].x = 0;
        image_blit.dstOffsets[0].y = 0;
        image_blit.dstOffsets[0].z = 0;
        image_blit.dstOffsets[1].x = next_mip_width;
        image_blit.dstOffsets[1].y = next_mip_height;
        image_blit.dstOffsets[1].z = 1;

        vkCmdBlitImage(command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_blit, VK_FILTER_LINEAR);

        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);

        mip_width = next_mip_width;
        mip_height = next_mip_height;
    }

    image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_memory_barrier.subresourceRange.baseMipLevel = mip_levels - 1;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
    tgvk_end_command_buffer(command_buffer);
}
void tgvk_create_shader_module(const char* filename, VkShaderModule* p_shader_module)
{
    ui64 size;
    char* content;
    tg_file_io_read(filename, &size, &content);

    VkShaderModuleCreateInfo shader_module_create_info = { 0 };
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.pNext = NULL;
    shader_module_create_info.flags = 0;
    shader_module_create_info.codeSize = size / sizeof(char);
    shader_module_create_info.pCode = (const ui32*)content;

    VK_CALL(vkCreateShaderModule(device, &shader_module_create_info, NULL, p_shader_module));
    tg_file_io_free(content);
}
void tgvk_copy_buffer(VkDeviceSize size, VkBuffer* p_source, VkBuffer* p_target)
{
    VkCommandBuffer command_buffer;
    tgvk_begin_command_buffer(&command_buffer);

    VkBufferCopy buffer_copy = { 0 };
    buffer_copy.srcOffset = 0;
    buffer_copy.dstOffset = 0;
    buffer_copy.size = size;

    vkCmdCopyBuffer(command_buffer, *p_source, *p_target, 1, &buffer_copy);
    tgvk_end_command_buffer(command_buffer);
}
void tgvk_copy_buffer_to_image(ui32 width, ui32 height, VkBuffer* p_source, VkImage* p_target)
{
    VkCommandBuffer command_buffer;
    tgvk_begin_command_buffer(&command_buffer);

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
    buffer_image_copy.imageExtent.width = width;
    buffer_image_copy.imageExtent.height = height;
    buffer_image_copy.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(command_buffer, *p_source, *p_target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
    tgvk_end_command_buffer(command_buffer);
}
void tgvk_transition_image_layout(VkImageLayout old_image_layout, VkImageLayout new_image_layout, ui32 mip_levels, VkImage* p_image)
{
    VkCommandBuffer command_buffer;
    tgvk_begin_command_buffer(&command_buffer);

    VkPipelineStageFlags src_stage = 0;
    VkPipelineStageFlags dst_stage = 0;

    VkImageMemoryBarrier image_memory_barrier = { 0 };
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = NULL;

    TG_ASSERT(
        (old_image_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) ||
        (old_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    );

    if (old_image_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        image_memory_barrier.srcAccessMask = 0;
        image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    image_memory_barrier.oldLayout = old_image_layout;
    image_memory_barrier.newLayout = new_image_layout;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.image = *p_image;
    image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = mip_levels;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
    tgvk_end_command_buffer(command_buffer);
}

// initialization
void tgvk_init_instance()
{
#ifdef TG_DEBUG
    ui32 layer_property_count;
    vkEnumerateInstanceLayerProperties(&layer_property_count, NULL);
    VkLayerProperties* layer_properties = tg_malloc(layer_property_count * sizeof(*layer_properties));
    vkEnumerateInstanceLayerProperties(&layer_property_count, layer_properties);

    for (ui32 i = 0; i < VALIDATION_LAYER_COUNT; i++)
    {
        bool layer_found = false;
        for (ui32 j = 0; j < layer_property_count; j++)
        {
            if (strcmp(VALIDATION_LAYER_NAMES[i], layer_properties[j].layerName) == 0)
            {
                layer_found = true;
                break;
            }
        }
        TG_ASSERT(layer_found);
    }
    tg_free(layer_properties);
#endif

    VkApplicationInfo application_info = { 0 };
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pNext = NULL;
    application_info.pApplicationName = NULL;
    application_info.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
    application_info.pEngineName = NULL;
    application_info.engineVersion = VK_MAKE_VERSION(0, 0, 0);
    application_info.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo instance_create_info = { 0 };
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = NULL;
    instance_create_info.flags = 0;
    instance_create_info.pApplicationInfo = &application_info;
    instance_create_info.enabledLayerCount = VALIDATION_LAYER_COUNT;
    instance_create_info.ppEnabledLayerNames = VALIDATION_LAYER_NAMES;
    instance_create_info.enabledExtensionCount = INSTANCE_EXTENSION_COUNT;
    instance_create_info.ppEnabledExtensionNames = INSTANCE_EXTENSION_NAMES;

    VK_CALL(vkCreateInstance(&instance_create_info, NULL, &instance));
}
#ifdef TG_DEBUG
void tgvk_init_debug_utils_manager()
{
    VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info = { 0 };
    debug_utils_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_utils_messenger_create_info.pNext = NULL;
    debug_utils_messenger_create_info.flags = 0;
    debug_utils_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_utils_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_utils_messenger_create_info.pfnUserCallback = tgvk_debug_callback;
    debug_utils_messenger_create_info.pUserData = NULL;

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    TG_ASSERT(vkCreateDebugUtilsMessengerEXT);
    VK_CALL(vkCreateDebugUtilsMessengerEXT(instance, &debug_utils_messenger_create_info, NULL, &debug_utils_messenger));
}
#endif
#ifdef TG_WIN32
void tgvk_init_surface()
{
    VkWin32SurfaceCreateInfoKHR win32_surface_create_info = { 0 };
    win32_surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    win32_surface_create_info.pNext = NULL;
    win32_surface_create_info.flags = 0;
    win32_surface_create_info.hinstance = GetModuleHandle(NULL);
    win32_surface_create_info.hwnd = tg_platform_get_window_handle();

    VK_CALL(vkCreateWin32SurfaceKHR(instance, &win32_surface_create_info, NULL, &surface));
}
#endif
void tgvk_init_physical_device()
{
    ui32 physical_device_count;
    VK_CALL(vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL));
    TG_ASSERT(physical_device_count);
    VkPhysicalDevice* physical_devices = tg_malloc(physical_device_count * sizeof(*physical_devices));
    VK_CALL(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices));

    for (ui32 i = 0; i < physical_device_count; i++)
    {
        VkPhysicalDeviceProperties physical_device_properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &physical_device_properties);

        VkPhysicalDeviceFeatures physical_device_features;
        vkGetPhysicalDeviceFeatures(physical_devices[i], &physical_device_features);

        const bool is_discrete_gpu = physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        const bool supports_geometry_shader = physical_device_features.geometryShader;
        const bool supports_sampler_anisotropy = physical_device_features.samplerAnisotropy;

        tgvk_queue_families qfi = { 0 };
        bool qfi_complete;
        tgvk_find_physical_device_queue_families(physical_devices[i], &qfi, &qfi_complete);

        bool supports_extensions;
        tgvk_check_physical_device_extension_support(physical_devices[i], &supports_extensions);

        ui32 physical_device_surface_format_count;
        VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_devices[i], surface, &physical_device_surface_format_count, NULL));
        ui32 physical_device_present_mode_count;
        VK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_devices[i], surface, &physical_device_present_mode_count, NULL));

        if (is_discrete_gpu && supports_geometry_shader && supports_sampler_anisotropy && qfi_complete && supports_extensions && physical_device_surface_format_count && physical_device_present_mode_count)
        {
            physical_device = physical_devices[i];
            queue_family_indices = qfi;
            break;
        }
    }
    TG_ASSERT(physical_device != VK_NULL_HANDLE);

    tg_free(physical_devices);
}
void tgvk_init_device()
{
    const float queue_priority = 1.0f;

    VkDeviceQueueCreateInfo device_queue_create_infos[QUEUE_INDEX_COUNT] = { 0 };
    for (ui32 i = 0; i < QUEUE_INDEX_COUNT; i++)
    {
        VkDeviceQueueCreateInfo device_queue_create_info = { 0 };
        device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        device_queue_create_info.pNext = NULL;
        device_queue_create_info.flags = 0;
        device_queue_create_info.queueFamilyIndex = *((ui32*)(&queue_family_indices) + i);
        device_queue_create_info.queueCount = 1;
        device_queue_create_info.pQueuePriorities = &queue_priority;

        device_queue_create_infos[i] = device_queue_create_info;
    }

    VkPhysicalDeviceFeatures physical_device_features = { 0 };
    physical_device_features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo device_create_info = { 0 };
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = NULL;
    device_create_info.flags = 0;
    device_create_info.queueCreateInfoCount = QUEUE_INDEX_COUNT;
    device_create_info.pQueueCreateInfos = device_queue_create_infos;
    device_create_info.enabledLayerCount = VALIDATION_LAYER_COUNT;
    device_create_info.ppEnabledLayerNames = VALIDATION_LAYER_NAMES;
    device_create_info.enabledExtensionCount = DEVICE_EXTENSION_COUNT;
    device_create_info.ppEnabledExtensionNames = DEVICE_EXTENSION_NAMES;
    device_create_info.pEnabledFeatures = &physical_device_features;

    VK_CALL(vkCreateDevice(physical_device, &device_create_info, NULL, &device));

    vkGetDeviceQueue(device, queue_family_indices.graphics, 0, &graphics_queue);
    vkGetDeviceQueue(device, queue_family_indices.present, 0, &present_queue);
}
void tgvk_init_command_pool()
{
    VkCommandPoolCreateInfo command_pool_create_info = { 0 };
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext = NULL;
    command_pool_create_info.flags = 0;
    command_pool_create_info.queueFamilyIndex = queue_family_indices.graphics;

    VK_CALL(vkCreateCommandPool(device, &command_pool_create_info, NULL, &command_pool));
}
void tgvk_init_semaphores_and_fences()
{
    VkSemaphoreCreateInfo semaphore_create_info = { 0 };
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = NULL;
    semaphore_create_info.flags = 0;

    VkFenceCreateInfo fence_create_info = { 0 };
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = NULL;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (ui32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VK_CALL(vkCreateSemaphore(device, &semaphore_create_info, NULL, &image_available_semaphores[i]));
        VK_CALL(vkCreateSemaphore(device, &semaphore_create_info, NULL, &rendering_finished_semaphores[i]));
        VK_CALL(vkCreateFence(device, &fence_create_info, NULL, &in_flight_fences[i]));
    }
}



// shader layout and input
void tgvk_init_descriptor_set_layout()
{
    VkDescriptorSetLayoutBinding descriptor_set_layout_bindings[2] = { 0 };

    descriptor_set_layout_bindings[0].binding = 0;
    descriptor_set_layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_set_layout_bindings[0].descriptorCount = 1;
    descriptor_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    descriptor_set_layout_bindings[0].pImmutableSamplers = NULL;

    descriptor_set_layout_bindings[1].binding = 1;
    descriptor_set_layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_set_layout_bindings[1].descriptorCount = 1;
    descriptor_set_layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptor_set_layout_bindings[1].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = { 0 };
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.pNext = NULL;
    descriptor_set_layout_create_info.flags = 0;
    descriptor_set_layout_create_info.bindingCount = sizeof(descriptor_set_layout_bindings) / sizeof(*descriptor_set_layout_bindings);
    descriptor_set_layout_create_info.pBindings = descriptor_set_layout_bindings;

    VK_CALL(vkCreateDescriptorSetLayout(device, &descriptor_set_layout_create_info, NULL, &descriptor_set_layout));
}
void tgvk_init_descriptor_pool()
{
    VkDescriptorPoolSize descriptor_pool_size[2] = { 0 };

    descriptor_pool_size[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_pool_size[0].descriptorCount = SURFACE_IMAGE_COUNT;

    descriptor_pool_size[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_pool_size[1].descriptorCount = SURFACE_IMAGE_COUNT;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = { 0 };
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext = NULL;
    descriptor_pool_create_info.flags = 0;
    descriptor_pool_create_info.maxSets = SURFACE_IMAGE_COUNT;
    descriptor_pool_create_info.poolSizeCount = sizeof(descriptor_pool_size) / sizeof(*descriptor_pool_size);
    descriptor_pool_create_info.pPoolSizes = descriptor_pool_size;

    VK_CALL(vkCreateDescriptorPool(device, &descriptor_pool_create_info, NULL, &descriptor_pool));
}
void tgvk_init_uniform_buffers()
{
    const VkDeviceSize size = sizeof(tgvk_ubo);

    for (ui32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {
        tgvk_create_buffer(
            size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &uniform_buffers[i],
            &uniform_buffer_memories[i]);
    }
}
void tgvk_init_texture_image()
{
    ui32 w = 0;
    ui32 h = 0;
    tgi_pixel* d = NULL;
    tgi_load_bmp("test_icon.bmp", &w, &h, &d);

    texture_mip_levels = TGI_MAX_MIP_LEVELS(w, h);

    const VkDeviceSize image_size = (ui64)w * (ui64)h * (ui64)sizeof(tgi_pixel);
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    tgvk_create_buffer(
        image_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &staging_buffer,
        &staging_buffer_memory
    );

    void* data;
    VK_CALL(vkMapMemory(device, staging_buffer_memory, 0, image_size, 0, &data));
    memcpy(data, d, (size_t)image_size);
    vkUnmapMemory(device, staging_buffer_memory);

    tgi_free(d);

    tgvk_create_image(
        w,
        h,
        texture_mip_levels,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &texture_image,
        &texture_image_memory
    );

    tgvk_transition_image_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture_mip_levels, &texture_image);
    tgvk_copy_buffer_to_image(w, h, &staging_buffer, &texture_image);

    vkDestroyBuffer(device, staging_buffer, NULL);
    vkFreeMemory(device, staging_buffer_memory, NULL);

    tgvk_generate_mipmaps(texture_image, w, h, VK_FORMAT_R8G8B8A8_SRGB, texture_mip_levels);
    tgvk_create_image_view(texture_image, VK_FORMAT_R8G8B8A8_SRGB, texture_mip_levels, VK_IMAGE_ASPECT_COLOR_BIT, &texture_image_view);
    tgvk_create_sampler(texture_image, texture_mip_levels, VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, &texture_sampler);
}
void tgvk_init_vertex_buffer()
{
    const ui32 device_size = sizeof(vertices);

    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;
    tgvk_create_buffer(
        device_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &staging_buffer,
        &staging_buffer_memory);

    void* data;
    vkMapMemory(device, staging_buffer_memory, 0, device_size, 0, &data);
    memcpy(data, vertices, device_size);
    vkUnmapMemory(device, staging_buffer_memory);

    tgvk_create_buffer(
        device_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &vertex_buffer,
        &vertex_buffer_memory);

    tgvk_copy_buffer(device_size, &staging_buffer, &vertex_buffer);

    vkDestroyBuffer(device, staging_buffer, NULL);
    vkFreeMemory(device, staging_buffer_memory, NULL);
}
void tgvk_init_index_buffer()
{
    const VkDeviceSize size = sizeof(indices);

    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;
    tgvk_create_buffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &staging_buffer,
        &staging_buffer_memory);

    void* data;
    vkMapMemory(device, staging_buffer_memory, 0, size, 0, &data);
    memcpy(data, indices, (size_t)size);
    vkUnmapMemory(device, staging_buffer_memory);

    tgvk_create_buffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &index_buffer,
        &index_buffer_memory);

    tgvk_copy_buffer(size, &staging_buffer, &index_buffer);

    vkDestroyBuffer(device, staging_buffer, NULL);
    vkFreeMemory(device, staging_buffer_memory, NULL);
}

// pipeline
void tgvk_init_swapchain()
{
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VK_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities));

    ui32 surface_format_count;
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, NULL));
    VkSurfaceFormatKHR* surface_formats = tg_malloc(surface_format_count * sizeof(*surface_formats));
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats));
    VkSurfaceFormatKHR surface_format = surface_formats[0];
    for (ui32 i = 0; i < surface_format_count; i++)
    {
        if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            surface_format = surface_formats[i];
            break;
        }
    }
    tg_free(surface_formats);

    VkPresentModeKHR present_modes[MAX_PRESENT_MODE_COUNT] = { 0 };
    ui32 max_present_mode_count = MAX_PRESENT_MODE_COUNT;
    VK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &max_present_mode_count, present_modes));

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (ui32 i = 0; i < MAX_PRESENT_MODE_COUNT; i++)
    {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            present_mode = present_modes[i];
            break;
        }
    }

    if (surface_capabilities.currentExtent.width != UINT32_MAX)
    {
        swapchain_extent = surface_capabilities.currentExtent;
    }
    else
    {
        tg_platform_get_window_size(&swapchain_extent.width, &swapchain_extent.height);
        swapchain_extent.width = tgm_ui32_clamp(swapchain_extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
        swapchain_extent.height = tgm_ui32_clamp(swapchain_extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
    }

    TG_ASSERT(tgm_ui32_clamp(SURFACE_IMAGE_COUNT, surface_capabilities.minImageCount, surface_capabilities.maxImageCount) == SURFACE_IMAGE_COUNT);

    VkSwapchainCreateInfoKHR swapchain_create_info = { 0 };
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = NULL;
    swapchain_create_info.flags = 0;
    swapchain_create_info.surface = surface;
    swapchain_create_info.minImageCount = SURFACE_IMAGE_COUNT;
    swapchain_create_info.imageFormat = surface_format.format;
    swapchain_create_info.imageColorSpace = surface_format.colorSpace;
    swapchain_create_info.imageExtent = swapchain_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (queue_family_indices.graphics != queue_family_indices.present)
    {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = (ui32*)&queue_family_indices;
    }
    else
    {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = NULL;
    }

    swapchain_create_info.preTransform = surface_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

    VK_CALL(vkCreateSwapchainKHR(device, &swapchain_create_info, NULL, &swapchain));
    
    ui32 surface_image_count = SURFACE_IMAGE_COUNT;
    VK_CALL(vkGetSwapchainImagesKHR(device, swapchain, &surface_image_count, NULL));
    TG_ASSERT(surface_image_count == SURFACE_IMAGE_COUNT);
    VK_CALL(vkGetSwapchainImagesKHR(device, swapchain, &surface_image_count, images));

    for (ui32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {
        tgvk_create_image_view(images[i], surface_format.format, 1, VK_IMAGE_ASPECT_COLOR_BIT, &image_views[i]);
    }

    VkAttachmentReference color_attachment_reference = { 0 };
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_reference = { 0 };
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description = { 0 };
    subpass_description.flags = 0;
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = NULL;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &color_attachment_reference;
    subpass_description.pResolveAttachments = NULL;
    subpass_description.pDepthStencilAttachment = &depth_attachment_reference;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = NULL;

    VkSubpassDependency subpass_dependency = { 0 };
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachment_descriptions[2] = { 0 };
    VkFormat depth_format;
    tgvk_find_depth_format(&depth_format);

    attachment_descriptions[0].flags = 0;
    attachment_descriptions[0].format = surface_format.format;
    attachment_descriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_descriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachment_descriptions[1].flags = 0;
    attachment_descriptions[1].format = depth_format;
    attachment_descriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkRenderPassCreateInfo render_pass_create_info = { 0 };
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = NULL;
    render_pass_create_info.flags = 0;
    render_pass_create_info.attachmentCount = sizeof(attachment_descriptions) / sizeof(*attachment_descriptions);
    render_pass_create_info.pAttachments = attachment_descriptions;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_description;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &subpass_dependency;

    VK_CALL(vkCreateRenderPass(device, &render_pass_create_info, NULL, &render_pass));

    tgvk_create_image(
        swapchain_extent.width,
        swapchain_extent.height,
        1,
        depth_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &depth_image,
        &depth_image_memory
    );
    tgvk_create_image_view(depth_image, depth_format, 1, VK_IMAGE_ASPECT_DEPTH_BIT, &depth_image_view);

    for (ui32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {
        const VkImageView image_view_attachments[] = {
            image_views[i],
            depth_image_view
        };

        VkFramebufferCreateInfo framebuffer_create_info = { 0 };
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = NULL;
        framebuffer_create_info.flags = 0;
        framebuffer_create_info.renderPass = render_pass;
        framebuffer_create_info.attachmentCount = sizeof(image_view_attachments) / sizeof(*image_view_attachments);
        framebuffer_create_info.pAttachments = image_view_attachments;
        framebuffer_create_info.width = swapchain_extent.width;
        framebuffer_create_info.height = swapchain_extent.height;
        framebuffer_create_info.layers = 1;

        VK_CALL(vkCreateFramebuffer(device, &framebuffer_create_info, NULL, &framebuffers[i]));
    }
}
void tgvk_init_graphics_pipeline()
{
    // TODO: look at comments for simpler compilation of shaders: https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules

    VkShaderModule vert_shader_module;
    tgvk_create_shader_module("shaders/vert.spv", &vert_shader_module);

    VkShaderModule frag_shader_module;
    tgvk_create_shader_module("shaders/frag.spv", &frag_shader_module);

    VkPipelineShaderStageCreateInfo pipeline_shader_state_create_info_vert = { 0 };
    pipeline_shader_state_create_info_vert.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_shader_state_create_info_vert.pNext = NULL;
    pipeline_shader_state_create_info_vert.flags = 0;
    pipeline_shader_state_create_info_vert.stage = VK_SHADER_STAGE_VERTEX_BIT;
    pipeline_shader_state_create_info_vert.module = vert_shader_module;
    pipeline_shader_state_create_info_vert.pName = "main";
    pipeline_shader_state_create_info_vert.pSpecializationInfo = NULL;

    VkPipelineShaderStageCreateInfo pipeline_shader_state_create_info_frag = { 0 };
    pipeline_shader_state_create_info_frag.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_shader_state_create_info_frag.pNext = NULL;
    pipeline_shader_state_create_info_frag.flags = 0;
    pipeline_shader_state_create_info_frag.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipeline_shader_state_create_info_frag.module = frag_shader_module;
    pipeline_shader_state_create_info_frag.pName = "main";
    pipeline_shader_state_create_info_frag.pSpecializationInfo = NULL;

    const VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_infos[] = {
        pipeline_shader_state_create_info_vert,
        pipeline_shader_state_create_info_frag
    };

    VkVertexInputBindingDescription vertex_input_binding_description = { 0 };
    vertex_input_binding_description.binding = 0;
    vertex_input_binding_description.stride = sizeof(tg_vertex);
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_input_attribute_descriptions[3] = { 0 };
    vertex_input_attribute_descriptions[0].binding = 0;
    vertex_input_attribute_descriptions[0].location = 0;
    vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_descriptions[0].offset = offsetof(tg_vertex, position);
    vertex_input_attribute_descriptions[1].binding = 0;
    vertex_input_attribute_descriptions[1].location = 1;
    vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_descriptions[1].offset = offsetof(tg_vertex, color);
    vertex_input_attribute_descriptions[2].binding = 0;
    vertex_input_attribute_descriptions[2].location = 2;
    vertex_input_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    vertex_input_attribute_descriptions[2].offset = offsetof(tg_vertex, uv);

    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = { 0 };
    pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipeline_vertex_input_state_create_info.pNext = NULL;
    pipeline_vertex_input_state_create_info.flags = 0;
    pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
    pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = sizeof(vertex_input_attribute_descriptions) / sizeof(*vertex_input_attribute_descriptions);
    pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_input_attribute_descriptions;

    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info = { 0 };
    pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipeline_input_assembly_state_create_info.pNext = NULL;
    pipeline_input_assembly_state_create_info.flags = 0;
    pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = { 0 };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchain_extent.width;
    viewport.height = (float)swapchain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D rect = { 0 };
    rect.offset = (VkOffset2D){ 0, 0 };
    rect.extent = swapchain_extent;

    VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = { 0 };
    pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipeline_viewport_state_create_info.pNext = NULL;
    pipeline_viewport_state_create_info.flags = 0;
    pipeline_viewport_state_create_info.viewportCount = 1;
    pipeline_viewport_state_create_info.pViewports = &viewport;
    pipeline_viewport_state_create_info.scissorCount = 1;
    pipeline_viewport_state_create_info.pScissors = &rect;

    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info = { 0 };
    pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipeline_rasterization_state_create_info.pNext = NULL;
    pipeline_rasterization_state_create_info.flags = 0;
    pipeline_rasterization_state_create_info.depthClampEnable = VK_FALSE;
    pipeline_rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    pipeline_rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    pipeline_rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    pipeline_rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipeline_rasterization_state_create_info.depthBiasEnable = VK_FALSE;
    pipeline_rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
    pipeline_rasterization_state_create_info.depthBiasClamp = 0.0f;
    pipeline_rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;
    pipeline_rasterization_state_create_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = { 0 };
    pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipeline_multisample_state_create_info.pNext = NULL;
    pipeline_multisample_state_create_info.flags = 0;
    pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipeline_multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    pipeline_multisample_state_create_info.minSampleShading = 1.0f;
    pipeline_multisample_state_create_info.pSampleMask = NULL;
    pipeline_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    pipeline_multisample_state_create_info.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = { 0 };
    pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipeline_depth_stencil_state_create_info.pNext = NULL;
    pipeline_depth_stencil_state_create_info.flags = 0;
    pipeline_depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
    pipeline_depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
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

    VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state = { 0 };
    pipeline_color_blend_attachment_state.blendEnable = VK_FALSE;
    pipeline_color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    pipeline_color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipeline_color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    pipeline_color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    pipeline_color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipeline_color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
    pipeline_color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = { 0 };
    pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipeline_color_blend_state_create_info.pNext = NULL;
    pipeline_color_blend_state_create_info.flags = 0;
    pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
    pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    pipeline_color_blend_state_create_info.attachmentCount = 1;
    pipeline_color_blend_state_create_info.pAttachments = &pipeline_color_blend_attachment_state;
    pipeline_color_blend_state_create_info.blendConstants[0] = 0.0f;
    pipeline_color_blend_state_create_info.blendConstants[1] = 0.0f;
    pipeline_color_blend_state_create_info.blendConstants[2] = 0.0f;
    pipeline_color_blend_state_create_info.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = { 0 };
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pNext = NULL;
    pipeline_layout_create_info.flags = 0;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = NULL;

    VK_CALL(vkCreatePipelineLayout(device, &pipeline_layout_create_info, NULL, &pipeline_layout));

    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = { 0 };
    graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_create_info.pNext = NULL;
    graphics_pipeline_create_info.flags = 0;
    graphics_pipeline_create_info.stageCount = sizeof(pipeline_shader_stage_create_infos) / sizeof(*pipeline_shader_stage_create_infos);
    graphics_pipeline_create_info.pStages = pipeline_shader_stage_create_infos;
    graphics_pipeline_create_info.pVertexInputState = &pipeline_vertex_input_state_create_info;
    graphics_pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
    graphics_pipeline_create_info.pTessellationState = NULL;
    graphics_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
    graphics_pipeline_create_info.pRasterizationState = &pipeline_rasterization_state_create_info;
    graphics_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
    graphics_pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;
    graphics_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
    graphics_pipeline_create_info.pDynamicState = NULL;
    graphics_pipeline_create_info.layout = pipeline_layout;
    graphics_pipeline_create_info.renderPass = render_pass;
    graphics_pipeline_create_info.subpass = 0;
    graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    graphics_pipeline_create_info.basePipelineIndex = -1;

    VK_CALL(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, NULL, &graphics_pipeline));

    vkDestroyShaderModule(device, frag_shader_module, NULL);
    vkDestroyShaderModule(device, vert_shader_module, NULL);
}
void tgvk_init_descriptor_sets()
{
    VkDescriptorSetLayout descriptor_set_layouts[SURFACE_IMAGE_COUNT] = { 0 };
    for (ui32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {
        descriptor_set_layouts[i] = descriptor_set_layout;
    }

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = { 0 };
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = NULL;
    descriptor_set_allocate_info.descriptorPool = descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = SURFACE_IMAGE_COUNT;
    descriptor_set_allocate_info.pSetLayouts = descriptor_set_layouts;

    VK_CALL(vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, descriptor_sets));

    for (ui32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {
        VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
        descriptor_buffer_info.buffer = uniform_buffers[i];
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = sizeof(tgvk_ubo);

        VkDescriptorImageInfo descriptor_image_info = { 0 };
        descriptor_image_info.sampler = texture_sampler;
        descriptor_image_info.imageView = texture_image_view;
        descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write_descriptor_sets[2] = { 0 };

        write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_sets[0].pNext = NULL;
        write_descriptor_sets[0].dstSet = descriptor_sets[i];
        write_descriptor_sets[0].dstBinding = 0;
        write_descriptor_sets[0].dstArrayElement = 0;
        write_descriptor_sets[0].descriptorCount = 1;
        write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write_descriptor_sets[0].pImageInfo = NULL;
        write_descriptor_sets[0].pBufferInfo = &descriptor_buffer_info;
        write_descriptor_sets[0].pTexelBufferView = NULL;

        write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_sets[1].pNext = NULL;
        write_descriptor_sets[1].dstSet = descriptor_sets[i];
        write_descriptor_sets[1].dstBinding = 1;
        write_descriptor_sets[1].dstArrayElement = 0;
        write_descriptor_sets[1].descriptorCount = 1;
        write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_descriptor_sets[1].pImageInfo = &descriptor_image_info;
        write_descriptor_sets[1].pBufferInfo = NULL;
        write_descriptor_sets[1].pTexelBufferView = NULL;

        vkUpdateDescriptorSets(device, sizeof(write_descriptor_sets) / sizeof(*write_descriptor_sets), write_descriptor_sets, 0, NULL);
    }
}
void tgvk_init_command_buffers()
{
    VkCommandBufferAllocateInfo command_buffer_allocate_info = { 0 };
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = NULL;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = SURFACE_IMAGE_COUNT;

    VK_CALL(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, command_buffers));

    for (ui32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {
        VkCommandBufferBeginInfo command_buffer_begin_info = { 0 };
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = NULL;
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = NULL;

        VK_CALL(vkBeginCommandBuffer(command_buffers[i], &command_buffer_begin_info));

        VkClearValue clear_values[2] = { 0 };
        clear_values[0].color = (VkClearColorValue){ 0.0f, 0.0f, 0.0f, 1.0f };
        clear_values[0].depthStencil = (VkClearDepthStencilValue){ 0.0f, 0 };
        clear_values[1].color = (VkClearColorValue){ 0.0f, 0.0f, 0.0f, 0.0f };
        clear_values[1].depthStencil = (VkClearDepthStencilValue){ 1.0f, 0 };

        VkRenderPassBeginInfo render_pass_begin_info = { 0 };
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext = NULL;
        render_pass_begin_info.renderPass = render_pass;
        render_pass_begin_info.framebuffer = framebuffers[i];
        render_pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
        render_pass_begin_info.renderArea.extent = swapchain_extent;
        render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
        render_pass_begin_info.pClearValues = clear_values;

        const VkDeviceSize offsets[] = { 0 };

        vkCmdBeginRenderPass(command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
        vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffer, offsets);
        vkCmdBindIndexBuffer(command_buffers[i], index_buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets[i], 0, NULL);
        vkCmdDrawIndexed(command_buffers[i], sizeof(indices) / sizeof(*indices), 1, 0, 0, 0);
        vkCmdEndRenderPass(command_buffers[i]);
        VK_CALL(vkEndCommandBuffer(command_buffers[i]));
    }
}



// main
void tgvk_init()
{
    tgvk_init_instance();
#ifdef TG_DEBUG
    tgvk_init_debug_utils_manager();
#endif
    tgvk_init_surface();
    tgvk_init_physical_device();
    tgvk_init_device();
    tgvk_init_command_pool();
    tgvk_init_semaphores_and_fences();



    tgvk_init_descriptor_set_layout();
    tgvk_init_descriptor_pool();

    tgvk_init_uniform_buffers();
    tgvk_init_texture_image();
    tgvk_init_vertex_buffer();
    tgvk_init_index_buffer();



    tgvk_init_swapchain();
    tgvk_init_graphics_pipeline();
    tgvk_init_descriptor_sets();
    tgvk_init_command_buffers();
}
void tgvk_render()
{
    VK_CALL(vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX));

    ui32 next_image;
    VK_CALL(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available_semaphores[current_frame], VK_NULL_HANDLE, &next_image));

    if (images_in_flight[next_image] != VK_NULL_HANDLE)
    {
        VK_CALL(vkWaitForFences(device, 1, &images_in_flight[next_image], VK_TRUE, UINT64_MAX));
    }
    images_in_flight[next_image] = in_flight_fences[current_frame];

    const VkPipelineStageFlags wait_dst_stage_mask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    fff += 0.001f;

    tgm_vec3f from = { -1.0f, 1.0f, 1.0f };
    tgm_vec3f to = { 0.0f, 0.0f, -2.0f };
    tgm_vec3f up = { 0.0f, 1.0f, 0.0f };

    // model
    const float angle = fff * TGM_TO_RADIANS(45.0f);
    tgm_vec3f axis = { 0.2f, 1.0f, 0.0f };
    tgm_vec3f translation_vector = { 0.0f, 0.0f, -2.0f };
    
    // projection
    const f32 fov_y = TGM_TO_DEGREES(70.0f);
    const f32 aspect = (f32)swapchain_extent.width / (f32)swapchain_extent.height;
    const f32 n = -0.1f;
    const f32 f = -1000.0f;
    
    tgvk_ubo uniform_buffer_object = { 0 };
    tgm_m4f_identity(&uniform_buffer_object.model);
    tgm_m4f_identity(&uniform_buffer_object.view);
    tgm_m4f_identity(&uniform_buffer_object.projection);

    //tgm_mat4f rotation = *tgm_m4f_angle_axis(&rotation, angle, &axis);
    //tgm_mat4f translation = *tgm_m4f_translate(&translation, &translation_vector);
    //tgm_m4f_multiply_m4f(&uniform_buffer_object.model, &translation, &rotation);
    tgm_m4f_translate(&uniform_buffer_object.model, &translation_vector);

    tgm_m4f_look_at(&uniform_buffer_object.view, &from, &to, &up);
    tgm_m4f_orthographic(&uniform_buffer_object.projection, -aspect, aspect, -1.0f, 1.0f, -10.0f, 10.0f);
    tgm_m4f_perspective(&uniform_buffer_object.projection, fov_y, aspect, n, f);

    void* data;
    VK_CALL(vkMapMemory(device, uniform_buffer_memories[next_image], 0, sizeof(uniform_buffer_object), 0, &data));
    memcpy(data, &uniform_buffer_object, sizeof(uniform_buffer_object));
    vkUnmapMemory(device, uniform_buffer_memories[next_image]);
    
    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = image_available_semaphores + current_frame;
    submit_info.pWaitDstStageMask = wait_dst_stage_mask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[next_image];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = rendering_finished_semaphores + current_frame;

    VK_CALL(vkResetFences(device, 1, in_flight_fences + current_frame));
    VK_CALL(vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[current_frame]));

    VkPresentInfoKHR present_info = { 0 };
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = NULL;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = rendering_finished_semaphores + current_frame;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &next_image;
    present_info.pResults = NULL;

    VK_CALL(vkQueuePresentKHR(present_queue, &present_info));

    current_frame = ++current_frame == MAX_FRAMES_IN_FLIGHT ? 0 : current_frame;
}
void tgvk_shutdown()
{
    vkDeviceWaitIdle(device);

    vkFreeCommandBuffers(device, command_pool, SURFACE_IMAGE_COUNT, command_buffers);
    vkDestroyImageView(device, depth_image_view, NULL);
    vkDestroyImage(device, depth_image, NULL);
    vkFreeMemory(device, depth_image_memory, NULL);
    for (ui32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {
        vkDestroyFramebuffer(device, framebuffers[i], NULL);
    }
    vkDestroyPipeline(device, graphics_pipeline, NULL);
    vkDestroyPipelineLayout(device, pipeline_layout, NULL);
    vkDestroyRenderPass(device, render_pass, NULL);
    for (ui32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {
        vkDestroyImageView(device, image_views[i], NULL);
    }
    vkDestroySwapchainKHR(device, swapchain, NULL);
    vkDestroyDescriptorPool(device, descriptor_pool, NULL);
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, NULL);
    vkDestroySampler(device, texture_sampler, NULL);
    vkDestroyImageView(device, texture_image_view, NULL);
    vkDestroyImage(device, texture_image, NULL);
    vkFreeMemory(device, texture_image_memory, NULL);
    vkDestroyBuffer(device, index_buffer, NULL);
    vkFreeMemory(device, index_buffer_memory, NULL);
    vkDestroyBuffer(device, vertex_buffer, NULL);
    vkFreeMemory(device, vertex_buffer_memory, NULL);
    for (ui32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {
        vkDestroyBuffer(device, uniform_buffers[i], NULL);
        vkFreeMemory(device, uniform_buffer_memories[i], NULL);
    }
    for (ui32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyFence(device, in_flight_fences[i], NULL);
        vkDestroySemaphore(device, rendering_finished_semaphores[i], NULL);
        vkDestroySemaphore(device, image_available_semaphores[i], NULL);
    }
    vkDestroyCommandPool(device, command_pool, NULL);
    vkDestroyDevice(device, NULL);
#ifdef TG_DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    TG_ASSERT(vkDestroyDebugUtilsMessengerEXT);
    vkDestroyDebugUtilsMessengerEXT(instance, debug_utils_messenger, NULL);
#endif
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
}
void tgvk_on_window_resize(ui32 width, ui32 height)
{
    if (instance == VK_NULL_HANDLE)
    {
        return;
    }

    vkDeviceWaitIdle(device);
    
    vkFreeCommandBuffers(device, command_pool, SURFACE_IMAGE_COUNT, command_buffers);
    vkDestroyImageView(device, depth_image_view, NULL);
    vkDestroyImage(device, depth_image, NULL);
    vkFreeMemory(device, depth_image_memory, NULL);
    for (ui32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {
        vkDestroyFramebuffer(device, framebuffers[i], NULL);
    }
    vkDestroyPipeline(device, graphics_pipeline, NULL);
    vkDestroyPipelineLayout(device, pipeline_layout, NULL);
    vkDestroyRenderPass(device, render_pass, NULL);
    for (ui32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {
        vkDestroyImageView(device, image_views[i], NULL);
    }
    vkDestroySwapchainKHR(device, swapchain, NULL);

    tgvk_init_swapchain();
    tgvk_init_graphics_pipeline();
    tgvk_init_command_buffers();
}
