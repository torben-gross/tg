#ifndef TG_PLATFORM_H
#define TG_PLATFORM_H

#include "tg_common.h"

#ifdef TG_WIN32
#define TG_MAX_PATH        260
#endif

#ifdef TG_DEBUG
#define TG_DEBUG_LOG(x)    tg_platform_debug_log(x)
#else
#define TG_DEBUG_LOG(x)
#endif



typedef void* tg_window_h;
typedef void* tg_file_iterator_h;
TG_DECLARE_HANDLE(tg_timer);



typedef struct tg_system_time {
    u16    year;
    u16    month;
    u16    dayOfWeek;
    u16    day;
    u16    hour;
    u16    minute;
    u16    second;
    u16    milliseconds;
} tg_system_time;

typedef struct tg_file_properties
{
    b32               is_directory;
    tg_system_time    creation_time;
    tg_system_time    last_access_time;
    tg_system_time    last_write_time;
    u64               size;
    char              p_relative_directory[TG_MAX_PATH];
    char              p_filename[TG_MAX_PATH];
} tg_file_properties;



#ifdef TG_DEBUG
void                  tg_platform_debug_log(const char* p_message);
#endif

void*                 tg_platform_memory_alloc(u64 size);
void                  tg_platform_memory_free(void* p_memory);
void*                 tg_platform_memory_realloc(u64 size, void* p_memory);

void                  tg_platform_get_mouse_position(u32* p_x, u32* p_y);
void                  tg_platform_get_screen_size(u32* p_width, u32* p_height);
f32                   tg_platform_get_window_aspect_ratio();
tg_window_h           tg_platform_get_window_handle();
void                  tg_platform_get_window_size(u32* p_width, u32* p_height);
void                  tg_platform_handle_events();

tg_file_iterator_h    tg_platform_begin_file_iteration(const char* p_directory, tg_file_properties* p_properties);
b32                   tg_platform_continue_file_iteration(const char* p_directory, tg_file_iterator_h file_iterator_h, tg_file_properties* p_properties);
b32                   tg_platform_file_exists(const char* p_filename);
void                  tg_platform_free_file(char* p_data);
char                  tg_platform_get_file_separator();
u64                   tg_platform_get_full_directory_size(const char* p_directory);
void                  tg_platform_read_file(const char* p_filename, u32* p_size, char** pp_data);

tg_timer_h            tg_platform_timer_create();
void                  tg_platform_timer_start(tg_timer_h timer_h);
void                  tg_platform_timer_stop(tg_timer_h timer_h);
void                  tg_platform_timer_reset(tg_timer_h timer_h);
f32                   tg_platform_timer_elapsed_milliseconds(tg_timer_h timer_h);
void                  tg_platform_timer_destroy(tg_timer_h timer_h);

#endif
