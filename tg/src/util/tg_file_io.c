#include "util/tg_file_io.h"

#include "memory/tg_memory.h"
#include "tg_application.h"
#include "tg_common.h"
#include "util/tg_string.h"
#include <stdio.h>
#include <sys/stat.h>

#ifdef TG_WIN32
#define MAX_PATH_LENGTH 260
#endif

void tg_file_io_internal_prepend_asset_path(const char* p_filename, u32 buffer_size, char* p_buffer)
{
    tg_string_format(buffer_size, p_buffer, "%s/%s", tg_application_get_asset_path(), p_filename);
}

void tg_file_io_read(const char* p_filename, u64* p_size, char** pp_data)
{
    TG_ASSERT(p_filename && p_size && pp_data);

    char p_path[MAX_PATH_LENGTH] = { 0 };
    tg_file_io_internal_prepend_asset_path(p_filename, sizeof(p_path), p_path);

    FILE* file = fopen(p_path, "rb");
    TG_ASSERT(file);

    fseek(file, 0, SEEK_END);
    *p_size = (u64)ftell(file);
    rewind(file);

    *pp_data = TG_MEMORY_ALLOC(*p_size);
    fread(*pp_data, 1, *p_size, file);
    fclose(file);
}

void tg_file_io_free(char* p_data)
{
    TG_ASSERT(p_data);

    TG_MEMORY_FREE(p_data);
}

b32 tg_file_io_exists(const char* p_filename)
{
    TG_ASSERT(p_filename);

    char p_path[MAX_PATH_LENGTH] = { 0 };
    tg_file_io_internal_prepend_asset_path(p_filename, sizeof(p_path), p_path);

    struct stat buffer;
    return (stat(p_path, &buffer) == 0);
}
