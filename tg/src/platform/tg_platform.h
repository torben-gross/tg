#ifndef TG_PLATFORM_H
#define TG_PLATFORM_H

#include "tg_common.h"

#ifdef TG_DEBUG
#define TG_DEBUG_BREAK()     TG_ASSERT(false)
#define TG_DEBUG_PRINT(x)    tg_platform_debug_print(x)
#else
#define TG_DEBUG_BREAK()
#define TG_DEBUG_PRINT(x)
#endif



typedef void* tg_window_h;



#ifdef TG_DEBUG
void    tg_platform_debug_print(const char* p_message);
#endif

void    tg_platform_get_mouse_position(u32* p_x, u32* p_y);
void    tg_platform_get_screen_size(u32* p_width, u32* p_height);
f32     tg_platform_get_window_aspect_ratio();
void    tg_platform_get_window_handle(tg_window_h* p_window_h);
void    tg_platform_get_window_size(u32* p_width, u32* p_height);
void    tg_platform_handle_events();

#endif
