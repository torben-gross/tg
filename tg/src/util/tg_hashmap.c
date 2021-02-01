#include "util/tg_hashmap.h"

#include "math/tg_math.h"
#include "memory/tg_memory.h"
#include "util/tg_list.h"
#include "util/tg_string.h"



static u32 tg__hash_key(const tg_hashmap* p_hashmap, const void* p_key)
{
	u32 result = 0;

	for (u32 i = 0; i < p_hashmap->key_size; i++)
	{
		result += (u32)((u8*)p_key)[i] * 2654435761;
	}

	return result;
}

static b32 tg__keys_equal(const tg_hashmap* p_hashmap, const void* p_key0, const void* p_key1)
{
	b32 result = TG_TRUE;

	for (u32 i = 0; i < p_hashmap->key_size; i++)
	{
		result &= ((u8*)p_key0)[i] == ((u8*)p_key1)[i];
	}

	return result;
}



tg_hashmap tg_hashmap_create_impl(u32 key_size, u32 value_size, u32 bucket_count, u32 bucket_capacity)
{
	TG_ASSERT(key_size && value_size && bucket_count && !tgm_u32_is_power_of_two(bucket_count) && bucket_capacity);

	tg_hashmap hashmap = { 0 };
	hashmap.bucket_count = bucket_count;
	hashmap.element_count = 0;
	hashmap.key_size = key_size;
	hashmap.value_size = value_size;
	hashmap.p_buckets = TG_MALLOC(bucket_count * sizeof(*hashmap.p_buckets));

	for (u32 i = 0; i < bucket_count; i++)
	{
		hashmap.p_buckets[i].keys = tg_list_create(key_size, bucket_capacity, TG_NULL);
		hashmap.p_buckets[i].values = tg_list_create(value_size, bucket_capacity, TG_NULL);
	}

	return hashmap;
}

tg_hashmap tg_hashmap_create_copy(const tg_hashmap* p_hashmap)
{
	tg_hashmap copied_hashmap = tg_hashmap_create_impl(p_hashmap->key_size, p_hashmap->value_size, p_hashmap->bucket_count, TG_HASHMAP_DEFAULT_BUCKET_CAPACITY);

	for (u32 i = 0; i < p_hashmap->bucket_count; i++)
	{
		tg_hashmap_bucket* p_bucket = &p_hashmap->p_buckets[i];
		tg_list_insert_list(&copied_hashmap.p_buckets[i].keys, &p_bucket->keys);
		tg_list_insert_list(&copied_hashmap.p_buckets[i].values, &p_bucket->values);
	}

	return copied_hashmap;
}

tg_list tg_hashmap_key_list_create(const tg_hashmap* p_hashmap)
{
	tg_list list = tg_list_create(p_hashmap->key_size, p_hashmap->element_count, TG_NULL);
	for (u32 i = 0; i < p_hashmap->bucket_count; i++)
	{
		tg_hashmap_bucket* p_bucket = &p_hashmap->p_buckets[i];
		tg_list_insert_list(&list, &p_bucket->keys);
	}
	return list;
}

tg_list tg_hashmap_value_list_create(const tg_hashmap* p_hashmap)
{
	tg_list list = tg_list_create(p_hashmap->value_size, p_hashmap->element_count, TG_NULL);
	for (u32 i = 0; i < p_hashmap->bucket_count; i++)
	{
		tg_hashmap_bucket* p_bucket = &p_hashmap->p_buckets[i];
		tg_list_insert_list(&list, &p_bucket->values);
	}
	return list;
}

void tg_hashmap_destroy(tg_hashmap* p_hashmap)
{
	TG_ASSERT(p_hashmap);

	for (u32 i = 0; i < p_hashmap->bucket_count; i++)
	{
		tg_list_destroy(&p_hashmap->p_buckets[i].keys);
		tg_list_destroy(&p_hashmap->p_buckets[i].values);
	}
	TG_FREE(p_hashmap->p_buckets);
}



u32 tg_hashmap_bucket_count(tg_hashmap* p_hashmap)
{
	TG_ASSERT(p_hashmap);

	return p_hashmap->bucket_count;
}

b32 tg_hashmap_contains(tg_hashmap* p_hashmap, const void* p_key)
{
	TG_ASSERT(p_hashmap && p_key);

	const u32 hash = tg__hash_key(p_hashmap, p_key);
	const u32 index = hash % p_hashmap->bucket_count;

	tg_hashmap_bucket* p_bucket = &p_hashmap->p_buckets[index];
	for (u32 i = 0; i < p_bucket->keys.count; i++)
	{
		const void* p_bucket_entry_key = TG_LIST_AT(p_bucket->keys, i);
		const b32 equal = tg__keys_equal(p_hashmap, p_key, p_bucket_entry_key);
		if (equal)
		{
			return TG_TRUE;
		}
	}
	return TG_FALSE;
}

u32 tg_hashmap_element_count(tg_hashmap* p_hashmap)
{
	return p_hashmap->element_count;
}

b32 tg_hashmap_empty(tg_hashmap* p_hashmap)
{
	return p_hashmap->element_count == 0;
}

void tg_hashmap_insert(tg_hashmap* p_hashmap, const void* p_key, const void* p_value)
{
	TG_ASSERT(p_hashmap && p_key && p_value);

	const u32 hash = tg__hash_key(p_hashmap, p_key);
	const u32 index = hash % p_hashmap->bucket_count;

	tg_hashmap_bucket* p_bucket = &p_hashmap->p_buckets[index];

	const u32 bucket_element_count = p_bucket->keys.count;
	for (u32 i = 0; i < bucket_element_count; i++)
	{
		const void* p_bucket_entry_key = TG_LIST_AT(p_bucket->keys, i);
		const b32 equal = tg__keys_equal(p_hashmap, p_key, p_bucket_entry_key);
		if (equal)
		{
			tg_list_replace_at(&p_bucket->values, i, p_value);
			return;
		}
	}

	p_hashmap->element_count++;
	tg_list_insert(&p_bucket->keys, p_key);
	tg_list_insert(&p_bucket->values, p_value);
}

void* tg_hashmap_pointer_to(tg_hashmap* p_hashmap, const void* p_key)
{
	TG_ASSERT(p_hashmap && p_key);

	const u32 hash = tg__hash_key(p_hashmap, p_key);
	const u32 index = hash % p_hashmap->bucket_count;

	tg_hashmap_bucket* p_bucket = &p_hashmap->p_buckets[index];

	void* p_found_bucket_entry_value = TG_NULL;
	const u32 bucket_element_count = p_bucket->keys.count;
	for (u32 i = 0; i < bucket_element_count; i++)
	{
		const void* p_bucket_entry_key = TG_LIST_AT(p_bucket->keys, i);
		const b32 equal = tg__keys_equal(p_hashmap, p_key, p_bucket_entry_key);
		if (equal)
		{
			p_found_bucket_entry_value = TG_LIST_AT(p_bucket->values, i);
			break;
		}
	}

	return p_found_bucket_entry_value;
}

void tg_hashmap_remove(tg_hashmap* p_hashmap, const void* p_key)
{
	TG_ASSERT(p_hashmap && p_key);

	const b32 result = tg_hashmap_try_remove(p_hashmap, p_key);
	TG_ASSERT(result);
}

b32 tg_hashmap_try_remove(tg_hashmap* p_hashmap, const void* p_key)
{
	TG_ASSERT(p_hashmap && p_key);

	const u32 hash = tg__hash_key(p_hashmap, p_key);
	const u32 index = hash % p_hashmap->bucket_count;

	tg_hashmap_bucket* p_bucket = &p_hashmap->p_buckets[index];

	const u32 bucket_element_count = p_bucket->keys.count;
	for (u32 i = 0; i < bucket_element_count; i++)
	{
		const void* p_bucket_entry_key = TG_LIST_AT(p_bucket->keys, i);
		const b32 equal = tg__keys_equal(p_hashmap, p_key, p_bucket_entry_key);
		if (equal)
		{
			p_hashmap->element_count--;
			tg_list_remove_at(&p_bucket->keys, i);
			tg_list_remove_at(&p_bucket->values, i);
			return TG_TRUE;
		}
	}

	return TG_FALSE;
}





static u32 tg__hash_string(const char* p_key)
{
	u32 result = 0;

	while (*p_key)
	{
		result += (u32)*p_key++ * 2654435761;
	}

	return result;
}



tg_string_hashmap tg_string_hashmap_create_impl(u32 value_size, u32 bucket_count, u32 bucket_capacity)
{
	TG_ASSERT(value_size && bucket_count && !tgm_u32_is_power_of_two(bucket_count) && bucket_capacity);

	tg_string_hashmap string_hashmap = { 0 };
	string_hashmap.bucket_count = bucket_count;
	string_hashmap.element_count = 0;
	string_hashmap.value_size = value_size;
	string_hashmap.p_buckets = TG_MALLOC(bucket_count * sizeof(*string_hashmap.p_buckets));

	for (u32 i = 0; i < bucket_count; i++)
	{
		string_hashmap.p_buckets[i].keys = tg_list_create(sizeof(char*), bucket_capacity, TG_NULL);
		string_hashmap.p_buckets[i].values = tg_list_create(value_size, bucket_capacity, TG_NULL);
	}

	return string_hashmap;
}

void tg_string_hashmap_destroy(tg_string_hashmap* p_string_hashmap)
{
	TG_ASSERT(p_string_hashmap);

	for (u32 i = 0; i < p_string_hashmap->bucket_count; i++)
	{
		for (u32 j = 0; j < p_string_hashmap->p_buckets[i].keys.count; j++)
		{
			TG_FREE(((char**)p_string_hashmap->p_buckets[i].keys.p_elements)[j]);
		}
		tg_list_destroy(&p_string_hashmap->p_buckets[i].keys);
		tg_list_destroy(&p_string_hashmap->p_buckets[i].values);
	}
	TG_FREE(p_string_hashmap->p_buckets);
}

void tg_string_hashmap_insert(tg_string_hashmap* p_string_hashmap, const char* p_key, const void* p_value)
{
	TG_ASSERT(p_string_hashmap && p_key && p_value);

	const u32 hash = tg__hash_string(p_key);
	const u32 index = hash % p_string_hashmap->bucket_count;

	tg_string_hashmap_bucket* p_bucket = &p_string_hashmap->p_buckets[index];

	const u32 bucket_element_count = p_bucket->keys.count;
	for (u32 i = 0; i < bucket_element_count; i++)
	{
		const char* p_bucket_entry_key = *(char**)TG_LIST_AT(p_bucket->keys, i);
		const b32 equal = tg_string_equal(p_key, p_bucket_entry_key);
		if (equal)
		{
			tg_list_replace_at(&p_bucket->values, i, p_value);
			return;
		}
	}

	p_string_hashmap->element_count++;
	tg_size key_size = ((tg_size)tg_strlen_no_nul(p_key) + 1) * sizeof(char);
	char* p_key_copy = TG_MALLOC(key_size);
	tg_memcpy(key_size, p_key, p_key_copy);
	tg_list_insert(&p_bucket->keys, &p_key_copy);
	tg_list_insert(&p_bucket->values, p_value);
}

void* tg_string_hashmap_pointer_to(tg_string_hashmap* p_string_hashmap, const char* p_key)
{
	TG_ASSERT(p_string_hashmap && p_key);

	const u32 hash = tg__hash_string(p_key);
	const u32 index = hash % p_string_hashmap->bucket_count;

	tg_string_hashmap_bucket* p_bucket = &p_string_hashmap->p_buckets[index];

	void* p_found_bucket_entry_value = TG_NULL;
	const u32 bucket_element_count = p_bucket->keys.count;
	for (u32 i = 0; i < bucket_element_count; i++)
	{
		const char* p_bucket_entry_key = *(char**)TG_LIST_AT(p_bucket->keys, i);
		const b32 equal = tg_string_equal(p_key, p_bucket_entry_key);
		if (equal)
		{
			p_found_bucket_entry_value = TG_LIST_AT(p_bucket->values, i);
			break;
		}
	}

	return p_found_bucket_entry_value;
}
