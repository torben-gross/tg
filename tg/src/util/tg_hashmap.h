#ifndef TG_HASHMAP_H
#define TG_HASHMAP_H

#include "tg_common.h"



#define TG_HASHMAP_DEFAULT_BUCKET_COUNT              32
#define TG_HASHMAP_DEFAULT_BUCKET_CAPACITY           4


#define tg_hashmap_create(key_type, value_type) \
    tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), tg_##key_type##_hash_fn, tg_##key_type##_equals_fn, TG_HASHMAP_DEFAULT_BUCKET_COUNT, TG_HASHMAP_DEFAULT_BUCKET_CAPACITY)

#define tg_hashmap_create__bucket_capacity(key_type, value_type, bucket_capacity) \
    tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), tg_##key_type##_hash_fn, tg_##key_type##_equals_fn, TG_HASHMAP_DEFAULT_BUCKET_COUNT, bucket_capacity)

#define tg_hashmap_create__bucket_count(key_type, value_type, bucket_count) \
    tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), tg_##key_type##_hash_fn, tg_##key_type##_equals_fn, bucket_count, TG_HASHMAP_DEFAULT_BUCKET_CAPACITY)

#define tg_hashmap_create__bucket_count__bucket_capacity(key_type, value_type, bucket_count, bucket_capacity) \
    tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), tg_##key_type##_hash_fn, tg_##key_type##_equals_fn, bucket_count, bucket_capacity)


#define tg_hashmap_create__custom_key(key_type, value_type, key_hash_fn, key_equals_fn) \
    tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), key_hash_fn, key_equals_fn, TG_HASHMAP_DEFAULT_BUCKET_COUNT, TG_HASHMAP_DEFAULT_BUCKET_CAPACITY)

#define tg_hashmap_create__custom_key__capacity(key_type, value_type, key_hash_fn, key_equals_fn, bucket_capacity) \
    tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), key_hash_fn, key_equals_fn, TG_HASHMAP_DEFAULT_BUCKET_COUNT, bucket_capacity)

#define tg_hashmap_create__custom_key__count(key_type, value_type, key_hash_fn, key_equals_fn, bucket_count) \
    tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), key_hash_fn, key_equals_fn, bucket_count, TG_HASHMAP_DEFAULT_BUCKET_CAPACITY)

#define tg_hashmap_create__custom_key__count__capacity(key_type, value_type, key_hash_fn, key_equals_fn, bucket_count, bucket_capacity) \
    tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), key_hash_fn, key_equals_fn, bucket_count, bucket_capacity)



TG_DECLARE_HANDLE(tg_hashmap);
TG_DECLARE_HANDLE(tg_list);

typedef u32(*tg_hash_fn)(const void* p_v);
typedef b32(*tg_equals_fn)(const void* p_v0, const void* p_v1);



b32             tg_f32_equals_fn(const void* p_v0, const void* p_v1);
b32             tg_f64_equals_fn(const void* p_v0, const void* p_v1);
b32             tg_i8_equals_fn(const void* p_v0, const void* p_v1);
b32             tg_i16_equals_fn(const void* p_v0, const void* p_v1);
b32             tg_i32_equals_fn(const void* p_v0, const void* p_v1);
b32             tg_i64_equals_fn(const void* p_v0, const void* p_v1);
b32             tg_u8_equals_fn(const void* p_v0, const void* p_v1);
b32             tg_u16_equals_fn(const void* p_v0, const void* p_v1);
b32             tg_u32_equals_fn(const void* p_v0, const void* p_v1);
b32             tg_u64_equals_fn(const void* p_v0, const void* p_v1);

u32             tg_f32_hash_fn(const void* p_v);
u32             tg_f64_hash_fn(const void* p_v);
u32             tg_i8_hash_fn(const void* p_v);
u32             tg_i16_hash_fn(const void* p_v);
u32             tg_i32_hash_fn(const void* p_v);
u32             tg_i64_hash_fn(const void* p_v);
u32             tg_u8_hash_fn(const void* p_v);
u32             tg_u16_hash_fn(const void* p_v);
u32             tg_u32_hash_fn(const void* p_v);
u32             tg_u64_hash_fn(const void* p_v);



tg_hashmap_h    tg_hashmap_create_impl(u32 key_size, u32 value_size, const tg_hash_fn key_hash_fn, const tg_equals_fn key_equals_fn, u32 bucket_count, u32 bucket_capacity);
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
