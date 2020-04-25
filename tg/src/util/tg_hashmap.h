#ifndef TG_HASHMAP_H
#define TG_HASHMAP_H

#include "tg_common.h"



#define TG_HASHMAP_DEFAULT_BUCKET_COUNT              32
#define TG_HASHMAP_DEFAULT_BUCKET_CAPACITY           4


#define TG_HASHMAP_CREATE(key_type, value_type) \
    tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), TG_HASHMAP_DEFAULT_BUCKET_COUNT, TG_HASHMAP_DEFAULT_BUCKET_CAPACITY)

#define TG_HASHMAP_CREATE__BUCKET_CAPACITY(key_type, value_type, bucket_capacity) \
    tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), TG_HASHMAP_DEFAULT_BUCKET_COUNT, bucket_capacity)

#define TG_HASHMAP_CREATE__BUCKET_COUNT(key_type, value_type, bucket_count) \
    tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), bucket_count, TG_HASHMAP_DEFAULT_BUCKET_CAPACITY)

#define TG_HASHMAP_CREATE__BUCKET_COUNT__BUCKET_CAPACITY(key_type, value_type, bucket_count, bucket_capacity) \
    tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), bucket_count, bucket_capacity)



TG_DECLARE_HANDLE(tg_hashmap);
TG_DECLARE_HANDLE(tg_list);



tg_hashmap_h    tg_hashmap_create_impl(u32 key_size, u32 value_size, u32 bucket_count, u32 bucket_capacity);
tg_hashmap_h    tg_hashmap_create_copy(const tg_hashmap_h hashmap_h);
tg_list_h       tg_hashmap_key_list_create(const tg_hashmap_h hashmap_h);
tg_list_h       tg_hashmap_value_list_create(const tg_hashmap_h hashmap_h);
void            tg_hashmap_destroy(tg_hashmap_h hashmap_h);

u32             tg_hashmap_bucket_count(tg_hashmap_h hashmap_h);
b32             tg_hashmap_contains(tg_hashmap_h hashmap_h, const void* p_key);
u32             tg_hashmap_element_count(tg_hashmap_h hashmap_h);
b32             tg_hashmap_empty(tg_hashmap_h hashmap_h);
void            tg_hashmap_insert(tg_hashmap_h hashmap_h, const void* p_key, const void* p_value);
void*           tg_hashmap_pointer_to(tg_hashmap_h hashmap_h, const void* p_key);
void            tg_hashmap_remove(tg_hashmap_h hashmap_h, const void* p_key);
b32             tg_hashmap_try_remove(tg_hashmap_h hashmap_h, const void* p_key);

#endif
