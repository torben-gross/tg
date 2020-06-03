#include "platform/tg_platform.h"

#define _CRT_SECURE_NO_WARNINGS

#include "graphics/tg_graphics.h"
#include "memory/tg_memory.h"
#include "tg_application.h"
#include "tg_input.h"
#include "util/tg_list.h"
#include "util/tg_string.h"
#include <windows.h>

#ifdef TG_DEBUG
#define WIN32_CALL(x) TG_ASSERT(x)
#else
#define WIN32_CALL(x) x
#endif

HWND    window_h = TG_NULL;



/*------------------------------------------------------------+
| File IO                                                     |
+------------------------------------------------------------*/

b32 tg_platform_file_exists(const char* p_filename)
{
    TG_ASSERT(p_filename);

    char p_path[MAX_PATH] = { 0 };
    tg_string_format(sizeof(p_path), p_path, "%s/%s", tg_application_get_asset_path(), p_filename);

    const u32 result = GetFileAttributesA(p_path) != INVALID_FILE_ATTRIBUTES;
    return result;
}

void tg_platform_free_file(char* p_data)
{
    TG_ASSERT(p_data);

    TG_MEMORY_FREE(p_data);
}

void tg_platform_read_file(const char* p_filename, u32* p_size, char** pp_data)
{
    TG_ASSERT(p_filename && p_size && pp_data);

    char p_path[MAX_PATH] = { 0 };
    tg_string_format(sizeof(p_path), p_path, "%s/%s", tg_application_get_asset_path(), p_filename);

    HANDLE file_handle = CreateFileA(p_path, GENERIC_READ | GENERIC_WRITE, 0, TG_NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, TG_NULL);
    TG_ASSERT(GetLastError() != ERROR_FILE_NOT_FOUND);

    *p_size = GetFileSize(file_handle, TG_NULL);

    *pp_data = TG_MEMORY_ALLOC(*p_size);
    u32 number_of_bytes_read = 0;
    ReadFile(file_handle, *pp_data, *p_size, &number_of_bytes_read, TG_NULL);

    CloseHandle(file_handle);
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
    tg_timer_h timer_h = TG_MEMORY_ALLOC(sizeof(*timer_h));

    timer_h->running = TG_FALSE;
    QueryPerformanceFrequency(&timer_h->performance_frequency);
    QueryPerformanceCounter(&timer_h->start_performance_counter);
    QueryPerformanceCounter(&timer_h->end_performance_counter);

    return timer_h;
}

void tg_platform_timer_start(tg_timer_h timer_h)
{
    TG_ASSERT(timer_h);

    if (!timer_h->running)
    {
        timer_h->running = TG_TRUE;
        QueryPerformanceCounter(&timer_h->start_performance_counter);
    }
}

void tg_platform_timer_stop(tg_timer_h timer_h)
{
    TG_ASSERT(timer_h);

    if (timer_h->running)
    {
        timer_h->running = TG_FALSE;
        QueryPerformanceCounter(&timer_h->end_performance_counter);
        timer_h->counter_elapsed += timer_h->end_performance_counter.QuadPart - timer_h->start_performance_counter.QuadPart;
    }
}

void tg_platform_timer_reset(tg_timer_h timer_h)
{
    TG_ASSERT(timer_h);

    timer_h->running = TG_FALSE;
    timer_h->counter_elapsed = 0;
    QueryPerformanceCounter(&timer_h->start_performance_counter);
    QueryPerformanceCounter(&timer_h->end_performance_counter);
}

f32  tg_platform_timer_elapsed_milliseconds(tg_timer_h timer_h)
{
    TG_ASSERT(timer_h);

    return (f32)((1000000000LL * timer_h->counter_elapsed) / timer_h->performance_frequency.QuadPart) / 1000000.0f;
}

void tg_platform_timer_destroy(tg_timer_h timer_h)
{
    TG_ASSERT(timer_h);

    TG_MEMORY_FREE(timer_h);
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

void tg_platform_get_mouse_position(u32* p_x, u32* p_y)
{
    POINT point = { 0 };
    WIN32_CALL(GetCursorPos(&point));
    WIN32_CALL(ScreenToClient(window_h, &point));
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
    return window_h;
}

void tg_platform_get_window_size(u32* p_width, u32* p_height)
{
    RECT rect;
    WIN32_CALL(GetWindowRect(window_h, &rect));
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



/*------------------------------------------------------------+
| Windows Internals                                           |
+------------------------------------------------------------*/

LRESULT CALLBACK tg_platform_win32_window_proc(HWND window_h, UINT message, WPARAM w_param, LPARAM l_param)
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
        DestroyWindow(window_h);
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
    default: return DefWindowProcA(window_h, message, w_param, l_param);
    }
    return 0;
}


// TODO: use this for shader compilation
void print_all_files(const char* dir)
{
    char szDir[MAX_PATH] = { 0 };

    u32 dir_length = tg_string_length(dir);
    TG_ASSERT(dir_length <= MAX_PATH - 3);

    tg_string_format(dir_length, szDir, "%s%s", dir, "\\*");

    WIN32_FIND_DATAA ffd;
    HANDLE hFind = hFind = FindFirstFileA(szDir, &ffd);
    TG_ASSERT(hFind != INVALID_HANDLE_VALUE);

    LARGE_INTEGER filesize;
    do
    {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            TG_DEBUG_LOG(ffd.cFileName);
            if (!(ffd.cFileName[0] == '.' && (ffd.cFileName[1] == '\0' || ffd.cFileName[1] == '.')))
            {
                char dir_buffer[MAX_PATH] = { 0 };
                tg_string_format(MAX_PATH, dir_buffer, "%s\\%s", dir, ffd.cFileName);
                print_all_files(dir_buffer);
            }
        }
        else
        {
            TG_DEBUG_LOG(ffd.cFileName);
            filesize.LowPart = ffd.nFileSizeLow;
            filesize.HighPart = ffd.nFileSizeHigh;
            //_tprintf(TEXT("  %s   %ld bytes\n"), ffd.cFileName, filesize.QuadPart);
        }
    } while (FindNextFileA(hFind, &ffd) != 0);

    TG_ASSERT(GetLastError() == ERROR_NO_MORE_FILES);

    FindClose(hFind);
}

int CALLBACK WinMain(_In_ HINSTANCE instance_h, _In_opt_ HINSTANCE prev_instance_h, _In_ LPSTR cmd_line, _In_ int show_cmd)
{
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
    window_class_info.hInstance = instance_h;
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
    window_h = CreateWindowExA(0, window_class_id, window_title, WS_OVERLAPPEDWINDOW, 0, 0, w, h, TG_NULL, TG_NULL, instance_h, TG_NULL);
    TG_ASSERT(window_h);

    ShowWindow(window_h, show_cmd);
    SetWindowLongPtr(window_h, GWLP_WNDPROC, (LONG_PTR)&tg_platform_win32_window_proc);





    print_all_files("assets");





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
