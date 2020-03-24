#include "tg_application.h"

#include "tg/graphics/tg_graphics.h"
#include "tg/platform/tg_platform.h"
#include <stdbool.h>
#include <stdlib.h>

bool running = true;

void tg_application_start()
{
    tg_graphics_init();
    tg_graphics_renderer_3d_init();
    //tg_graphics_renderer_2d_init();
    //tg_image_h img = NULL;
    //tg_graphics_image_create("test_icon.bmp", &img);
    //tg_image_h numbers[9] = { 0 };
    //tg_graphics_image_create("1.bmp", &numbers[0]);
    //tg_graphics_image_create("2.bmp", &numbers[1]);
    //tg_graphics_image_create("3.bmp", &numbers[2]);
    //tg_graphics_image_create("4.bmp", &numbers[3]);
    //tg_graphics_image_create("5.bmp", &numbers[4]);
    //tg_graphics_image_create("6.bmp", &numbers[5]);
    //tg_graphics_image_create("7.bmp", &numbers[6]);
    //tg_graphics_image_create("8.bmp", &numbers[7]);
    //tg_graphics_image_create("9.bmp", &numbers[8]);

    while (running)
    {
        tg_platform_handle_events();
        //tg_graphics_renderer_2d_begin();
        //for (ui32 x = 0; x < 16; x++)
        //{
        //    for (ui32 y = 0; y < 16; y++)
        //    {
        //        ui32 idx = (x * 16 + y) % 9;
        //        tg_graphics_renderer_2d_draw_sprite((f32)x - 3.0f, (f32)y - 10.0f, -1.0f, 1.0f, 1.0f, numbers[idx]);
        //    }
        //}
        //tg_graphics_renderer_2d_draw_sprite(0.0f, 0.0f, 0.0f, 3.0f, 3.0f, img);
        //tg_graphics_renderer_2d_end();
        //tg_graphics_renderer_2d_present();
        tg_graphics_renderer_3d_draw(NULL);
        tg_graphics_renderer_3d_present();
        tg_platform_debug_print_performance();
    }

    //for (ui8 i = 0; i < 9; i++)
    //{
    //    tg_graphics_image_destroy(numbers[i]);
    //}
    //tg_graphics_image_destroy(img);
    //tg_graphics_renderer_2d_shutdown();
    tg_graphics_shutdown();
}

void tg_application_quit()
{
	running = false;
}
