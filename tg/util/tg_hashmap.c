#include "tg/util/tg_hashmap.h"

#include "tg/memory/tg_memory_allocator.h"
#include "tg/util/tg_list.h"
#include <string.h> // TODO: implement memset yourself



typedef struct tg_hashmap
{
	u32             count;
	u32             key_size;
	u32             value_size;
	tg_hash_fn      key_hash_fn;
	tg_equals_fn    key_equals_fn;
	u8*             p_bucket_entry_buffer;
	tg_list_h       buckets[0];
} tg_hashmap;



void tg_hashmap_create_impl(u32 key_size, u32 value_size, u32 count, u32 bucket_capacity, const tg_hash_fn key_hash_fn, const tg_equals_fn key_equals_fn, tg_hashmap_h* p_hashmap_h)
{
	TG_ASSERT(key_size && value_size && count && bucket_capacity && key_hash_fn && key_equals_fn && p_hashmap_h);

	*p_hashmap_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(**p_hashmap_h) + count * sizeof(*(**p_hashmap_h).buckets));
	
	(**p_hashmap_h).count = count;
	(**p_hashmap_h).key_size = key_size;
	(**p_hashmap_h).value_size = value_size;
	(**p_hashmap_h).key_hash_fn = key_hash_fn;
	(**p_hashmap_h).key_equals_fn = key_equals_fn;

	const u32 bucket_entry_size = key_size + value_size;
	(**p_hashmap_h).p_bucket_entry_buffer = TG_MEMORY_ALLOCATOR_ALLOCATE(bucket_entry_size);
	for (u32 i = 0; i < count; i++)
	{
		tg_list_create_capacity_size(bucket_capacity, bucket_entry_size, &(**p_hashmap_h).buckets[i]);
	}
}

void tg_hashmap_insert(tg_hashmap_h hashmap_h, const void* p_key, const void* p_value)
{
	TG_ASSERT(hashmap_h && p_key && p_value);

	const u32 hash = hashmap_h->key_hash_fn(p_key);
	const u32 index = hash % hashmap_h->count;

	memcpy(hashmap_h->p_bucket_entry_buffer, p_key, hashmap_h->key_size);
	memcpy(&hashmap_h->p_bucket_entry_buffer[hashmap_h->key_size], p_value, hashmap_h->value_size);

	tg_list_insert(hashmap_h->buckets[index], hashmap_h->p_bucket_entry_buffer);

#ifdef TG_DEBUG
	memset(hashmap_h->p_bucket_entry_buffer, 0, (u64)hashmap_h->key_size + (u64)hashmap_h->value_size);
#endif
}

void* tg_hashmap_at(tg_hashmap_h hashmap_h, const void* p_key)
{
	TG_ASSERT(hashmap_h && p_key);

	const u32 hash = hashmap_h->key_hash_fn(p_key);
	const u32 index = hash % hashmap_h->count;

	tg_list_h bucket = hashmap_h->buckets[index];
	void* p_found_bucket_entry_value = TG_NULL;

	for (u32 i = 0; i < tg_list_count(bucket); i++)
	{
		u8* p_bucket_entry = (u8*)tg_list_at(bucket, i);
		const void* p_bucket_entry_key = p_bucket_entry;
		const b32 equal = hashmap_h->key_equals_fn(p_key, p_bucket_entry_key);
		if (equal)
		{
			void* p_bucket_entry_value = &p_bucket_entry[hashmap_h->key_size];
			p_found_bucket_entry_value = p_bucket_entry_value;
			break;
		}
	}

	TG_ASSERT(p_found_bucket_entry_value);

	return p_found_bucket_entry_value;
}

void tg_hashmap_remove(tg_hashmap_h hashmap_h, const void* p_key)
{
	TG_ASSERT(hashmap_h && p_key);

	const u32 hash = hashmap_h->key_hash_fn(p_key);
	const u32 index = hash % hashmap_h->count;

	tg_list_h bucket = hashmap_h->buckets[index];
	for (u32 i = 0; i < tg_list_count(bucket); i++)
	{
		u8* p_bucket_entry = (u8*)tg_list_at(bucket, i);
		const void* p_bucket_entry_key = p_bucket_entry;
		const b32 equal = hashmap_h->key_equals_fn(p_key, p_bucket_entry_key);
		if (equal)
		{
			tg_list_remove_at(bucket, i);
			return;
		}
	}

	TG_ASSERT(TG_FALSE);
}

void tg_hashmap_destroy(tg_hashmap_h hashmap_h)
{
	TG_ASSERT(hashmap_h);

	TG_MEMORY_ALLOCATOR_FREE(hashmap_h->p_bucket_entry_buffer);
	TG_MEMORY_ALLOCATOR_FREE(hashmap_h);
}
