#include "platform/tg_platform.h"

#define _CRT_SECURE_NO_WARNINGS

#include "graphics/tg_graphics.h"
#include "memory/tg_memory.h"
#include "tg_application.h"
#include "tg_assets.h"
#include "tg_input.h"
#include "util/tg_list.h"
#include "util/tg_string.h"
#include <windows.h>

#ifdef TG_DEBUG
#define WIN32_CALL(x) TG_ASSERT(x)
#else
#define WIN32_CALL(x) x
#endif

HANDLE    h_process_heap = INVALID_HANDLE_VALUE;
HWND      h_window = INVALID_HANDLE_VALUE;



/*------------------------------------------------------------+
| File IO                                                     |
+------------------------------------------------------------*/

typedef struct tg_file_iterator
{
    HANDLE    handle;
    char      p_directory[MAX_PATH];
} tg_file_iterator;



void tg_platform_internal_convert_filetime_to_systemtime(const FILETIME* p_file_time, tg_system_time* p_system_time)
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

void tg_platform_internal_fill_file_properties(const char* p_directory, WIN32_FIND_DATAA* p_find_data, tg_file_properties* p_properties)
{
    p_properties->is_directory = (p_find_data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

    tg_platform_internal_convert_filetime_to_systemtime(&p_find_data->ftCreationTime, &p_properties->creation_time);
    tg_platform_internal_convert_filetime_to_systemtime(&p_find_data->ftLastAccessTime, &p_properties->last_access_time);
    tg_platform_internal_convert_filetime_to_systemtime(&p_find_data->ftLastWriteTime, &p_properties->last_write_time);

    LARGE_INTEGER size = { 0 };
    size.LowPart = p_find_data->nFileSizeLow;
    size.HighPart = p_find_data->nFileSizeHigh;
    p_properties->size = size.QuadPart;

    tg_memory_copy(MAX_PATH, p_directory, p_properties->p_relative_directory);
    tg_memory_copy(MAX_PATH, p_find_data->cFileName, p_properties->p_filename);
}



tg_file_iterator_h tg_platform_begin_file_iteration(const char* p_directory, tg_file_properties* p_properties)
{
    TG_ASSERT(p_directory && p_properties);

    char p_filename_buffer[MAX_PATH] = { 0 };
    u32 dir_length = tg_string_length(p_directory);
    TG_ASSERT(dir_length <= MAX_PATH - 3);

    tg_string_format(dir_length, p_filename_buffer, "%s%s", p_directory, "\\*");

    WIN32_FIND_DATAA find_data = { 0 };
    tg_file_iterator_h h_file_iterator = FindFirstFileA(p_filename_buffer, &find_data);
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
    tg_platform_internal_fill_file_properties(p_directory, &find_data, p_properties);

    return h_file_iterator;
}

b32 tg_platform_continue_file_iteration(const char* p_directory, tg_file_iterator_h h_file_iterator, tg_file_properties* p_properties)
{
    TG_ASSERT(h_file_iterator && p_properties);

    WIN32_FIND_DATAA find_data = { 0 };
    if (!FindNextFileA(h_file_iterator, &find_data))
    {
        TG_ASSERT(GetLastError() == ERROR_NO_MORE_FILES);
        FindClose(h_file_iterator);
        return TG_FALSE;
    }
    tg_platform_internal_fill_file_properties(p_directory, &find_data, p_properties);
    return TG_TRUE;
}

void tg_platform_create_file(const char* p_filename, u32 size, char* p_data, b32 replace_existing)
{
    TG_ASSERT(p_filename && size && p_data);

    char p_filename_buffer[MAX_PATH] = { 0 };
    tg_string_format(MAX_PATH, p_filename_buffer, "%s/%s", tg_assets_get_asset_path(), p_filename);

    HANDLE h_file = CreateFileA(p_filename_buffer, GENERIC_READ | GENERIC_WRITE, 0, TG_NULL, replace_existing ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, TG_NULL);
    u32 written_size = 0;
    WriteFile(h_file, p_data, size, &written_size, TG_NULL);
    CloseHandle(h_file);
}

void tg_platform_extract_file_directory(u64 size, char* p_buffer, const char* p_filename)
{
    u32 character_count = 0;
    const char* p_it = p_filename;
    while (*p_it != '\0')
    {
        if (*p_it == '/' || *p_it == '\\')
        {
            character_count = (u32)(p_it - p_filename);
        }
        p_it++;
    }

    TG_ASSERT(character_count < size);
    tg_memory_copy((u64)character_count, p_filename, p_buffer);
    p_buffer[character_count] = '\0';
}

b32 tg_platform_file_exists(const char* p_filename)
{
    TG_ASSERT(p_filename);

    char p_filename_buffer[MAX_PATH] = { 0 };
    tg_string_format(sizeof(p_filename_buffer), p_filename_buffer, "%s/%s", tg_assets_get_asset_path(), p_filename);

    const u32 result = GetFileAttributesA(p_filename_buffer) != INVALID_FILE_ATTRIBUTES;
    return result;
}

void tg_platform_free_file(char* p_data)
{
    TG_ASSERT(p_data);

    TG_MEMORY_FREE(p_data);
}

b32 tg_platform_get_file_properties(const char* p_filename, tg_file_properties* p_properties)
{
    char p_filename_buffer[MAX_PATH] = { 0 };
    tg_string_format(sizeof(p_filename_buffer), p_filename_buffer, "%s/%s", tg_assets_get_asset_path(), p_filename);

    WIN32_FIND_DATAA find_data = { 0 };
    HANDLE handle = FindFirstFileA(p_filename_buffer, &find_data);
    if (handle == INVALID_HANDLE_VALUE)
    {
        return TG_FALSE;
    }

    char p_relative_directory_buffer[MAX_PATH] = { 0 };
    tg_platform_extract_file_directory(MAX_PATH, p_relative_directory_buffer, p_filename);

    char p_directory_buffer[MAX_PATH] = { 0 };
    tg_string_format(MAX_PATH, p_directory_buffer, "%s\\%s", tg_assets_get_asset_path(), p_relative_directory_buffer);
    tg_string_replace_characters(p_directory_buffer, '/', '\\');

    tg_platform_internal_fill_file_properties(p_directory_buffer, &find_data, p_properties);
    FindClose(handle);

    return TG_TRUE;
}

char tg_platform_get_file_separator()
{
    return '\\';
}

u64 tg_platform_get_full_directory_size(const char* p_directory)
{
    tg_file_properties file_properties = { 0 };
    tg_file_iterator_h h_file_iterator = tg_platform_begin_file_iteration(p_directory, &file_properties);

    if (h_file_iterator == TG_NULL)
    {
        return 0;
    }

    u64 size = 0;
    do
    {
        if (file_properties.is_directory)
        {
            char p_buffer[TG_MAX_PATH] = { 0 };
            tg_string_format(sizeof(p_buffer), p_buffer, "%s%c%s", p_directory, tg_platform_get_file_separator(), file_properties.p_filename);
            size += tg_platform_get_full_directory_size(p_buffer);
        }
        else
        {
            size += file_properties.size;
        }
    } while (tg_platform_continue_file_iteration(p_directory, h_file_iterator, &file_properties));
    return size;
}

void tg_platform_read_file(const char* p_filename, u32* p_size, char** pp_data)
{
    TG_ASSERT(p_filename && p_size && pp_data);

    char p_filename_buffer[MAX_PATH] = { 0 };
    tg_string_format(sizeof(p_filename_buffer), p_filename_buffer, "%s/%s", tg_assets_get_asset_path(), p_filename);

    HANDLE h_file = CreateFileA(p_filename_buffer, GENERIC_READ | GENERIC_WRITE, 0, TG_NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, TG_NULL);
    TG_ASSERT(h_file != INVALID_HANDLE_VALUE);
    TG_ASSERT(GetLastError() != ERROR_FILE_NOT_FOUND);

    *p_size = GetFileSize(h_file, TG_NULL);

    *pp_data = TG_MEMORY_ALLOC(*p_size);
    u32 number_of_bytes_read = 0;
    ReadFile(h_file, *pp_data, *p_size, &number_of_bytes_read, TG_NULL);

    CloseHandle(h_file);
}



/*------------------------------------------------------------+
| Timer                                                       |
+------------------------------------------------------------*/

typedef struct tg_timer
{
    b32              running;
    LONGLONG         counter_elapsed;
    LARGE_INTEGER    performance_frequency;
    LARGE_INTEGER    start_performance_counter;
    LARGE_INTEGER    end_performance_counter;
} tg_timer;



tg_timer_h tg_platform_timer_create()
{
    tg_timer_h h_timer = TG_MEMORY_ALLOC(sizeof(*h_timer));

    h_timer->running = TG_FALSE;
    QueryPerformanceFrequency(&h_timer->performance_frequency);
    QueryPerformanceCounter(&h_timer->start_performance_counter);
    QueryPerformanceCounter(&h_timer->end_performance_counter);

    return h_timer;
}

void tg_platform_timer_start(tg_timer_h h_timer)
{
    TG_ASSERT(h_timer);

    if (!h_timer->running)
    {
        h_timer->running = TG_TRUE;
        QueryPerformanceCounter(&h_timer->start_performance_counter);
    }
}

void tg_platform_timer_stop(tg_timer_h h_timer)
{
    TG_ASSERT(h_timer);

    if (h_timer->running)
    {
        h_timer->running = TG_FALSE;
        QueryPerformanceCounter(&h_timer->end_performance_counter);
        h_timer->counter_elapsed += h_timer->end_performance_counter.QuadPart - h_timer->start_performance_counter.QuadPart;
    }
}

void tg_platform_timer_reset(tg_timer_h h_timer)
{
    TG_ASSERT(h_timer);

    h_timer->running = TG_FALSE;
    h_timer->counter_elapsed = 0;
    QueryPerformanceCounter(&h_timer->start_performance_counter);
    QueryPerformanceCounter(&h_timer->end_performance_counter);
}

f32  tg_platform_timer_elapsed_milliseconds(tg_timer_h h_timer)
{
    TG_ASSERT(h_timer);

    return (f32)((1000000000LL * h_timer->counter_elapsed) / h_timer->performance_frequency.QuadPart) / 1000000.0f;
}

void tg_platform_timer_destroy(tg_timer_h h_timer)
{
    TG_ASSERT(h_timer);

    TG_MEMORY_FREE(h_timer);
}



/*------------------------------------------------------------+
| Platform                                                    |
+------------------------------------------------------------*/

#ifdef TG_DEBUG
void tg_platform_debug_log(const char* p_message)
{
    OutputDebugStringA(p_message);
    OutputDebugStringA("\n");
}
#endif



void* tg_platform_memory_alloc(u64 size)
{
#ifdef TG_DEBUG
    void* p_memory = HeapAlloc(h_process_heap, HEAP_ZERO_MEMORY, size);
#else
    void* p_memory = HeapAlloc(h_process_heap, 0, size);
#endif
    TG_ASSERT(p_memory);
    return p_memory;
}

void tg_platform_memory_free(void* p_memory)
{
    const BOOL result = HeapFree(h_process_heap, 0, p_memory);
    TG_ASSERT(result);
}

void* tg_platform_memory_realloc(u64 size, void* p_memory)
{
#ifdef TG_DEBUG
    void* p_reallocated_memory = HeapReAlloc(h_process_heap, HEAP_ZERO_MEMORY, p_memory, size);
#else
    void* p_reallocated_memory = HeapReAlloc(h_process_heap, 0, p_memory, size);
#endif
    TG_ASSERT(p_reallocated_memory);
    return p_reallocated_memory;
}



void tg_platform_get_mouse_position(u32* p_x, u32* p_y)
{
    POINT point = { 0 };
    WIN32_CALL(GetCursorPos(&point));
    WIN32_CALL(ScreenToClient(h_window, &point));
    u32 width;
    u32 height;
    tg_platform_get_window_size(&width, &height);
    *p_x = (u32)tgm_i32_clamp(point.x, 0, width - 1);
    *p_y = (u32)tgm_i32_clamp(point.y, 0, height - 1);
}

void tg_platform_get_screen_size(u32* p_width, u32* p_height)
{
    RECT rect;
    WIN32_CALL(GetWindowRect(GetDesktopWindow(), &rect));
    *p_width = rect.right - rect.left;
    *p_height = rect.bottom - rect.top;
}

f32  tg_platform_get_window_aspect_ratio()
{
    u32 width;
    u32 height;
    tg_platform_get_window_size(&width, &height);
    return (f32)width / (f32)height;
}

tg_window_h tg_platform_get_window_handle()
{
    return h_window;
}

void tg_platform_get_window_size(u32* p_width, u32* p_height)
{
    RECT rect;
    WIN32_CALL(GetWindowRect(h_window, &rect));
    *p_width = rect.right - rect.left;
    *p_height = rect.bottom - rect.top;
}

void tg_platform_handle_events()
{
    MSG msg = { 0 };
    while (PeekMessageA(&msg, TG_NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}



i8 tg_platform_system_time_compare(tg_system_time* p_time0, tg_system_time* p_time1)
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



/*------------------------------------------------------------+
| Windows Internals                                           |
+------------------------------------------------------------*/

LRESULT CALLBACK tg_platform_win32_window_proc(HWND h_window, UINT message, WPARAM w_param, LPARAM l_param)
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
        DestroyWindow(h_window);
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
        char buffer[156] = { 0 };
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
    default: return DefWindowProcA(h_window, message, w_param, l_param);
    }
    return 0;
}

int CALLBACK WinMain(_In_ HINSTANCE h_instance, _In_opt_ HINSTANCE h_prev_instance, _In_ LPSTR cmd_line, _In_ int show_cmd)
{
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
    tg_platform_get_screen_size(&w, &h);
    h_window = CreateWindowExA(0, window_class_id, window_title, WS_OVERLAPPEDWINDOW, 0, 0, w, h, TG_NULL, TG_NULL, h_instance, TG_NULL);
    TG_ASSERT(h_window);

    ShowWindow(h_window, show_cmd);
    SetWindowLongPtr(h_window, GWLP_WNDPROC, (LONG_PTR)&tg_platform_win32_window_proc);

    tg_application_start();

#ifdef TG_DEBUG
    const u32 alloc_count = tg_memory_unfreed_allocation_count();
    if (alloc_count != 0)
    {
        tg_list unfreed_allocations_list = tg_memory_create_unfreed_allocations_list();
        char buffer[512] = { 0 };
        TG_DEBUG_LOG("");
        TG_DEBUG_LOG("MEMORY LEAKS:");
        for (u32 i = 0; i < unfreed_allocations_list.count; i++)
        {
            const tg_memory_allocator_allocation* p_allocation = TG_LIST_POINTER_TO(unfreed_allocations_list, i);
            tg_string_format(sizeof(buffer), buffer, "\tFilename: %s, Line: %u", p_allocation->p_filename, p_allocation->line);
            TG_DEBUG_LOG(buffer);
            memset(buffer, 0, sizeof(buffer));
        }
        TG_DEBUG_LOG("");
        tg_list_destroy(&unfreed_allocations_list);
    }
    TG_ASSERT(alloc_count == 0);
#endif
    tg_memory_shutdown();
    return EXIT_SUCCESS;
}
