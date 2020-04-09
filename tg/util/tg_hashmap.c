#include "tg/util/tg_hashmap.h"

#include "tg/memory/tg_memory_allocator.h"
#include "tg/util/tg_list.h"



typedef struct tg_hashmap_bucket
{
	tg_list_h    keys;
	tg_list_h    values;
} tg_hashmap_bucket;

typedef struct tg_hashmap
{
	u32                  count;
	u32                  key_size;
	u32                  value_size;
	tg_hash_fn           key_hash_fn;
	tg_equals_fn         key_equals_fn;
	tg_hashmap_bucket    buckets[0];
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

	for (u32 i = 0; i < count; i++)
	{
		tg_list_create_capacity_size(bucket_capacity, key_size, &(**p_hashmap_h).buckets[i].keys);
		tg_list_create_capacity_size(bucket_capacity, value_size, &(**p_hashmap_h).buckets[i].values);
	}
}

void tg_hashmap_destroy(tg_hashmap_h hashmap_h)
{
	TG_ASSERT(hashmap_h);

	for (u32 i = 0; i < hashmap_h->count; i++)
	{
		tg_list_destroy(hashmap_h->buckets[i].keys);
		tg_list_destroy(hashmap_h->buckets[i].values);
	}
	TG_MEMORY_ALLOCATOR_FREE(hashmap_h);
}



u32 tg_hashmap_bucket_count(tg_hashmap_h hashmap_h)
{
	TG_ASSERT(hashmap_h);

	return hashmap_h->count;
}

b32 tg_hashmap_contains(tg_hashmap_h hashmap_h, const void* p_key)
{
	TG_ASSERT(hashmap_h && p_key);

	const u32 hash = hashmap_h->key_hash_fn(p_key);
	const u32 index = hash % hashmap_h->count;

	tg_hashmap_bucket* p_bucket = &hashmap_h->buckets[index];

	for (u32 i = 0; i < tg_list_count(p_bucket->keys); i++)
	{
		const void* p_bucket_entry_key = tg_list_pointer_to(p_bucket->keys, i);
		const b32 equal = hashmap_h->key_equals_fn(p_key, p_bucket_entry_key);
		if (equal)
		{
			return TG_TRUE;
		}
	}
	return TG_FALSE;
}

void tg_hashmap_insert(tg_hashmap_h hashmap_h, const void* p_key, const void* p_value)
{
	TG_ASSERT(hashmap_h && p_key && p_value);

	const u32 hash = hashmap_h->key_hash_fn(p_key);
	const u32 index = hash % hashmap_h->count;

	tg_hashmap_bucket* p_bucket = &hashmap_h->buckets[index];

	for (u32 i = 0; i < tg_list_count(p_bucket->keys); i++)
	{
		const void* p_bucket_entry_key = tg_list_pointer_to(p_bucket->keys, i);
		const b32 equal = hashmap_h->key_equals_fn(p_key, p_bucket_entry_key);
		if (equal)
		{
			tg_list_replace_at(p_bucket->values, i, p_value);
			return;
		}
	}

	tg_list_insert(p_bucket->keys, p_key);
	tg_list_insert(p_bucket->values, p_value);
}

void* tg_hashmap_pointer_to(tg_hashmap_h hashmap_h, const void* p_key)
{
	TG_ASSERT(hashmap_h && p_key);

	const u32 hash = hashmap_h->key_hash_fn(p_key);
	const u32 index = hash % hashmap_h->count;

	tg_hashmap_bucket* p_bucket = &hashmap_h->buckets[index];

	void* p_found_bucket_entry_value = TG_NULL;
	for (u32 i = 0; i < tg_list_count(p_bucket->keys); i++)
	{
		const void* p_bucket_entry_key = tg_list_pointer_to(p_bucket->keys, i);
		const b32 equal = hashmap_h->key_equals_fn(p_key, p_bucket_entry_key);
		if (equal)
		{
			p_found_bucket_entry_value = tg_list_pointer_to(p_bucket->values, i);
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

	tg_hashmap_bucket* p_bucket = &hashmap_h->buckets[index];

	for (u32 i = 0; i < tg_list_count(p_bucket->keys); i++)
	{
		const void* p_bucket_entry_key = tg_list_pointer_to(p_bucket->keys, i);
		const b32 equal = hashmap_h->key_equals_fn(p_key, p_bucket_entry_key);
		if (equal)
		{
			tg_list_remove_at(p_bucket->keys, i);
			tg_list_remove_at(p_bucket->values, i);
			return;
		}
	}

	TG_ASSERT(TG_FALSE);
}
