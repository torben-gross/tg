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

void           tg_application_start();
void           tg_application_quit();
const char*    tg_application_get_asset_path();

#endif