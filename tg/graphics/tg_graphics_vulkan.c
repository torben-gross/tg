#include "tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "tg/math/tg_math.h"
#include "tg/platform/tg_allocator.h"
#include "tg/platform/tg_platform.h"
#include "tg/util/tg_file_io.h"
#include <stdbool.h>
#include <stdlib.h>



#ifdef TG_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL tgvk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{
    TG_PRINT(callback_data->pMessage);
    return VK_TRUE;
}
#endif

// utility
void tg_graphics_vulkan_physical_device_find_queue_families(VkPhysicalDevice physical_device, tg_queue* p_graphics_queue, tg_queue* p_present_queue, bool* p_complete)
{
    ui32 queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);
    TG_ASSERT(queue_family_count);
    VkQueueFamilyProperties* queue_family_properties = tg_allocator_allocate(queue_family_count * sizeof(*queue_family_properties));
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties);

    bool supports_graphics_family = false;
    VkBool32 supports_present_family = 0;
    for (ui32 i = 0; i < queue_family_count; i++)
    {
        if (!supports_graphics_family)
        {
            if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                p_graphics_queue->index = i;
                supports_graphics_family = true;
                continue;
            }
        }
        if (!supports_present_family)
        {
            VK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface.surface, &supports_present_family));
            if (supports_present_family)
            {
                p_present_queue->index = i;
            }
        }
        if (supports_graphics_family && supports_present_family)
        {
            break;
        }
    }
    *p_complete = supports_graphics_family && supports_present_family;

    tg_allocator_free(queue_family_properties);
}
void tg_graphics_vulkan_physical_device_check_extension_support(VkPhysicalDevice pd, bool* result)
{
    ui32 device_extension_property_count;
    VK_CALL(vkEnumerateDeviceExtensionProperties(pd, NULL, &device_extension_property_count, NULL));
    VkExtensionProperties* device_extension_properties = tg_allocator_allocate(device_extension_property_count * sizeof(*device_extension_properties));
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

    tg_allocator_free(device_extension_properties);
    *result = supports_extensions;
}
void tg_graphics_vulkan_physical_device_find_max_sample_count(VkPhysicalDevice physical_device, VkSampleCountFlagBits* max_sample_count_flag_bits)
{
    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

    const VkSampleCountFlags sample_count_flags = physical_device_properties.limits.framebufferColorSampleCounts & physical_device_properties.limits.framebufferDepthSampleCounts;

    if (sample_count_flags & VK_SAMPLE_COUNT_64_BIT)
    {
        *max_sample_count_flag_bits = VK_SAMPLE_COUNT_64_BIT;
    }
    else if (sample_count_flags & VK_SAMPLE_COUNT_32_BIT)
    {
        *max_sample_count_flag_bits = VK_SAMPLE_COUNT_32_BIT;
    }
    else if (sample_count_flags & VK_SAMPLE_COUNT_16_BIT)
    {
        *max_sample_count_flag_bits = VK_SAMPLE_COUNT_16_BIT;
    }
    else if (sample_count_flags & VK_SAMPLE_COUNT_8_BIT)
    {
        *max_sample_count_flag_bits = VK_SAMPLE_COUNT_8_BIT;
    }
    else if (sample_count_flags & VK_SAMPLE_COUNT_4_BIT)
    {
        *max_sample_count_flag_bits = VK_SAMPLE_COUNT_4_BIT;
    }
    else if (sample_count_flags & VK_SAMPLE_COUNT_2_BIT)
    {
        *max_sample_count_flag_bits = VK_SAMPLE_COUNT_2_BIT;
    }
    else
    {
        *max_sample_count_flag_bits = VK_SAMPLE_COUNT_1_BIT;
    }
}
void tg_graphics_vulkan_physical_device_is_suitable(VkPhysicalDevice physical_device, bool* p_is_suitable)
{
    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

    VkPhysicalDeviceFeatures physical_device_features;
    vkGetPhysicalDeviceFeatures(physical_device, &physical_device_features);

    const bool is_discrete_gpu = physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    const bool supports_geometry_shader = physical_device_features.geometryShader;
    const bool supports_sampler_anisotropy = physical_device_features.samplerAnisotropy;

    bool qfi_complete;
    tg_graphics_vulkan_physical_device_find_queue_families(physical_device, &graphics_queue, &present_queue, &qfi_complete);

    bool supports_extensions;
    tg_graphics_vulkan_physical_device_check_extension_support(physical_device, &supports_extensions);

    ui32 physical_device_surface_format_count;
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface.surface, &physical_device_surface_format_count, NULL));
    ui32 physical_device_present_mode_count;
    VK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface.surface, &physical_device_present_mode_count, NULL));

    *p_is_suitable = is_discrete_gpu && supports_geometry_shader && supports_sampler_anisotropy && qfi_complete && supports_extensions && physical_device_surface_format_count && physical_device_present_mode_count;
}
void tg_graphics_vulkan_depth_format_acquire(VkFormat* p_format)
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
void tg_graphics_vulkan_command_buffer_begin(VkCommandBuffer* p_command_buffer)
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
void tg_graphics_vulkan_command_buffer_end(VkCommandBuffer command_buffers)
{
    VK_CALL(vkEndCommandBuffer(command_buffers));

    VkSubmitInfo submit_info = { 0 };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    VK_CALL(vkQueueSubmit(graphics_queue.queue, 1, &submit_info, VK_NULL_HANDLE));
    VK_CALL(vkQueueWaitIdle(graphics_queue.queue));
    vkFreeCommandBuffers(device, command_pool, 1, &command_buffers);
}
void tg_graphics_vulkan_memory_type_find(ui32 memory_type_bits, VkMemoryPropertyFlags memory_property_flags, ui32* p_memory_type)
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
void tg_graphics_vulkan_buffer_create(VkDeviceSize size, VkBufferUsageFlags buffer_usage_flags, VkMemoryPropertyFlags memory_property_flags, VkBuffer* p_buffer, VkDeviceMemory* p_device_memory)
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
    tg_graphics_vulkan_memory_type_find(memory_requirements.memoryTypeBits, memory_property_flags, &memory_allocate_info.memoryTypeIndex);

    VK_CALL(vkAllocateMemory(device, &memory_allocate_info, NULL, p_device_memory));
    VK_CALL(vkBindBufferMemory(device, *p_buffer, *p_device_memory, 0));
}
void tg_graphics_vulkan_buffer_destroy(VkBuffer buffer, VkDeviceMemory device_memory)
{
    vkFreeMemory(device, device_memory, NULL);
    vkDestroyBuffer(device, buffer, NULL);
}
void tg_graphics_vulkan_image_allocate_memory(VkImage image, VkMemoryPropertyFlags memory_property_flags, VkDeviceMemory* p_device_memory)
{
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device, image, &memory_requirements);

    VkMemoryAllocateInfo memory_allocate_info = { 0 };
    {
        memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_allocate_info.pNext = NULL;
        memory_allocate_info.allocationSize = memory_requirements.size;
        tg_graphics_vulkan_memory_type_find(memory_requirements.memoryTypeBits, memory_property_flags, &memory_allocate_info.memoryTypeIndex);
    }
    VK_CALL(vkAllocateMemory(device, &memory_allocate_info, NULL, p_device_memory));
    VK_CALL(vkBindImageMemory(device, image, *p_device_memory, 0));
}
void tg_graphics_vulkan_image_create(ui32 width, ui32 height, ui32 mip_levels, VkFormat format, VkSampleCountFlagBits sample_count_flag_bits, VkImageTiling image_tiling, VkImageUsageFlags image_usage_flags, VkMemoryPropertyFlags memory_property_flags, VkImage* p_image, VkDeviceMemory* p_device_memory)
{
    VkImageCreateInfo image_create_info = { 0 };
    {
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
        image_create_info.samples = sample_count_flag_bits;
        image_create_info.tiling = image_tiling;
        image_create_info.usage = image_usage_flags;
        image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_create_info.queueFamilyIndexCount = 0;
        image_create_info.pQueueFamilyIndices = 0;
        image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
    VK_CALL(vkCreateImage(device, &image_create_info, NULL, p_image));
    tg_graphics_vulkan_image_allocate_memory(*p_image, memory_property_flags, p_device_memory);
}
void tg_graphics_vulkan_image_view_create(VkImage image, VkFormat format, ui32 mip_levels, VkImageAspectFlags image_aspect_flags, VkImageView* p_image_view)
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
void tg_graphics_vulkan_sampler_create(ui32 mip_levels, VkFilter min_filter, VkFilter mag_filter, VkSamplerAddressMode address_mode_u, VkSamplerAddressMode address_mode_v, VkSamplerAddressMode address_mode_w, VkSampler* p_sampler)
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
    sampler_create_info.maxLod = (float)mip_levels;
    sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;

    VK_CALL(vkCreateSampler(device, &sampler_create_info, NULL, p_sampler));
}
void tg_graphics_vulkan_image_mipmaps_generate(VkImage image, ui32 width, ui32 height, VkFormat format, ui32 mip_levels)
{
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(physical_device, format, &format_properties);
    TG_ASSERT(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

    VkCommandBuffer command_buffers;
    tg_graphics_vulkan_command_buffer_begin(&command_buffers);

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

        vkCmdPipelineBarrier(command_buffers, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);

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

        vkCmdBlitImage(command_buffers, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_blit, VK_FILTER_LINEAR);

        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(command_buffers, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);

        mip_width = next_mip_width;
        mip_height = next_mip_height;
    }

    image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_memory_barrier.subresourceRange.baseMipLevel = mip_levels - 1;

    vkCmdPipelineBarrier(command_buffers, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
    tg_graphics_vulkan_command_buffer_end(command_buffers);
}
void tg_graphics_vulkan_shader_module_create(const char* filename, VkShaderModule* p_shader_module)
{
    ui64 size = 0;
    char* content = NULL;
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
void tg_graphics_vulkan_buffer_copy(VkDeviceSize size, VkBuffer* p_source, VkBuffer* p_target)
{
    VkCommandBuffer command_buffers;
    tg_graphics_vulkan_command_buffer_begin(&command_buffers);

    VkBufferCopy buffer_copy = { 0 };
    buffer_copy.srcOffset = 0;
    buffer_copy.dstOffset = 0;
    buffer_copy.size = size;

    vkCmdCopyBuffer(command_buffers, *p_source, *p_target, 1, &buffer_copy);
    tg_graphics_vulkan_command_buffer_end(command_buffers);
}
void tg_graphics_vulkan_buffer_copy_to_image(ui32 width, ui32 height, VkBuffer* p_source, VkImage* p_target)
{
    VkCommandBuffer command_buffers;
    tg_graphics_vulkan_command_buffer_begin(&command_buffers);

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

    vkCmdCopyBufferToImage(command_buffers, *p_source, *p_target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
    tg_graphics_vulkan_command_buffer_end(command_buffers);
}
void tg_graphics_vulkan_image_transition_layout(VkImageLayout old_image_layout, VkImageLayout new_image_layout, ui32 mip_levels, VkImage* p_image)
{
    VkCommandBuffer command_buffers;
    tg_graphics_vulkan_command_buffer_begin(&command_buffers);

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

    vkCmdPipelineBarrier(command_buffers, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
    tg_graphics_vulkan_command_buffer_end(command_buffers);
}
void tg_graphics_vulkan_vertex_shader_layout_element_type_convert_to_vulkan_format(tg_vertex_shader_layout_element_type type, VkFormat* p_format)
{
    switch (type)
    {
    case TG_VERTEX_SHADER_LAYOUT_ELEMENT_TYPE_R32_SFLOAT:          *p_format = VK_FORMAT_R32_SFLOAT;          break;
    case TG_VERTEX_SHADER_LAYOUT_ELEMENT_TYPE_R32G32_SFLOAT:       *p_format = VK_FORMAT_R32G32_SFLOAT;       break;
    case TG_VERTEX_SHADER_LAYOUT_ELEMENT_TYPE_R32G32B32_SFLOAT:    *p_format = VK_FORMAT_R32G32B32_SFLOAT;    break;
    case TG_VERTEX_SHADER_LAYOUT_ELEMENT_TYPE_R32G32B32A32_SFLOAT: *p_format = VK_FORMAT_R32G32B32A32_SFLOAT; break;
    default: TG_ASSERT(false); break;
    }
}

void tg_graphics_vulkan_fence_create(VkFenceCreateFlags fence_create_flags, VkFence* p_fence)
{
    VkFenceCreateInfo fence_create_info = { 0 };
    {
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.pNext = NULL;
        fence_create_info.flags = fence_create_flags;
    }
    VK_CALL(vkCreateFence(device, &fence_create_info, NULL, p_fence));
}

void tg_graphics_vulkan_fence_destroy(VkFence fence)
{
    vkDestroyFence(device, fence, NULL);
}

void tg_graphics_vulkan_semaphore_create(VkSemaphore* p_semaphore)
{
    VkSemaphoreCreateInfo semaphore_create_info = { 0 };
    {
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_create_info.pNext = NULL;
        semaphore_create_info.flags = 0;
    }
    VK_CALL(vkCreateSemaphore(device, &semaphore_create_info, NULL, p_semaphore));
}

void tg_graphics_vulkan_semaphore_destroy(VkSemaphore semaphore)
{
    vkDestroySemaphore(device, semaphore, NULL);
}


// initialization
void tg_graphics_vulkan_instance_create(VkInstance* p_instance)
{
    TG_ASSERT(p_instance);

#ifdef TG_DEBUG
    ui32 layer_property_count;
    vkEnumerateInstanceLayerProperties(&layer_property_count, NULL);
    VkLayerProperties* layer_properties = tg_allocator_allocate(layer_property_count * sizeof(*layer_properties));
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
    tg_allocator_free(layer_properties);
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

    VK_CALL(vkCreateInstance(&instance_create_info, NULL, p_instance));
}
#ifdef TG_DEBUG
void tg_graphics_vulkan_debug_utils_manager_create(VkDebugUtilsMessengerEXT* p_debug_utils_messenger)
{
    TG_ASSERT(p_debug_utils_messenger);

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
    VK_CALL(vkCreateDebugUtilsMessengerEXT(instance, &debug_utils_messenger_create_info, NULL, p_debug_utils_messenger));
}
#endif
#ifdef TG_WIN32
void tg_graphics_vulkan_surface_create(tg_surface* p_surface)
{
    TG_ASSERT(p_surface);

    VkWin32SurfaceCreateInfoKHR win32_surface_create_info = { 0 };
    win32_surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    win32_surface_create_info.pNext = NULL;
    win32_surface_create_info.flags = 0;
    win32_surface_create_info.hinstance = GetModuleHandle(NULL);
    win32_surface_create_info.hwnd = tg_platform_get_window_handle();

    VK_CALL(vkCreateWin32SurfaceKHR(instance, &win32_surface_create_info, NULL, &p_surface->surface));
}
#endif
void tg_graphics_vulkan_physical_device_create(VkPhysicalDevice* p_physical_device)
{
    TG_ASSERT(p_physical_device);

    ui32 physical_device_count;
    VK_CALL(vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL));
    TG_ASSERT(physical_device_count);
    VkPhysicalDevice* physical_devices = tg_allocator_allocate(physical_device_count * sizeof(*physical_devices));
    VK_CALL(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices));

    for (ui32 i = 0; i < physical_device_count; i++)
    {
        bool is_suitable;
        tg_graphics_vulkan_physical_device_is_suitable(physical_devices[i], &is_suitable);
        if (is_suitable)
        {
            *p_physical_device = physical_devices[i];
            tg_graphics_vulkan_physical_device_find_max_sample_count(physical_devices[i], &surface.msaa_sample_count);
            break;
        }
    }
    TG_ASSERT(*p_physical_device != VK_NULL_HANDLE);

    tg_allocator_free(physical_devices);
}
void tg_graphics_vulkan_device_create(VkDevice* p_device)
{
    TG_ASSERT(p_device);

    const float queue_priority = 1.0f;

    VkDeviceQueueCreateInfo device_queue_create_infos[QUEUE_INDEX_COUNT] = { 0 };
    const ui32 queue_family_indices[] = { graphics_queue.index, present_queue.index };
    for (ui32 i = 0; i < QUEUE_INDEX_COUNT; i++)
    {
        VkDeviceQueueCreateInfo device_queue_create_info = { 0 };
        device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        device_queue_create_info.pNext = NULL;
        device_queue_create_info.flags = 0;
        device_queue_create_info.queueFamilyIndex = queue_family_indices[i];
        device_queue_create_info.queueCount = 1;
        device_queue_create_info.pQueuePriorities = &queue_priority;

        device_queue_create_infos[i] = device_queue_create_info;
    }

    VkPhysicalDeviceFeatures physical_device_features = { 0 };
    physical_device_features.robustBufferAccess = VK_FALSE;
    physical_device_features.fullDrawIndexUint32 = VK_FALSE;
    physical_device_features.imageCubeArray = VK_FALSE;
    physical_device_features.independentBlend = VK_FALSE;
    physical_device_features.geometryShader = VK_FALSE;
    physical_device_features.tessellationShader = VK_FALSE;
    physical_device_features.sampleRateShading = VK_TRUE;
    physical_device_features.dualSrcBlend = VK_FALSE;
    physical_device_features.logicOp = VK_FALSE;
    physical_device_features.multiDrawIndirect = VK_FALSE;
    physical_device_features.drawIndirectFirstInstance = VK_FALSE;
    physical_device_features.depthClamp = VK_FALSE;
    physical_device_features.depthBiasClamp = VK_FALSE;
    physical_device_features.fillModeNonSolid = VK_TRUE;
    physical_device_features.depthBounds = VK_FALSE;
    physical_device_features.wideLines = VK_TRUE;
    physical_device_features.largePoints = VK_FALSE;
    physical_device_features.alphaToOne = VK_FALSE;
    physical_device_features.multiViewport = VK_FALSE;
    physical_device_features.samplerAnisotropy = VK_TRUE;
    physical_device_features.textureCompressionETC2 = VK_FALSE;
    physical_device_features.textureCompressionASTC_LDR = VK_FALSE;
    physical_device_features.textureCompressionBC = VK_FALSE;
    physical_device_features.occlusionQueryPrecise = VK_FALSE;
    physical_device_features.pipelineStatisticsQuery = VK_FALSE;
    physical_device_features.vertexPipelineStoresAndAtomics = VK_FALSE;
    physical_device_features.fragmentStoresAndAtomics = VK_FALSE;
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

    VK_CALL(vkCreateDevice(physical_device, &device_create_info, NULL, p_device));
}
void tg_graphics_vulkan_queues_create(tg_queue* p_graphics_queue, tg_queue* p_present_queue)
{
    TG_ASSERT(p_graphics_queue && p_present_queue);

    vkGetDeviceQueue(device, p_graphics_queue->index, 0, &p_graphics_queue->queue);
    vkGetDeviceQueue(device, p_present_queue->index, 0, &p_present_queue->queue);
}
void tg_graphics_vulkan_command_pool_create(VkCommandPool* p_command_pool)
{
    TG_ASSERT(p_command_pool);

    VkCommandPoolCreateInfo command_pool_create_info = { 0 };
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext = NULL;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = graphics_queue.index;

    VK_CALL(vkCreateCommandPool(device, &command_pool_create_info, NULL, p_command_pool));
}
void tg_graphics_vulkan_swapchain_create()
{
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VK_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface.surface, &surface_capabilities));

    ui32 surface_format_count;
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface.surface, &surface_format_count, NULL));
    VkSurfaceFormatKHR* surface_formats = tg_allocator_allocate(surface_format_count * sizeof(*surface_formats));
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface.surface, &surface_format_count, surface_formats));
    surface.format = surface_formats[0];
    for (ui32 i = 0; i < surface_format_count; i++)
    {
        if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            surface.format = surface_formats[i];
            break;
        }
    }
    tg_allocator_free(surface_formats);

    VkPresentModeKHR present_modes[MAX_PRESENT_MODE_COUNT] = { 0 };
    ui32 max_present_mode_count = MAX_PRESENT_MODE_COUNT;
    VK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface.surface, &max_present_mode_count, present_modes));

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (ui32 i = 0; i < MAX_PRESENT_MODE_COUNT; i++)
    {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            present_mode = present_modes[i];
            break;
        }
    }

    tg_platform_get_window_size(&swapchain_extent.width, &swapchain_extent.height);
    swapchain_extent.width = tgm_ui32_clamp(swapchain_extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
    swapchain_extent.height = tgm_ui32_clamp(swapchain_extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);

    TG_ASSERT(tgm_ui32_clamp(SURFACE_IMAGE_COUNT, surface_capabilities.minImageCount, surface_capabilities.maxImageCount) == SURFACE_IMAGE_COUNT);

    VkSwapchainCreateInfoKHR swapchain_create_info = { 0 };
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = NULL;
    swapchain_create_info.flags = 0;
    swapchain_create_info.surface = surface.surface;
    swapchain_create_info.minImageCount = SURFACE_IMAGE_COUNT;
    swapchain_create_info.imageFormat = surface.format.format;
    swapchain_create_info.imageColorSpace = surface.format.colorSpace;
    swapchain_create_info.imageExtent = swapchain_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (graphics_queue.index != present_queue.index)
    {
        const ui32 queue_family_indices[] = { graphics_queue.index, present_queue.index };

        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
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
    VK_CALL(vkGetSwapchainImagesKHR(device, swapchain, &surface_image_count, swapchain_images));

    for (ui32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {
        tg_graphics_vulkan_image_view_create(swapchain_images[i], surface.format.format, 1, VK_IMAGE_ASPECT_COLOR_BIT, &swapchain_image_views[i]);
    }
}



// main
void tg_graphics_init()
{
    tg_graphics_vulkan_instance_create(&instance);
#ifdef TG_DEBUG
    tg_graphics_vulkan_debug_utils_manager_create(&debug_utils_messenger);
#endif
    tg_graphics_vulkan_surface_create(&surface);
    tg_graphics_vulkan_physical_device_create(&physical_device);
    tg_graphics_vulkan_device_create(&device);
    tg_graphics_vulkan_queues_create(&graphics_queue, &present_queue);
    tg_graphics_vulkan_command_pool_create(&command_pool);
    tg_graphics_vulkan_swapchain_create();
}
void tg_graphics_shutdown()
{
    vkDeviceWaitIdle(device);

    for (ui32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {
        vkDestroyImageView(device, swapchain_image_views[i], NULL);
    }
    vkDestroySwapchainKHR(device, swapchain, NULL);
    vkDestroyCommandPool(device, command_pool, NULL);
    vkDestroyDevice(device, NULL);
#ifdef TG_DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    TG_ASSERT(vkDestroyDebugUtilsMessengerEXT);
    vkDestroyDebugUtilsMessengerEXT(instance, debug_utils_messenger, NULL);
#endif
    vkDestroySurfaceKHR(instance, surface.surface, NULL);
    vkDestroyInstance(instance, NULL);
}
void tg_graphics_on_window_resize(ui32 width, ui32 height)
{
    vkDeviceWaitIdle(device);
    
    for (ui32 i = 0; i < SURFACE_IMAGE_COUNT; i++)
    {
        vkDestroyImageView(device, swapchain_image_views[i], NULL);
    }
    vkDestroySwapchainKHR(device, swapchain, NULL);
    tg_graphics_vulkan_swapchain_create();
}

#endif
