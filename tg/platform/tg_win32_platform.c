#include "tg/platform/tg_platform.h"

#define _CRT_SECURE_NO_WARNINGS

#include "tg/graphics/tg_graphics.h"
#include "tg/memory/tg_allocator.h"
#include "tg/tg_application.h"
#include "tg/tg_input.h"
#include "tg/util/tg_timer.h"
#include <windows.h>

#ifdef TG_DEBUG
#define WIN32_CALL(x) TG_ASSERT(x)
#else
#define WIN32_CALL(x) x
#endif

HWND    window_h = NULL;


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

void tg_timer_create(tg_timer_h* p_timer_h)
{
    TG_ASSERT(p_timer_h);

    *p_timer_h = TG_ALLOCATOR_ALLOCATE(sizeof(**p_timer_h));
    (**p_timer_h).running = TG_FALSE;
    QueryPerformanceFrequency(&(**p_timer_h).performance_frequency);
    QueryPerformanceCounter(&(**p_timer_h).start_performance_counter);
    QueryPerformanceCounter(&(**p_timer_h).end_performance_counter);
}
void tg_timer_start(tg_timer_h timer_h)
{
    TG_ASSERT(timer_h);

    if (!timer_h->running)
    {
        timer_h->running = TG_TRUE;
        QueryPerformanceCounter(&timer_h->start_performance_counter);
    }
}
void tg_timer_stop(tg_timer_h timer_h)
{
    TG_ASSERT(timer_h);

    if (timer_h->running)
    {
        timer_h->running = TG_FALSE;
        QueryPerformanceCounter(&timer_h->end_performance_counter);
        timer_h->counter_elapsed += timer_h->end_performance_counter.QuadPart - timer_h->start_performance_counter.QuadPart;
    }
}
void tg_timer_reset(tg_timer_h timer_h)
{
    TG_ASSERT(timer_h);

    timer_h->running = TG_FALSE;
    timer_h->counter_elapsed = 0;
    QueryPerformanceCounter(&timer_h->start_performance_counter);
    QueryPerformanceCounter(&timer_h->end_performance_counter);
}
f32  tg_timer_elapsed_milliseconds(tg_timer_h timer_h)
{
    TG_ASSERT(timer_h);

    return (f32)((1000000000LL * timer_h->counter_elapsed) / timer_h->performance_frequency.QuadPart) / 1000000.0f;
}
void tg_timer_destroy(tg_timer_h timer_h)
{
    TG_ASSERT(timer_h);

    TG_ALLOCATOR_FREE(timer_h);
}



/*------------------------------------------------------------+
| Platform                                                    |
+------------------------------------------------------------*/

#ifdef TG_DEBUG
void tg_platform_debug_print(const char* p_message)
{
    OutputDebugStringA(p_message);
    OutputDebugStringA("\n");
}
#endif
void tg_platform_get_mouse_position(ui32* p_x, ui32* p_y)
{
    POINT point = { 0 };
    WIN32_CALL(GetCursorPos(&point));
    WIN32_CALL(ScreenToClient(window_h, &point));
    ui32 width;
    ui32 height;
    tg_platform_get_window_size(&width, &height);
    *p_x = (ui32)tgm_i32_clamp(point.x, 0, width - 1);
    *p_y = (ui32)tgm_i32_clamp(point.y, 0, height - 1);
}
void tg_platform_get_screen_size(ui32* p_width, ui32* p_height)
{
    RECT rect;
    WIN32_CALL(GetWindowRect(GetDesktopWindow(), &rect));
    *p_width = rect.right - rect.left;
    *p_height = rect.bottom - rect.top;
}
f32  tg_platform_get_window_aspect_ratio()
{
    ui32 width;
    ui32 height;
    tg_platform_get_window_size(&width, &height);
    return (f32)width / (f32)height;
}
void tg_platform_get_window_handle(tg_window_h* p_window_h)
{
    *p_window_h = window_h;
}
void tg_platform_get_window_size(ui32* p_width, ui32* p_height)
{
    RECT rect;
    WIN32_CALL(GetWindowRect(window_h, &rect));
    *p_width = rect.right - rect.left;
    *p_height = rect.bottom - rect.top;
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
        tg_graphics_on_window_resize((ui32)LOWORD(l_param), (ui32)HIWORD(l_param));
        //tg_graphics_renderer_2d_on_window_resize((ui32)LOWORD(l_param), (ui32)HIWORD(l_param)); TODO
        //tg_graphics_renderer_3d_on_window_resize((ui32)LOWORD(l_param), (ui32)HIWORD(l_param)); TODO
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
        const ui32 additional_key_repeat_count = (ui32)(0xffffULL & l_param);
        tg_input_on_key_pressed(key, repeated, additional_key_repeat_count);
    } break;
    case WM_KEYUP: tg_input_on_key_released((tg_key)w_param);                           break;
    default: return DefWindowProcA(window_h, message, w_param, l_param);
    }
    return 0;
}
int CALLBACK WinMain(_In_ HINSTANCE instance_h, _In_opt_ HINSTANCE prev_instance_h, _In_ LPSTR cmd_line, _In_ int show_cmd)
{
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
        window_class_info.hCursor = LoadCursor(NULL, IDC_ARROW);
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
