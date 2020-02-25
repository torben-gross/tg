#include "tg_image.h"

#include "tg/util/tg_file_io.h"
#include "tg/platform/tg_allocator.h"

#define BITMAP_FILE_HEADER_SIZE 14

void tgi_load_bmp(const char* filename, ui32* width, ui32* height, f32* data)
{
	TG_ASSERT(filename && width && height);

	if (!data)
	{
		const ui64 bitmap_file_header_size = BITMAP_FILE_HEADER_SIZE;
		char bitmap_file_header[BITMAP_FILE_HEADER_SIZE];
		char* bfh = bitmap_file_header;
		tg_file_io_read(filename, &bitmap_file_header_size, &bfh);

		ui64 bmp_size = (ui64)*(ui32*)(bfh + 2);
		char* bmp_content = tg_malloc(bmp_size);
		tg_file_io_read(filename, &bmp_size, &bmp_content);
		tg_free(bmp_content);
	}
}
