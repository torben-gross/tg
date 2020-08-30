/*
        ______
       /_____/\
       \     \ \
        \     \ \
         \_____\ \
         /     /  \
        /     / /\ \
       /     / /  \ \
      /     / /____\/
     /     / /   ______
    /_____/ /   /     /\
    \_____\/   /     /  \
              /     / /\ \
             /     / /  \ \
            /     / /____\/
           /     / /___
          /_____/ /___/\
          \     \ \   \ \
           \     \ \  / /
            \     \ \/ /
             \     \  /
              \_____\/
*/

#ifndef TG_APPLICATION_H
#define TG_APPLICATION_H

#include "tg_common.h"

void           tg_application_start(void);
void           tg_application_on_window_resize(u32 width, u32 height);
void           tg_application_quit(void);

#endif
