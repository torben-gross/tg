#ifndef TG_HASHMAP_H
#define TG_HASHMAP_H

#include "tg/tg_common.h"



#define TG_HASHMAP_DEFAULT_SIZE 32



typedef u32(*tg_hash_fn)(const void*);

typedef struct tg_hashmap tg_hashmap;
typedef tg_hashmap* tg_hashmap_h;



void    tg_hashmap_create(u32 size, const tg_hash_fn key_hash_fn, tg_hashmap_h* p_hashmap_h);
void    tg_hashmap_insert(tg_hashmap_h hashmap_h, const void* key, const void* value);
void*   tg_hashmap_remove(tg_hashmap_h hashmap_h, const void* key);
void    tg_hashmap_destroy(tg_hashmap_h hashmap_h);

#endif
