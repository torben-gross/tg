#ifndef TG_HASHMAP_H
#define TG_HASHMAP_H

#include "tg/tg_common.h"



#define TG_HASHMAP_DEFAULT_COUNT              32
#define TG_HASHMAP_DEFAULT_BUCKET_CAPACITY    4

#define tg_hashmap_create(                    \
    key_type, value_type,                     \
    key_hash_fn, key_equals_fn,               \
    p_hashmap_h)                              tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), TG_HASHMAP_DEFAULT_COUNT, TG_HASHMAP_DEFAULT_BUCKET_CAPACITY, key_hash_fn, key_equals_fn, p_hashmap_h)

#define tg_hashmap_create_capacity(           \
    key_type, value_type,                     \
    bucket_capacity,                          \
    key_hash_fn, key_equals_fn,               \
    p_hashmap_h)                              tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), TG_HASHMAP_DEFAULT_COUNT, bucket_capacity, key_hash_fn, key_equals_fn, p_hashmap_h)

#define tg_hashmap_create_count(              \
    key_type, value_type,                     \
    count,                                    \
    key_hash_fn, key_equals_fn,               \
    p_hashmap_h)                              tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), count, TG_HASHMAP_DEFAULT_BUCKET_CAPACITY, key_hash_fn, key_equals_fn, p_hashmap_h)

#define tg_hashmap_create_count_capacity(     \
    key_type, value_type,                     \
    count, bucket_capacity,                   \
    key_hash_fn, key_equals_fn,               \
    p_hashmap_h)                              tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), count, bucket_capacity, key_hash_fn, key_equals_fn, p_hashmap_h)



typedef u32(*tg_hash_fn)(const void* v);
typedef b32(*tg_equals_fn)(const void* v0, const void* v1);

typedef struct tg_hashmap tg_hashmap;
typedef tg_hashmap* tg_hashmap_h;



void    tg_hashmap_create_impl(u32 key_size, u32 value_size, u32 count, u32 bucket_capacity, const tg_hash_fn key_hash_fn, const tg_equals_fn key_equals_fn, tg_hashmap_h* p_hashmap_h);
void    tg_hashmap_destroy(tg_hashmap_h hashmap_h);

u32     tg_hashmap_bucket_count(tg_hashmap_h hashmap_h);
b32     tg_hashmap_contains(tg_hashmap_h hashmap_h, const void* p_key);
void    tg_hashmap_insert(tg_hashmap_h hashmap_h, const void* p_key, const void* p_value);
void*   tg_hashmap_pointer_to(tg_hashmap_h hashmap_h, const void* p_key);
void    tg_hashmap_remove(tg_hashmap_h hashmap_h, const void* p_key);

#endif
