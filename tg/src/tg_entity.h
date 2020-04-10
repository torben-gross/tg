#ifndef TG_ENTITY_H
#define TG_ENTITY_H

#include "tg_common.h"



TG_DECLARE_HANDLE(tg_entity);
TG_DECLARE_HANDLE(tg_model);



void    tg_entity_create(tg_entity_h* p_entity_h);
void    tg_entity_destroy(tg_entity_h entity_h);

void    tg_entity_set_model(tg_entity_h entity_h, tg_model_h model_h);

#endif
