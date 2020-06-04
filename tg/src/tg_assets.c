#include "tg_assets.h"

#include "platform/tg_platform.h"
#include "util/tg_string.h"

void tg_assets_internal_extract_extension(u64 size, char* p_buffer, const char* p_filename)
{
	const u32 filename_length = tg_string_length(p_filename);

	u32 extension_length = 0;
	for (u32 i = 0; i < filename_length; i++)
	{
		if (p_filename[filename_length - 1 - i] == '.')
		{
			break;
		}
		extension_length++;
	}
	
	TG_ASSERT(extension_length < filename_length);
	TG_ASSERT(size >= (u64)extension_length + 1);

	for (u32 i = 0; i < extension_length; i++)
	{
		p_buffer[i] = p_filename[filename_length - extension_length + i];
	}
	p_buffer[extension_length] = '\0';
}

void tg_assets_internal_try_load(const tg_file_properties* p_properties)
{
	char p_buffer[TG_MAX_PATH] = { 0 };
	tg_string_format(TG_MAX_PATH, p_buffer, "%s%c%s", p_properties->p_relative_directory, tg_platform_get_file_separator(), p_properties->p_filename);
	TG_DEBUG_LOG(p_buffer);
	tg_assets_internal_extract_extension(TG_MAX_PATH, p_buffer, p_buffer);
	TG_DEBUG_LOG(p_buffer);
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
	tg_assets_internal_try_load_directory("assets");
}
