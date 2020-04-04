#ifndef TG_TIMER_H
#define TG_TIMER_H

#include "tg/tg_common.h"



typedef struct tg_timer tg_timer;
typedef tg_timer* tg_timer_h;



void    tg_timer_create(tg_timer_h* p_timer_h);
void    tg_timer_start(tg_timer_h timer_h);
void    tg_timer_stop(tg_timer_h timer_h);
void    tg_timer_reset(tg_timer_h timer_h);
f32     tg_timer_elapsed_milliseconds(tg_timer_h timer_h);
void    tg_timer_destroy(tg_timer_h timer_h);

#endif
