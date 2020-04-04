#define _CRT_SECURE_NO_WARNINGS

#include "tg_file_io.h"

#include "tg/tg_application.h"
#include "tg/tg_common.h"
#include "tg/platform/tg_allocator.h"
#include <stdio.h>

#ifdef TG_WIN32
#define MAX_PATH_LENGTH 260
#endif

void tg_file_io_read(const char* p_filename, ui64* p_size, char** pp_data)
{
    TG_ASSERT(p_filename && p_size && pp_data);

    char path[MAX_PATH_LENGTH];
    snprintf(path, sizeof(path), "%s/%s", tg_application_get_asset_path(), p_filename);

    FILE* file = fopen(path, "rb");
    TG_ASSERT(file);

    fseek(file, 0, SEEK_END);
    *p_size = (ui64)ftell(file);
    rewind(file);

    *pp_data = TG_ALLOCATOR_ALLOCATE(*p_size);
    fread(*pp_data, 1, *p_size, file);
    fclose(file);
}

void tg_file_io_free(char* p_data)
{
    TG_ALLOCATOR_FREE(p_data);
}
