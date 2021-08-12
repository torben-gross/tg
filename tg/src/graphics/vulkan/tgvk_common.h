#ifndef TG_VULKAN_COMMON_H
#define TG_VULKAN_COMMON_H

#include "tg_common.h"

#ifdef TG_VULKAN

#include "platform/tg_platform.h"

#pragma warning(push, 3)
#include <vulkan/vulkan.h>
#pragma warning(pop)

#ifdef TG_WIN32
#undef near
#undef far
#undef min
#undef max
#endif

#ifdef TG_DEBUG
#define TGVK_CALL_NAME2(x, y)    x ## y
#define TGVK_CALL_NAME(x, y)     TGVK_CALL_NAME2(x, y)

#define TGVK_CALL(x)                                                                                                   \
    const VkResult TGVK_CALL_NAME(vk_call_result, __LINE__) = (x);                                                     \
    if (TGVK_CALL_NAME(vk_call_result, __LINE__) != VK_SUCCESS)                                                        \
    {                                                                                                                  \
        char TGVK_CALL_NAME(p_buffer, __LINE__)[1024] = { 0 };                                                         \
        tgvk_vkresult_convert_to_string(TGVK_CALL_NAME(p_buffer, __LINE__), TGVK_CALL_NAME(vk_call_result, __LINE__)); \
        TG_DEBUG_LOG("VkResult: %s\n", TGVK_CALL_NAME(p_buffer, __LINE__));                                            \
    }                                                                                                                  \
    if (TGVK_CALL_NAME(vk_call_result, __LINE__) != VK_SUCCESS) TG_INVALID_CODEPATH()

void tgvk_vkresult_convert_to_string(char* p_buffer, VkResult result);
#else
#define TGVK_CALL(x) x
#endif

#endif

#endif
