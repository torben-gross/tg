#include "tg_assets.h"

#include "graphics/tg_graphics.h"
#include "platform/tg_platform.h"
#include "util/tg_string.h"



#define TG_ASSETS_PATH                  "assets"
#define TG_MAX_ASSET_COUNT              1024
#define TG_MAX_ASSETS_DIRECTORY_SIZE    1073741824



typedef struct tg_assets
{
	u32                   count;
	tg_handle             p_handles[TG_MAX_ASSET_COUNT];
	tg_file_properties    p_properties[TG_MAX_ASSET_COUNT];
} tg_assets;

tg_assets assets = { 0 };



const char* tg_assets_internal_extract_filename_extension(const char* p_filename)
{
	const char* p_it = &p_filename[tg_string_length(p_filename) - 1];
	while (*p_it != '.')
	{
		if (p_it-- == p_filename)
		{
			break;
		}
	}
	p_it++;
	return p_it;
}

u32 tg_assets_internal_hash(const char* p_filename)
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

const char* tg_assets_internal_remove_filename_prefix(const char* p_filename)
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
	return p_it;
}



void tg_assets_internal_try_load(const tg_file_properties* p_properties)
{
	TG_ASSERT(assets.count < TG_MAX_ASSET_COUNT);

	char p_filename_buffer[TG_MAX_PATH] = { 0 };
	tg_string_format(TG_MAX_PATH, p_filename_buffer, "%s%c%s", p_properties->p_relative_directory, tg_platform_get_file_separator(), p_properties->p_filename);
	TG_DEBUG_LOG(p_filename_buffer);

	const char* p_file_extension = tg_assets_internal_extract_filename_extension(p_properties->p_filename);
	TG_DEBUG_LOG(p_file_extension);

	const u32 hash = tg_assets_internal_hash(p_properties->p_filename);
	u32 index = hash % TG_MAX_ASSET_COUNT;
	while (assets.p_handles[index] != TG_NULL)
	{
		index++;
	}

	// TODO: this has to go!
	const u32 prefix_size = 7;
	for (u32 i = 0; i < TG_MAX_PATH; i++)
	{
		p_filename_buffer[i] = p_filename_buffer[i + prefix_size];
		if (p_filename_buffer[i + prefix_size] == '\0')
		{
			break;
		}
	}

	b32 loaded_successfully = TG_TRUE;

	if (tg_string_equal(p_file_extension, "frag"))
	{
		assets.p_handles[index] = tg_fragment_shader_create(p_filename_buffer);
	}
	else if (tg_string_equal(p_file_extension, "vert"))
	{
		assets.p_handles[index] = tg_vertex_shader_create(p_filename_buffer);
	}
	else
	{
		loaded_successfully = TG_FALSE;
	}

	if (loaded_successfully)
	{
		assets.p_properties[index] = *p_properties;
		assets.count++;
	}
}

void tg_assets_internal_try_load_directory(const char* p_relative_directory)
{
	tg_file_properties file_properties = { 0 };
	tg_file_iterator_h file_iterator_h = tg_platform_begin_file_iteration(p_relative_directory, &file_properties);

	do
	{
		if (file_properties.is_directory)
		{
			char p_buffer[TG_MAX_PATH] = { 0 };
			tg_string_format(TG_MAX_PATH, p_buffer, "%s%c%s", p_relative_directory, tg_platform_get_file_separator(), file_properties.p_filename);
			tg_assets_internal_try_load_directory(p_buffer);
		}
		else
		{
			tg_assets_internal_try_load(&file_properties);
		}
	} while (tg_platform_continue_file_iteration(p_relative_directory, file_iterator_h, &file_properties));
}



void tg_assets_init()
{
	TG_ASSERT(tg_platform_get_full_directory_size(TG_ASSETS_PATH) <= TG_MAX_ASSETS_DIRECTORY_SIZE);
	tg_assets_internal_try_load_directory(TG_ASSETS_PATH);
}

void tg_assets_shutdown()
{
	for (u32 i = 0; i < TG_MAX_ASSET_COUNT; i++)
	{
		if (assets.p_handles[i])
		{
			const char* p_file_extension = tg_assets_internal_extract_filename_extension(assets.p_properties[i].p_filename);
			if (tg_string_equal(p_file_extension, "frag"))
			{
				tg_fragment_shader_destroy(assets.p_handles[i]);
			}
			else if (tg_string_equal(p_file_extension, "vert"))
			{
				tg_vertex_shader_destroy(assets.p_handles[i]);
			}
			else
			{
				TG_ASSERT(TG_FALSE);
			}
		}
	}
}

tg_handle tg_get_asset(const char* p_filename)
{
	char p_filename_buffer[TG_MAX_PATH] = { 0 };
	tg_string_format(TG_MAX_PATH, p_filename_buffer, "%s%c%s", TG_ASSETS_PATH, tg_platform_get_file_separator(), p_filename);
	const char* p_short_filename = tg_assets_internal_remove_filename_prefix(p_filename);

	const u32 hash = tg_assets_internal_hash(p_filename_buffer);
	u32 index = hash % TG_MAX_ASSET_COUNT;

	while (assets.p_handles[index] && !tg_string_equal(assets.p_properties[index].p_filename, p_short_filename))
	{
		index = tgm_u32_incmod(index, TG_MAX_ASSET_COUNT);
	}
	return assets.p_handles[index];
}
