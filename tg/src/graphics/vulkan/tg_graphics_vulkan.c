#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "math/tg_math.h"
#include "memory/tg_memory.h"
#include "platform/tg_platform.h"
#include "tg_application.h"
#include "util/tg_file_io.h"
#include "util/tg_string.h"



#ifdef TG_WIN32

#ifdef TG_DEBUG
#define tg_VULKAN_INSTANCE_EXTENSION_COUNT 3
#define tg_VULKAN_INSTANCE_EXTENSION_NAMES (char*[]){ VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME }
#else
#define tg_VULKAN_INSTANCE_EXTENSION_COUNT 2
#define tg_VULKAN_INSTANCE_EXTENSION_NAMES (char*[]){ VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME }
#endif

#else

#define tg_VULKAN_INSTANCE_EXTENSION_COUNT 0
#define tg_VULKAN_INSTANCE_EXTENSION_NAMES TG_NULL

#endif

#define tg_VULKAN_DEVICE_EXTENSION_COUNT 1
#define tg_VULKAN_DEVICE_EXTENSION_NAMES (char*[]){ VK_KHR_SWAPCHAIN_EXTENSION_NAME }

#ifdef TG_DEBUG
#define tg_VULKAN_VALIDATION_LAYER_COUNT 1
#define tg_VULKAN_VALIDATION_LAYER_NAMES (char*[]){ "VK_LAYER_KHRONOS_validation" }
#else
#define tg_VULKAN_VALIDATION_LAYER_COUNT 0
#define tg_VULKAN_VALIDATION_LAYER_NAMES TG_NULL
#endif



#ifdef TG_DEBUG
VkDebugUtilsMessengerEXT    debug_utils_messenger;
#endif



#ifdef TG_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL tgvk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{
    TG_DEBUG_PRINT(callback_data->pMessage);
    return VK_TRUE;
}
#endif



/*------------------------------------------------------------+
| General utilities                                           |
+------------------------------------------------------------*/

u32 tg_vulkan_memory_type_find(u32 memory_type_bits, VkMemoryPropertyFlags memory_property_flags)
{
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_memory_properties);

    for (u32 i = 0; i < physical_device_memory_properties.memoryTypeCount; i++)
    {
        const b32 is_memory_type_suitable = memory_type_bits & (1 << i);
        const b32 are_property_flags_suitable = (physical_device_memory_properties.memoryTypes[i].propertyFlags & memory_property_flags) == memory_property_flags;
        if (is_memory_type_suitable && are_property_flags_suitable)
        {
            return i;
        }
    }
    TG_ASSERT(0);
    return -1;
}



VkDescriptorPool tg_vulkan_descriptor_pool_create(VkDescriptorPoolCreateFlags flags, u32 max_sets, u32 pool_size_count, const VkDescriptorPoolSize* pool_sizes)
{
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = { 0 };
    {
        descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool_create_info.pNext = TG_NULL;
        descriptor_pool_create_info.flags = flags;
        descriptor_pool_create_info.maxSets = max_sets;
        descriptor_pool_create_info.poolSizeCount = pool_size_count;
        descriptor_pool_create_info.pPoolSizes = pool_sizes;
    }
    VK_CALL(vkCreateDescriptorPool(device, &descriptor_pool_create_info, TG_NULL, &descriptor_pool));

    return descriptor_pool;
}

void tg_vulkan_descriptor_pool_destroy(VkDescriptorPool descriptor_pool)
{
    vkDestroyDescriptorPool(device, descriptor_pool, TG_NULL);
}



VkDescriptorSetLayout tg_vulkan_descriptor_set_layout_create(VkDescriptorSetLayoutCreateFlags flags, u32 binding_count, const VkDescriptorSetLayoutBinding* p_bindings)
{
    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = { 0 };
    {
        descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_set_layout_create_info.pNext = TG_NULL;
        descriptor_set_layout_create_info.flags = flags;
        descriptor_set_layout_create_info.bindingCount = binding_count;
        descriptor_set_layout_create_info.pBindings = p_bindings;
    }
    VK_CALL(vkCreateDescriptorSetLayout(device, &descriptor_set_layout_create_info, TG_NULL, &descriptor_set_layout));

    return descriptor_set_layout;
}

void tg_vulkan_descriptor_set_layout_destroy(VkDescriptorSetLayout descriptor_set_layout)
{
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, TG_NULL);
}



VkDescriptorSet tg_vulkan_descriptor_set_allocate(VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout)
{
    VkDescriptorSet descriptor_set = VK_NULL_HANDLE;

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = { 0 };
    {
        descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_allocate_info.pNext = TG_NULL;
        descriptor_set_allocate_info.descriptorPool = descriptor_pool;
        descriptor_set_allocate_info.descriptorSetCount = 1;
        descriptor_set_allocate_info.pSetLayouts = &descriptor_set_layout;
    }
    VK_CALL(vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &descriptor_set));

    return descriptor_set;
}

void tg_vulkan_descriptor_set_free(VkDescriptorPool descriptor_pool, VkDescriptorSet descriptor_set)
{
    VK_CALL(vkFreeDescriptorSets(device, descriptor_pool, 1, &descriptor_set));
}





void tg_vulkan_buffer_copy(VkDeviceSize size, VkBuffer source, VkBuffer destination)
{
    VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);

    VkBufferCopy buffer_copy = { 0 };
    {
        buffer_copy.srcOffset = 0;
        buffer_copy.dstOffset = 0;
        buffer_copy.size = size;
    }
    vkCmdCopyBuffer(command_buffer, source, destination, 1, &buffer_copy);

    tg_vulkan_command_buffer_end_and_submit(command_buffer, &graphics_queue);
    tg_vulkan_command_buffer_free(graphics_command_pool, command_buffer);
}

tg_vulkan_buffer tg_vulkan_buffer_create(VkDeviceSize size, VkBufferUsageFlags buffer_usage_flags, VkMemoryPropertyFlags memory_property_flags)
{
    tg_vulkan_buffer buffer = { 0 };

    buffer.size = size;

    VkBufferCreateInfo buffer_create_info = { 0 };
    {
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.pNext = TG_NULL;
        buffer_create_info.flags = 0;
        buffer_create_info.size = size;
        buffer_create_info.usage = buffer_usage_flags;
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.queueFamilyIndexCount = 0;
        buffer_create_info.pQueueFamilyIndices = TG_NULL;
    }
    VK_CALL(vkCreateBuffer(device, &buffer_create_info, TG_NULL, &buffer.buffer));

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, buffer.buffer, &memory_requirements);
    VkMemoryAllocateInfo memory_allocate_info = { 0 };
    {
        memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_allocate_info.pNext = TG_NULL;
        memory_allocate_info.allocationSize = memory_requirements.size;
        memory_allocate_info.memoryTypeIndex = tg_vulkan_memory_type_find(memory_requirements.memoryTypeBits, memory_property_flags);
    }
    VK_CALL(vkAllocateMemory(device, &memory_allocate_info, TG_NULL, &buffer.device_memory));
    VK_CALL(vkBindBufferMemory(device, buffer.buffer, buffer.device_memory, 0));

    if (memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        VK_CALL(vkMapMemory(device, buffer.device_memory, 0, buffer.size, 0, &buffer.p_mapped_device_memory));
    }
    else
    {
        buffer.p_mapped_device_memory = TG_NULL;
    }

    return buffer;
}

void tg_vulkan_buffer_destroy(tg_vulkan_buffer* p_vulkan_buffer)
{
    vkFreeMemory(device, p_vulkan_buffer->device_memory, TG_NULL);
    vkDestroyBuffer(device, p_vulkan_buffer->buffer, TG_NULL);
}



tg_color_image tg_vulkan_color_image_create(const tg_vulkan_color_image_create_info* p_vulkan_color_image_create_info)
{
    tg_color_image color_image = { 0 };

    color_image.width = p_vulkan_color_image_create_info->width;
    color_image.height = p_vulkan_color_image_create_info->height;
    color_image.mip_levels = p_vulkan_color_image_create_info->mip_levels;
    color_image.format = p_vulkan_color_image_create_info->format;

    VkImageCreateInfo image_create_info = { 0 };
    {
        image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.pNext = TG_NULL;
        image_create_info.flags = 0;
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.format = color_image.format;
        image_create_info.extent.width = color_image.width;
        image_create_info.extent.height = color_image.height;
        image_create_info.extent.depth = 1;
        image_create_info.mipLevels = color_image.mip_levels;
        image_create_info.arrayLayers = 1;
        image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_create_info.queueFamilyIndexCount = 0;
        image_create_info.pQueueFamilyIndices = 0;
        image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
    VK_CALL(vkCreateImage(device, &image_create_info, TG_NULL, &color_image.color_image));

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device, color_image.color_image, &memory_requirements);
    VkMemoryAllocateInfo memory_allocate_info = { 0 };
    {
        memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_allocate_info.pNext = TG_NULL;
        memory_allocate_info.allocationSize = memory_requirements.size;
        memory_allocate_info.memoryTypeIndex = tg_vulkan_memory_type_find(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
    VK_CALL(vkAllocateMemory(device, &memory_allocate_info, TG_NULL, &color_image.device_memory));
    VK_CALL(vkBindImageMemory(device, color_image.color_image, color_image.device_memory, 0));

    // TODO: mipmapping
    //VkCommandBuffer command_buffer = tg_vulkan_command_buffer_allocate(graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    //tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
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
    //tg_vulkan_command_buffer_end_and_submit(command_buffer, &graphics_queue);
    //tg_vulkan_command_buffer_free(graphics_command_pool, command_buffer);

    VkImageViewCreateInfo image_view_create_info = { 0 };
    {
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.pNext = TG_NULL;
        image_view_create_info.flags = 0;
        image_view_create_info.image = color_image.color_image;
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format = color_image.format;
        image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = color_image.mip_levels;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;
    }
    VK_CALL(vkCreateImageView(device, &image_view_create_info, TG_NULL, &color_image.image_view));

    VkSamplerCreateInfo sampler_create_info = { 0 };
    {
        sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_create_info.pNext = TG_NULL;
        sampler_create_info.flags = 0;
        sampler_create_info.magFilter = p_vulkan_color_image_create_info->mag_filter;
        sampler_create_info.minFilter = p_vulkan_color_image_create_info->min_filter;
        sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_create_info.addressModeU = p_vulkan_color_image_create_info->address_mode_u;
        sampler_create_info.addressModeV = p_vulkan_color_image_create_info->address_mode_v;
        sampler_create_info.addressModeW = p_vulkan_color_image_create_info->address_mode_w;
        sampler_create_info.mipLodBias = 0.0f;
        sampler_create_info.anisotropyEnable = VK_TRUE;
        sampler_create_info.maxAnisotropy = 16.0f;
        sampler_create_info.compareEnable = VK_FALSE;
        sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_create_info.minLod = 0.0f;
        sampler_create_info.maxLod = (f32)color_image.mip_levels;
        sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_create_info.unnormalizedCoordinates = VK_FALSE;
    }
    VK_CALL(vkCreateSampler(device, &sampler_create_info, TG_NULL, &color_image.sampler));

    return color_image;
}

VkFormat tg_vulkan_color_image_convert_format(tg_color_image_format color_image_format)
{
    switch (color_image_format)
    {
    case TG_COLOR_IMAGE_FORMAT_A8B8G8R8:            return VK_FORMAT_R8G8B8A8_SRGB;
    case TG_COLOR_IMAGE_FORMAT_A8R8G8B8:            return VK_FORMAT_A8B8G8R8_SRGB_PACK32;
    case TG_COLOR_IMAGE_FORMAT_B8G8R8A8:            return VK_FORMAT_B8G8R8A8_SRGB;
    case TG_COLOR_IMAGE_FORMAT_R16G16B16A16_SFLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
    case TG_COLOR_IMAGE_FORMAT_R32G32B32A32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case TG_COLOR_IMAGE_FORMAT_R8:                  return VK_FORMAT_R8_SRGB;
    case TG_COLOR_IMAGE_FORMAT_R8G8:                return VK_FORMAT_R8G8_SRGB;
    case TG_COLOR_IMAGE_FORMAT_R8G8B8:              return VK_FORMAT_R8G8B8_SRGB;
    case TG_COLOR_IMAGE_FORMAT_R8G8B8A8:            return VK_FORMAT_R8G8B8A8_SRGB;
    }

    TG_ASSERT(TG_FALSE);
    return -1;
}

void tg_vulkan_color_image_destroy(tg_color_image* p_color_image)
{
    vkDestroySampler(device, p_color_image->sampler, TG_NULL);
    vkDestroyImageView(device, p_color_image->image_view, TG_NULL);
    vkFreeMemory(device, p_color_image->device_memory, TG_NULL);
    vkDestroyImage(device, p_color_image->color_image, TG_NULL);
}



VkCommandBuffer tg_vulkan_command_buffer_allocate(VkCommandPool command_pool, VkCommandBufferLevel level)
{
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;

    VkCommandBufferAllocateInfo command_buffer_allocate_info = { 0 };
    {
        command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_allocate_info.pNext = TG_NULL;
        command_buffer_allocate_info.commandPool = command_pool;
        command_buffer_allocate_info.level = level;
        command_buffer_allocate_info.commandBufferCount = 1;
    }
    VK_CALL(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &command_buffer));

    return command_buffer;
}

void tg_vulkan_command_buffer_begin(VkCommandBuffer command_buffer, VkCommandBufferUsageFlags usage_flags, const VkCommandBufferInheritanceInfo* p_inheritance_info)
{
    VkCommandBufferBeginInfo command_buffer_begin_info = { 0 };
    {
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = TG_NULL;
        command_buffer_begin_info.flags = usage_flags;
        command_buffer_begin_info.pInheritanceInfo = p_inheritance_info;
    }
    VK_CALL(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));
}

void tg_vulkan_command_buffer_cmd_begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, VkSubpassContents subpass_contents)
{
    VkRenderPassBeginInfo render_pass_begin_info = { 0 };
    {
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext = TG_NULL;
        render_pass_begin_info.renderPass = render_pass;
        render_pass_begin_info.framebuffer = framebuffer;
        render_pass_begin_info.renderArea.offset = (VkOffset2D){ 0, 0 };
        render_pass_begin_info.renderArea.extent = swapchain_extent;
        render_pass_begin_info.clearValueCount = 0;
        render_pass_begin_info.pClearValues = TG_NULL;
    }
    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, subpass_contents);
}

void tg_vulkan_command_buffer_cmd_clear_color_image(VkCommandBuffer command_buffer, tg_color_image* p_color_image)
{
    VkImageSubresourceRange image_subresource_range = { 0 };
    {
        image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_subresource_range.baseMipLevel = 0;
        image_subresource_range.levelCount = p_color_image->mip_levels;
        image_subresource_range.baseArrayLayer = 0;
        image_subresource_range.layerCount = 1;
    }
    VkClearColorValue clear_color_value = { 0 };
    {
        clear_color_value.float32[0] = 0.0f;
        clear_color_value.float32[1] = 0.0f;
        clear_color_value.float32[2] = 0.0f;
        clear_color_value.float32[3] = 0.0f;
    }
    vkCmdClearColorImage(command_buffer, p_color_image->color_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color_value, 1, &image_subresource_range);
}

void tg_vulkan_command_buffer_cmd_clear_depth_image(VkCommandBuffer command_buffer, tg_depth_image* p_depth_image)
{

    VkImageSubresourceRange image_subresource_range = { 0 };
    {
        image_subresource_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        image_subresource_range.baseMipLevel = 0;
        image_subresource_range.levelCount = 1;
        image_subresource_range.baseArrayLayer = 0;
        image_subresource_range.layerCount = 1;
    }
    VkClearDepthStencilValue clear_depth_stencil_value = { 0 };
    {
        clear_depth_stencil_value.depth = 1.0f;
        clear_depth_stencil_value.stencil = 0;
    }
    vkCmdClearDepthStencilImage(command_buffer, p_depth_image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_depth_stencil_value, 1, &image_subresource_range);
}

void tg_vulkan_command_buffer_cmd_copy_buffer_to_color_image(VkCommandBuffer command_buffer, VkBuffer source, tg_color_image* p_destination)
{
    VkBufferImageCopy buffer_image_copy = { 0 };
    {
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
    }
    vkCmdCopyBufferToImage(command_buffer, source, p_destination->color_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
}

void tg_vulkan_command_buffer_cmd_copy_buffer_to_depth_image(VkCommandBuffer command_buffer, VkBuffer source, tg_depth_image* p_destination)
{
    VkBufferImageCopy buffer_image_copy = { 0 };
    {
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
    }
    vkCmdCopyBufferToImage(command_buffer, source, p_destination->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
}

void tg_vulkan_command_buffer_cmd_copy_color_image_to_buffer(VkCommandBuffer command_buffer, tg_color_image* p_source, VkBuffer destination)
{
    VkBufferImageCopy buffer_image_copy = { 0 };
    {
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
    }
    vkCmdCopyImageToBuffer(command_buffer, p_source->color_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, destination, 1, &buffer_image_copy);
}

void tg_vulkan_command_buffer_cmd_transition_color_image_layout(VkCommandBuffer command_buffer, tg_color_image* p_color_image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits)
{
    VkImageMemoryBarrier image_memory_barrier = { 0 };
    {
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = TG_NULL;
        image_memory_barrier.srcAccessMask = src_access_mask;
        image_memory_barrier.dstAccessMask = dst_access_mask;
        image_memory_barrier.oldLayout = old_layout;
        image_memory_barrier.newLayout = new_layout;
        image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.image = p_color_image->color_image;
        image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_memory_barrier.subresourceRange.baseMipLevel = 0;
        image_memory_barrier.subresourceRange.levelCount = p_color_image->mip_levels;
        image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        image_memory_barrier.subresourceRange.layerCount = 1;
    }
    vkCmdPipelineBarrier(command_buffer, src_stage_bits, dst_stage_bits, 0, 0, TG_NULL, 0, TG_NULL, 1, &image_memory_barrier);
}

void tg_vulkan_command_buffer_cmd_transition_depth_image_layout(VkCommandBuffer command_buffer, tg_depth_image* p_depth_image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage_bits, VkPipelineStageFlags dst_stage_bits)
{
    VkImageMemoryBarrier image_memory_barrier = { 0 };
    {
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = TG_NULL;
        image_memory_barrier.srcAccessMask = src_access_mask;
        image_memory_barrier.dstAccessMask = dst_access_mask;
        image_memory_barrier.oldLayout = old_layout;
        image_memory_barrier.newLayout = new_layout;
        image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.image = p_depth_image->image;
        image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        image_memory_barrier.subresourceRange.baseMipLevel = 0;
        image_memory_barrier.subresourceRange.levelCount = 1;
        image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        image_memory_barrier.subresourceRange.layerCount = 1;
    }
    vkCmdPipelineBarrier(command_buffer, src_stage_bits, dst_stage_bits, 0, 0, TG_NULL, 0, TG_NULL, 1, &image_memory_barrier);
}

void tg_vulkan_command_buffer_end_and_submit(VkCommandBuffer command_buffer, tg_vulkan_queue* p_vulkan_queue)
{
    VK_CALL(vkEndCommandBuffer(command_buffer));

    VkSubmitInfo submit_info = { 0 };
    {
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = TG_NULL;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = TG_NULL;
        submit_info.pWaitDstStageMask = TG_NULL;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = TG_NULL;
    }
    VK_CALL(vkQueueSubmit(p_vulkan_queue->queue, 1, &submit_info, VK_NULL_HANDLE));

    VK_CALL(vkQueueWaitIdle(p_vulkan_queue->queue));
}

void tg_vulkan_command_buffer_free(VkCommandPool command_pool, VkCommandBuffer command_buffer)
{
    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

void tg_vulkan_command_buffers_allocate(VkCommandPool command_pool, VkCommandBufferLevel level, u32 command_buffer_count, VkCommandBuffer* p_command_buffers)
{
    VkCommandBufferAllocateInfo command_buffer_allocate_info = { 0 };
    {
        command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_allocate_info.pNext = TG_NULL;
        command_buffer_allocate_info.commandPool = command_pool;
        command_buffer_allocate_info.level = level;
        command_buffer_allocate_info.commandBufferCount = command_buffer_count;
    }
    VK_CALL(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, p_command_buffers));
}

void tg_vulkan_command_buffers_free(VkCommandPool command_pool, u32 command_buffer_count, const VkCommandBuffer* p_command_buffers)
{
    vkFreeCommandBuffers(device, command_pool, command_buffer_count, p_command_buffers);
}



VkPipeline tg_vulkan_compute_pipeline_create(VkShaderModule shader_module, VkPipelineLayout pipeline_layout)
{
    VkPipeline compute_pipeline = VK_NULL_HANDLE;

    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info = { 0 };
    {
        pipeline_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_shader_stage_create_info.pNext = TG_NULL;
        pipeline_shader_stage_create_info.flags = 0;
        pipeline_shader_stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        pipeline_shader_stage_create_info.module = shader_module;
        pipeline_shader_stage_create_info.pName = "main";
        pipeline_shader_stage_create_info.pSpecializationInfo = TG_NULL;
    }
    VkComputePipelineCreateInfo compute_pipeline_create_info = { 0 };
    {
        compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        compute_pipeline_create_info.pNext = TG_NULL;
        compute_pipeline_create_info.flags = 0;
        compute_pipeline_create_info.stage = pipeline_shader_stage_create_info;
        compute_pipeline_create_info.layout = pipeline_layout;
        compute_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        compute_pipeline_create_info.basePipelineIndex = -1;
    }
    VK_CALL(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &compute_pipeline_create_info, TG_NULL, &compute_pipeline));

    return compute_pipeline;
}

void tg_vulkan_compute_pipeline_destroy(VkPipeline compute_pipeline)
{
    vkDestroyPipeline(device, compute_pipeline, TG_NULL);
}



tg_vulkan_compute_shader tg_vulkan_compute_shader_create(const char* filename, u32 binding_count, VkDescriptorSetLayoutBinding* p_bindings)
{
    tg_vulkan_compute_shader vulkan_compute_shader = { 0 };

    vulkan_compute_shader.shader_module = tg_vulkan_shader_module_create(filename);
    vulkan_compute_shader.descriptor = tg_vulkan_descriptor_create(binding_count, p_bindings);
    vulkan_compute_shader.pipeline_layout = tg_vulkan_pipeline_layout_create(1, &vulkan_compute_shader.descriptor.descriptor_set_layout, 0, TG_NULL);
    vulkan_compute_shader.compute_pipeline = tg_vulkan_compute_pipeline_create(vulkan_compute_shader.shader_module, vulkan_compute_shader.pipeline_layout);

    return vulkan_compute_shader;
}

void tg_vulkan_compute_shader_destroy(tg_vulkan_compute_shader* p_vulkan_compute_shader)
{
    tg_vulkan_shader_module_destroy(p_vulkan_compute_shader->shader_module);
    tg_vulkan_descriptor_destroy(&p_vulkan_compute_shader->descriptor);
    tg_vulkan_pipeline_layout_destroy(p_vulkan_compute_shader->pipeline_layout);
    tg_vulkan_compute_pipeline_destroy(p_vulkan_compute_shader->compute_pipeline);
}



tg_depth_image tg_vulkan_depth_image_create(const tg_vulkan_depth_image_create_info* p_vulkan_depth_image_create_info)
{
    tg_depth_image depth_image = { 0 };

    depth_image.width = p_vulkan_depth_image_create_info->width;
    depth_image.height = p_vulkan_depth_image_create_info->height;
    depth_image.format = p_vulkan_depth_image_create_info->format;

    VkImageCreateInfo image_create_info = { 0 };
    {
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
        image_create_info.pQueueFamilyIndices = 0;
        image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
    VK_CALL(vkCreateImage(device, &image_create_info, TG_NULL, &depth_image.image));

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device, depth_image.image, &memory_requirements);
    VkMemoryAllocateInfo memory_allocate_info = { 0 };
    {
        memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_allocate_info.pNext = TG_NULL;
        memory_allocate_info.allocationSize = memory_requirements.size;
        memory_allocate_info.memoryTypeIndex = tg_vulkan_memory_type_find(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
    VK_CALL(vkAllocateMemory(device, &memory_allocate_info, TG_NULL, &depth_image.device_memory));
    VK_CALL(vkBindImageMemory(device, depth_image.image, depth_image.device_memory, 0));

    VkImageViewCreateInfo image_view_create_info = { 0 };
    {
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.pNext = TG_NULL;
        image_view_create_info.flags = 0;
        image_view_create_info.image = depth_image.image;
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format = depth_image.format;
        image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;
    }
    VK_CALL(vkCreateImageView(device, &image_view_create_info, TG_NULL, &depth_image.image_view));

    VkSamplerCreateInfo sampler_create_info = { 0 };
    {
        sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_create_info.pNext = TG_NULL;
        sampler_create_info.flags = 0;
        sampler_create_info.magFilter = p_vulkan_depth_image_create_info->mag_filter;
        sampler_create_info.minFilter = p_vulkan_depth_image_create_info->min_filter;
        sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_create_info.addressModeU = p_vulkan_depth_image_create_info->address_mode_u;
        sampler_create_info.addressModeV = p_vulkan_depth_image_create_info->address_mode_v;
        sampler_create_info.addressModeW = p_vulkan_depth_image_create_info->address_mode_w;
        sampler_create_info.mipLodBias = 0.0f;
        sampler_create_info.anisotropyEnable = VK_TRUE;
        sampler_create_info.maxAnisotropy = 16.0f;
        sampler_create_info.compareEnable = VK_FALSE;
        sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_create_info.minLod = 0.0f;
        sampler_create_info.maxLod = 1.0f;
        sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_create_info.unnormalizedCoordinates = VK_FALSE;
    }
    VK_CALL(vkCreateSampler(device, &sampler_create_info, TG_NULL, &depth_image.sampler));

    return depth_image;
}

VkFormat tg_vulkan_depth_image_convert_format(tg_depth_image_format depth_image_format)
{
    switch (depth_image_format)
    {
    case TG_DEPTH_IMAGE_FORMAT_D16_UNORM:  return VK_FORMAT_D16_UNORM;
    case TG_DEPTH_IMAGE_FORMAT_D32_SFLOAT: return VK_FORMAT_D32_SFLOAT;
    }

    TG_ASSERT(TG_FALSE);
    return -1;
}

void tg_vulkan_depth_image_destroy(tg_depth_image* p_depth_image)
{
    vkDestroySampler(device, p_depth_image->sampler, TG_NULL);
    vkDestroyImageView(device, p_depth_image->image_view, TG_NULL);
    vkFreeMemory(device, p_depth_image->device_memory, TG_NULL);
    vkDestroyImage(device, p_depth_image->image, TG_NULL);
}



tg_vulkan_descriptor tg_vulkan_descriptor_create(u32 binding_count, VkDescriptorSetLayoutBinding* p_bindings)
{
    tg_vulkan_descriptor vulkan_descriptor = { 0 };

    VkDescriptorPoolSize* p_descriptor_pool_sizes = TG_MEMORY_ALLOC(binding_count * sizeof(*p_descriptor_pool_sizes));
    for (u32 i = 0; i < binding_count; i++)
    {
        p_descriptor_pool_sizes[i].type = p_bindings[i].descriptorType;
        p_descriptor_pool_sizes[i].descriptorCount = p_bindings[i].descriptorCount;
    }
    vulkan_descriptor.descriptor_pool = tg_vulkan_descriptor_pool_create(0, 1, 1, p_descriptor_pool_sizes);
    TG_MEMORY_FREE(p_descriptor_pool_sizes);

    vulkan_descriptor.descriptor_set_layout = tg_vulkan_descriptor_set_layout_create(0, binding_count, p_bindings);
    vulkan_descriptor.descriptor_set = tg_vulkan_descriptor_set_allocate(vulkan_descriptor.descriptor_pool, vulkan_descriptor.descriptor_set_layout);

    return vulkan_descriptor;
}

void tg_vulkan_descriptor_destroy(tg_vulkan_descriptor* p_vulkan_descriptor)
{
    tg_vulkan_descriptor_set_layout_destroy(p_vulkan_descriptor->descriptor_set_layout);
    tg_vulkan_descriptor_pool_destroy(p_vulkan_descriptor->descriptor_pool);
}



void tg_vulkan_descriptor_set_update(VkDescriptorSet descriptor_set, tg_shader_input_element* p_shader_input_element, tg_handle shader_input_element_handle, u32 dst_binding)
{
    switch (p_shader_input_element->type) // TODO: arrays not supported: see e.g. tg_vulkan_descriptor_set_update_storage_buffer_array
    {
    case TG_SHADER_INPUT_ELEMENT_TYPE_COLOR_IMAGE:
    {
        tg_color_image_h color_image_h = (tg_color_image_h)shader_input_element_handle;
        tg_vulkan_descriptor_set_update_color_image(descriptor_set, color_image_h, dst_binding);
    } break;
    case TG_SHADER_INPUT_ELEMENT_TYPE_COMPUTE_BUFFER:
    {
        tg_compute_buffer_h compute_buffer_h = (tg_compute_buffer_h)shader_input_element_handle;
        tg_vulkan_descriptor_set_update_storage_buffer(descriptor_set, compute_buffer_h->buffer.buffer, dst_binding);
    } break;
    case TG_SHADER_INPUT_ELEMENT_TYPE_UNIFORM_BUFFER:
    {
        tg_uniform_buffer_h uniform_buffer_h = (tg_uniform_buffer_h)shader_input_element_handle;
        tg_vulkan_descriptor_set_update_uniform_buffer(descriptor_set, uniform_buffer_h->buffer.buffer, dst_binding);
    } break;
    default: TG_ASSERT(TG_FALSE);
    }
}

void tg_vulkan_descriptor_set_update_color_image(VkDescriptorSet descriptor_set, tg_color_image* p_color_image, u32 dst_binding)
{
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    {
        descriptor_image_info.sampler = p_color_image->sampler;
        descriptor_image_info.imageView = p_color_image->image_view;
        descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    VkWriteDescriptorSet write_descriptor_set = { 0 };
    {
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
    }
    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
}

void tg_vulkan_descriptor_set_update_color_image_array(VkDescriptorSet descriptor_set, tg_color_image* p_color_image, u32 dst_binding, u32 array_index)
{
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    {
        descriptor_image_info.sampler = p_color_image->sampler;
        descriptor_image_info.imageView = p_color_image->image_view;
        descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    VkWriteDescriptorSet write_descriptor_set = { 0 };
    {
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
    }
    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
}

void tg_vulkan_descriptor_set_update_depth_image(VkDescriptorSet descriptor_set, tg_depth_image* p_depth_image, u32 dst_binding)
{
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    {
        descriptor_image_info.sampler = p_depth_image->sampler;
        descriptor_image_info.imageView = p_depth_image->image_view;
        descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    VkWriteDescriptorSet write_descriptor_set = { 0 };
    {
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
    }
    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
}

void tg_vulkan_descriptor_set_update_depth_image_array(VkDescriptorSet descriptor_set, tg_color_image* p_depth_image, u32 dst_binding, u32 array_index)
{
    VkDescriptorImageInfo descriptor_image_info = { 0 };
    {
        descriptor_image_info.sampler = p_depth_image->sampler;
        descriptor_image_info.imageView = p_depth_image->image_view;
        descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    VkWriteDescriptorSet write_descriptor_set = { 0 };
    {
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
    }
    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
}

void tg_vulkan_descriptor_set_update_storage_buffer(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding)
{
    VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
    {
        descriptor_buffer_info.buffer = buffer;
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = VK_WHOLE_SIZE;
    }
    VkWriteDescriptorSet write_descriptor_set = { 0 };
    {
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
    }
    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
}

void tg_vulkan_descriptor_set_update_storage_buffer_array(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding, u32 array_index)
{
    VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
    {
        descriptor_buffer_info.buffer = buffer;
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = VK_WHOLE_SIZE;
    }
    VkWriteDescriptorSet write_descriptor_set = { 0 };
    {
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
    }
    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
}

void tg_vulkan_descriptor_set_update_uniform_buffer(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding)
{
    VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
    {
        descriptor_buffer_info.buffer = buffer;
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = VK_WHOLE_SIZE;
    }
    VkWriteDescriptorSet write_descriptor_set = { 0 };
    {
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
    }
    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
}

void tg_vulkan_descriptor_set_update_uniform_buffer_array(VkDescriptorSet descriptor_set, VkBuffer buffer, u32 dst_binding, u32 array_index)
{
    VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
    {
        descriptor_buffer_info.buffer = buffer;
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = VK_WHOLE_SIZE;
    }
    VkWriteDescriptorSet write_descriptor_set = { 0 };
    {
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
    }
    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
}

void tg_vulkan_descriptor_sets_update(u32 write_descriptor_set_count, const VkWriteDescriptorSet* p_write_descriptor_sets)
{
    vkUpdateDescriptorSets(device, write_descriptor_set_count, p_write_descriptor_sets, 0, TG_NULL);
}



VkFence tg_vulkan_fence_create(VkFenceCreateFlags fence_create_flags)
{
    VkFence fence = VK_NULL_HANDLE;

    VkFenceCreateInfo fence_create_info = { 0 };
    {
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.pNext = TG_NULL;
        fence_create_info.flags = fence_create_flags;
    }
    VK_CALL(vkCreateFence(device, &fence_create_info, TG_NULL, &fence));

    return fence;
}

void tg_vulkan_fence_destroy(VkFence fence)
{
    vkDestroyFence(device, fence, TG_NULL);
}

void tg_vulkan_fence_reset(VkFence fence)
{
    VK_CALL(vkResetFences(device, 1, &fence));
}

void tg_vulkan_fence_wait(VkFence fence)
{
    VK_CALL(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));
}



VkFramebuffer tg_vulkan_framebuffer_create(VkRenderPass render_pass, u32 attachment_count, const VkImageView* p_attachments, u32 width, u32 height)
{
    VkFramebuffer framebuffer = VK_NULL_HANDLE;

    VkFramebufferCreateInfo framebuffer_create_info = { 0 };
    {
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = TG_NULL;
        framebuffer_create_info.flags = 0;
        framebuffer_create_info.renderPass = render_pass;
        framebuffer_create_info.attachmentCount = attachment_count;
        framebuffer_create_info.pAttachments = p_attachments;
        framebuffer_create_info.width = width;
        framebuffer_create_info.height = height;
        framebuffer_create_info.layers = 1;
    }
    VK_CALL(vkCreateFramebuffer(device, &framebuffer_create_info, TG_NULL, &framebuffer));

    return framebuffer;
}

void tg_vulkan_framebuffer_destroy(VkFramebuffer framebuffer)
{
    vkDestroyFramebuffer(device, framebuffer, TG_NULL);
}



VkPipeline tg_vulkan_graphics_pipeline_create(const tg_vulkan_graphics_pipeline_create_info* p_vulkan_graphics_pipeline_create_info)
{
    VkPipeline graphics_pipeline = VK_NULL_HANDLE;

    VkPipelineShaderStageCreateInfo p_pipeline_shader_stage_create_infos[2] = { 0 };
    {
        p_pipeline_shader_stage_create_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        p_pipeline_shader_stage_create_infos[0].pNext = TG_NULL;
        p_pipeline_shader_stage_create_infos[0].flags = 0;
        p_pipeline_shader_stage_create_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        p_pipeline_shader_stage_create_infos[0].module = p_vulkan_graphics_pipeline_create_info->vertex_shader;
        p_pipeline_shader_stage_create_infos[0].pName = "main";
        p_pipeline_shader_stage_create_infos[0].pSpecializationInfo = TG_NULL;

        p_pipeline_shader_stage_create_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        p_pipeline_shader_stage_create_infos[1].pNext = TG_NULL;
        p_pipeline_shader_stage_create_infos[1].flags = 0;
        p_pipeline_shader_stage_create_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        p_pipeline_shader_stage_create_infos[1].module = p_vulkan_graphics_pipeline_create_info->fragment_shader;
        p_pipeline_shader_stage_create_infos[1].pName = "main";
        p_pipeline_shader_stage_create_infos[1].pSpecializationInfo = TG_NULL;
    }
    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info = { 0 };
    {
        pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        pipeline_input_assembly_state_create_info.pNext = TG_NULL;
        pipeline_input_assembly_state_create_info.flags = 0;
        pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
    }
    VkViewport viewport = { 0 };
    {
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (f32)swapchain_extent.width;
        viewport.height = (f32)swapchain_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
    }
    VkRect2D scissors = { 0 };
    {
        scissors.offset = (VkOffset2D){ 0, 0 };
        scissors.extent = swapchain_extent;
    }
    VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = { 0 };
    {
        pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        pipeline_viewport_state_create_info.pNext = TG_NULL;
        pipeline_viewport_state_create_info.flags = 0;
        pipeline_viewport_state_create_info.viewportCount = 1;
        pipeline_viewport_state_create_info.pViewports = &viewport;
        pipeline_viewport_state_create_info.scissorCount = 1;
        pipeline_viewport_state_create_info.pScissors = &scissors;
    }
    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info = { 0 };
    {
        pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        pipeline_rasterization_state_create_info.pNext = TG_NULL;
        pipeline_rasterization_state_create_info.flags = 0;
        pipeline_rasterization_state_create_info.depthClampEnable = VK_FALSE;
        pipeline_rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        pipeline_rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        pipeline_rasterization_state_create_info.cullMode = p_vulkan_graphics_pipeline_create_info->cull_mode;
        pipeline_rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        pipeline_rasterization_state_create_info.depthBiasEnable = VK_FALSE;
        pipeline_rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
        pipeline_rasterization_state_create_info.depthBiasClamp = 0.0f;
        pipeline_rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;
        pipeline_rasterization_state_create_info.lineWidth = 1.0f;
    }
    VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = { 0 };
    {
        pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        pipeline_multisample_state_create_info.pNext = TG_NULL;
        pipeline_multisample_state_create_info.flags = 0;
        pipeline_multisample_state_create_info.rasterizationSamples = p_vulkan_graphics_pipeline_create_info->sample_count;
        pipeline_multisample_state_create_info.sampleShadingEnable = p_vulkan_graphics_pipeline_create_info->sample_count == VK_SAMPLE_COUNT_1_BIT ? VK_FALSE : VK_TRUE;
        pipeline_multisample_state_create_info.minSampleShading = p_vulkan_graphics_pipeline_create_info->sample_count == VK_SAMPLE_COUNT_1_BIT ? 0.0f : 1.0f;
        pipeline_multisample_state_create_info.pSampleMask = TG_NULL;
        pipeline_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        pipeline_multisample_state_create_info.alphaToOneEnable = VK_FALSE;
    }
    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = { 0 };
    {
        pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipeline_depth_stencil_state_create_info.pNext = TG_NULL;
        pipeline_depth_stencil_state_create_info.flags = 0;
        pipeline_depth_stencil_state_create_info.depthTestEnable = p_vulkan_graphics_pipeline_create_info->depth_test_enable;
        pipeline_depth_stencil_state_create_info.depthWriteEnable = p_vulkan_graphics_pipeline_create_info->depth_write_enable;
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
    }
    VkPipelineColorBlendAttachmentState* p_pipeline_color_blend_attachment_states = TG_MEMORY_ALLOC(p_vulkan_graphics_pipeline_create_info->attachment_count * sizeof(*p_pipeline_color_blend_attachment_states));
    for (u32 i = 0; i < p_vulkan_graphics_pipeline_create_info->attachment_count; i++)
    {
        p_pipeline_color_blend_attachment_states[i].blendEnable = p_vulkan_graphics_pipeline_create_info->blend_enable;
        p_pipeline_color_blend_attachment_states[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        p_pipeline_color_blend_attachment_states[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        p_pipeline_color_blend_attachment_states[i].colorBlendOp = VK_BLEND_OP_ADD;
        p_pipeline_color_blend_attachment_states[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        p_pipeline_color_blend_attachment_states[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        p_pipeline_color_blend_attachment_states[i].alphaBlendOp = VK_BLEND_OP_ADD;
        p_pipeline_color_blend_attachment_states[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }
    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = { 0 };
    {
        pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        pipeline_color_blend_state_create_info.pNext = TG_NULL;
        pipeline_color_blend_state_create_info.flags = 0;
        pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
        pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
        pipeline_color_blend_state_create_info.attachmentCount = p_vulkan_graphics_pipeline_create_info->attachment_count;
        pipeline_color_blend_state_create_info.pAttachments = p_pipeline_color_blend_attachment_states;
        pipeline_color_blend_state_create_info.blendConstants[0] = 0.0f;
        pipeline_color_blend_state_create_info.blendConstants[1] = 0.0f;
        pipeline_color_blend_state_create_info.blendConstants[2] = 0.0f;
        pipeline_color_blend_state_create_info.blendConstants[3] = 0.0f;
    }
    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = { 0 };
    {
        graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphics_pipeline_create_info.pNext = TG_NULL;
        graphics_pipeline_create_info.flags = 0;
        graphics_pipeline_create_info.stageCount = 2;
        graphics_pipeline_create_info.pStages = p_pipeline_shader_stage_create_infos;
        graphics_pipeline_create_info.pVertexInputState = p_vulkan_graphics_pipeline_create_info->p_pipeline_vertex_input_state_create_info;
        graphics_pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
        graphics_pipeline_create_info.pTessellationState = TG_NULL;
        graphics_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
        graphics_pipeline_create_info.pRasterizationState = &pipeline_rasterization_state_create_info;
        graphics_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
        graphics_pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;
        graphics_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
        graphics_pipeline_create_info.pDynamicState = TG_NULL;
        graphics_pipeline_create_info.layout = p_vulkan_graphics_pipeline_create_info->pipeline_layout;
        graphics_pipeline_create_info.renderPass = p_vulkan_graphics_pipeline_create_info->render_pass;
        graphics_pipeline_create_info.subpass = 0;
        graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        graphics_pipeline_create_info.basePipelineIndex = -1;
    }
    VK_CALL(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, TG_NULL, &graphics_pipeline));
    TG_MEMORY_FREE(p_pipeline_color_blend_attachment_states);

    return graphics_pipeline;
}

void tg_vulkan_graphics_pipeline_destroy(VkPipeline graphics_pipeline)
{
    vkDestroyPipeline(device, graphics_pipeline, TG_NULL);
}



VkFilter tg_vulkan_image_convert_filter(tg_image_filter filter)
{
    switch (filter)
    {
    case TG_IMAGE_FILTER_LINEAR:  return VK_FILTER_LINEAR;
    case TG_IMAGE_FILTER_NEAREST: return VK_FILTER_NEAREST;
    }

    TG_ASSERT(TG_FALSE);
    return -1;
}

VkSamplerAddressMode tg_vulkan_image_convert_address_mode(tg_image_address_mode address_mode)
{
    switch (address_mode)
    {
    case TG_IMAGE_ADDRESS_MODE_CLAMP_TO_BORDER:      return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case TG_IMAGE_ADDRESS_MODE_CLAMP_TO_EDGE:        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case TG_IMAGE_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    case TG_IMAGE_ADDRESS_MODE_MIRRORED_REPEAT:      return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case TG_IMAGE_ADDRESS_MODE_REPEAT:               return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }

    TG_ASSERT(TG_FALSE);
    return -1;
}



VkPhysicalDeviceProperties tg_vulkan_physical_device_get_properties()
{
    VkPhysicalDeviceProperties physical_device_properties = { 0 };
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
    return physical_device_properties;
}


VkPipelineLayout tg_vulkan_pipeline_layout_create(u32 descriptor_set_layout_count, const VkDescriptorSetLayout* descriptor_set_layouts, u32 push_constant_range_count, const VkPushConstantRange* push_constant_ranges)
{
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = { 0 };
    {
        pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.pNext = TG_NULL;
        pipeline_layout_create_info.flags = 0;
        pipeline_layout_create_info.setLayoutCount = descriptor_set_layout_count;
        pipeline_layout_create_info.pSetLayouts = descriptor_set_layouts;
        pipeline_layout_create_info.pushConstantRangeCount = push_constant_range_count;
        pipeline_layout_create_info.pPushConstantRanges = push_constant_ranges;
    }
    VK_CALL(vkCreatePipelineLayout(device, &pipeline_layout_create_info, TG_NULL, &pipeline_layout));

    return pipeline_layout;
}

void tg_vulkan_pipeline_layout_destroy(VkPipelineLayout pipeline_layout)
{
    vkDestroyPipelineLayout(device, pipeline_layout, TG_NULL);
}



VkRenderPass tg_vulkan_render_pass_create(u32 attachment_count, const VkAttachmentDescription* p_attachments, u32 subpass_count, const VkSubpassDescription* p_subpasses, u32 dependency_count, const VkSubpassDependency* p_dependencies)
{
    VkRenderPass render_pass = VK_NULL_HANDLE;

    VkRenderPassCreateInfo render_pass_create_info = { 0 };
    {
        render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_create_info.pNext = TG_NULL;
        render_pass_create_info.flags = 0;
        render_pass_create_info.attachmentCount = attachment_count;
        render_pass_create_info.pAttachments = p_attachments;
        render_pass_create_info.subpassCount = subpass_count;
        render_pass_create_info.pSubpasses = p_subpasses;
        render_pass_create_info.dependencyCount = dependency_count;
        render_pass_create_info.pDependencies = p_dependencies;
    }
    VK_CALL(vkCreateRenderPass(device, &render_pass_create_info, TG_NULL, &render_pass));

    return render_pass;
}

void tg_vulkan_render_pass_destroy(VkRenderPass render_pass)
{
    vkDestroyRenderPass(device, render_pass, TG_NULL);
}



VkSampler tg_vulkan_sampler_create(u32 mip_levels, VkFilter min_filter, VkFilter mag_filter, VkSamplerAddressMode address_mode_u, VkSamplerAddressMode address_mode_v, VkSamplerAddressMode address_mode_w)
{
    VkSampler sampler = VK_NULL_HANDLE;

    VkSamplerCreateInfo sampler_create_info = { 0 };
    {
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
    }
    VK_CALL(vkCreateSampler(device, &sampler_create_info, TG_NULL, &sampler));

    return sampler;
}

void tg_vulkan_sampler_destroy(VkSampler sampler)
{
    vkDestroySampler(device, sampler, TG_NULL);
}

VkSemaphore tg_vulkan_semaphore_create()
{
    VkSemaphore semaphore = VK_NULL_HANDLE;

    VkSemaphoreCreateInfo semaphore_create_info = { 0 };
    {
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_create_info.pNext = TG_NULL;
        semaphore_create_info.flags = 0;
    }
    VK_CALL(vkCreateSemaphore(device, &semaphore_create_info, TG_NULL, &semaphore));

    return semaphore;
}

void tg_vulkan_semaphore_destroy(VkSemaphore semaphore)
{
    vkDestroySemaphore(device, semaphore, TG_NULL);
}



VkDescriptorType tg_vulkan_shader_input_element_type_convert(tg_shader_input_element_type type)
{
    switch (type)
    {
    case TG_SHADER_INPUT_ELEMENT_TYPE_COLOR_IMAGE:    return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case TG_SHADER_INPUT_ELEMENT_TYPE_COMPUTE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case TG_SHADER_INPUT_ELEMENT_TYPE_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }

    TG_ASSERT(TG_FALSE);
    return -1;
}



VkShaderModule tg_vulkan_shader_module_create(const char* p_filename)
{
    VkShaderModule shader_module = VK_NULL_HANDLE;

#ifdef TG_DEBUG
    char p_debug_buffer[256] = { 0 };
    tg_string_format(sizeof(p_debug_buffer), p_debug_buffer, "%s.spv", p_filename);
    if (!tg_file_io_exists(p_debug_buffer))
    {
        tg_string_format(sizeof(p_debug_buffer), p_debug_buffer, "C:/VulkanSDK/1.2.131.2/Bin/glslc.exe %s/%s -o %s/%s.spv", tg_application_get_asset_path(), p_filename, tg_application_get_asset_path(), p_filename);
        TG_ASSERT(system(p_debug_buffer) != -1);
    }
#endif

    char p_filename_buffer[256] = { 0 };
    tg_string_format(sizeof(p_filename_buffer), p_filename_buffer, "%s.spv", p_filename);

    u64 size = 0;
    char* content = TG_NULL;
    tg_file_io_read(p_filename_buffer, &size, &content);

    VkShaderModuleCreateInfo shader_module_create_info = { 0 };
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.pNext = TG_NULL;
    shader_module_create_info.flags = 0;
    shader_module_create_info.codeSize = size;
    shader_module_create_info.pCode = (const u32*)content;

    VK_CALL(vkCreateShaderModule(device, &shader_module_create_info, TG_NULL, &shader_module));
    tg_file_io_free(content);

    return shader_module;
}

void tg_vulkan_shader_module_destroy(VkShaderModule shader_module)
{
    vkDestroyShaderModule(device, shader_module, TG_NULL);
}



/*------------------------------------------------------------+
| Main utilities                                              |
+------------------------------------------------------------*/

VkCommandPool tg_vulkan_command_pool_create(VkCommandPoolCreateFlags command_pool_create_flags, u32 queue_family_index)
{
    VkCommandPool command_pool = VK_NULL_HANDLE;

    VkCommandPoolCreateInfo command_pool_create_info = { 0 };
    {
        command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_create_info.pNext = TG_NULL;
        command_pool_create_info.flags = command_pool_create_flags;
        command_pool_create_info.queueFamilyIndex = queue_family_index;
    }
    VK_CALL(vkCreateCommandPool(device, &command_pool_create_info, TG_NULL, &command_pool));

    return command_pool;
}

void tg_vulkan_command_pool_destroy(VkCommandPool command_pool)
{
    vkDestroyCommandPool(device, command_pool, TG_NULL);
}

b32 tg_vulkan_physical_device_check_extension_support(VkPhysicalDevice physical_device)
{
    u32 device_extension_property_count;
    VK_CALL(vkEnumerateDeviceExtensionProperties(physical_device, TG_NULL, &device_extension_property_count, TG_NULL));
    VkExtensionProperties* device_extension_properties = TG_MEMORY_ALLOC(device_extension_property_count * sizeof(*device_extension_properties));
    VK_CALL(vkEnumerateDeviceExtensionProperties(physical_device, TG_NULL, &device_extension_property_count, device_extension_properties));

    b32 supports_extensions = TG_TRUE;
    for (u32 i = 0; i < tg_VULKAN_DEVICE_EXTENSION_COUNT; i++)
    {
        b32 supports_extension = TG_FALSE;
        for (u32 j = 0; j < device_extension_property_count; j++)
        {
            if (strcmp(tg_VULKAN_DEVICE_EXTENSION_NAMES[i], device_extension_properties[j].extensionName) == 0)
            {
                supports_extension = TG_TRUE;
                break;
            }
        }
        supports_extensions &= supports_extension;
    }

    TG_MEMORY_FREE(device_extension_properties);
    return supports_extensions;
}

b32 tg_vulkan_physical_device_find_queue_families(VkPhysicalDevice physical_device, tg_vulkan_queue* p_graphics_queue, tg_vulkan_queue* p_present_queue, tg_vulkan_queue* p_compute_queue)
{
    u32 queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, TG_NULL);
    TG_ASSERT(queue_family_count);
    VkQueueFamilyProperties* queue_family_properties = TG_MEMORY_ALLOC(queue_family_count * sizeof(*queue_family_properties));
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties);

    b32 supports_graphics_family = TG_FALSE;
    VkBool32 supports_present_family = VK_FALSE;
    b32 supports_compute_family = TG_FALSE;
    for (u32 i = 0; i < queue_family_count; i++)
    {
        if (!supports_graphics_family)
        {
            if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                p_graphics_queue->index = i;
                supports_graphics_family = TG_TRUE;
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
        if (!supports_compute_family)
        {
            if (queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                p_compute_queue->index = i;
                supports_compute_family = TG_TRUE;
            }
        }
        if (supports_graphics_family && supports_present_family && supports_compute_family)
        {
            break;
        }
    }

    const b32 complete = supports_graphics_family && supports_present_family && supports_compute_family;
    TG_MEMORY_FREE(queue_family_properties);
    return complete;
}

VkSampleCountFlagBits tg_vulkan_physical_device_find_max_sample_count(VkPhysicalDevice physical_device)
{
    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

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

b32 tg_vulkan_physical_device_is_suitable(VkPhysicalDevice physical_device)
{
    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

    VkPhysicalDeviceFeatures physical_device_features;
    vkGetPhysicalDeviceFeatures(physical_device, &physical_device_features);

    const b32 is_discrete_gpu = physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    const b32 supports_geometry_shader = physical_device_features.geometryShader;
    const b32 supports_sampler_anisotropy = physical_device_features.samplerAnisotropy;

    const b32 qfi_complete = tg_vulkan_physical_device_find_queue_families(physical_device, &graphics_queue, &present_queue, &compute_queue);
    const b32 supports_extensions = tg_vulkan_physical_device_check_extension_support(physical_device);

    u32 physical_device_surface_format_count;
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface.surface, &physical_device_surface_format_count, TG_NULL));
    u32 physical_device_present_mode_count;
    VK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface.surface, &physical_device_present_mode_count, TG_NULL));

    const b32 is_suitable = is_discrete_gpu && supports_geometry_shader && supports_sampler_anisotropy && qfi_complete && supports_extensions && physical_device_surface_format_count && physical_device_present_mode_count;
    return is_suitable;
}





VkInstance tg_vulkan_instance_create()
{
    VkInstance instance = VK_NULL_HANDLE;

#ifdef TG_DEBUG
    u32 layer_property_count;
    vkEnumerateInstanceLayerProperties(&layer_property_count, TG_NULL);
    VkLayerProperties* layer_properties = TG_MEMORY_ALLOC(layer_property_count * sizeof(*layer_properties));
    vkEnumerateInstanceLayerProperties(&layer_property_count, layer_properties);

    for (u32 i = 0; i < tg_VULKAN_VALIDATION_LAYER_COUNT; i++)
    {
        b32 layer_found = TG_FALSE;
        for (u32 j = 0; j < layer_property_count; j++)
        {
            if (strcmp(tg_VULKAN_VALIDATION_LAYER_NAMES[i], layer_properties[j].layerName) == 0)
            {
                layer_found = TG_TRUE;
                break;
            }
        }
        TG_ASSERT(layer_found);
    }
    TG_MEMORY_FREE(layer_properties);
#endif

    VkApplicationInfo application_info = { 0 };
    {
        application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        application_info.pNext = TG_NULL;
        application_info.pApplicationName = TG_NULL;
        application_info.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
        application_info.pEngineName = TG_NULL;
        application_info.engineVersion = VK_MAKE_VERSION(0, 0, 0);
        application_info.apiVersion = VK_API_VERSION_1_2;
    }
    VkInstanceCreateInfo instance_create_info = { 0 };
    {
        instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_create_info.pNext = TG_NULL;
        instance_create_info.flags = 0;
        instance_create_info.pApplicationInfo = &application_info;
        instance_create_info.enabledLayerCount = tg_VULKAN_VALIDATION_LAYER_COUNT;
        instance_create_info.ppEnabledLayerNames = tg_VULKAN_VALIDATION_LAYER_NAMES;
        instance_create_info.enabledExtensionCount = tg_VULKAN_INSTANCE_EXTENSION_COUNT;
        instance_create_info.ppEnabledExtensionNames = tg_VULKAN_INSTANCE_EXTENSION_NAMES;
    }
    VK_CALL(vkCreateInstance(&instance_create_info, TG_NULL, &instance));

    return instance;
}

#ifdef TG_DEBUG
VkDebugUtilsMessengerEXT tg_vulkan_debug_utils_manager_create()
{
    VkDebugUtilsMessengerEXT debug_utils_messenger = VK_NULL_HANDLE;

    VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info = { 0 };
    {
        debug_utils_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_utils_messenger_create_info.pNext = TG_NULL;
        debug_utils_messenger_create_info.flags = 0;
        debug_utils_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_utils_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_utils_messenger_create_info.pfnUserCallback = tgvk_debug_callback;
        debug_utils_messenger_create_info.pUserData = TG_NULL;
    }
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    TG_ASSERT(vkCreateDebugUtilsMessengerEXT);
    VK_CALL(vkCreateDebugUtilsMessengerEXT(instance, &debug_utils_messenger_create_info, TG_NULL, &debug_utils_messenger));

    return debug_utils_messenger;
}
#endif

#ifdef TG_WIN32
tg_vulkan_surface tg_vulkan_surface_create()
{
    tg_vulkan_surface vulkan_surface = { 0 };

    VkWin32SurfaceCreateInfoKHR win32_surface_create_info = { 0 };
    {
        win32_surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        win32_surface_create_info.pNext = TG_NULL;
        win32_surface_create_info.flags = 0;
        win32_surface_create_info.hinstance = GetModuleHandle(TG_NULL);
        win32_surface_create_info.hwnd = tg_platform_get_window_handle();
    }
    VK_CALL(vkCreateWin32SurfaceKHR(instance, &win32_surface_create_info, TG_NULL, &vulkan_surface.surface));

    return vulkan_surface;
}
#endif

VkPhysicalDevice tg_vulkan_physical_device_create()
{
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;

    u32 physical_device_count;
    VK_CALL(vkEnumeratePhysicalDevices(instance, &physical_device_count, TG_NULL));
    TG_ASSERT(physical_device_count);
    VkPhysicalDevice* physical_devices = TG_MEMORY_ALLOC(physical_device_count * sizeof(*physical_devices));
    VK_CALL(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices));

    for (u32 i = 0; i < physical_device_count; i++)
    {
        const b32 is_suitable = tg_vulkan_physical_device_is_suitable(physical_devices[i]);
        if (is_suitable)
        {
            physical_device = physical_devices[i];
            break;
        }
    }

    TG_MEMORY_FREE(physical_devices);
    TG_ASSERT(physical_device != VK_NULL_HANDLE);

    return physical_device;
}

VkDevice tg_vulkan_device_create()
{
    VkDevice device = VK_NULL_HANDLE;

    const f32 queue_priority = 1.0f;

    VkDeviceQueueCreateInfo device_queue_create_infos[3] = { 0 };
    {
        device_queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        device_queue_create_infos[0].pNext = TG_NULL;
        device_queue_create_infos[0].flags = 0;
        device_queue_create_infos[0].queueFamilyIndex = graphics_queue.index;
        device_queue_create_infos[0].queueCount = 1;
        device_queue_create_infos[0].pQueuePriorities = &queue_priority;

        device_queue_create_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        device_queue_create_infos[1].pNext = TG_NULL;
        device_queue_create_infos[1].flags = 0;
        device_queue_create_infos[1].queueFamilyIndex = present_queue.index;
        device_queue_create_infos[1].queueCount = 1;
        device_queue_create_infos[1].pQueuePriorities = &queue_priority;

        device_queue_create_infos[2].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        device_queue_create_infos[2].pNext = TG_NULL;
        device_queue_create_infos[2].flags = 0;
        device_queue_create_infos[2].queueFamilyIndex = compute_queue.index;
        device_queue_create_infos[2].queueCount = 1;
        device_queue_create_infos[2].pQueuePriorities = &queue_priority;
    }

    VkPhysicalDeviceFeatures physical_device_features = { 0 };
    {
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
    }
    VkDeviceCreateInfo device_create_info = { 0 };
    {
        device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.pNext = TG_NULL;
        device_create_info.flags = 0;
        device_create_info.queueCreateInfoCount = 2;
        device_create_info.pQueueCreateInfos = device_queue_create_infos;
        device_create_info.enabledLayerCount = tg_VULKAN_VALIDATION_LAYER_COUNT;
        device_create_info.ppEnabledLayerNames = tg_VULKAN_VALIDATION_LAYER_NAMES;
        device_create_info.enabledExtensionCount = tg_VULKAN_DEVICE_EXTENSION_COUNT;
        device_create_info.ppEnabledExtensionNames = tg_VULKAN_DEVICE_EXTENSION_NAMES;
        device_create_info.pEnabledFeatures = &physical_device_features;
    }
    VK_CALL(vkCreateDevice(physical_device, &device_create_info, TG_NULL, &device));

    return device;
}

void tg_vulkan_queues_create(tg_vulkan_queue* p_graphics_queue, tg_vulkan_queue* p_present_queue, tg_vulkan_queue* p_compute_queue)
{
    TG_ASSERT(p_graphics_queue && p_present_queue);

    vkGetDeviceQueue(device, p_graphics_queue->index, 0, &p_graphics_queue->queue);
    vkGetDeviceQueue(device, p_present_queue->index, 0, &p_present_queue->queue);
    vkGetDeviceQueue(device, p_compute_queue->index, 0, &p_compute_queue->queue);
}

void tg_vulkan_swapchain_create()
{
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VK_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface.surface, &surface_capabilities));

    u32 surface_format_count;
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface.surface, &surface_format_count, TG_NULL));
    VkSurfaceFormatKHR* surface_formats = TG_MEMORY_ALLOC(surface_format_count * sizeof(*surface_formats));
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface.surface, &surface_format_count, surface_formats));
    surface.format = surface_formats[0];
    for (u32 i = 0; i < surface_format_count; i++)
    {
        if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            surface.format = surface_formats[i];
            break;
        }
    }
    TG_MEMORY_FREE(surface_formats);

    VkPresentModeKHR p_present_modes[9] = { 0 };
    u32 present_mode_count = 0;
    VK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface.surface, &present_mode_count, TG_NULL));
    VK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface.surface, &present_mode_count, p_present_modes));

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

    TG_ASSERT(tgm_u32_clamp(TG_VULKAN_SURFACE_IMAGE_COUNT, surface_capabilities.minImageCount, surface_capabilities.maxImageCount) == TG_VULKAN_SURFACE_IMAGE_COUNT);

    VkSwapchainCreateInfoKHR swapchain_create_info = { 0 };
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = TG_NULL;
    swapchain_create_info.flags = 0;
    swapchain_create_info.surface = surface.surface;
    swapchain_create_info.minImageCount = TG_VULKAN_SURFACE_IMAGE_COUNT;
    swapchain_create_info.imageFormat = surface.format.format;
    swapchain_create_info.imageColorSpace = surface.format.colorSpace;
    swapchain_create_info.imageExtent = swapchain_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const u32 p_queue_family_indices[] = { graphics_queue.index, present_queue.index };
    if (graphics_queue.index != present_queue.index)
    {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = p_queue_family_indices;
    }
    else
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

    VK_CALL(vkCreateSwapchainKHR(device, &swapchain_create_info, TG_NULL, &swapchain));
    
    u32 surface_image_count = TG_VULKAN_SURFACE_IMAGE_COUNT;
#ifdef TG_DEBUG
    VK_CALL(vkGetSwapchainImagesKHR(device, swapchain, &surface_image_count, TG_NULL));
    TG_ASSERT(surface_image_count == TG_VULKAN_SURFACE_IMAGE_COUNT);
#endif
    VK_CALL(vkGetSwapchainImagesKHR(device, swapchain, &surface_image_count, swapchain_images));

    for (u32 i = 0; i < TG_VULKAN_SURFACE_IMAGE_COUNT; i++)
    {
        VkImageViewCreateInfo image_view_create_info = { 0 };
        {
            image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            image_view_create_info.pNext = TG_NULL;
            image_view_create_info.flags = 0;
            image_view_create_info.image = swapchain_images[i];
            image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
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
        }
        VK_CALL(vkCreateImageView(device, &image_view_create_info, TG_NULL, &swapchain_image_views[i]));
    }
}



/*------------------------------------------------------------+
| Initialization, shutdown and other main functionality       |
+------------------------------------------------------------*/

void tg_init()
{
#ifdef TG_DEBUG
    // TODO: relative asset path?
    char* p_system_buffer = "del \"assets\\shaders\\*.spv\"";
    TG_ASSERT(system(p_system_buffer) != -1);
#endif

    instance = tg_vulkan_instance_create();
#ifdef TG_DEBUG
    debug_utils_messenger = tg_vulkan_debug_utils_manager_create();
#endif
    surface = tg_vulkan_surface_create();
    physical_device = tg_vulkan_physical_device_create();
    device = tg_vulkan_device_create();
    tg_vulkan_queues_create(&graphics_queue, &present_queue, &compute_queue);
    graphics_command_pool = tg_vulkan_command_pool_create(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, graphics_queue.index);
    compute_command_pool = tg_vulkan_command_pool_create(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, compute_queue.index);
    tg_vulkan_swapchain_create();
}

void tg_shutdown()
{
    vkDeviceWaitIdle(device);

    for (u32 i = 0; i < TG_VULKAN_SURFACE_IMAGE_COUNT; i++)
    {
        vkDestroyImageView(device, swapchain_image_views[i], TG_NULL);
    }
    vkDestroySwapchainKHR(device, swapchain, TG_NULL);
    vkDestroyCommandPool(device, graphics_command_pool, TG_NULL);
    vkDestroyDevice(device, TG_NULL);
#ifdef TG_DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    TG_ASSERT(vkDestroyDebugUtilsMessengerEXT);
    vkDestroyDebugUtilsMessengerEXT(instance, debug_utils_messenger, TG_NULL);
#endif
    vkDestroySurfaceKHR(instance, surface.surface, TG_NULL);
    vkDestroyInstance(instance, TG_NULL);
}

void tg_on_window_resize(u32 width, u32 height)
{
    vkDeviceWaitIdle(device);
    
    for (u32 i = 0; i < TG_VULKAN_SURFACE_IMAGE_COUNT; i++)
    {
        vkDestroyImageView(device, swapchain_image_views[i], TG_NULL);
    }
    vkDestroySwapchainKHR(device, swapchain, TG_NULL);
    tg_vulkan_swapchain_create();
}

#endif
