#ifndef TG_PLATFORM_H
#define TG_PLATFORM_H

#include "tg/tg_common.h"

#ifdef TG_DEBUG
#define TG_DEBUG_BREAK()                __debugbreak()
#define TG_DEBUG_PRINT(x)               tg_platform_debug_print(x)
#define TG_DEBUG_PRINT_PERFORMANCE()    tg_platform_debug_print_performance()
#else
#define TG_DEBUG_BREAK()
#define TG_DEBUG_PRINT(x)
#define TG_DEBUG_PRINT_PERFORMANCE()
#endif



typedef void* tg_window_h;



#ifdef TG_DEBUG
void tg_platform_debug_print(const char* string);
void tg_platform_debug_print_performance();
#endif

void tg_platform_get_screen_size(ui32* width, ui32* height);
void tg_platform_get_window_handle(tg_window_h* p_window_h);
void tg_platform_get_window_size(ui32* width, ui32* height);
void tg_platform_handle_events();

#endif
