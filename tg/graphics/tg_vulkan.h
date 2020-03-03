#ifndef TG_VULKAN_H
#define TG_VULKAN_H

#include "tg/tg_common.h"

typedef struct tg_image tg_image;
typedef tg_image* tg_image_h;

void tgvk_init();
void tgvk_render();
void tgvk_shutdown();
void tgvk_on_window_resize(ui32 width, ui32 height);

void tg_graphics_create_image(const char* filename, tg_image_h* p_image_h);
void tg_graphics_destroy_image(tg_image_h image_h);

#endif
