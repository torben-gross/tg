#ifndef TG_VULKAN_H
#define TG_VULKAN_H

#include "tg/tg_common.h"
#include "tg_graphics.h"

void tgvk_init();
void tgvk_render();
void tgvk_shutdown();
void tgvk_on_window_resize(ui32 width, ui32 height);

#endif
