#define _CRT_SECURE_NO_WARNINGS

#include "tg_file_io.h"

#include "tg/tg_common.h"
#include "tg/platform/tg_allocator.h"
#include <stdio.h>
#include <stdlib.h>

void tg_file_io_read(const char* filename, uint64* size, char** content)
{
    // TODO: runtime path
    ASSERT(filename && size && content);
    FILE* file = fopen(filename, "rb");
    ASSERT(file);
    fseek(file, 0, SEEK_END);
    *size = (uint64)ftell(file);
    rewind(file);
    *content = glob_alloc(*size + 1);
    ASSERT(*content);
    fread(*content, 1, *size, file);
    fclose(file);
    (*content)[*size] = 0;
}

void tg_file_io_free(char* content)
{
	free(content);
}
