#include "graphics/vulkan/tgvk_common.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"

void tgvk_vkresult_convert_to_string(char* p_buffer, VkResult result)
{
    switch (result)
    {
    case VK_SUCCESS:                                            { const char p_string[] = "VK_SUCCESS";                                            tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_NOT_READY:                                          { const char p_string[] = "VK_NOT_READY";                                          tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_TIMEOUT:                                            { const char p_string[] = "VK_TIMEOUT";                                            tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_EVENT_SET:                                          { const char p_string[] = "VK_EVENT_SET";                                          tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_EVENT_RESET:                                        { const char p_string[] = "VK_EVENT_RESET";                                        tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_INCOMPLETE:                                         { const char p_string[] = "VK_INCOMPLETE";                                         tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_OUT_OF_HOST_MEMORY:                           { const char p_string[] = "VK_ERROR_OUT_OF_HOST_MEMORY";                           tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:                         { const char p_string[] = "VK_ERROR_OUT_OF_DEVICE_MEMORY";                         tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_INITIALIZATION_FAILED:                        { const char p_string[] = "VK_ERROR_INITIALIZATION_FAILED";                        tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_DEVICE_LOST:                                  { const char p_string[] = "VK_ERROR_DEVICE_LOST";                                  tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_MEMORY_MAP_FAILED:                            { const char p_string[] = "VK_ERROR_MEMORY_MAP_FAILED";                            tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_LAYER_NOT_PRESENT:                            { const char p_string[] = "VK_ERROR_LAYER_NOT_PRESENT";                            tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_EXTENSION_NOT_PRESENT:                        { const char p_string[] = "VK_ERROR_EXTENSION_NOT_PRESENT";                        tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_FEATURE_NOT_PRESENT:                          { const char p_string[] = "VK_ERROR_FEATURE_NOT_PRESENT";                          tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_INCOMPATIBLE_DRIVER:                          { const char p_string[] = "VK_ERROR_INCOMPATIBLE_DRIVER";                          tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_TOO_MANY_OBJECTS:                             { const char p_string[] = "VK_ERROR_TOO_MANY_OBJECTS";                             tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_FORMAT_NOT_SUPPORTED:                         { const char p_string[] = "VK_ERROR_FORMAT_NOT_SUPPORTED";                         tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_FRAGMENTED_POOL:                              { const char p_string[] = "VK_ERROR_FRAGMENTED_POOL";                              tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_UNKNOWN:                                      { const char p_string[] = "VK_ERROR_UNKNOWN";                                      tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_OUT_OF_POOL_MEMORY:                           { const char p_string[] = "VK_ERROR_OUT_OF_POOL_MEMORY";                           tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:                      { const char p_string[] = "VK_ERROR_INVALID_EXTERNAL_HANDLE";                      tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_FRAGMENTATION:                                { const char p_string[] = "VK_ERROR_FRAGMENTATION";                                tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:               { const char p_string[] = "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";               tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_SURFACE_LOST_KHR:                             { const char p_string[] = "VK_ERROR_SURFACE_LOST_KHR";                             tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:                     { const char p_string[] = "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";                     tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_SUBOPTIMAL_KHR:                                     { const char p_string[] = "VK_SUBOPTIMAL_KHR";                                     tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_OUT_OF_DATE_KHR:                              { const char p_string[] = "VK_ERROR_OUT_OF_DATE_KHR";                              tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:                     { const char p_string[] = "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";                     tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_VALIDATION_FAILED_EXT:                        { const char p_string[] = "VK_ERROR_VALIDATION_FAILED_EXT";                        tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_INVALID_SHADER_NV:                            { const char p_string[] = "VK_ERROR_INVALID_SHADER_NV";                            tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: { const char p_string[] = "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT"; tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_NOT_PERMITTED_EXT:                            { const char p_string[] = "VK_ERROR_NOT_PERMITTED_EXT";                            tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:          { const char p_string[] = "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";          tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_THREAD_IDLE_KHR:                                    { const char p_string[] = "VK_THREAD_IDLE_KHR";                                    tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_THREAD_DONE_KHR:                                    { const char p_string[] = "VK_THREAD_DONE_KHR";                                    tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_OPERATION_DEFERRED_KHR:                             { const char p_string[] = "VK_OPERATION_DEFERRED_KHR";                             tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_OPERATION_NOT_DEFERRED_KHR:                         { const char p_string[] = "VK_OPERATION_NOT_DEFERRED_KHR";                         tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    case VK_PIPELINE_COMPILE_REQUIRED_EXT:                      { const char p_string[] = "VK_PIPELINE_COMPILE_REQUIRED_EXT";                      tg_memcpy(sizeof(p_string), p_string, p_buffer); } break;
    default: TG_INVALID_CODEPATH(); break;
    }
}

#endif
