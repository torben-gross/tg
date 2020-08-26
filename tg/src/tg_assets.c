#include "tg_assets.h"

#include "graphics/tg_graphics.h"
#include "platform/tg_platform.h"
#include "util/tg_string.h"



#define TG_MAX_ASSET_COUNT              1024
#define TG_MAX_ASSETS_DIRECTORY_SIZE    1073741824



typedef struct tg_assets
{
	u32                   count;
	tg_handle             p_handles[TG_MAX_ASSET_COUNT];
	tg_file_properties    p_properties[TG_MAX_ASSET_COUNT];
} tg_assets;

tg_assets assets = { 0 };



static u32 tg__hash(const char* p_filename)
{
	const char* p_it = &p_filename[tg_string_length(p_filename) - 1];
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

static void tg__try_load(const tg_file_properties* p_properties)
{
	TG_ASSERT(assets.count < TG_MAX_ASSET_COUNT);

	const u32 hash = tg__hash(p_properties->p_filename);
	u32 index = hash % TG_MAX_ASSET_COUNT;
	while (assets.p_handles[index] != TG_NULL)
	{
		index++;
	}

	b32 loaded_successfully = TG_TRUE;

	if (tg_string_equal(p_properties->p_extension, "comp"))
	{
		assets.p_handles[index] = tg_compute_shader_create(p_properties->p_filename);
	}
	else if (tg_string_equal(p_properties->p_extension, "frag"))
	{
		assets.p_handles[index] = tg_fragment_shader_create(p_properties->p_filename);
	}
	else if (tg_string_equal(p_properties->p_extension, "vert"))
	{
		assets.p_handles[index] = tg_vertex_shader_create(p_properties->p_filename);
	}
	else
	{
		loaded_successfully = TG_FALSE;
	}

	if (loaded_successfully)
	{
		assets.p_properties[index] = *p_properties;
		assets.p_properties[index].p_short_filename = assets.p_properties[index].p_filename + (p_properties->p_short_filename - p_properties->p_filename);
		assets.p_properties[index].p_extension = assets.p_properties[index].p_filename + (p_properties->p_extension - p_properties->p_filename);
		assets.count++;
	}
}

static void tg__try_load_directory(const char* p_directory)
{
	tg_file_properties file_properties = { 0 };
	tg_file_iterator_h h_file_iterator = tg_platform_directory_begin_iteration(p_directory, &file_properties);

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
	} while (tg_platform_directory_continue_iteration(h_file_iterator, p_directory, &file_properties));
}



void tg_assets_init()
{
	TG_ASSERT(tg_platform_directory_get_size(".") <= TG_MAX_ASSETS_DIRECTORY_SIZE);
	tg__try_load_directory(".");
}

void tg_assets_shutdown()
{
	for (u32 i = 0; i < TG_MAX_ASSET_COUNT; i++)
	{
		if (assets.p_handles[i])
		{
			if (tg_string_equal(assets.p_properties[i].p_extension, "comp"))
			{
				tg_compute_shader_destroy(assets.p_handles[i]);
			}
			else if (tg_string_equal(assets.p_properties[i].p_extension, "frag"))
			{
				tg_fragment_shader_destroy(assets.p_handles[i]);
			}
			else if (tg_string_equal(assets.p_properties[i].p_extension, "vert"))
			{
				tg_vertex_shader_destroy(assets.p_handles[i]);
			}
			else
			{
				TG_INVALID_CODEPATH();
			}
		}
	}
}

tg_handle tg_assets_get_asset(const char* p_filename)
{
	char p_buffer[TG_MAX_PATH] = { 0 };
	tg_string_copy(TG_MAX_PATH, p_buffer, p_filename);
#ifdef TG_WIN32
	tg_string_replace_characters(p_buffer, '/', TG_FILE_SEPERATOR);
#else
	TG_INVALID_CODEPATH();
#endif

	const u32 hash = tg__hash(p_buffer);
	u32 index = hash % TG_MAX_ASSET_COUNT;

	while (assets.p_handles[index] && !tg_string_equal(assets.p_properties[index].p_filename, p_buffer))
	{
		index = tgm_u32_incmod(index, TG_MAX_ASSET_COUNT);
	}
	return assets.p_handles[index];
}
