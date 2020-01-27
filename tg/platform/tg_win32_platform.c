#define _CRT_SECURE_NO_WARNINGS

#include "tg/tg_common.h"
#include "tg/graphics/tg_vulkan.h"
#include <stdbool.h>
#include <stdio.h>
#include <windows.h>
#include <sys/timeb.h>


bool running = true;
HWND handle_window = NULL;

void* tg_platform_get_window_handle()
{
    return handle_window;
}

void tg_platform_print(const char* string)
{
    OutputDebugStringA(string);
}

void tg_platform_screen_size(uint32* width, uint32* height)
{
    RECT rect;
    GetWindowRect(GetDesktopWindow(), &rect);
    *width = rect.right - rect.left;
    *height = rect.bottom - rect.top;
}

LRESULT CALLBACK tg_win32_platform_window_proc(
    HWND   window_handle,
    UINT   message,
    WPARAM w_param,
    LPARAM l_param
)
{
    switch (message)
    {
    case WM_CLOSE:
    case WM_QUIT:
        DestroyWindow(window_handle);
        break;
    case WM_DESTROY:
        running = false;
        PostQuitMessage(EXIT_SUCCESS);
        break;
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
    default:
        return DefWindowProcA(window_handle, message, w_param, l_param);
    }
    return 0;
}

int CALLBACK WinMain(
    _In_     HINSTANCE handle_instance,
    _In_opt_ HINSTANCE handle_prev_instance,
    _In_     LPSTR     cmd_line,
    _In_     int       show_cmd
)
{
    // initialize window
    const char* window_class_id     = "win32_platform";
    const char* window_title        = "This is a window title!";

    WNDCLASSEXA window_class_info   = { 0 };
    window_class_info.cbSize        = sizeof(window_class_info);
    window_class_info.lpfnWndProc   = tg_win32_platform_window_proc;
    window_class_info.hInstance     = handle_instance;
    window_class_info.lpszClassName = window_class_id;

    if (!RegisterClassExA(&window_class_info))
    {
        DebugBreak();
        return EXIT_FAILURE;
    }

    uint32 w, h;
    tg_platform_screen_size(&w, &h);
    handle_window = CreateWindowExA(
        0, window_class_id, window_title, WS_OVERLAPPEDWINDOW,
        0, 0, w, h,
        NULL, NULL, handle_instance, NULL
    );

    if (handle_window == NULL)
    {
        DebugBreak();
        return EXIT_FAILURE;
    }

    ShowWindow(handle_window, show_cmd);
    UpdateWindow(handle_window);

    // initialize vulkan
    tg_vulkan_init();

    // game loop
    char buf[256];
    struct timeb start, end;
    uint64 fps = 0;
    ftime(&start);
    MSG msg = { 0 };
    while (running)
    {
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE));
        ftime(&end);
        float delta_time = (1000.0f * (end.time - start.time) + (end.millitm - start.millitm));
        if (delta_time > 1000.0f)
        {
            sprintf(buf, "ms:  %f ms\n", delta_time / (float)fps);
            OutputDebugStringA(buf);
            sprintf(buf, "FPS: %llu\n", fps);
            OutputDebugStringA(buf);
            start = end;
            fps = 0;
        }
        fps++;

        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    // shutdown
    tg_vulkan_shutdown();

    return (int)msg.wParam;
}

void test()
{

}
