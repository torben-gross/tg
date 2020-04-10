#ifndef TG_TIMER_H
#define TG_TIMER_H

#include "tg_common.h"



TG_DECLARE_HANDLE(tg_timer);



tg_timer_h    tg_timer_create();
void          tg_timer_start(tg_timer_h timer_h);
void          tg_timer_stop(tg_timer_h timer_h);
void          tg_timer_reset(tg_timer_h timer_h);
f32           tg_timer_elapsed_milliseconds(tg_timer_h timer_h);
void          tg_timer_destroy(tg_timer_h timer_h);

#endif
