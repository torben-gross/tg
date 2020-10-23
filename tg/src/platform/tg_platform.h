#ifndef TG_PLATFORM_H
#define TG_PLATFORM_H

#include "tg_common.h"



#define TG_WORKER_THREADS             7
#define TG_MAX_REQUESTABLE_THREADS    4
#define TG_MAX_THREADS                (1 + TG_WORKER_THREADS + TG_MAX_REQUESTABLE_THREADS)

#ifdef TG_WIN32
#pragma warning(push, 3)
#include <windows.h>
#pragma warning(pop)
#undef near
#undef far
#undef min
#undef max
#endif



#ifdef TG_WIN32

#define TG_MAX_PATH                                              MAX_PATH
#define TG_FILE_SEPERATOR                                        '\\'



#define TG_ATOMIC_ADD_I32(p_append, value) \
    InterlockedExchangeAdd((volatile LONG*)(p_append), (LONG)(value))

#define TG_ATOMIC_COMPARE_EXCHANGE(p_destination, exchange, comperand) \
    InterlockedCompareExchange((volatile LONG*)(p_destination), (LONG)(exchange), (LONG)(comperand))

#define TG_ATOMIC_DECREMENT_I32(p_append) \
    InterlockedDecrement((volatile LONG*)(p_append))

#define TG_ATOMIC_DECREMENT_I64(p_append) \
    InterlockedDecrement64((volatile LONG64*)(p_append))

#define TG_ATOMIC_INCREMENT_I32(p_append) \
    InterlockedIncrement((volatile LONG*)(p_append))

#define TG_ATOMIC_INCREMENT_I64(p_append) \
    InterlockedIncrement64((volatile LONG64*)(p_append))



#define TG_CONDITION_VARIABLE_CREATE() \
    ((tg_condition_variable)CONDITION_VARIABLE_INIT)

#define TG_CONDITION_VARIABLE_WAKE(condition_variable) \
    WakeConditionVariable((PCONDITION_VARIABLE)&(condition_variable))

#define TG_CONDITION_VARIABLE_WAKE_ALL(condition_variable) \
    WakeAllConditionVariable((PCONDITION_VARIABLE)&(condition_variable))

#define TG_CONDITION_VARIABLE_WAIT_RWL(condition_variable, read_write_lock, is_locked_for_read) \
    ((void)(SleepConditionVariableSRW((PCONDITION_VARIABLE)&(condition_variable), (PSRWLOCK)&(read_write_lock), INFINITE, (ULONG)(is_locked_for_read))))



#define TG_EVENT_CREATE() \
    ((tg_event_h)CreateEvent(NULL, FALSE, FALSE, NULL))

#define TG_EVENT_DESTROY(h_event) \
    CloseHandle((HANDLE)(h_event))

#define TG_EVENT_TRY_SIGNAL(h_event) \
    (SetEvent((HANDLE)(h_event)) == TRUE)

#ifdef TG_DEBUG
#define TG_EVENT_SIGNAL(h_event) \
    TG_ASSERT(SetEvent((HANDLE)(h_event)) == TRUE)

#define TG_EVENT_WAIT(h_event) \
    TG_ASSERT(WaitForSingleObject((HANDLE)(h_event), INFINITE) == WAIT_OBJECT_0)
#else
#define TG_EVENT_SIGNAL(h_event) \
    SetEvent((HANDLE)(h_event))

#define TG_EVENT_WAIT(h_event) \
    WaitForSingleObject((HANDLE)(h_event), INFINITE)
#endif



#define TG_MUTEX_CREATE() \
    ((tg_mutex_h)CreateMutex(NULL, FALSE, NULL))

#define TG_MUTEX_CREATE_LOCKED() \
    ((tg_mutex_h)CreateMutex(NULL, TRUE, NULL))

#define TG_MUTEX_TRY_LOCK(h_mutex) \
    (WaitForSingleObject((HANDLE)(h_mutex), 0) == WAIT_OBJECT_0)

#ifdef TG_DEBUG
#define TG_MUTEX_DESTROY(h_mutex) \
    TG_ASSERT(CloseHandle((HANDLE)(h_mutex)) == TRUE)

#define TG_MUTEX_LOCK(h_mutex) \
    TG_ASSERT(WaitForSingleObject((HANDLE)(h_mutex), INFINITE) == WAIT_OBJECT_0)

#define TG_MUTEX_UNLOCK(h_mutex) \
    TG_ASSERT(ReleaseMutex((HANDLE)(h_mutex)) == TRUE)
#else
#define TG_MUTEX_DESTROY(h_mutex) \
    CloseHandle((HANDLE)(h_mutex))

#define TG_MUTEX_LOCK(h_mutex) \
    WaitForSingleObject((HANDLE)(h_mutex), INFINITE)

#define TG_MUTEX_UNLOCK(h_mutex) \
    ReleaseMutex((HANDLE)(h_mutex))
#endif



#define TG_SEMAPHORE_CREATE(initial_count, maximum_count) \
    ((tg_semaphore_h)CreateSemaphoreEx(NULL, 0, maximum_count, NULL, 0, SEMAPHORE_ALL_ACCESS))

#define TG_SEMAPHORE_TRY_RELEASE(h_semaphore) \
    ReleaseSemaphore((HANDLE)(h_semaphore), 1, NULL)

#ifdef TG_DEBUG
#define TG_SEMAPHORE_DESTROY(h_semaphore) \
    TG_ASSERT(CloseHandle((HANDLE)(h_semaphore)) == TRUE)

#define TG_SEMAPHORE_WAIT(h_semaphore) \
    TG_ASSERT(WaitForSingleObject((HANDLE)(h_semaphore), INFINITE) == WAIT_OBJECT_0)

#define TG_SEMAPHORE_RELEASE(h_semaphore) \
    TG_ASSERT(ReleaseSemaphore((HANDLE)(h_semaphore), 1, NULL) == TRUE)
#else
#define TG_SEMAPHORE_DESTROY(h_semaphore) \
    CloseHandle((HANDLE)(h_semaphore))

#define TG_SEMAPHORE_WAIT(h_semaphore) \
    WaitForSingleObjectEx((HANDLE)(h_semaphore), INFINITE, FALSE)

#define TG_SEMAPHORE_RELEASE(h_semaphore) \
    ReleaseSemaphore((HANDLE)(h_semaphore), 1, NULL)
#endif



#define TG_RWL_CREATE() \
    ((tg_read_write_lock)SRWLOCK_INIT)

#define TG_RWL_LOCK_FOR_WRITE(read_write_lock) \
    AcquireSRWLockExclusive((PSRWLOCK)&(read_write_lock))

#define TG_RWL_LOCK_FOR_READ(read_write_lock) \
    AcquireSRWLockShared((PSRWLOCK)&(read_write_lock))

#define TG_RWL_UNLOCK_FOR_WRITE(read_write_lock) \
    ReleaseSRWLockExclusive((PSRWLOCK)&(read_write_lock))

#define TG_RWL_UNLOCK_FOR_READ(read_write_lock) \
    ReleaseSRWLockShared((PSRWLOCK)&(read_write_lock))



typedef SRWLOCK               tg_read_write_lock;
typedef CONDITION_VARIABLE    tg_condition_variable;
typedef HANDLE                tg_event_h;
typedef HANDLE                tg_mutex_h;
typedef HANDLE                tg_semaphore_h;
typedef HANDLE                tg_window_h;
typedef void*                 tg_file_iterator_h;
typedef void*                 tg_thread_h;
TG_DECLARE_HANDLE(tg_timer);

#endif



#ifdef TG_DEBUG
#define TG_DEBUG_LOG(x, ...) tg_platform_debug_log(x, __VA_ARGS__)
#else
#define TG_DEBUG_LOG(x, ...)
#endif



typedef void tg_thread_fn(volatile void* p_user_data);



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
    char              p_filename[TG_MAX_PATH];
    char              p_directory[TG_MAX_PATH];
    char*             p_short_filename;
    char*             p_extension;
} tg_file_properties;



#ifdef TG_DEBUG
void                  tg_platform_debug_log(const char* p_format, ...);
#endif

void                  tg_platform_handle_events(void);

void*                 tg_platform_memory_alloc(u64 size);
void*                 tg_platform_memory_alloc_nullify(u64 size);
void*                 tg_platform_memory_realloc(u64 size, void* p_memory);
void*                 tg_platform_memory_realloc_nullify(u64 size, void* p_memory);
void                  tg_platform_memory_free(void* p_memory);

void                  tg_platform_get_mouse_position(u32* p_x, u32* p_y);
void                  tg_platform_get_screen_size(u32* p_width, u32* p_height);
tg_system_time        tg_platform_get_system_time(void);
f32                   tg_platform_get_seconds_since_startup(void);
tg_window_h           tg_platform_get_window_handle(void);
void                  tg_platform_get_window_size(u32* p_width, u32* p_height);
f32                   tg_platform_get_window_aspect_ratio(void);

b32                   tg_platform_file_exists(const char* p_filename);
void                  tg_platform_file_read(const char* p_filename, u64 buffer_size, char* p_buffer);
b32                   tg_platform_file_create(const char* p_filename, u32 size, const char* p_data, b32 replace_existing);
b32                   tg_platform_file_get_properties(const char* p_filename, tg_file_properties* p_properties);
tg_file_iterator_h    tg_platform_directory_begin_iteration(const char* p_directory, tg_file_properties* p_properties);
b32                   tg_platform_directory_continue_iteration(tg_file_iterator_h h_file_iterator, const char* p_directory, tg_file_properties* p_properties);
u64                   tg_platform_directory_get_size(const char* p_directory);
i8                    tg_platform_system_time_compare(tg_system_time* p_time0, tg_system_time* p_time1);
void                  tg_platform_prepend_asset_directory(const char* p_filename, u32 size, char* p_buffer);

tg_timer_h            tg_platform_timer_create(void);
void                  tg_platform_timer_start(tg_timer_h h_timer);
void                  tg_platform_timer_stop(tg_timer_h h_timer);
void                  tg_platform_timer_reset(tg_timer_h h_timer);
f32                   tg_platform_timer_elapsed_milliseconds(tg_timer_h h_timer);
void                  tg_platform_timer_destroy(tg_timer_h h_timer);

tg_thread_h           tg_thread_create(tg_thread_fn* p_thread_fn, volatile void* p_user_data);
void                  tg_thread_destroy(tg_thread_h h_thread);

u32                   tg_platform_get_thread_id(void);
void                  tg_platform_work_queue_add_entry(tg_thread_fn* p_thread_fn, volatile void* p_user_data);
void                  tg_platform_work_queue_wait_for_completion(void);

#endif
