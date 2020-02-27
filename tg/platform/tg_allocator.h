#ifndef TG_ALLOCATOR_H
#define TG_ALLOCATOR_H

#include "tg_platform.h"
#include "tg/tg_common.h"

#ifdef TG_DEBUG

void* tg_malloc(ui64 size);
void tg_free(void* memory);

#else

#include <stdlib.h>

#define tg_malloc(size) malloc((size_t)size)
#define tg_free(memory) free(memory);

#endif

#endif
