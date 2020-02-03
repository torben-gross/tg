#ifndef TG_PLATFORM_H
#define TG_PLATFORM_H

#include "tg/tg_common.h"

#ifdef TG_DEBUG
#define TG_PRINT(x) tg_platform_print(x)
#else
#define TG_PRINT(x)
#endif

void* tg_platform_get_window_handle();
void tg_platform_get_screen_size(uint32* width, uint32* height);
void tg_platform_get_window_size(uint32* width, uint32* height);
#ifdef TG_DEBUG
void tg_platform_print(const char* string);
#endif

#endif
