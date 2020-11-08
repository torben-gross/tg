#include "memory/tg_memory.h"

#ifdef TG_DEBUG
#include "math/tg_math.h"
#include "platform/tg_platform.h"
#include "util/tg_string.h"
#endif



#define TG_MEMORY_MAX_ALLOCATION_COUNT    42131
#define TG_MEMORY_STACK_SIZE              (1LL << 30LL)
#define TG_MEMORY_STACK_SIZE_ASYNC        (1LL << 25LL)
#define TG_MEMORY_STACK_MAGIC_NUMBER      163ui8
#define TG_MEMORY_HASH(p_key)             ((u32)((u64)(p_key) >> 3LL) % TG_MEMORY_MAX_ALLOCATION_COUNT)



typedef struct tg_memory_stack
{
	u64    exhausted_size;
	u8*    p_memory;
} tg_memory_stack;

#ifdef TG_DEBUG
typedef struct tg_memory_allocator_allocation
{
	char    p_filename[TG_MAX_PATH];
	u32     line;
	// TODO: thread id?
} tg_memory_allocator_allocation;

typedef struct tg_memory_hashmap
{
	u32                               count;
	void*                             pp_keys[TG_MEMORY_MAX_ALLOCATION_COUNT];
	tg_memory_allocator_allocation    p_values[TG_MEMORY_MAX_ALLOCATION_COUNT];
	u64                               count_since_startup;
	u32                               max_count;
	u32                               max_step_count;
} tg_memory_hashmap;
#endif



#ifdef TG_DEBUG
static tg_mutex_h                        h_mutex;
static tg_memory_hashmap                 hashmap = { 0 };
static void*                             pp_rehash_keys[TG_MEMORY_MAX_ALLOCATION_COUNT] = { 0 };
static tg_memory_allocator_allocation    p_rehash_values[TG_MEMORY_MAX_ALLOCATION_COUNT] = { 0 };
#endif

static tg_memory_stack       p_memory_stacks[TG_MAX_THREADS] = { 0 };



#ifdef TG_DEBUG

void tg__insert(void* p_memory, const char* p_filename, u32 line)
{
	TG_ASSERT(p_memory);

	TG_ASSERT(hashmap.count < TG_MEMORY_MAX_ALLOCATION_COUNT);
	hashmap.count_since_startup++;
	hashmap.count++;
	hashmap.max_count = TG_MAX(hashmap.max_count, hashmap.count);

	u32 hash = TG_MEMORY_HASH(p_memory);
	u8 step_count = 0;
	while (hashmap.pp_keys[hash] != TG_NULL)
	{
		hash = tgm_u32_incmod(hash, TG_MEMORY_MAX_ALLOCATION_COUNT);
		step_count++;
	}
	hashmap.max_step_count = TG_MAX(hashmap.max_step_count, step_count);

	hashmap.pp_keys[hash] = p_memory;
	hashmap.p_values[hash].line = line;
	tg_memcpy(tg_strlen_no_nul(p_filename) * sizeof(*p_filename), p_filename, hashmap.p_values[hash].p_filename);
}

void tg__remove(void* p_memory)
{
	u32 hash = TG_MEMORY_HASH(p_memory);
	while (hashmap.pp_keys[hash] != p_memory)
	{
		TG_ASSERT(hashmap.pp_keys[hash] != TG_NULL);
		hash = tgm_u32_incmod(hash, TG_MEMORY_MAX_ALLOCATION_COUNT);
	}
	hashmap.pp_keys[hash] = TG_NULL;
	hashmap.p_values[hash] = (tg_memory_allocator_allocation){ 0 };
	hashmap.count--;

	hash = tgm_u32_incmod(hash, TG_MEMORY_MAX_ALLOCATION_COUNT);
	u32 rehash_entry_count = 0;
	while (hashmap.pp_keys[hash] != TG_NULL)
	{
		pp_rehash_keys[rehash_entry_count] = hashmap.pp_keys[hash];
		p_rehash_values[rehash_entry_count++] = hashmap.p_values[hash];

		hashmap.pp_keys[hash] = TG_NULL;
		hashmap.p_values[hash] = (tg_memory_allocator_allocation){ 0 };

		hashmap.count--;
		hash = tgm_u32_incmod(hash, TG_MEMORY_MAX_ALLOCATION_COUNT);
	}
	for (u32 i = 0; i < rehash_entry_count; i++)
	{
		tg__insert(pp_rehash_keys[i], p_rehash_values->p_filename, p_rehash_values->line);
	}
}

#endif



void tg_memory_init(void)
{
#ifdef TG_DEBUG
	h_mutex = TG_MUTEX_CREATE();

	p_memory_stacks[0].exhausted_size = 1;
	p_memory_stacks[0].p_memory = tgp_malloc(TG_MEMORY_STACK_SIZE);
	TG_ASSERT(p_memory_stacks[0].p_memory);
	p_memory_stacks[0].p_memory[0] = TG_MEMORY_STACK_MAGIC_NUMBER;

	for (u32 i = 1; i < TG_MAX_THREADS; i++)
	{
		p_memory_stacks[i].exhausted_size = 1;
		p_memory_stacks[i].p_memory = tgp_malloc(TG_MEMORY_STACK_SIZE_ASYNC);
		TG_ASSERT(p_memory_stacks[i].p_memory);
		p_memory_stacks[i].p_memory[0] = TG_MEMORY_STACK_MAGIC_NUMBER;
	}

#else
	p_memory_stacks[0].exhausted_size = 0;
	p_memory_stacks[0].p_memory = tgp_malloc(TG_MEMORY_STACK_SIZE);

	for (u32 i = 1; i < TG_MAX_THREADS; i++)
	{
		p_memory_stacks[i].exhausted_size = 0;
		p_memory_stacks[i].p_memory = tgp_malloc(TG_MEMORY_STACK_SIZE_ASYNC);
	}
#endif
}

void tg_memory_shutdown(void)
{
#ifdef TG_DEBUG
	TG_MUTEX_LOCK(h_mutex);

	if (hashmap.count != 0)
	{
		TG_DEBUG_LOG("\nMEMORY LEAKS DETECTED:\n");
		for (u32 i = 0; i < TG_MEMORY_MAX_ALLOCATION_COUNT; i++)
		{
			if (hashmap.pp_keys[i] != TG_NULL)
			{
				const tg_memory_allocator_allocation* p_allocation = &hashmap.p_values[i];
				TG_DEBUG_LOG("\tFilename: %s, Line: %u\n", p_allocation->p_filename, p_allocation->line);
			}
		}
		TG_DEBUG_LOG("\n");
	}
	TG_ASSERT(p_memory_stacks[0].exhausted_size == 1);
#endif

	for (u32 i = 0; i < TG_MAX_THREADS; i++)
	{
		tgp_free(p_memory_stacks[i].p_memory);
		p_memory_stacks[i].exhausted_size = 0;
	}

#ifdef TG_DEBUG
	TG_MUTEX_UNLOCK(h_mutex);
	TG_MUTEX_DESTROY(h_mutex);
#endif
}



void tg_memcpy(u64 size, const void* p_source, void* p_destination)
{
	for (u64 i = 0; i < size; i++)
	{
		((u8*)p_destination)[i] = ((u8*)p_source)[i];
	}
}

void tg_memory_nullify(u64 size, void* p_memory)
{
	for (u64 i = 0; i < size; i++)
	{
		((u8*)p_memory)[i] = 0;
	}
}

void tg_memory_set_all_bits(u64 size, void* p_memory)
{
	for (u64 i = 0; i < size; i++)
	{
		((u8*)p_memory)[i] = 0xff;
	}
}

#ifdef TG_DEBUG

void* tg_memory_alloc_impl(u64 size, const char* p_filename, u32 line, b32 nullify)
{
	TG_ASSERT(size);
	void* p_memory = TG_NULL;

	if (nullify)
	{
		p_memory = tgp_malloc_nullify(size);
	}
	else
	{
		p_memory = tgp_malloc(size);
	}
	TG_ASSERT(p_memory);

	TG_MUTEX_LOCK(h_mutex);
	tg__insert(p_memory, p_filename, line);
	TG_MUTEX_UNLOCK(h_mutex);

	return p_memory;
}

void* tg_memory_realloc_impl(u64 size, void* p_memory, const char* p_filename, u32 line, b32 nullify)
{
	TG_ASSERT(size);
	
	void* p_reallocated_memory = TG_NULL;
	if (p_memory)
	{
		if (nullify)
		{
			p_reallocated_memory = tgp_realloc_nullify(size, p_memory);
		}
		else
		{
			p_reallocated_memory = tgp_realloc(size, p_memory);
		}
		TG_ASSERT(p_reallocated_memory);

		if (p_memory != p_reallocated_memory)
		{
			TG_MUTEX_LOCK(h_mutex);
			tg__remove(p_memory);
			tg__insert(p_reallocated_memory, p_filename, line);
			TG_MUTEX_UNLOCK(h_mutex);
		}
	}
	else
	{
		p_reallocated_memory = tg_memory_alloc_impl(size, p_filename, line, nullify);
	}

	return p_reallocated_memory;
}

void tg_memory_free_impl(void* p_memory)
{
	TG_MUTEX_LOCK(h_mutex);
	tg__remove(p_memory);
	TG_MUTEX_UNLOCK(h_mutex);

	tgp_free(p_memory);
}

u32 tg_memory_active_allocation_count(void)
{
	return hashmap.count;
}

#endif



void* tg_memory_stack_alloc(u64 size)
{
#ifdef TG_DEBUG
	TG_ASSERT(tgp_get_thread_id() == 0);
	TG_ASSERT(size && p_memory_stacks[0].exhausted_size + size + 1 < TG_MEMORY_STACK_SIZE);

	void* memory = &p_memory_stacks[0].p_memory[p_memory_stacks[0].exhausted_size];
	p_memory_stacks[0].exhausted_size += size + 1;
	p_memory_stacks[0].p_memory[p_memory_stacks[0].exhausted_size - 1] = TG_MEMORY_STACK_MAGIC_NUMBER;

	return memory;
#else
	void* memory = &p_memory_stacks[0].p_memory[p_memory_stacks[0].exhausted_size];
	p_memory_stacks[0].exhausted_size += size;

	return memory;
#endif
}

void tg_memory_stack_free(u64 size)
{
#ifdef TG_DEBUG
	TG_ASSERT(tgp_get_thread_id() == 0);
	TG_ASSERT(size && p_memory_stacks[0].exhausted_size - 1 >= size + 1);

	TG_ASSERT(p_memory_stacks[0].p_memory[p_memory_stacks[0].exhausted_size - 1] == TG_MEMORY_STACK_MAGIC_NUMBER);
	p_memory_stacks[0].exhausted_size -= size + 1;
	TG_ASSERT(p_memory_stacks[0].p_memory[p_memory_stacks[0].exhausted_size - 1] == TG_MEMORY_STACK_MAGIC_NUMBER);
#else
	p_memory_stacks[0].exhausted_size -= size;
#endif
}

void* tg_memory_stack_alloc_async(u64 size)
{
	const u32 i = tgp_get_thread_id();

#ifdef TG_DEBUG
	TG_ASSERT(size && p_memory_stacks[i].exhausted_size + size + 1 < TG_MEMORY_STACK_SIZE_ASYNC);

	void* memory = &p_memory_stacks[i].p_memory[p_memory_stacks[i].exhausted_size];
	p_memory_stacks[i].exhausted_size += size + 1;
	p_memory_stacks[i].p_memory[p_memory_stacks[i].exhausted_size - 1] = TG_MEMORY_STACK_MAGIC_NUMBER;

	return memory;
#else
	void* memory = &p_memory_stacks[i].p_memory[p_memory_stacks[i].exhausted_size];
	p_memory_stacks[i].exhausted_size += size;

	return memory;
#endif
}

void tg_memory_stack_free_async(u64 size)
{
	const u32 i = tgp_get_thread_id();

#ifdef TG_DEBUG
	TG_ASSERT(size && p_memory_stacks[i].exhausted_size - 1 >= size + 1);

	TG_ASSERT(p_memory_stacks[i].p_memory[p_memory_stacks[i].exhausted_size - 1] == TG_MEMORY_STACK_MAGIC_NUMBER);
	p_memory_stacks[i].exhausted_size -= size + 1;
	TG_ASSERT(p_memory_stacks[i].p_memory[p_memory_stacks[i].exhausted_size - 1] == TG_MEMORY_STACK_MAGIC_NUMBER);
#else
	p_memory_stacks[i].exhausted_size -= size;
#endif
}
