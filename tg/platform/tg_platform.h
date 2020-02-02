#ifndef TG_PLATFORM_H
#define TG_PLATFORM_H

void* tg_platform_get_window_handle();
void tg_platform_get_screen_size(uint32* width, uint32* height);
void tg_platform_get_window_size(uint32* width, uint32* height);
void tg_platform_print(const char* string);

#endif
