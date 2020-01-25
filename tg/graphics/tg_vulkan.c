#include "tg_vulkan.h"

#include "tg/tg_common.h"
#include "tg/platform/tg_platform.h"
#include <stdbool.h>
#include <stdlib.h>

#ifdef TG_WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#endif

#ifdef TG_WIN32
#define ENABLED_EXTENSION_COUNT 3
#define ENABLED_EXTENSION_NAMES (char*[]){ VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME }
#endif

#ifdef TG_DEBUG
#define VALIDATION_LAYER_COUNT 1
#define VALIDATION_LAYER_NAMES (char*[]){ "VK_LAYER_LUNARG_standard_validation" }
#else
#define VALIDATION_LAYER_COUNT 0
#define VALIDATION_LAYER_NAMES NULL
#endif

VkInstance instance = VK_NULL_HANDLE;
VkPhysicalDevice physical_device = VK_NULL_HANDLE;
VkDevice device = VK_NULL_HANDLE;
VkQueue queue = VK_NULL_HANDLE;

#ifdef TG_DEBUG
VkDebugUtilsMessengerEXT debug_utils_messenger;
#endif

typedef struct
{
    uint32 graphics_family;
} tg_vulkan_queue_family_indices;

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    if (message_severity >= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
    {
        tg_platform_print(callback_data->pMessage + '\n');
        return VK_TRUE;
    }
    return VK_FALSE;
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

    // instance
    VkApplicationInfo application_info  = { 0 };
    application_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName   = "TG";
    application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info.pEngineName        = "TG";
    application_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    application_info.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instance_create_info    = { 0 };
    instance_create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo        = &application_info;
    instance_create_info.enabledLayerCount       = VALIDATION_LAYER_COUNT;
    instance_create_info.ppEnabledLayerNames     = VALIDATION_LAYER_NAMES;
    instance_create_info.enabledExtensionCount   = ENABLED_EXTENSION_COUNT;
    instance_create_info.ppEnabledExtensionNames = ENABLED_EXTENSION_NAMES;

    VkResult create_instance_result = vkCreateInstance(&instance_create_info, NULL, &instance);
    ASSERT(create_instance_result == VK_SUCCESS);

    // debug utils messenger
#ifdef TG_DEBUG
    VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info = { 0 };
    debug_utils_messenger_create_info.sType                              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_utils_messenger_create_info.messageSeverity                    = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_utils_messenger_create_info.messageType                        = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_utils_messenger_create_info.pfnUserCallback                    = debug_callback;
    debug_utils_messenger_create_info.pUserData                          = NULL;

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    ASSERT(vkCreateDebugUtilsMessengerEXT);
    VkResult create_debug_utils_messenger_result = vkCreateDebugUtilsMessengerEXT(instance, &debug_utils_messenger_create_info, NULL, &debug_utils_messenger);
    ASSERT(create_debug_utils_messenger_result == VK_SUCCESS);
#endif

    // physical device
    uint32 physical_device_count;
    vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL);
    ASSERT(physical_device_count);
    VkPhysicalDevice* physical_devices = malloc(physical_device_count * sizeof(*physical_devices));
    ASSERT(physical_devices);
    vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices);

    tg_vulkan_queue_family_indices queue_family_indices = { 0 };
    for (uint32 i = 0; i < physical_device_count; i++)
    {
        VkPhysicalDeviceProperties physical_device_properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &physical_device_properties);

        VkPhysicalDeviceFeatures physical_device_features;
        vkGetPhysicalDeviceFeatures(physical_devices[i], &physical_device_features);

        const bool is_discrete_gpu          = physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        const bool supports_geometry_shader = physical_device_features.geometryShader;
        bool supports_graphics_family       = false;

        uint32 queue_family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, NULL);
        VkQueueFamilyProperties* queue_family_properties = malloc(queue_family_count * sizeof(*queue_family_properties));
        ASSERT(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, NULL);

        tg_vulkan_queue_family_indices queue_family_indices_temp = { 0 };
        for (uint32 j = 0; j < queue_family_count; j++)
        {
            if (queue_family_properties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                queue_family_indices_temp.graphics_family = j;
                supports_graphics_family                  = true;
            }
        }
        free(queue_family_properties);

        if (is_discrete_gpu && supports_geometry_shader && supports_graphics_family)
        {
            physical_device      = physical_devices[i];
            queue_family_indices = queue_family_indices_temp;
            break;
        }
    }
    ASSERT(physical_device != VK_NULL_HANDLE);
    free(physical_devices);

    // device
    VkDeviceQueueCreateInfo device_queue_create_info = { 0 };
    const float queue_priority = 1.0f;
    device_queue_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_queue_create_info.queueFamilyIndex        = queue_family_indices.graphics_family;
    device_queue_create_info.queueCount              = 1;
    device_queue_create_info.pQueuePriorities        = &queue_priority;

    VkPhysicalDeviceFeatures physical_device_features = { 0 };

    VkDeviceCreateInfo device_create_info      = { 0 };
    device_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount    = 1;
    device_create_info.pQueueCreateInfos       = &device_queue_create_info;
    device_create_info.enabledLayerCount       = VALIDATION_LAYER_COUNT;
    device_create_info.ppEnabledLayerNames     = VALIDATION_LAYER_NAMES;
    device_create_info.enabledExtensionCount   = 0;
    device_create_info.ppEnabledExtensionNames = NULL;
    device_create_info.pEnabledFeatures        = &physical_device_features;

    VkResult create_device_result = vkCreateDevice(physical_device, &device_create_info, NULL, &device);
    ASSERT(create_device_result == VK_SUCCESS);

    // graphics queue
    vkGetDeviceQueue(device, queue_family_indices.graphics_family, 0, &queue);
}

void tg_vulkan_shutdown()
{
    vkDestroyDevice(device, NULL);
#ifdef TG_DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    ASSERT(vkDestroyDebugUtilsMessengerEXT);
    vkDestroyDebugUtilsMessengerEXT(instance, debug_utils_messenger, NULL);
#endif
    vkDestroyInstance(instance, NULL);
}
