#ifndef TG_FILE_IO_H
#define TG_FILE_IO_H

#include "tg/tg_common.h"

void tg_file_io_read(const char* filename, ui64* size, char** content);

#endif
