#ifndef TG_PLATFORM_H
#define TG_PLATFORM_H

#include "tg_common.h"



#define TG_WORKER_THREAD_COUNT    7

#ifdef TG_WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#undef near
#undef far
#undef min
#undef max
#endif

#ifdef TG_WIN32
#define TG_MAX_PATH                                                            MAX_PATH
#define TG_FILE_SEPERATOR                                                      '\\'
#define TG_INTERLOCKED_COMPARE_EXCHANGE(p_destination, exchange, comperand)    _InterlockedCompareExchange((volatile LONG*)(p_destination), (LONG)(exchange), (LONG)(comperand))
#define TG_INTERLOCKED_INCREMENT_I32(p_append)                                 _InterlockedIncrement((volatile LONG*)(p_append))
#define TG_INTERLOCKED_DECREMENT_I32(p_append)                                 _InterlockedDecrement((volatile LONG*)(p_append))
#define TG_INTERLOCKED_INCREMENT_I64(p_append)                                 _InterlockedIncrement64((volatile LONG64*)(p_append))
#define TG_INTERLOCKED_DECREMENT_I64(p_append)                                 _InterlockedDecrement64((volatile LONG64*)(p_append))
typedef i32 tg_lock;
#endif

#ifdef TG_DEBUG
#define TG_DEBUG_LOG(x, ...)      tg_platform_debug_log(x, __VA_ARGS__)
#else
#define TG_DEBUG_LOG(x, ...)
#endif

typedef void* tg_window_h;
typedef void* tg_file_iterator_h;
TG_DECLARE_HANDLE(tg_timer);

typedef void tg_work_fn(volatile void* p_user_data);



typedef enum tg_lock_state
{
    TG_LOCK_STATE_FREE      = 0,
    TG_LOCK_STATE_LOCKED    = 1
} tg_lock_state;



typedef struct tg_system_time
{
    u16    year;
    u16    month;
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
void                  tg_platform_debug_log(const char* p_format, ...);
#endif

void                  tg_platform_handle_events();

void*                 tg_platform_memory_alloc(u64 size);
void*                 tg_platform_memory_alloc_nullify(u64 size);
void*                 tg_platform_memory_realloc(u64 size, void* p_memory);
void*                 tg_platform_memory_realloc_nullify(u64 size, void* p_memory);
void                  tg_platform_memory_free(void* p_memory);

void                  tg_platform_get_mouse_position(u32* p_x, u32* p_y);
void                  tg_platform_get_screen_size(u32* p_width, u32* p_height);
tg_window_h           tg_platform_get_window_handle();
void                  tg_platform_get_window_size(u32* p_width, u32* p_height);
f32                   tg_platform_get_window_aspect_ratio();

b32                   tg_platform_file_exists(const char* p_filename);
void                  tg_platform_file_read(const char* p_filename, u64 buffer_size, char* p_buffer);
void                  tg_platform_file_create(const char* p_filename, u32 size, char* p_data, b32 replace_existing);
b32                   tg_platform_file_get_properties(const char* p_filename, tg_file_properties* p_properties);
void                  tg_platform_file_extract_directory(u64 size, char* p_buffer, const char* p_filename);
tg_file_iterator_h    tg_platform_directory_begin_iteration(const char* p_directory, tg_file_properties* p_properties);
b32                   tg_platform_directory_continue_iteration(tg_file_iterator_h h_file_iterator, const char* p_directory, tg_file_properties* p_properties);
u64                   tg_platform_directory_get_size(const char* p_directory);
i8                    tg_platform_system_time_compare(tg_system_time* p_time0, tg_system_time* p_time1);

tg_timer_h            tg_platform_timer_create();
void                  tg_platform_timer_start(tg_timer_h h_timer);
void                  tg_platform_timer_stop(tg_timer_h h_timer);
void                  tg_platform_timer_reset(tg_timer_h h_timer);
f32                   tg_platform_timer_elapsed_milliseconds(tg_timer_h h_timer);
void                  tg_platform_timer_destroy(tg_timer_h h_timer);

u32                   tg_platform_get_current_thread_id();
tg_lock               tg_platform_lock_create(tg_lock_state initial_state);
b32                   tg_platform_try_lock(volatile tg_lock* p_lock);
void                  tg_platform_unlock(volatile tg_lock* p_lock);
void                  tg_platform_work_queue_add_entry(tg_work_fn* p_work_fn, volatile void* p_user_data);
void                  tg_platform_work_queue_wait_for_completion();

#endif
