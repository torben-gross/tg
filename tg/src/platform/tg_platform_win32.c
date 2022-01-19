#include "platform/tg_platform.h"

#include "graphics/tg_graphics.h"
#include "memory/tg_memory.h"
#include "tg_application.h"
#include "tg_input.h"
#include "util/tg_list.h"
#include "util/tg_string.h"



#ifdef TG_DEBUG
#define WIN32_CALL(x) TG_ASSERT(x)
#else
#define WIN32_CALL(x) x
#endif



static HANDLE            h_process_heap = INVALID_HANDLE_VALUE;
static HWND              h_window = INVALID_HANDLE_VALUE;
static SYSTEMTIME        startup_system_time = { 0 };
static FILETIME          startup_file_time = { 0 };
static ULARGE_INTEGER    startup_time_ularge_integer = { 0 };



static void tg__convert_filetime_to_systemtime(const FILETIME* p_file_time, tg_system_time* p_system_time)
{
    SYSTEMTIME systemtime = { 0 };
    FileTimeToSystemTime(p_file_time, &systemtime);
    p_system_time->year = systemtime.wYear;
    p_system_time->month = systemtime.wMonth;
    p_system_time->day = systemtime.wDay;
    p_system_time->hour = systemtime.wHour;
    p_system_time->minute = systemtime.wMinute;
    p_system_time->second = systemtime.wSecond;
    p_system_time->milliseconds = systemtime.wMilliseconds;
}

void tg__extract_directory(char* p_buffer, const char* p_filename)
{
    u32 character_count = 0;
    const char* p_it = p_filename;
    while (*p_it != '\0')
    {
        if (*p_it == TG_FILE_SEPERATOR)
        {
            character_count = (u32)(p_it - p_filename);
        }
        p_it++;
    }

    tg_memcpy((tg_size)character_count, p_filename, p_buffer);
    p_buffer[character_count] = '\0';
}

static void tg__fill_file_properties(const char* p_directory, WIN32_FIND_DATAA* p_find_data, tg_file_properties* p_file_properties)
{
    p_file_properties->is_directory = (p_find_data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

    tg__convert_filetime_to_systemtime(&p_find_data->ftCreationTime, &p_file_properties->creation_time);
    tg__convert_filetime_to_systemtime(&p_find_data->ftLastAccessTime, &p_file_properties->last_access_time);
    tg__convert_filetime_to_systemtime(&p_find_data->ftLastWriteTime, &p_file_properties->last_write_time);

    LARGE_INTEGER size = { 0 };
    size.LowPart = p_find_data->nFileSizeLow;
    size.HighPart = p_find_data->nFileSizeHigh;
    p_file_properties->size = size.QuadPart;

    if (*p_directory == '.')
    {
        TG_ASSERT(p_directory[1] == '\0');
        tg_strcpy(MAX_PATH, p_file_properties->p_filename, p_find_data->cFileName);
        p_file_properties->p_short_filename = p_file_properties->p_filename;
    }
    else
    {
        tg_stringf(MAX_PATH, p_file_properties->p_filename, "%s%c%s", p_directory, TG_FILE_SEPERATOR, p_find_data->cFileName);
        p_file_properties->p_short_filename = &p_file_properties->p_filename[tg_strlen_no_nul(p_directory) + 1];
    }
    tg_strcpy(MAX_PATH, p_file_properties->p_directory, p_directory);
    if (p_file_properties->is_directory)
    {
        p_file_properties->p_extension = TG_NULL;
    }
    else
    {
        p_file_properties->p_extension = p_file_properties->p_short_filename;
        while (*p_file_properties->p_extension != '\0' && *p_file_properties->p_extension++ != '.');
        TG_ASSERT(*p_file_properties->p_extension != '\0');
    }
}

void tg__prepend_asset_directory(const char* p_filename, u32 size, char* p_buffer)
{
    tg_stringf(size, p_buffer, "%s%c%s", "assets", TG_FILE_SEPERATOR, p_filename);
}





#ifdef TG_DEBUG
void tgp_debug_log(const char* p_format, ...)
{
    char p_buffer[4096] = { 0 };
    va_list va = TG_NULL;
    tg_variadic_start(va, p_format);
    tg_stringf_va(sizeof(p_buffer), p_buffer, p_format, va);

    OutputDebugStringA(p_buffer);
}
#endif




void tgp_handle_events(void)
{
    MSG msg = { 0 };
    while (PeekMessageA(&msg, TG_NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}



void* tgp_malloc(tg_size size)
{
    void* p_memory = HeapAlloc(h_process_heap, HEAP_ZERO_MEMORY, (SIZE_T)size);
    TG_ASSERT(p_memory);
    return p_memory;
}

void* tgp_realloc(tg_size size, void* p_memory)
{
    void* p_reallocated_memory = HeapReAlloc(h_process_heap, HEAP_ZERO_MEMORY, p_memory, (SIZE_T)size);
    TG_ASSERT(p_reallocated_memory);
    return p_reallocated_memory;
}

void tgp_free(void* p_memory)
{
    const BOOL result = HeapFree(h_process_heap, 0, p_memory);
    TG_ASSERT(result);
}



void tgp_get_mouse_position(u32* p_x, u32* p_y)
{
    POINT point = { 0 };
    WIN32_CALL(GetCursorPos(&point));
    WIN32_CALL(ScreenToClient(h_window, &point));
    u32 width;
    u32 height;
    tgp_get_window_size(&width, &height);
    *p_x = (u32)tgm_i32_clamp(point.x, 0, width - 1);
    *p_y = (u32)tgm_i32_clamp(point.y, 0, height - 1);
}

void tgp_get_screen_size(u32* p_width, u32* p_height)
{
    RECT rect;
    WIN32_CALL(GetWindowRect(GetDesktopWindow(), &rect));
    *p_width = rect.right - rect.left;
    *p_height = rect.bottom - rect.top;
}

tg_system_time tgp_get_system_time(void)
{
    SYSTEMTIME system_time = { 0 };
    GetSystemTime(&system_time);
    tg_system_time result = { 0 };
    result.year = system_time.wYear;
    result.month = system_time.wMonth;
    result.day = system_time.wDay;
    result.hour = system_time.wHour;
    result.minute = system_time.wMinute;
    result.second = system_time.wSecond;
    result.milliseconds = system_time.wMilliseconds;
    return result;
}

f32 tgp_get_seconds_since_startup(void)
{
    FILETIME file_time = { 0 };
    GetSystemTimeAsFileTime(&file_time);

    ULARGE_INTEGER ularge_integer = { 0 };
    ularge_integer.LowPart = file_time.dwLowDateTime;
    ularge_integer.HighPart = file_time.dwHighDateTime;

    ularge_integer.QuadPart = ularge_integer.QuadPart - startup_time_ularge_integer.QuadPart;
    const f32 seconds = (f32)((f64)ularge_integer.QuadPart / 10000000.0);

    return seconds;
}

tg_window_h tgp_get_window_handle(void)
{
    return h_window;
}

f32 tgp_get_window_aspect_ratio(void)
{
    u32 width;
    u32 height;
    tgp_get_window_size(&width, &height);
    return (f32)width / (f32)height;
}

u32 tgp_get_window_dpi(void)
{
    const u32 result = (u32)GetDpiForWindow(h_window);
    return result;
}

void tgp_get_window_size(u32* p_width, u32* p_height)
{
    RECT rect;
    WIN32_CALL(GetWindowRect(h_window, &rect));
    *p_width = rect.right - rect.left;
    *p_height = rect.bottom - rect.top;
}



b32 tgp_file_exists(const char* p_filename)
{
    TG_ASSERT(p_filename);

    char p_buffer[MAX_PATH] = { 0 };
    tg__prepend_asset_directory(p_filename, MAX_PATH, p_buffer);

    const u32 result = GetFileAttributesA(p_buffer) != INVALID_FILE_ATTRIBUTES;
    return result;
}

b32 tgp_file_load(const char* p_filename, tg_size buffer_size, void* p_buffer)
{
    TG_ASSERT(p_filename && buffer_size && p_buffer);

    b32 result = TG_FALSE;

    char p_filename_buffer[MAX_PATH] = { 0 };
    tg__prepend_asset_directory(p_filename, MAX_PATH, p_filename_buffer);

    HANDLE h_file = CreateFileA(p_filename_buffer, GENERIC_READ | GENERIC_WRITE, 0, TG_NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, TG_NULL);
    if (h_file != INVALID_HANDLE_VALUE)
    {
        result = TG_TRUE;

        const u32 size = GetFileSize(h_file, TG_NULL);
        TG_ASSERT(buffer_size >= size);
        u32 number_of_bytes_read = 0;
#pragma warning(push)
#pragma warning(disable:6031)
        WIN32_CALL(ReadFile(h_file, p_buffer, size, (LPDWORD)&number_of_bytes_read, TG_NULL));
#pragma warning(pop)

        CloseHandle(h_file);
    }
#ifdef TG_DEBUG
    else
    {
        const u32 last_error = GetLastError();
        TG_DEBUG_LOG("Error while trying to read file \"%s\". #%u\n", p_filename, last_error);
        TG_INVALID_CODEPATH();
    }
    TG_ASSERT(GetLastError() != ERROR_FILE_NOT_FOUND);
#endif

    return result;
}

b32 tgp_file_create(const char* p_filename, tg_size size, const void* p_data, b32 replace_existing)
{
    TG_ASSERT(p_filename && size && size <= TG_U32_MAX && p_data);

    b32 result = TG_FALSE;

    char p_buffer[MAX_PATH] = { 0 };
    tg__prepend_asset_directory(p_filename, MAX_PATH, p_buffer);

    HANDLE h_file = CreateFileA(p_buffer, GENERIC_READ | GENERIC_WRITE, 0, TG_NULL, replace_existing ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, TG_NULL);
#ifdef TG_DEBUG
    if (h_file == INVALID_HANDLE_VALUE)
    {
        const DWORD last_error = GetLastError();
        if (replace_existing)
        {
            LPVOID p_message_buffer = TG_NULL;
            FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                TG_NULL,
                last_error,
                MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                (LPSTR)&p_message_buffer,
                0, TG_NULL
            );
            TG_DEBUG_LOG(p_message_buffer);
            LocalFree(p_message_buffer);
            p_message_buffer = TG_NULL;
            TG_INVALID_CODEPATH();
        }
        else
        {
            TG_ASSERT(last_error == ERROR_FILE_EXISTS);
        }
    }
    else
#endif
    {
        u32 written_size = 0;
        const BOOL write_result = WriteFile(h_file, p_data, (DWORD)size, (LPDWORD)&written_size, TG_NULL);
        TG_ASSERT(write_result);

        const BOOL close_result = CloseHandle(h_file);
        TG_ASSERT(close_result);

        result = TG_TRUE;
    }

    return result;
}

b32 tgp_file_get_properties(const char* p_filename, tg_file_properties* p_file_properties)
{
    b32 result = TG_FALSE;

    char p_buffer[MAX_PATH] = { 0 };
    tg__prepend_asset_directory(p_filename, MAX_PATH, p_buffer);

    WIN32_FIND_DATAA find_data = { 0 };
    HANDLE handle = FindFirstFileA(p_buffer, &find_data);
    if (handle != INVALID_HANDLE_VALUE)
    {
        result = TG_TRUE;

        char p_directory_buffer[MAX_PATH] = { 0 };
        tg__extract_directory(p_directory_buffer, p_filename);

        tg__fill_file_properties(p_directory_buffer, &find_data, p_file_properties);
        FindClose(handle);
    }

    return result;
}

tg_file_iterator_h tgp_directory_begin_iteration(const char* p_directory, tg_file_properties* p_file_properties)
{
    TG_ASSERT(p_directory && p_file_properties);

    char p_postfix_buffer[MAX_PATH] = { 0 };
    tg_stringf(MAX_PATH, p_postfix_buffer, "%s%c%c", p_directory, TG_FILE_SEPERATOR, '*');

    char p_buffer[MAX_PATH] = { 0 };
    tg__prepend_asset_directory(p_postfix_buffer, MAX_PATH, p_buffer);

    WIN32_FIND_DATAA find_data = { 0 };
    HANDLE h_file_iterator = FindFirstFileA(p_buffer, &find_data);
    TG_ASSERT(h_file_iterator != INVALID_HANDLE_VALUE);

    while (find_data.cFileName[0] == '.' && (find_data.cFileName[1] == '\0' || find_data.cFileName[1] == '.'))
    {
        if (!FindNextFileA(h_file_iterator, &find_data))
        {
            TG_ASSERT(GetLastError() == ERROR_NO_MORE_FILES);
            FindClose(h_file_iterator);
            return TG_NULL;
        }
    }
    tg__fill_file_properties(p_directory, &find_data, p_file_properties);

    return h_file_iterator;
}

b32 tgp_directory_continue_iteration(tg_file_iterator_h h_file_iterator, const char* p_directory, tg_file_properties* p_file_properties)
{
    TG_ASSERT(h_file_iterator && p_file_properties);

    WIN32_FIND_DATAA find_data = { 0 };
    if (!FindNextFileA(h_file_iterator, &find_data))
    {
        TG_ASSERT(GetLastError() == ERROR_NO_MORE_FILES);
        FindClose(h_file_iterator);
        return TG_FALSE;
    }
    tg__fill_file_properties(p_directory, &find_data, p_file_properties);
    return TG_TRUE;
}

tg_size tgp_directory_get_size(const char* p_directory)
{
    tg_file_properties file_properties = { 0 };
    tg_file_iterator_h h_file_iterator = tgp_directory_begin_iteration(p_directory, &file_properties);

    if (h_file_iterator == TG_NULL)
    {
        return 0;
    }

    tg_size size = 0;
    do
    {
        if (file_properties.is_directory)
        {
            if (*p_directory == '.')
            {
                TG_ASSERT(p_directory[1] == '\0');
                size += tgp_directory_get_size(file_properties.p_filename);
            }
            else
            {
                char p_buffer[MAX_PATH] = { 0 };
                tg_stringf(MAX_PATH, p_buffer, "%s%c%s", p_directory, TG_FILE_SEPERATOR, file_properties.p_short_filename);
                size += tgp_directory_get_size(p_buffer);
            }
        }
        else
        {
            size += file_properties.size;
        }
    } while (tgp_directory_continue_iteration(h_file_iterator, p_directory, &file_properties));
    return size;
}

i8 tgp_system_time_compare(tg_system_time* p_time0, tg_system_time* p_time1)
{
    if (p_time0->year < p_time1->year)
    {
        return -1;
    }
    else if (p_time0->year > p_time1->year)
    {
        return 1;
    }

    if (p_time0->month < p_time1->month)
    {
        return -1;
    }
    else if (p_time0->month > p_time1->month)
    {
        return 1;
    }

    if (p_time0->day < p_time1->day)
    {
        return -1;
    }
    else if (p_time0->day > p_time1->day)
    {
        return 1;
    }

    if (p_time0->hour < p_time1->hour)
    {
        return -1;
    }
    else if (p_time0->hour > p_time1->hour)
    {
        return 1;
    }

    if (p_time0->minute < p_time1->minute)
    {
        return -1;
    }
    else if (p_time0->minute > p_time1->minute)
    {
        return 1;
    }

    if (p_time0->second < p_time1->second)
    {
        return -1;
    }
    else if (p_time0->second > p_time1->second)
    {
        return 1;
    }

    if (p_time0->milliseconds < p_time1->milliseconds)
    {
        return -1;
    }
    else if (p_time0->milliseconds > p_time1->milliseconds)
    {
        return 1;
    }

    return 0;
}



void tgp_timer_init(tg_timer* p_timer)
{
    TG_ASSERT(p_timer->running == TG_FALSE);
    TG_ASSERT(p_timer->counter_elapsed == 0ll);
    TG_ASSERT(p_timer->performance_frequency.QuadPart == 0ll);
    TG_ASSERT(p_timer->start_performance_counter.QuadPart == 0ll);
    TG_ASSERT(p_timer->end_performance_counter.QuadPart == 0ll);

    p_timer->running = TG_FALSE;
    QueryPerformanceFrequency(&p_timer->performance_frequency);
    QueryPerformanceCounter(&p_timer->start_performance_counter);
    QueryPerformanceCounter(&p_timer->end_performance_counter);
}

void tgp_timer_start(tg_timer* p_timer)
{
    TG_ASSERT(p_timer);

    if (!p_timer->running)
    {
        p_timer->running = TG_TRUE;
        QueryPerformanceCounter(&p_timer->start_performance_counter);
    }
}

void tgp_timer_stop(tg_timer* p_timer)
{
    TG_ASSERT(p_timer);

    if (p_timer->running)
    {
        p_timer->running = TG_FALSE;
        QueryPerformanceCounter(&p_timer->end_performance_counter);
        p_timer->counter_elapsed += p_timer->end_performance_counter.QuadPart - p_timer->start_performance_counter.QuadPart;
    }
}

void tgp_timer_reset(tg_timer* p_timer)
{
    TG_ASSERT(p_timer);

    p_timer->running = TG_FALSE;
    p_timer->counter_elapsed = 0;
    QueryPerformanceCounter(&p_timer->start_performance_counter);
    QueryPerformanceCounter(&p_timer->end_performance_counter);
}

f32 tgp_timer_elapsed_ms(const tg_timer* p_timer)
{
    TG_ASSERT(p_timer);
    TG_ASSERT(!p_timer->running);

    return (f32)((1000000000ll * p_timer->counter_elapsed) / p_timer->performance_frequency.QuadPart) / 1000000.0f;
}



/*------------------------------------------------------------+
| Multithreading                                              |
+------------------------------------------------------------*/

#define TG_WORK_QUEUE_MAX_ENTRY_COUNT    65536

typedef struct tg_work_queue
{
    volatile HANDLE    h_semaphore;
    tg_thread_fn*      pp_work_fns[TG_WORK_QUEUE_MAX_ENTRY_COUNT];
    volatile void*     pp_user_datas[TG_WORK_QUEUE_MAX_ENTRY_COUNT];
    volatile u32       first;
    volatile u32       one_past_last;
    volatile HANDLE    h_push_mutex;
    volatile HANDLE    h_pop_mutex;
    volatile u32       active_thread_count;
} tg_work_queue;

static volatile tg_work_queue work_queue = { 0 };
static volatile HANDLE h_thread_mutex;
static volatile u32 p_thread_ids[TG_MAX_THREADS] = { 0 };

static b32 tg__work_queue_execute_entry(void)
{
    b32 result = TG_FALSE;

    if (WaitForSingleObject(work_queue.h_pop_mutex, 0) == WAIT_OBJECT_0)
    {
        tg_thread_fn* p_work_fn = TG_NULL;
        volatile void* p_user_data = TG_NULL;

        if (work_queue.first != work_queue.one_past_last)
        {
            result = TG_TRUE;
            _InterlockedIncrement((volatile LONG*)&work_queue.active_thread_count);

            p_work_fn = work_queue.pp_work_fns[work_queue.first];
            p_user_data = work_queue.pp_user_datas[work_queue.first];

            work_queue.pp_work_fns[work_queue.first] = TG_NULL;
            work_queue.pp_user_datas[work_queue.first] = TG_NULL;
            work_queue.first = tgm_u32_incmod(work_queue.first, TG_WORK_QUEUE_MAX_ENTRY_COUNT);
        }

        ReleaseMutex(work_queue.h_pop_mutex);

        if (result)
        {
            p_work_fn(p_user_data); // if this crashes because p_thread_fn is TG_NULL, than TG_WORK_QUEUE_MAX_ENTRY_COUNT is too small
            _InterlockedDecrement((volatile LONG*)&work_queue.active_thread_count);
        }
    }

    return result;
}

#pragma warning(push)
#pragma warning(disable:4100)
static DWORD WINAPI tg__worker_thread_proc(LPVOID p_param)
{
    while (TG_TRUE) // TODO: this could instead check for 'running'
    {
        if (!tg__work_queue_execute_entry())
        {
            TG_SEMAPHORE_WAIT(work_queue.h_semaphore);
        }
    }
    return 0;
}
#pragma warning(pop)

tg_thread_h tgp_thread_create(tg_thread_fn* p_thread_fn, volatile void* p_user_data)
{
    TG_ASSERT(p_thread_fn);

    HANDLE result = TG_NULL;
    TG_MUTEX_LOCK(h_thread_mutex);
    for (u32 i = 0; i < TG_MAX_THREADS; i++)
    {
        if (p_thread_ids[i] == 0)
        {
            u32 thread_id = 0;
            result = CreateThread(TG_NULL, 0, (LPTHREAD_START_ROUTINE)p_thread_fn, (LPVOID)p_user_data, 0, (LPDWORD)&thread_id);
            p_thread_ids[i] = thread_id;
            break;
        }
    }
    TG_MUTEX_UNLOCK(h_thread_mutex);
    return result;
}

void tgp_thread_destroy(tg_thread_h h_thread)
{
    TG_ASSERT(h_thread);

    TG_MUTEX_LOCK(h_thread_mutex);
    const u32 thread_id = GetThreadId(h_thread);
    for (u32 i = 0; i < TG_MAX_THREADS; i++)
    {
        if (p_thread_ids[i] == thread_id)
        {
#pragma warning(push)
#pragma warning(disable:6001)
            WaitForSingleObject(h_thread, INFINITE);
            WIN32_CALL(CloseHandle(h_thread));
#pragma warning(pop)
            p_thread_ids[i] = 0;
        }
    }
    TG_MUTEX_UNLOCK(h_thread_mutex);
}

u32 tgp_get_thread_id(void)
{
    u32 result = 0;

    const u32 thread_id = GetThreadId(GetCurrentThread());
    for (u32 i = 0; i < TG_MAX_THREADS; i++)
    {
        if (p_thread_ids[i] == thread_id)
        {
            result = i;
            break;
        }
    }

    return result;
}

void tgp_work_queue_add_entry(tg_thread_fn* p_thread_fn, volatile void* p_user_data)
{
    TG_ASSERT(p_thread_fn);

    TG_MUTEX_LOCK(work_queue.h_push_mutex);
    work_queue.pp_user_datas[work_queue.one_past_last] = p_user_data;
    work_queue.pp_work_fns[work_queue.one_past_last] = p_thread_fn;
    work_queue.one_past_last = tgm_u32_incmod(work_queue.one_past_last, TG_WORK_QUEUE_MAX_ENTRY_COUNT);
    TG_MUTEX_UNLOCK(work_queue.h_push_mutex);
    TG_SEMAPHORE_RELEASE(work_queue.h_semaphore);
}

void tgp_work_queue_wait_for_completion(void)
{
    while (work_queue.first != work_queue.one_past_last || work_queue.active_thread_count)
    {
        tg__work_queue_execute_entry();
    }
}





LRESULT CALLBACK tg__window_proc(HWND hwdn, UINT message, WPARAM w_param, LPARAM l_param)
{
    switch (message)
    {
    case WM_DESTROY:
    case WM_CLOSE:
    {
        tg_application_quit();
        PostQuitMessage(EXIT_SUCCESS);
    } break;
    case WM_QUIT:
    {
        DestroyWindow(hwdn);
    } break;
    case WM_SIZE:
    {
        tg_graphics_on_window_resize((u32)LOWORD(l_param), (u32)HIWORD(l_param));
        tg_application_on_window_resize((u32)LOWORD(l_param), (u32)HIWORD(l_param));
    } break;

    case WM_LBUTTONDOWN: tg_input_on_mouse_button_pressed(TG_BUTTON_LEFT);              break;
    case WM_RBUTTONDOWN: tg_input_on_mouse_button_pressed(TG_BUTTON_RIGHT);             break;
    case WM_MBUTTONDOWN: tg_input_on_mouse_button_pressed(TG_BUTTON_MIDDLE);            break;
    case WM_XBUTTONDOWN: tg_input_on_mouse_button_pressed((tg_button)HIWORD(w_param));  break;
    case WM_LBUTTONUP:   tg_input_on_mouse_button_released(TG_BUTTON_LEFT);             break;
    case WM_RBUTTONUP:   tg_input_on_mouse_button_released(TG_BUTTON_RIGHT);            break;
    case WM_MBUTTONUP:   tg_input_on_mouse_button_released(TG_BUTTON_MIDDLE);           break;
    case WM_XBUTTONUP:   tg_input_on_mouse_button_released((tg_button)HIWORD(w_param)); break;
    case WM_MOUSEWHEEL:
    {
        const i16 mouse_wheel_delta = GET_WHEEL_DELTA_WPARAM(w_param);
        const f32 mouse_wheel_delta_normalized = (f32)mouse_wheel_delta / (f32)WHEEL_DELTA;
        tg_input_on_mouse_wheel_rotated(mouse_wheel_delta_normalized);
    } break;
    case WM_KEYDOWN:
    {
        const tg_key key = (tg_key)w_param;
        const b32 repeated = ((1ULL << 30) & (i64)l_param) >> 30;
        const u32 additional_key_repeat_count = (u32)(0xffffULL & l_param);
        tg_input_on_key_pressed(key, repeated, additional_key_repeat_count);
    } break;
    case WM_KEYUP: tg_input_on_key_released((tg_key)w_param);                           break;
    default: return DefWindowProcA(hwdn, message, w_param, l_param);
    }
    return 0;
}

#pragma warning(push)
#pragma warning(disable:4100)
int CALLBACK WinMain(_In_ HINSTANCE h_instance, _In_opt_ HINSTANCE h_prev_instance, _In_ LPSTR cmd_line, _In_ int show_cmd)
{
    work_queue.h_semaphore = TG_SEMAPHORE_CREATE(0, TG_WORK_QUEUE_MAX_ENTRY_COUNT);
    work_queue.h_push_mutex = TG_MUTEX_CREATE();
    work_queue.h_pop_mutex = TG_MUTEX_CREATE(); // TODO: nothing is destroyed...

    h_thread_mutex = TG_MUTEX_CREATE();
    p_thread_ids[0] = GetThreadId(GetCurrentThread());
    for (u8 i = 0; i < TG_WORKER_THREADS; i++)
    {
        u32 thread_id = 0;
        HANDLE h_thread = CreateThread(TG_NULL, 0, tg__worker_thread_proc, TG_NULL, 0, (LPDWORD)&thread_id);
        p_thread_ids[i + 1] = thread_id;
#pragma warning(push) // TODO: this is ridiculous
#pragma warning(disable:6001)
        WIN32_CALL(CloseHandle(h_thread));
#pragma warning(pop)
    }

    h_process_heap = GetProcessHeap();
    tg_memory_init();

    const char* window_class_id = "tg";
    const char* window_title = "tg";

    WNDCLASSEXA window_class_info   = { 0 };
    window_class_info.lpszClassName = window_class_id;
    window_class_info.cbSize = sizeof(window_class_info);
    window_class_info.style = 0;
    window_class_info.lpfnWndProc = DefWindowProcA;
    window_class_info.cbClsExtra = 0;
    window_class_info.cbWndExtra = 0;
    window_class_info.hInstance = h_instance;
    window_class_info.hIcon = TG_NULL;
    window_class_info.hCursor = LoadCursor(TG_NULL, IDC_ARROW);
    window_class_info.hbrBackground = TG_NULL;
    window_class_info.lpszMenuName = TG_NULL;
    window_class_info.lpszClassName = window_class_id;
    window_class_info.hIconSm = TG_NULL;

    const ATOM atom = RegisterClassExA(&window_class_info);
    TG_ASSERT(atom);

    SetProcessDPIAware();
    u32 w, h;
    tgp_get_screen_size(&w, &h);
    h_window = CreateWindowExA(0, window_class_id, window_title, WS_OVERLAPPEDWINDOW, 0, 0, w, h, TG_NULL, TG_NULL, h_instance, TG_NULL);
    TG_ASSERT(h_window);

    ShowWindow(h_window, show_cmd);
    SetWindowLongPtr(h_window, GWLP_WNDPROC, (LONG_PTR)&tg__window_proc);

    GetSystemTime(&startup_system_time);
    SystemTimeToFileTime(&startup_system_time, &startup_file_time);
    startup_time_ularge_integer.LowPart = startup_file_time.dwLowDateTime;
    startup_time_ularge_integer.HighPart = startup_file_time.dwHighDateTime;

    tg_application_start();

    tg_memory_shutdown();

    // TODO: use this for killing threads without stdlib?
    // ExitProcess(0);
    return EXIT_SUCCESS;
}
#pragma warning(pop)
