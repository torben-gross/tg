#ifndef TG_VULKAN_H
#define TG_VULKAN_H

#include "tg/tg_common.h"

void tg_vulkan_init();
void tg_vulkan_render();
void tg_vulkan_shutdown();
void tg_vulkan_on_window_resize(uint width, uint height);

#endif
