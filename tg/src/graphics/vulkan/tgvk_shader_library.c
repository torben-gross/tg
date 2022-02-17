#include "graphics/vulkan/tgvk_shader_library.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "platform/tg_platform.h"
#include "util/tg_string.h"



#define TG_SHADER_LIBRARY_CAPACITY    128



typedef struct tg_shader_library_hashmap
{
#ifdef TG_DEBUG
	u32            count;
#endif
	tgvk_shader    p_shaders[TG_SHADER_LIBRARY_CAPACITY];
	char           pp_filenames[TG_SHADER_LIBRARY_CAPACITY][TG_MAX_PATH];
} tg_shader_library_hashmap;



tg_shader_library_hashmap    shader_library_hashmap = { 0 };



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

	const b32 is_shader =
		tg_string_equal(p_properties->p_extension, "comp") ||
		tg_string_equal(p_properties->p_extension, "frag") ||
		tg_string_equal(p_properties->p_extension, "vert");

	if (is_shader)
	{
		TG_ASSERT(2 * shader_library_hashmap.count < TG_SHADER_LIBRARY_CAPACITY);

		u32 index = hash % TG_SHADER_LIBRARY_CAPACITY;
		while (shader_library_hashmap.p_shaders[index].shader_module != TG_NULL)
		{
			index = tgm_u32_incmod(index, TG_SHADER_LIBRARY_CAPACITY);
		}
		shader_library_hashmap.p_shaders[index] = tgvk_shader_create(p_properties->p_filename);
		tg_memcpy(TG_MAX_PATH, p_properties->p_filename, shader_library_hashmap.pp_filenames[index]);
#ifdef TG_DEBUG
		shader_library_hashmap.count++;
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



void tgvk_shader_library_init(void)
{
	tg__try_load_directory(".");
}

void tgvk_shader_library_shutdown(void)
{
	for (u32 i = 0; i < TG_SHADER_LIBRARY_CAPACITY; i++)
	{
		if (shader_library_hashmap.p_shaders[i].shader_module)
		{
			tgvk_shader_destroy(&shader_library_hashmap.p_shaders[i]);
		}
	}
}

tgvk_shader* tgvk_shader_library_get(const char* p_filename)
{
	char p_buffer[TG_MAX_PATH] = { 0 };
	tg__prepare_filename(p_filename, p_buffer);

	const u32 hash = tg__hash(p_buffer);
	u32 index = hash % TG_SHADER_LIBRARY_CAPACITY;
	while (shader_library_hashmap.p_shaders[index].shader_module && !tg_string_equal(shader_library_hashmap.pp_filenames[index], p_buffer))
	{
		index = tgm_u32_incmod(index, TG_SHADER_LIBRARY_CAPACITY);
	}
	tgvk_shader* p_shader = &shader_library_hashmap.p_shaders[index];
	return p_shader;
}

#endif
