#define _CRT_SECURE_NO_WARNINGS

#include "tg/tg_common.h"
#include "tg_allocator.h"
#include "tg/graphics/tg_graphics.h"
#include <stdbool.h>
#include <stdio.h>
#include <windows.h>

bool running = true;
HWND window_handle = NULL;

void* tg_platform_get_window_handle()
{
    return window_handle;
}

void tg_platform_get_screen_size(ui32* width, ui32* height)
{
    RECT rect;
    BOOL result = GetWindowRect(GetDesktopWindow(), &rect);
    if (result)
    {
        *width = rect.right - rect.left;
        *height = rect.bottom - rect.top;
    }
}

void tg_platform_get_window_size(ui32* width, ui32* height)
{
    RECT rect;
    BOOL result = GetWindowRect(window_handle, &rect);
    if (result)
    {
        *width = rect.right - rect.left;
        *height = rect.bottom - rect.top;
    }
}

#ifdef TG_DEBUG
void tg_platform_print(const char* string)
{
    OutputDebugStringA(string);
    OutputDebugStringA("\n");
}
#endif

LRESULT CALLBACK tg_win32_platform_window_proc(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param)
{
    switch (message)
    {
    case WM_CLOSE:
    case WM_DESTROY:
    {
        running = false;
        PostQuitMessage(EXIT_SUCCESS);
    } break;
    case WM_KEYDOWN:
    {
        char c = (char)w_param;
        c = c;
    } break;
    case WM_KEYUP:
    {
        char c = (char)w_param;
        c = c;
    } break;
    case WM_QUIT:
    {
        DestroyWindow(window_handle);
    } break;
    case WM_SIZE:
    {
        tg_graphics_on_window_resize((ui)LOWORD(l_param), (ui)HIWORD(l_param));
    } break;
    default:
        return DefWindowProcA(window_handle, message, w_param, l_param);
    }
    return 0;
}

int CALLBACK WinMain(
    _In_     HINSTANCE instance_handle,
    _In_opt_ HINSTANCE prev_instance_handle,
    _In_     LPSTR     cmd_line,
    _In_     int       show_cmd
)
{
    const char* window_class_id     = "tg";
    const char* window_title        = "tg";

    WNDCLASSEXA window_class_info   = { 0 };
    window_class_info.cbSize        = sizeof(window_class_info);
    window_class_info.lpfnWndProc   = tg_win32_platform_window_proc;
    window_class_info.hInstance     = instance_handle;
    window_class_info.lpszClassName = window_class_id;

    const ATOM atom = RegisterClassExA(&window_class_info);
    TG_ASSERT(atom);

    SetProcessDPIAware();
    ui32 w, h;
    tg_platform_get_screen_size(&w, &h);
    window_handle = CreateWindowExA(
        0, window_class_id, window_title, WS_OVERLAPPEDWINDOW,
        0, 0, w, h,
        NULL, NULL, instance_handle, NULL
    );
    TG_ASSERT(window_handle);

    ShowWindow(window_handle, show_cmd);
    UpdateWindow(window_handle);

    tg_graphics_init();
    tg_graphics_renderer_2d_init();
    tg_image_h img = NULL;
    tg_graphics_image_create("test_icon.bmp", &img);

    LARGE_INTEGER performance_frequency;
    QueryPerformanceFrequency(&performance_frequency);

    LARGE_INTEGER last_performance_counter;
    QueryPerformanceCounter(&last_performance_counter);

    MSG msg = { 0 };
#ifdef TG_DEBUG
    char buffer[256];
    f32 milliseconds_sum = 0.0f;
    ui32 fps = 0;
#endif
    while (running)
    {
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        //tg_graphics_render();
        tg_graphics_renderer_2d_begin();
        tg_graphics_renderer_2d_draw_sprite(0.0f, 0.0f, -1.0f, 1.0f, 1.0f, img);
        tg_graphics_renderer_2d_draw_sprite(0.0f, 0.0f,  0.0f, 1.0f, 1.0f, img);
        tg_graphics_renderer_2d_end();

#ifdef TG_DEBUG
        LARGE_INTEGER end_performance_counter;
        QueryPerformanceCounter(&end_performance_counter);
        const LONGLONG counter_elapsed = end_performance_counter.QuadPart - last_performance_counter.QuadPart;
        last_performance_counter = end_performance_counter;
        const f32 delta_ms = (f32)((1000000000LL * counter_elapsed) / performance_frequency.QuadPart) / 1000000.0f;

        milliseconds_sum += delta_ms;
        fps++;
        if (milliseconds_sum > 1000.0f)
        {
            snprintf(buffer, sizeof(buffer), "%f ms\n", milliseconds_sum / fps);
            OutputDebugStringA(buffer);

            snprintf(buffer, sizeof(buffer), "%lu fps\n", fps);
            OutputDebugStringA(buffer);

            milliseconds_sum = 0.0f;
            fps = 0;
        }
#endif
    }

    tg_graphics_image_destroy(img);
    tg_graphics_renderer_2d_shutdown();
    tg_graphics_shutdown();
    TG_ASSERT(tg_allocator_unfreed_allocation_count() == 0);
    return (int)msg.wParam;
}
