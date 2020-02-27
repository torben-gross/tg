#define _CRT_SECURE_NO_WARNINGS

#include "tg_file_io.h"

#include "tg/tg_common.h"
#include "tg/platform/tg_allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSETS_PATH "assets/"
#define ASSETS_PATH_LENGTH 7

#ifdef TG_WIN32
#define MAX_PATH_LENGTH 260
#endif

void tg_file_io_read(const char* filename, ui64* size, char** content)
{
    TG_ASSERT(filename && size && content);

    char path[MAX_PATH_LENGTH];
    memset(path, 0, MAX_PATH_LENGTH * sizeof(char));
    strcpy(path, ASSETS_PATH);
    strcpy(path + ASSETS_PATH_LENGTH, filename);

    FILE* file = fopen(path, "rb");
    TG_ASSERT(file);

    fseek(file, 0, SEEK_END);
    *size = (ui64)ftell(file);
    rewind(file);

    *content = tg_malloc(*size);
    fread(*content, 1, *size, file);
    fclose(file);
}

void tg_file_io_free(char* content)
{
    tg_free(content);
}
