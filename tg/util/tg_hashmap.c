#include "tg/util/tg_hashmap.h"

#include "tg/memory/tg_memory_allocator.h"
#include <string.h> // TODO: implement memset yourself



typedef struct tg_hashmap_bucket
{
	void*                        key;
	void*                        value;
	struct tg_hashmap_bucket*    next; // TODO: this will be inefficient
} tg_hashmap_bucket;

typedef struct tg_hashmap
{
	u32                   size;
	u32                   key_size;
	u32                   value_size;
	tg_hash_fn            key_hash_fn;
	tg_hashmap_bucket     p_buckets[0]; // TODO: THIS MIGHT NOT WORK LIKE THIS! YOU NEED TO PROBABLY HAVE AN ARRAY THAT EXPANDS WITH REALLOC OR SIMILAR
} tg_hashmap;



void tg_hashmap_create(u32 size, const tg_hash_fn key_hash_fn, tg_hashmap_h* p_hashmap_h)
{
	TG_ASSERT(p_hashmap_h);

	*p_hashmap_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(**p_hashmap_h) + size * sizeof(*(**p_hashmap_h).p_buckets));
	(**p_hashmap_h).size = size;
	(**p_hashmap_h).key_hash_fn = key_hash_fn;
	memset((**p_hashmap_h).p_buckets, 0, size * sizeof(*(**p_hashmap_h).p_buckets));
}

void tg_hashmap_insert(tg_hashmap_h hashmap_h, const void* key, const void* value)
{
	const u32 hash = hashmap_h->key_hash_fn(key);
	const u32 index = hash % hashmap_h->size;

	if (&hashmap_h->p_buckets[index] == TG_NULL)
	{
	}
}

void* tg_hashmap_remove(tg_hashmap_h hashmap_h, const void* key)
{
	return TG_NULL;
}

void tg_hashmap_destroy(tg_hashmap_h hashmap_h)
{
	TG_ASSERT(hashmap_h);

	TG_MEMORY_ALLOCATOR_FREE(hashmap_h);
}
