#ifndef TGVK_SHADER_LIBRARY_H
#define TGVK_SHADER_LIBRARY_H

#include "graphics/vulkan/tgvk_core.h"

#ifdef TG_VULKAN

void            tgvk_shader_library_init(void);
void            tgvk_shader_library_shutdown(void);

tgvk_shader*    tgvk_shader_library_get(const char* p_filename);

#endif

#endif
