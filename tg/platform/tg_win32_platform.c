#define _CRT_SECURE_NO_WARNINGS

#include "tg/tg_application.h"
#include "tg/tg_input.h"
#include "tg/platform/tg_allocator.h"
#include "tg/graphics/tg_graphics.h"
#include <stdio.h>
#include <windows.h>

HWND             window_h = NULL;

#ifdef TG_DEBUG
LARGE_INTEGER    performance_frequency;
LARGE_INTEGER    last_performance_counter;
char             buffer[256];
f32              milliseconds_sum = 0.0f;
ui32             fps = 0;
#endif

#ifdef TG_DEBUG
void tg_platform_debug_print(const char* string)
{
    OutputDebugStringA(string);
    OutputDebugStringA("\n");
}
void tg_platform_debug_print_performance()
{
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
}
#endif

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
void tg_platform_get_window_handle(tg_window_h* p_window_h)
{
    *p_window_h = window_h;
}
void tg_platform_get_window_size(ui32* width, ui32* height)
{
    RECT rect;
    BOOL result = GetWindowRect(window_h, &rect);
    if (result)
    {
        *width = rect.right - rect.left;
        *height = rect.bottom - rect.top;
    }
}
void tg_platform_handle_events()
{
    MSG msg = { 0 };
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

/*
---- Windows Internals ----
*/
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
        tg_graphics_on_window_resize((ui32)LOWORD(l_param), (ui32)HIWORD(l_param));
        //tg_graphics_renderer_2d_on_window_resize((ui32)LOWORD(l_param), (ui32)HIWORD(l_param)); TODO
        //tg_graphics_renderer_3d_on_window_resize((ui32)LOWORD(l_param), (ui32)HIWORD(l_param)); TODO
    } break;



    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    {
        tg_input_on_mouse_button_pressed((tg_button)w_param);
    } break;
    case WM_XBUTTONDOWN:
    {
        tg_input_on_mouse_button_pressed((tg_button)HIWORD(w_param));
    } break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    {
        tg_input_on_mouse_button_released((tg_button)w_param);
    } break;
    case WM_XBUTTONUP:
    {
        tg_input_on_mouse_button_released((tg_button)HIWORD(w_param));
    } break;
    case WM_KEYDOWN:
    {
        tg_input_on_key_pressed((tg_key)w_param);
    } break;
    case WM_KEYUP:
    {
        tg_input_on_key_released((tg_key)w_param);
    } break;
    default:
        return DefWindowProcA(window_h, message, w_param, l_param);
    }
    return 0;
}
int CALLBACK WinMain(_In_ HINSTANCE instance_h, _In_opt_ HINSTANCE prev_instance_h, _In_ LPSTR cmd_line, _In_ int show_cmd)
{
#ifdef TG_DEBUG
    QueryPerformanceFrequency(&performance_frequency);
    QueryPerformanceCounter(&last_performance_counter);
#endif

    const char* window_class_id = "tg";
    const char* window_title = "tg";

    WNDCLASSEXA window_class_info   = { 0 };
    window_class_info.lpszClassName = window_class_id;
    {
        window_class_info.cbSize = sizeof(window_class_info);
        window_class_info.style = 0;
        window_class_info.lpfnWndProc = DefWindowProcA;
        window_class_info.cbClsExtra = 0;
        window_class_info.cbWndExtra = 0;
        window_class_info.hInstance = instance_h;
        window_class_info.hIcon = NULL;
        window_class_info.hCursor = NULL;
        window_class_info.hbrBackground = NULL;
        window_class_info.lpszMenuName = NULL;
        window_class_info.lpszClassName = window_class_id;
        window_class_info.hIconSm = NULL;
    }
    const ATOM atom = RegisterClassExA(&window_class_info);
    TG_ASSERT(atom);

    SetProcessDPIAware();
    ui32 w, h;
    tg_platform_get_screen_size(&w, &h);
    window_h = CreateWindowExA(0, window_class_id, window_title, WS_OVERLAPPEDWINDOW, 0, 0, w, h, NULL, NULL, instance_h, NULL);
    TG_ASSERT(window_h);

    ShowWindow(window_h, show_cmd);
    SetWindowLongPtr(window_h, GWLP_WNDPROC, (LONG_PTR)&tg_platform_win32_window_proc);

    tg_application_start();

    TG_ASSERT(tg_allocator_unfreed_allocation_count() == 0);
    return EXIT_SUCCESS;
}
