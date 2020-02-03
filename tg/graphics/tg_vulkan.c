#include "tg_vulkan.h"

#include "tg/tg_common.h"
#include "tg/platform/tg_platform.h"
#include "tg/util/tg_file_io.h"
#include "tg/math/tg_math_functional.h"
#include <stdbool.h>
#include <stdlib.h>

#ifdef TG_WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#endif

#ifdef TG_WIN32
#define INSTANCE_EXTENSION_COUNT 3
#define INSTANCE_EXTENSION_NAMES (char*[]){ VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME }
#else
#define INSTANCE_EXTENSION_COUNT 0
#define INSTANCE_EXTENSION_NAMES NULL
#endif

#define DEVICE_EXTENSION_COUNT 1
#define DEVICE_EXTENSION_NAMES (char*[]){ VK_KHR_SWAPCHAIN_EXTENSION_NAME }

#ifdef TG_DEBUG
#define VALIDATION_LAYER_COUNT 1
#define VALIDATION_LAYER_NAMES (char*[]){ "VK_LAYER_LUNARG_standard_validation" }
#else
#define VALIDATION_LAYER_COUNT 0
#define VALIDATION_LAYER_NAMES NULL
#endif

typedef struct
{
    uint32 graphics_family;
    uint32 present_family;
} tg_vulkan_queue_family_indices;

typedef struct
{
    uint32 swapchain_image_count;
    VkImage* swapchain_images;
} tg_vulkan_swapchain_images;

typedef struct
{
    uint32 swapchain_image_view_count;
    VkImageView* swapchain_image_views;
} tg_vulkan_swapchain_image_views;

VkInstance instance = VK_NULL_HANDLE;

#ifdef TG_DEBUG
VkDebugUtilsMessengerEXT debug_utils_messenger;
#endif

VkSurfaceKHR surface = VK_NULL_HANDLE;

VkPhysicalDevice physical_device = VK_NULL_HANDLE;
tg_vulkan_queue_family_indices queue_family_indices = { 0 };

VkDevice device = VK_NULL_HANDLE;
VkQueue graphics_queue = VK_NULL_HANDLE;
VkQueue present_queue = VK_NULL_HANDLE;

VkSwapchainKHR swapchain = VK_NULL_HANDLE;
tg_vulkan_swapchain_images swapchain_images = { 0 };
VkFormat swapchain_image_format = VK_FORMAT_UNDEFINED;
VkExtent2D swapchain_extent = { 0 };

tg_vulkan_swapchain_image_views swapchain_image_views = { 0 };

#ifdef TG_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    if (message_severity >= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
    {
        TG_PRINT(callback_data->pMessage + '\n');
        return VK_TRUE;
    }
    return VK_FALSE;
}
#endif

void tg_vulkan_init_instance()
{
    VkApplicationInfo application_info = { 0 };
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName = "TG";
    application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info.pEngineName = "TG";
    application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instance_create_info = { 0 };
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &application_info;
    instance_create_info.enabledLayerCount = VALIDATION_LAYER_COUNT;
    instance_create_info.ppEnabledLayerNames = VALIDATION_LAYER_NAMES;
    instance_create_info.enabledExtensionCount = INSTANCE_EXTENSION_COUNT;
    instance_create_info.ppEnabledExtensionNames = INSTANCE_EXTENSION_NAMES;

    const VkResult create_instance_result = vkCreateInstance(&instance_create_info, NULL, &instance);
    ASSERT(create_instance_result == VK_SUCCESS);
}

#ifdef TG_DEBUG
void tg_vulkan_init_debug_utils_manager()
{
    VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info = { 0 };
    debug_utils_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_utils_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_utils_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_utils_messenger_create_info.pfnUserCallback = debug_callback;
    debug_utils_messenger_create_info.pUserData = NULL;

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    ASSERT(vkCreateDebugUtilsMessengerEXT);
    const VkResult create_debug_utils_messenger_result = vkCreateDebugUtilsMessengerEXT(instance, &debug_utils_messenger_create_info, NULL, &debug_utils_messenger);
    ASSERT(create_debug_utils_messenger_result == VK_SUCCESS);
}
#endif

#ifdef TG_WIN32
void tg_vulkan_init_surface()
{
    VkWin32SurfaceCreateInfoKHR win32_surface_create_info = { 0 };
    win32_surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    win32_surface_create_info.hinstance = GetModuleHandle(NULL);
    win32_surface_create_info.hwnd = tg_platform_get_window_handle();

    const VkResult create_win32_surface_result = vkCreateWin32SurfaceKHR(instance, &win32_surface_create_info, NULL, &surface);
    ASSERT(create_win32_surface_result == VK_SUCCESS);
}
#endif

void tg_vulkan_init_physical_device_find_queue_family_indices(VkPhysicalDevice pd, tg_vulkan_queue_family_indices* qfi, bool* complete)
{
    uint32 queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &queue_family_count, NULL);
    ASSERT(queue_family_count);
    VkQueueFamilyProperties* queue_family_properties = malloc(queue_family_count * sizeof(*queue_family_properties));
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &queue_family_count, queue_family_properties);

    bool supports_graphics_family = false;
    VkBool32 supports_present_family = 0;
    for (uint32 i = 0; i < queue_family_count; i++)
    {
        if (!supports_graphics_family)
        {
            if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                qfi->graphics_family = i;
                supports_graphics_family = true;
            }
        }
        if (!supports_present_family)
        {
            vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, surface, &supports_present_family);
            if (supports_present_family)
            {
                qfi->present_family = i;
            }
        }
        if (supports_graphics_family && supports_present_family)
        {
            break;
        }
    }
    *complete = supports_graphics_family && supports_present_family;
    free(queue_family_properties);
}

bool tg_vulkan_init_physical_device_supports_extensions(VkPhysicalDevice pd)
{
    uint32 device_extension_property_count;
    vkEnumerateDeviceExtensionProperties(pd, NULL, &device_extension_property_count, NULL);
    VkExtensionProperties* device_extension_properties = malloc(device_extension_property_count * sizeof(*device_extension_properties));
    vkEnumerateDeviceExtensionProperties(pd, NULL, &device_extension_property_count, device_extension_properties);

    bool supports_extensions = true;
    for (uint32 i = 0; i < DEVICE_EXTENSION_COUNT; i++)
    {
        bool supports_extension = false;
        for (uint32 j = 0; j < device_extension_property_count; j++)
        {
            if (strcmp(DEVICE_EXTENSION_NAMES[i], device_extension_properties[j].extensionName) == 0)
            {
                supports_extension = true;
                break;
            }
        }
        supports_extensions &= supports_extension;
    }
    free(device_extension_properties);
    return supports_extensions;
}

void tg_vulkan_init_physical_device()
{
    uint32 physical_device_count;
    vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL);
    ASSERT(physical_device_count);
    VkPhysicalDevice* physical_devices = malloc(physical_device_count * sizeof(*physical_devices));
    ASSERT(physical_devices);
    vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices);

    for (uint32 i = 0; i < physical_device_count; i++)
    {
        VkPhysicalDeviceProperties physical_device_properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &physical_device_properties);

        VkPhysicalDeviceFeatures physical_device_features;
        vkGetPhysicalDeviceFeatures(physical_devices[i], &physical_device_features);

        const bool is_discrete_gpu = physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        const bool supports_geometry_shader = physical_device_features.geometryShader;

        tg_vulkan_queue_family_indices qfi = { 0 };
        bool qfi_complete;
        tg_vulkan_init_physical_device_find_queue_family_indices(physical_devices[i], &qfi, &qfi_complete);

        const bool supports_extensions = tg_vulkan_init_physical_device_supports_extensions(physical_devices[i]);

        uint32 physical_device_surface_format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_devices[i], surface, &physical_device_surface_format_count, NULL);
        uint32 physical_device_present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_devices[i], surface, &physical_device_present_mode_count, NULL);

        if (is_discrete_gpu && supports_geometry_shader && qfi_complete && supports_extensions && physical_device_surface_format_count && physical_device_present_mode_count)
        {
            physical_device = physical_devices[i];
            queue_family_indices = qfi;
            break;
        }
    }
    ASSERT(physical_device != VK_NULL_HANDLE);
    free(physical_devices);
}

void tg_vulkan_init_device()
{
    VkDeviceQueueCreateInfo device_queue_create_info = { 0 };
    const float queue_priority = 1.0f;
    device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_queue_create_info.queueFamilyIndex = queue_family_indices.graphics_family;
    device_queue_create_info.queueCount = 1;
    device_queue_create_info.pQueuePriorities = &queue_priority;

    VkPhysicalDeviceFeatures physical_device_features = { 0 };

    uint32 queue_family_count = sizeof(tg_vulkan_queue_family_indices) / sizeof(uint32);
    VkDeviceQueueCreateInfo* device_queue_create_infos = malloc(queue_family_count * sizeof(*device_queue_create_infos));
    ASSERT(device_queue_create_infos);
    for (uint32 i = 0; i < queue_family_count; i++)
    {
        VkDeviceQueueCreateInfo device_queue_create_info = { 0 };
        device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        device_queue_create_info.queueFamilyIndex = *((uint32*)(&queue_family_indices) + i);
        device_queue_create_info.queueCount = 1;
        device_queue_create_info.pQueuePriorities = &queue_priority;
        device_queue_create_infos[i] = device_queue_create_info;
    }

    VkDeviceCreateInfo device_create_info = { 0 };
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = queue_family_count;
    device_create_info.pQueueCreateInfos = device_queue_create_infos;
    device_create_info.enabledLayerCount = VALIDATION_LAYER_COUNT;
    device_create_info.ppEnabledLayerNames = VALIDATION_LAYER_NAMES;
    device_create_info.enabledExtensionCount = DEVICE_EXTENSION_COUNT;
    device_create_info.ppEnabledExtensionNames = DEVICE_EXTENSION_NAMES;
    device_create_info.pEnabledFeatures = &physical_device_features;

    const VkResult create_device_result = vkCreateDevice(physical_device, &device_create_info, NULL, &device);
    ASSERT(create_device_result == VK_SUCCESS);

    free(device_queue_create_infos);
}

void tg_vulkan_init_swap_chain()
{
    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);

    uint32 surface_format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, NULL);
    ASSERT(surface_format_count);
    VkSurfaceFormatKHR* surface_formats = malloc(surface_format_count * sizeof(*surface_formats));
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats);
    VkSurfaceFormatKHR surface_format = surface_formats[0];
    for (uint32 i = 0; i < surface_format_count; i++)
    {
        if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            surface_format = surface_formats[i];
            break;
        }
    }
    free(surface_formats);

    uint32 present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, NULL);
    ASSERT(present_mode_count);
    VkPresentModeKHR* present_modes = malloc(present_mode_count * sizeof(*present_modes));
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes);
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32 i = 0; i < present_mode_count; i++)
    {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            present_mode = present_modes[i];
            break;
        }
    }
    free(present_modes);

    if (surface_capabilities.currentExtent.width != UINT32_MAX)
    {
        swapchain_extent = surface_capabilities.currentExtent;
    }
    else
    {
        uint32 width = 0;
        uint32 height = 0;
        tg_platform_get_window_size(&width, &height);
        ASSERT(width && height);
        swapchain_extent.width = tg_uint32_clamp(width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
        swapchain_extent.height = tg_uint32_clamp(height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
    }

    uint32 image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount)
    {
        image_count = surface_capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_create_info = { 0 };
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = surface;
    swapchain_create_info.minImageCount = image_count;
    swapchain_create_info.imageFormat = surface_format.format;
    swapchain_create_info.imageColorSpace = surface_format.colorSpace;
    swapchain_create_info.imageExtent = swapchain_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (queue_family_indices.graphics_family != queue_family_indices.present_family)
    {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = (uint32*)&queue_family_indices;
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

    const VkResult create_swapchain_result = vkCreateSwapchainKHR(device, &swapchain_create_info, NULL, &swapchain);
    ASSERT(create_swapchain_result == VK_SUCCESS);
    
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_images.swapchain_image_count, NULL);
    swapchain_images.swapchain_images = malloc(swapchain_images.swapchain_image_count * sizeof(*swapchain_images.swapchain_images));
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_images.swapchain_image_count, swapchain_images.swapchain_images);

    swapchain_image_format = surface_format.format;
}

void tg_vulkan_init_image_views()
{
    swapchain_image_views.swapchain_image_view_count = swapchain_images.swapchain_image_count;
    swapchain_image_views.swapchain_image_views = malloc(swapchain_image_views.swapchain_image_view_count * sizeof(*swapchain_image_views.swapchain_image_views));
    for (uint32 i = 0; i < swapchain_image_views.swapchain_image_view_count; i++)
    {
        VkComponentMapping component_mapping = { 0 };
        component_mapping.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        component_mapping.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        component_mapping.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        component_mapping.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        VkImageSubresourceRange image_subresource_range = { 0 };
        image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_subresource_range.baseMipLevel = 0;
        image_subresource_range.levelCount = 1;
        image_subresource_range.baseArrayLayer = 0;
        image_subresource_range.layerCount = 1;

        VkImageViewCreateInfo image_view_create_info = { 0 };
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.image = swapchain_images.swapchain_images[i];
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format = swapchain_image_format;
        image_view_create_info.components = component_mapping;
        image_view_create_info.subresourceRange = image_subresource_range;

        const VkResult create_image_view_result = vkCreateImageView(device, &image_view_create_info, NULL, &swapchain_image_views.swapchain_image_views[i]);
        ASSERT(create_image_view_result == VK_SUCCESS);
    }
}

VkShaderModule tg_vulkan_init_graphics_pipeline_create_shader_module(uint64 size, const char* shader)
{
    VkShaderModuleCreateInfo shader_module_create_info = { 0 };
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.codeSize = size / sizeof(char);
    shader_module_create_info.pCode = (const uint32*)shader;

    VkShaderModule shader_module;
    const VkResult create_shader_module_result = vkCreateShaderModule(device, &shader_module_create_info, NULL, &shader_module);
    ASSERT(create_shader_module_result == VK_SUCCESS);
    return shader_module;
}

void tg_vulkan_init_graphics_pipeline()
{
    // TODO: comments for compilation: https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules
    uint64 vert_size;
    char* vert;
    uint64 frag_size;
    char* frag;
    tg_file_io_read("graphics/shaders/vert.spv", &vert_size, &vert);
    tg_file_io_read("graphics/shaders/frag.spv", &frag_size, &frag);

    VkShaderModule frag_shader_module = tg_vulkan_init_graphics_pipeline_create_shader_module(frag_size, frag);
    VkShaderModule vert_shader_module = tg_vulkan_init_graphics_pipeline_create_shader_module(vert_size, vert);

    VkPipelineShaderStageCreateInfo pipeline_shader_state_create_info_vert = { 0 };
    pipeline_shader_state_create_info_vert.stage = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_shader_state_create_info_vert.stage = VK_SHADER_STAGE_VERTEX_BIT;
    pipeline_shader_state_create_info_vert.module = vert_shader_module;
    pipeline_shader_state_create_info_vert.pName = "main";

    VkPipelineShaderStageCreateInfo pipeline_shader_state_create_info_frag = { 0 };
    pipeline_shader_state_create_info_frag.stage = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_shader_state_create_info_frag.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipeline_shader_state_create_info_frag.module = frag_shader_module;
    pipeline_shader_state_create_info_frag.pName = "main";

    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_infos[] = {
        pipeline_shader_state_create_info_vert,
        pipeline_shader_state_create_info_frag
    };

    // TODO: continue: https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions

    tg_file_io_free(frag);
    tg_file_io_free(vert);

    vkDestroyShaderModule(device, frag_shader_module, NULL);
    vkDestroyShaderModule(device, vert_shader_module, NULL);
}

void tg_vulkan_init()
{
#ifdef TG_DEBUG
    uint32 layer_property_count;
    vkEnumerateInstanceLayerProperties(&layer_property_count, NULL);
    VkLayerProperties* layer_properties = malloc(layer_property_count * sizeof(*layer_properties));
    ASSERT(layer_properties != NULL);
    vkEnumerateInstanceLayerProperties(&layer_property_count, layer_properties);

    for (uint32 i = 0; i < VALIDATION_LAYER_COUNT; i++)
    {
        bool layer_found = false;
        for (uint32 j = 0; j < layer_property_count; j++)
        {
            if (strcmp(VALIDATION_LAYER_NAMES[i], layer_properties[j].layerName) == 0)
            {
                layer_found = true;
                break;
            }
        }
        ASSERT(layer_found);
    }
    free(layer_properties);
#endif

    tg_vulkan_init_instance();
#ifdef TG_DEBUG
    tg_vulkan_init_debug_utils_manager();
#endif
    tg_vulkan_init_surface();
    tg_vulkan_init_physical_device();
    tg_vulkan_init_device();

    vkGetDeviceQueue(device, queue_family_indices.graphics_family, 0, &graphics_queue);
    vkGetDeviceQueue(device, queue_family_indices.present_family, 0, &present_queue);

    tg_vulkan_init_swap_chain();
    tg_vulkan_init_image_views();
    tg_vulkan_init_graphics_pipeline();
}

void tg_vulkan_shutdown()
{
    for (uint32 i = 0; i < swapchain_image_views.swapchain_image_view_count; i++)
    {
        vkDestroyImageView(device, swapchain_image_views.swapchain_image_views[i], NULL);
    }
    free(swapchain_image_views.swapchain_image_views);
    free(swapchain_images.swapchain_images);
    vkDestroySwapchainKHR(device, swapchain, NULL);
    vkDestroyDevice(device, NULL);
#ifdef TG_DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    ASSERT(vkDestroyDebugUtilsMessengerEXT);
    vkDestroyDebugUtilsMessengerEXT(instance, debug_utils_messenger, NULL);
#endif
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
}
