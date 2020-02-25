#define _CRT_SECURE_NO_WARNINGS

#include "tg_file_io.h"

#include "tg/tg_common.h"
#include "tg/platform/tg_allocator.h"
#include <stdio.h>
#include <stdlib.h>

void tg_file_io_read(const char* filename, ui64* size, char** content)
{
    // TODO: runtime path
    TG_ASSERT(filename && size && content);
    FILE* file = fopen(filename, "rb");
    TG_ASSERT(file);
    fseek(file, 0, SEEK_END);
    *size = (ui64)ftell(file);
    rewind(file);
    *content = tg_malloc(*size + 1);
    TG_ASSERT(*content);
    fread(*content, 1, *size, file);
    fclose(file);
    (*content)[*size] = 0;
}

void tg_file_io_free(char* content)
{
	free(content);
}
