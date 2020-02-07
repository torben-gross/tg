#ifndef TG_ALLOCATOR_H
#define TG_ALLOCATOR_H

#include "tg_platform.h"
#include "tg/tg_common.h"

#include <stdlib.h>

#ifdef TG_DEBUG
void* glob_alloc(size_t size);
#else
#define glob_alloc(size) malloc(size)
#endif

#endif
