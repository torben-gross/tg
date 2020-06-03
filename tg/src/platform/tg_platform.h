#ifndef TG_PLATFORM_H
#define TG_PLATFORM_H

#include "tg_common.h"

#ifdef TG_DEBUG
#define TG_DEBUG_LOG(x)    tg_platform_debug_log(x)
#else
#define TG_DEBUG_LOG(x)
#endif



typedef void* tg_window_h;
TG_DECLARE_HANDLE(tg_timer);



#ifdef TG_DEBUG
void           tg_platform_debug_log(const char* p_message);
#endif

void           tg_platform_get_mouse_position(u32* p_x, u32* p_y);
void           tg_platform_get_screen_size(u32* p_width, u32* p_height);
f32            tg_platform_get_window_aspect_ratio();
tg_window_h    tg_platform_get_window_handle();
void           tg_platform_get_window_size(u32* p_width, u32* p_height);
void           tg_platform_handle_events();

b32            tg_platform_file_exists(const char* p_filename);
void           tg_platform_free_file(char* p_data);
void           tg_platform_read_file(const char* p_filename, u32* p_size, char** pp_data);

tg_timer_h     tg_platform_timer_create();
void           tg_platform_timer_start(tg_timer_h timer_h);
void           tg_platform_timer_stop(tg_timer_h timer_h);
void           tg_platform_timer_reset(tg_timer_h timer_h);
f32            tg_platform_timer_elapsed_milliseconds(tg_timer_h timer_h);
void           tg_platform_timer_destroy(tg_timer_h timer_h);

#endif
