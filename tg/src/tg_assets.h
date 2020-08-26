#ifndef TG_ASSETS_H
#define TG_ASSETS_H

#include "graphics/tg_graphics.h"

void           tg_assets_init();
void           tg_assets_shutdown();
tg_handle      tg_assets_get_asset(const char* p_filename);

#endif
