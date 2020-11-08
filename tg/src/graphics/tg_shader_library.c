#include "graphics/tg_shader_library.h"

#include "graphics/tg_graphics.h"
#include "memory/tg_memory.h"
#include "platform/tg_platform.h"
#include "util/tg_string.h"



typedef struct tg_compute_shader_hashmap
{
#ifdef TG_DEBUG
	u32                    count;
#endif
	tg_compute_shader_h    ph_shaders[TG_MAX_COMPUTE_SHADERS];
	char                   pp_filenames[TG_MAX_COMPUTE_SHADERS][TG_MAX_PATH];
} tg_compute_shader_hashmap;

typedef struct tg_fragment_shader_hashmap
{
#ifdef TG_DEBUG
	u32                     count;
#endif
	tg_fragment_shader_h    ph_shaders[TG_MAX_FRAGMENT_SHADERS];
	char                    pp_filenames[TG_MAX_FRAGMENT_SHADERS][TG_MAX_PATH];
} tg_fragment_shader_hashmap;

typedef struct tg_vertex_shader_hashmap
{
#ifdef TG_DEBUG
	u32                   count;
#endif
	tg_vertex_shader_h    ph_shaders[TG_MAX_VERTEX_SHADERS];
	char                  pp_filenames[TG_MAX_VERTEX_SHADERS][TG_MAX_PATH];
} tg_vertex_shader_hashmap;



tg_compute_shader_hashmap compute_shader_hashmap = { 0 };
tg_fragment_shader_hashmap fragment_shader_hashmap = { 0 };
tg_vertex_shader_hashmap vertex_shader_hashmap = { 0 };



static u32 tg__hash(const char* p_filename)
{
	const char* p_it = &p_filename[tg_strlen_no_nul(p_filename) - 1];
	while (*p_it != '/' && *p_it != '\\')
	{
		if (p_it-- == p_filename)
		{
			break;
		}
	}
	p_it++;

	u32 hash = 0;
	do
	{
		hash += (u32)(*p_it) * 2654435761;
	} while (*++p_it != '.');
	return hash;
}

static void tg__prepare_filename(const char* p_filename, char* p_buffer)
{
	tg_strcpy(TG_MAX_PATH, p_buffer, p_filename);
#ifdef TG_WIN32
	tg_string_replace_characters(p_buffer, '/', TG_FILE_SEPERATOR);
#else
	TG_INVALID_CODEPATH();
#endif
}

static void tg__try_load(const tg_file_properties* p_properties)
{
	const u32 hash = tg__hash(p_properties->p_filename);

	if (tg_string_equal(p_properties->p_extension, "comp"))
	{
		TG_ASSERT(compute_shader_hashmap.count < TG_MAX_COMPUTE_SHADERS);

		u32 index = hash % TG_MAX_COMPUTE_SHADERS;
		while (compute_shader_hashmap.ph_shaders[index] != TG_NULL)
		{
			index++;
		}
		compute_shader_hashmap.ph_shaders[index] = tg_compute_shader_create(p_properties->p_filename);
		tg_memcpy(TG_MAX_PATH, p_properties->p_filename, compute_shader_hashmap.pp_filenames[index]);
#ifdef TG_DEBUG
		compute_shader_hashmap.count++;
#endif
	}
	else if (tg_string_equal(p_properties->p_extension, "frag"))
	{
		TG_ASSERT(fragment_shader_hashmap.count < TG_MAX_FRAGMENT_SHADERS);

		u32 index = hash % TG_MAX_FRAGMENT_SHADERS;
		while (fragment_shader_hashmap.ph_shaders[index] != TG_NULL)
		{
			index++;
		}
		fragment_shader_hashmap.ph_shaders[index] = tg_fragment_shader_create(p_properties->p_filename);
		tg_memcpy(TG_MAX_PATH, p_properties->p_filename, fragment_shader_hashmap.pp_filenames[index]);
#ifdef TG_DEBUG
		fragment_shader_hashmap.count++;
#endif
	}
	else if (tg_string_equal(p_properties->p_extension, "vert"))
	{
		TG_ASSERT(vertex_shader_hashmap.count < TG_MAX_VERTEX_SHADERS);

		u32 index = hash % TG_MAX_VERTEX_SHADERS;
		while (vertex_shader_hashmap.ph_shaders[index] != TG_NULL)
		{
			index++;
		}
		vertex_shader_hashmap.ph_shaders[index] = tg_vertex_shader_create(p_properties->p_filename);
		tg_memcpy(TG_MAX_PATH, p_properties->p_filename, vertex_shader_hashmap.pp_filenames[index]);
#ifdef TG_DEBUG
		vertex_shader_hashmap.count++;
#endif
	}
}

static void tg__try_load_directory(const char* p_directory)
{
	tg_file_properties file_properties = { 0 };
	tg_file_iterator_h h_file_iterator = tgp_directory_begin_iteration(p_directory, &file_properties);

	if (h_file_iterator == TG_NULL)
	{
		return;
	}

	do
	{
		if (file_properties.is_directory)
		{
			tg__try_load_directory(file_properties.p_filename);
		}
		else
		{
			tg__try_load(&file_properties);
		}
	} while (tgp_directory_continue_iteration(h_file_iterator, p_directory, &file_properties));
}



void tg_shader_library_init(void)
{
	tg__try_load_directory(".");
}

void tg_shader_library_shutdown(void)
{
	for (u32 i = 0; i < TG_MAX_COMPUTE_SHADERS; i++)
	{
		if (compute_shader_hashmap.ph_shaders[i])
		{
			tg_compute_shader_destroy(compute_shader_hashmap.ph_shaders[i]);
		}
	}
	for (u32 i = 0; i < TG_MAX_FRAGMENT_SHADERS; i++)
	{
		if (fragment_shader_hashmap.ph_shaders[i])
		{
			tg_fragment_shader_destroy(fragment_shader_hashmap.ph_shaders[i]);
		}
	}
	for (u32 i = 0; i < TG_MAX_VERTEX_SHADERS; i++)
	{
		if (vertex_shader_hashmap.ph_shaders[i])
		{
			tg_vertex_shader_destroy(vertex_shader_hashmap.ph_shaders[i]);
		}
	}
}

tg_compute_shader_h tg_shader_library_get_compute_shader(const char* p_filename)
{
	char p_buffer[TG_MAX_PATH] = { 0 };
	tg__prepare_filename(p_filename, p_buffer);

	const u32 hash = tg__hash(p_buffer);
	u32 index = hash % TG_MAX_COMPUTE_SHADERS;
	while (compute_shader_hashmap.ph_shaders[index] && !tg_string_equal(compute_shader_hashmap.pp_filenames[index], p_buffer))
	{
		index = tgm_u32_incmod(index, TG_MAX_COMPUTE_SHADERS);
	}
	tg_compute_shader_h h_shader = compute_shader_hashmap.ph_shaders[index];
	return h_shader;
}

tg_fragment_shader_h tg_shader_library_get_fragment_shader(const char* p_filename)
{
	char p_buffer[TG_MAX_PATH] = { 0 };
	tg__prepare_filename(p_filename, p_buffer);

	const u32 hash = tg__hash(p_buffer);
	u32 index = hash % TG_MAX_FRAGMENT_SHADERS;
	while (fragment_shader_hashmap.ph_shaders[index] && !tg_string_equal(fragment_shader_hashmap.pp_filenames[index], p_buffer))
	{
		index = tgm_u32_incmod(index, TG_MAX_FRAGMENT_SHADERS);
	}
	tg_fragment_shader_h h_shader = fragment_shader_hashmap.ph_shaders[index];
	return h_shader;
}

tg_vertex_shader_h tg_shader_library_get_vertex_shader(const char* p_filename)
{
	char p_buffer[TG_MAX_PATH] = { 0 };
	tg__prepare_filename(p_filename, p_buffer);

	const u32 hash = tg__hash(p_buffer);
	u32 index = hash % TG_MAX_VERTEX_SHADERS;
	while (vertex_shader_hashmap.ph_shaders[index] && !tg_string_equal(vertex_shader_hashmap.pp_filenames[index], p_buffer))
	{
		index = tgm_u32_incmod(index, TG_MAX_VERTEX_SHADERS);
	}
	tg_vertex_shader_h h_shader = vertex_shader_hashmap.ph_shaders[index];
	return h_shader;
}
