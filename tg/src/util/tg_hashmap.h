#ifndef TG_HASHMAP_H
#define TG_HASHMAP_H

#include "tg_common.h"
#include "util/tg_list.h"



#define TG_HASHMAP_DEFAULT_BUCKET_COUNT              31
#define TG_HASHMAP_DEFAULT_BUCKET_CAPACITY           4


#define TG_HASHMAP_CREATE(key_type, value_type) \
    tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), TG_HASHMAP_DEFAULT_BUCKET_COUNT, TG_HASHMAP_DEFAULT_BUCKET_CAPACITY)

#define TG_HASHMAP_CREATE__BUCKET_CAPACITY(key_type, value_type, bucket_capacity) \
    tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), TG_HASHMAP_DEFAULT_BUCKET_COUNT, bucket_capacity)

#define TG_HASHMAP_CREATE__BUCKET_COUNT(key_type, value_type, bucket_count) \
    tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), bucket_count, TG_HASHMAP_DEFAULT_BUCKET_CAPACITY)

#define TG_HASHMAP_CREATE__BUCKET_COUNT__BUCKET_CAPACITY(key_type, value_type, bucket_count, bucket_capacity) \
    tg_hashmap_create_impl(sizeof(key_type), sizeof(value_type), bucket_count, bucket_capacity)



typedef struct tg_hashmap_bucket
{
	tg_list    keys;
	tg_list    values;
} tg_hashmap_bucket;

typedef struct tg_hashmap
{
	u32                   bucket_count;
	u32                   element_count;
	u32                   key_size;
	u32                   value_size;
	tg_hashmap_bucket*    buckets;
} tg_hashmap;



tg_hashmap    tg_hashmap_create_impl(u32 key_size, u32 value_size, u32 bucket_count, u32 bucket_capacity);
tg_hashmap    tg_hashmap_create_copy(const tg_hashmap* p_hashmap);
tg_list       tg_hashmap_key_list_create(const tg_hashmap* p_hashmap);
tg_list       tg_hashmap_value_list_create(const tg_hashmap* p_hashmap);
void          tg_hashmap_destroy(tg_hashmap* p_hashmap);

u32           tg_hashmap_bucket_count(tg_hashmap* p_hashmap);
b32           tg_hashmap_contains(tg_hashmap* p_hashmap, const void* p_key);
u32           tg_hashmap_element_count(tg_hashmap* p_hashmap);
b32           tg_hashmap_empty(tg_hashmap* p_hashmap);
void          tg_hashmap_insert(tg_hashmap* p_hashmap, const void* p_key, const void* p_value);
void*         tg_hashmap_pointer_to(tg_hashmap* p_hashmap, const void* p_key);
void          tg_hashmap_remove(tg_hashmap* p_hashmap, const void* p_key);
b32           tg_hashmap_try_remove(tg_hashmap* p_hashmap, const void* p_key);

#endif
