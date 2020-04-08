#ifndef TG_FILE_IO_H
#define TG_FILE_IO_H

#include "tg/tg_common.h"

void    tg_file_io_read(const char* p_filename, u64* p_size, char** pp_data);
void    tg_file_io_free(char* p_data);

#endif
