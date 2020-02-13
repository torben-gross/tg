#ifndef TG_ALLOCATOR_H
#define TG_ALLOCATOR_H

#include "tg_platform.h"
#include "tg/tg_common.h"


#ifdef TG_DEBUG

void* tg_malloc(size_t size);
void tg_free(void* memory);

#else

#include <stdlib.h>

#define tg_malloc(size) malloc(size)
#define tg_free(memory) free(memory);

#endif

#endif
