#include "util/tg_hashmap.h"

#include "memory/tg_memory_allocator.h"
#include "util/tg_list.h"



typedef struct tg_hashmap_bucket
{
	tg_list_h    keys;
	tg_list_h    values;
} tg_hashmap_bucket;

typedef struct tg_hashmap
{
	u32                  bucket_count;
	u32                  element_count;
	u32                  key_size;
	u32                  value_size;
	tg_hash_fn           key_hash_fn;
	tg_equals_fn         key_equals_fn;
	tg_hashmap_bucket    buckets[0];
} tg_hashmap;



b32 tg_f32_equals_fn(const void* p_v0, const void* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	const b32 result = *(f32*)p_v0 == *(f32*)p_v1;
	return result;
}

b32 tg_f64_equals_fn(const void* p_v0, const void* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	const b32 result = *(f64*)p_v0 == *(f64*)p_v1;
	return result;
}

b32 tg_i8_equals_fn(const void* p_v0, const void* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	const b32 result = *(i8*)p_v0 == *(i8*)p_v1;
	return result;
}

b32 tg_i16_equals_fn(const void* p_v0, const void* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	const b32 result = *(i16*)p_v0 == *(i16*)p_v1;
	return result;
}

b32 tg_i32_equals_fn(const void* p_v0, const void* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	const b32 result = *(i32*)p_v0 == *(i32*)p_v1;
	return result;
}

b32 tg_i64_equals_fn(const void* p_v0, const void* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	const b32 result = *(i64*)p_v0 == *(i64*)p_v1;
	return result;
}

b32 tg_u8_equals_fn(const void* p_v0, const void* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	const b32 result = *(u8*)p_v0 == *(u8*)p_v1;
	return result;
}

b32 tg_u16_equals_fn(const void* p_v0, const void* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	const b32 result = *(u16*)p_v0 == *(u16*)p_v1;
	return result;
}

b32 tg_u32_equals_fn(const void* p_v0, const void* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	const b32 result = *(u32*)p_v0 == *(u32*)p_v1;
	return result;
}

b32 tg_u64_equals_fn(const void* p_v0, const void* p_v1)
{
	TG_ASSERT(p_v0 && p_v1);

	const b32 result = *(u64*)p_v0 == *(u64*)p_v1;
	return result;
}



u32 tg_f32_hash_fn(const void* p_v)
{
	TG_ASSERT(p_v);

	const u32 result = (u32)(*(f32*)p_v * 2654435761);
	return result;
}

u32 tg_f64_hash_fn(const void* p_v)
{
	TG_ASSERT(p_v);

	const u32 result = (u32)(*(f64*)p_v * 2654435761);
	return result;
}

u32 tg_i8_hash_fn(const void* p_v)
{
	TG_ASSERT(p_v);

	const u32 result = (u32)(*(i8*)p_v * 2654435761);
	return result;
}

u32 tg_i16_hash_fn(const void* p_v)
{
	TG_ASSERT(p_v);

	const u32 result = (u32)(*(i16*)p_v * 2654435761);
	return result;
}

u32 tg_i32_hash_fn(const void* p_v)
{
	TG_ASSERT(p_v);

	const u32 result = (u32)(*(i32*)p_v * 2654435761);
	return result;
}

u32 tg_i64_hash_fn(const void* p_v)
{
	TG_ASSERT(p_v);

	const u32 result = (u32)(*(i64*)p_v * 2654435761);
	return result;
}

u32 tg_u8_hash_fn(const void* p_v)
{
	TG_ASSERT(p_v);

	const u32 result = (u32)(*(u8*)p_v * 2654435761);
	return result;
}

u32 tg_u16_hash_fn(const void* p_v)
{
	TG_ASSERT(p_v);

	const u32 result = (u32)(*(u16*)p_v * 2654435761);
	return result;
}

u32 tg_u32_hash_fn(const void* p_v)
{
	TG_ASSERT(p_v);

	const u32 result = (u32)(*(u32*)p_v * 2654435761);
	return result;
}

u32 tg_u64_hash_fn(const void* p_v)
{
	TG_ASSERT(p_v);

	const u32 result = (u32)(*(u64*)p_v * 2654435761);
	return result;
}




tg_hashmap_h tg_hashmap_create_impl(u32 key_size, u32 value_size, const tg_hash_fn key_hash_fn, const tg_equals_fn key_equals_fn, u32 bucket_count, u32 bucket_capacity)
{
	TG_ASSERT(key_size && value_size && key_hash_fn && key_equals_fn && bucket_count && bucket_capacity);

	tg_hashmap_h hashmap_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(*hashmap_h) + bucket_count * sizeof(*hashmap_h->buckets));
	
	hashmap_h->bucket_count = bucket_count;
	hashmap_h->element_count = 0;
	hashmap_h->key_size = key_size;
	hashmap_h->value_size = value_size;
	hashmap_h->key_hash_fn = key_hash_fn;
	hashmap_h->key_equals_fn = key_equals_fn;

	for (u32 i = 0; i < bucket_count; i++)
	{
		hashmap_h->buckets[i].keys = tg_list_create__element_size__capacity(key_size, bucket_capacity);
		hashmap_h->buckets[i].values = tg_list_create__element_size__capacity(value_size, bucket_capacity);
	}

	return hashmap_h;
}

tg_hashmap_h tg_hashmap_create_copy(const tg_hashmap_h hashmap_h)
{
	tg_hashmap_h copied_hashmap_h = tg_hashmap_create_impl(hashmap_h->key_size, hashmap_h->value_size, hashmap_h->key_hash_fn, hashmap_h->key_equals_fn, hashmap_h->bucket_count, TG_HASHMAP_DEFAULT_BUCKET_CAPACITY);

	for (u32 i = 0; i < hashmap_h->bucket_count; i++)
	{
		const tg_hashmap_bucket* p_bucket = &hashmap_h->buckets[i];
		tg_list_insert_list(copied_hashmap_h->buckets[i].keys, p_bucket->keys);
		tg_list_insert_list(copied_hashmap_h->buckets[i].values, p_bucket->values);
	}

	return copied_hashmap_h;
}

tg_list_h tg_hashmap_key_list_create(const tg_hashmap_h hashmap_h)
{
	tg_list_h list_h = tg_list_create__element_size__capacity(hashmap_h->key_size, hashmap_h->element_count);
	for (u32 i = 0; i < hashmap_h->bucket_count; i++)
	{
		const tg_hashmap_bucket* p_bucket = &hashmap_h->buckets[i];
		tg_list_insert_list(list_h, p_bucket->keys);
	}
	return list_h;
}

tg_list_h tg_hashmap_value_list_create(const tg_hashmap_h hashmap_h)
{
	tg_list_h list_h = tg_list_create__element_size__capacity(hashmap_h->value_size, hashmap_h->element_count);
	for (u32 i = 0; i < hashmap_h->bucket_count; i++)
	{
		const tg_hashmap_bucket* p_bucket = &hashmap_h->buckets[i];
		tg_list_insert_list(list_h, p_bucket->values);
	}
	return list_h;
}

void tg_hashmap_destroy(tg_hashmap_h hashmap_h)
{
	TG_ASSERT(hashmap_h);

	for (u32 i = 0; i < hashmap_h->bucket_count; i++)
	{
		tg_list_destroy(hashmap_h->buckets[i].keys);
		tg_list_destroy(hashmap_h->buckets[i].values);
	}
	TG_MEMORY_ALLOCATOR_FREE(hashmap_h);
}



u32 tg_hashmap_bucket_count(tg_hashmap_h hashmap_h)
{
	TG_ASSERT(hashmap_h);

	return hashmap_h->bucket_count;
}

b32 tg_hashmap_contains(tg_hashmap_h hashmap_h, const void* p_key)
{
	TG_ASSERT(hashmap_h && p_key);

	const u32 hash = hashmap_h->key_hash_fn(p_key);
	const u32 index = hash % hashmap_h->bucket_count;

	tg_hashmap_bucket* p_bucket = &hashmap_h->buckets[index];

	const u32 bucket_element_count = tg_list_count(p_bucket->keys);
	for (u32 i = 0; i < bucket_element_count; i++)
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

u32 tg_hashmap_element_count(tg_hashmap_h hashmap_h)
{
	return hashmap_h->element_count;
}

b32 tg_hashmap_empty(tg_hashmap_h hashmap_h)
{
	return hashmap_h->element_count == 0;
}

void tg_hashmap_insert(tg_hashmap_h hashmap_h, const void* p_key, const void* p_value)
{
	TG_ASSERT(hashmap_h && p_key && p_value);

	const u32 hash = hashmap_h->key_hash_fn(p_key);
	const u32 index = hash % hashmap_h->bucket_count;

	tg_hashmap_bucket* p_bucket = &hashmap_h->buckets[index];

	const u32 bucket_element_count = tg_list_count(p_bucket->keys);
	for (u32 i = 0; i < bucket_element_count; i++)
	{
		const void* p_bucket_entry_key = tg_list_pointer_to(p_bucket->keys, i);
		const b32 equal = hashmap_h->key_equals_fn(p_key, p_bucket_entry_key);
		if (equal)
		{
			tg_list_replace_at(p_bucket->values, i, p_value);
			return;
		}
	}

	hashmap_h->element_count++;
	tg_list_insert(p_bucket->keys, p_key);
	tg_list_insert(p_bucket->values, p_value);
}

void* tg_hashmap_pointer_to(tg_hashmap_h hashmap_h, const void* p_key)
{
	TG_ASSERT(hashmap_h && p_key);

	const u32 hash = hashmap_h->key_hash_fn(p_key);
	const u32 index = hash % hashmap_h->bucket_count;

	tg_hashmap_bucket* p_bucket = &hashmap_h->buckets[index];

	void* p_found_bucket_entry_value = TG_NULL;
	const u32 bucket_element_count = tg_list_count(p_bucket->keys);
	for (u32 i = 0; i < bucket_element_count; i++)
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

	const b32 result = tg_hashmap_try_remove(hashmap_h, p_key);
	TG_ASSERT(result);
}

b32 tg_hashmap_try_remove(tg_hashmap_h hashmap_h, const void* p_key)
{
	TG_ASSERT(hashmap_h && p_key);

	const u32 hash = hashmap_h->key_hash_fn(p_key);
	const u32 index = hash % hashmap_h->bucket_count;

	tg_hashmap_bucket* p_bucket = &hashmap_h->buckets[index];

	const u32 bucket_element_count = tg_list_count(p_bucket->keys);
	for (u32 i = 0; i < bucket_element_count; i++)
	{
		const void* p_bucket_entry_key = tg_list_pointer_to(p_bucket->keys, i);
		const b32 equal = hashmap_h->key_equals_fn(p_key, p_bucket_entry_key);
		if (equal)
		{
			hashmap_h->element_count--;
			tg_list_remove_at(p_bucket->keys, i);
			tg_list_remove_at(p_bucket->values, i);
			return TG_TRUE;
		}
	}

	return TG_FALSE;
}
