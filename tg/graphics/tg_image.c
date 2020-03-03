#include "tg_image.h"

#include "tg/platform/tg_allocator.h"
#include "tg/util/tg_file_io.h"
#include <stdbool.h>
#include <string.h>

#define TG_BMP_IDENTIFIER                         'MB'
#define TG_BMP_MIN_SIZE                           (12 + 16)

#define TG_BMP_BITMAPFILEHEADER_OFFSET            0
#define TG_BMP_BITMAPINFOHEADER_OFFSET            14
#define TG_BMP_BITMAPV5HEADER_OFFSET              TG_BMP_BITMAPINFOHEADER_OFFSET

#define TG_BMP_COMPRESSION_BI_RGB                 0L
#define TG_BMP_COMPRESSION_BI_RLE8                1L
#define TG_BMP_COMPRESSION_BI_RLE4                2L
#define TG_BMP_COMPRESSION_BI_BITFIELDS           3L
#define TG_BMP_COMPRESSION_BI_JPEG                4L
#define TG_BMP_COMPRESSION_BI_PNG                 5L

#define TG_BMP_CS_TYPE_LCS_CALIBRATED_RGB         0x00000000L
#define TG_BMP_CS_TYPE_LCS_sRGB                   'sRGB'
#define TG_BMP_CS_TYPE_LCS_WINDOWS_COLOR_SPACE    'Win '
#define TG_BMP_CS_TYPE_PROFILE_LINKED             'LINK'
#define TG_BMP_CS_TYPE_PROFILE_EMBEDDED           'MBED'

#define TG_BMP_INTENT_LCS_GM_BUSINESS             0x00000001L
#define TG_BMP_INTENT_LCS_GM_GRAPHICS             0x00000002L
#define TG_BMP_INTENT_LCS_GM_IMAGES               0x00000004L
#define TG_BMP_INTENT_LCS_GM_ABS_COLORIMETRIC     0x00000008L

typedef struct tg_image
{
	ui32               width;
	ui32               height;
	ui32               r_mask;
	ui32               g_mask;
	ui32               b_mask;
	ui32               a_mask;
	tg_image_format    format;
	ui32               data[0];
} tg_image;

typedef struct tg_bmp_bitmapfileheader
{
	ui16    type;
	ui32    size;
	ui16    reserved1;
	ui16    reserved2;
	ui32    offset_bits;
} tg_bmp_bitmapfileheader;

typedef enum tg_bmp_bitmapinfoheader_type
{
	TG_BMP_BITMAPINFOHEADER_TYPE_BITMAPCOREHEADER      = 12,
	TG_BMP_BITMAPINFOHEADER_TYPE_OS21XBITMAPHEADER     = 12,
	TG_BMP_BITMAPINFOHEADER_TYPE_OS22XBITMAPHEADER     = 64,
	TG_BMP_BITMAPINFOHEADER_TYPE_BITMAPINFOHEADER      = 40,
	TG_BMP_BITMAPINFOHEADER_TYPE_BITMAPV2INFOHEADER    = 52,
	TG_BMP_BITMAPINFOHEADER_TYPE_BITMAPV3INFOHEADER    = 56,
	TG_BMP_BITMAPINFOHEADER_TYPE_BITMAPV4HEADER        = 108,
	TG_BMP_BITMAPINFOHEADER_TYPE_BITMAPV5HEADER        = 124
} tg_bmp_bitmapinfoheader_type;

typedef struct tg_bmp_fxpt_2dot30
{
	i32    x;
	i32    y;
	i32    z;
} tg_bmp_fxpt_2dot30;

typedef struct tg_bmp_ciexyz
{
	tg_bmp_fxpt_2dot30    ciexyz_x;
	tg_bmp_fxpt_2dot30    ciexyz_y;
	tg_bmp_fxpt_2dot30    ciexyz_z;
} tg_bmp_ciexyz;

typedef struct tg_ciexyz_triple
{
	tg_bmp_ciexyz    ciexyz_red;
	tg_bmp_ciexyz    ciexyz_green;
	tg_bmp_ciexyz    ciexyz_blue;
} tg_ciexyz_triple;

typedef struct tg_bitmapinfoheader
{
	ui32    size;
	i32     width;
	i32     height;
	ui16    planes;
	ui16    bit_count;
	ui32    compression;
	ui32    size_image;
	i32     x_pels_per_meter;
	i32     y_pels_per_meter;
	ui32    clr_used;
	ui32    clr_important;
} tg_bitmapinfoheader;

typedef struct tg_bitmapv5header
{
	union
	{
		tg_bitmapinfoheader    bitmapinfoheader;
		struct
		{
			ui32                   size;
			i32                    width;
			i32                    height;
			ui16                   planes;
			ui16                   bit_count;
			ui32                   compression;
			ui32                   size_image;
			i32                    x_pels_per_meter;
			i32                    y_pels_per_meter;
			ui32                   clr_used;
			ui32                   clr_important;
		};
	};
	ui32                           r_mask;
	ui32                           g_mask;
	ui32                           b_mask;
	ui32                           a_mask;
	ui32                           cs_type;
	tg_ciexyz_triple               endpoints;
	ui32                           gamma_red;
	ui32                           gamma_green;
	ui32                           gamma_blue;
	ui32                           intent;
	ui32                           profile_data;
	ui32                           profile_size;
	ui32                           reserved;
} tg_bitmapv5header;



void tg_image_convert_format_to_masks(tg_image_format format, ui32* r_mask, ui32* g_mask, ui32* b_mask, ui32* a_mask);
void tg_image_convert_masks_to_format(ui32 r_mask, ui32 g_mask, ui32 b_mask, ui32 a_mask, tg_image_format* format);
void tg_image_load_bmp_from_memory(tg_image_h* p_handle, ui64 size, const char* memory);



void tg_image_create(tg_image_h* p_handle, const char* filename)
{
	TG_ASSERT(p_handle && filename);

	ui64 size = 0;
	char* memory = NULL;
	tg_file_io_read(filename, &size, &memory);

	if (size >= 2 && *(ui16*)memory == TG_BMP_IDENTIFIER)
	{
		tg_image_load_bmp_from_memory(p_handle, size, memory);
	}

	tg_file_io_free(memory);
}
void tg_image_destroy(tg_image_h handle)
{
	tg_free(handle);
}

void tg_image_get_width(tg_image_h handle, ui32* p_width)
{
	*p_width = handle->width;
}
void tg_image_get_height(tg_image_h handle, ui32* p_height)
{
	*p_height = handle->height;
}
void tg_image_get_dimensions(tg_image_h handle, ui32* p_width, ui32* p_height)
{
	*p_width = handle->width;
	*p_height = handle->height;
}
void tg_image_get_format(tg_image_h handle, tg_image_format* p_format)
{
	*p_format = handle->format;
}
void tg_image_get_data_size(tg_image_h handle, ui64* p_size)
{
	*p_size = (ui64)handle->width * (ui64)handle->height * (ui64)sizeof(*handle->data);
}
void tg_image_get_data(tg_image_h handle, ui32** p_data)
{
	*p_data = handle->data;
}

void tg_image_convert_to_format(tg_image_h handle, tg_image_format format)
{
	const ui32 old_r_mask = handle->r_mask;
	const ui32 old_g_mask = handle->g_mask;
	const ui32 old_b_mask = handle->b_mask;
	const ui32 old_a_mask = handle->a_mask;

	tg_image_convert_format_to_masks(format, &handle->r_mask, &handle->g_mask, &handle->b_mask, &handle->a_mask);

	const ui32 new_r_mask = handle->r_mask;
	const ui32 new_g_mask = handle->g_mask;
	const ui32 new_b_mask = handle->b_mask;
	const ui32 new_a_mask = handle->a_mask;

	handle->format = format;

	ui32* pixel = handle->data;
	for (ui32 col = 0; col < handle->width; col++)
	{
		for (ui32 row = 0; row < handle->height; row++)
		{
			ui32 r = 0x00000000;
			ui32 g = 0x00000000;
			ui32 b = 0x00000000;
			ui32 a = 0x00000000;
			
			ui32 r_bit = 0x00000000;
			ui32 g_bit = 0x00000000;
			ui32 b_bit = 0x00000000;
			ui32 a_bit = 0x00000000;

			for (ui8 i = 0; i < 32; i++)
			{
				if (old_r_mask & (1 << i))
				{
					r |= (*pixel & (1 << i)) >> (i - r_bit++);
				}
				if (old_g_mask & (1 << i))
				{
					g |= (*pixel & (1 << i)) >> (i - g_bit++);
				}
				if (old_b_mask & (1 << i))
				{
					b |= (*pixel & (1 << i)) >> (i - b_bit++);
				}
				if (old_a_mask & (1 << i))
				{
					a |= (*pixel & (1 << i)) >> (i - a_bit++);
				}
			}

			*pixel = 0x00000000;

			for (ui8 i = 0; i < 32; i++)
			{
				if (new_r_mask & (1 << i))
				{
					ui32 bit = (r & 0x00000001) << i;
					r = r >> 0x00000001;
					*pixel |= bit;
				}
				if (new_g_mask & (1 << i))
				{
					ui32 bit = (g & 0x00000001) << i;
					g = g >> 0x00000001;
					*pixel |= bit;
				}
				if (new_b_mask & (1 << i))
				{
					ui32 bit = (b & 0x00000001) << i;
					b = b >> 0x00000001;
					*pixel |= bit;
				}
				if (new_a_mask & (1 << i))
				{
					ui32 bit = (a & 0x00000001) << i;
					a = a >> 0x00000001;
					*pixel |= bit;
				}
			}

			ui8 rrr = *(ui8*)pixel;

			pixel++;
		}
	}
}

void tg_image_convert_masks_to_format(ui32 r_mask, ui32 g_mask, ui32 b_mask, ui32 a_mask, tg_image_format* format)
{
	TG_ASSERT(r_mask | g_mask | b_mask | a_mask);

	const ui32 mask_0x000000ff = 0x000000ff;
	const ui32 mask_0x0000ff00 = 0x0000ff00;
	const ui32 mask_0x00ff0000 = 0x00ff0000;
	const ui32 mask_0xff000000 = 0xff000000;

	const bool r_0xff000000 = (r_mask & mask_0xff000000) == mask_0xff000000;
	const bool r_0x00ff0000 = (r_mask & mask_0x00ff0000) == mask_0x00ff0000;
	const bool r_0x0000ff00 = (r_mask & mask_0x0000ff00) == mask_0x0000ff00;
	const bool r_0x000000ff = (r_mask & mask_0x000000ff) == mask_0x000000ff;

	const bool g_0xff000000 = (g_mask & mask_0xff000000) == mask_0xff000000;
	const bool g_0x00ff0000 = (g_mask & mask_0x00ff0000) == mask_0x00ff0000;
	const bool g_0x0000ff00 = (g_mask & mask_0x0000ff00) == mask_0x0000ff00;
	const bool g_0x000000ff = (g_mask & mask_0x000000ff) == mask_0x000000ff;

	const bool b_0xff000000 = (b_mask & mask_0xff000000) == mask_0xff000000;
	const bool b_0x00ff0000 = (b_mask & mask_0x00ff0000) == mask_0x00ff0000;
	const bool b_0x0000ff00 = (b_mask & mask_0x0000ff00) == mask_0x0000ff00;
	const bool b_0x000000ff = (b_mask & mask_0x000000ff) == mask_0x000000ff;

	const bool a_0xff000000 = (a_mask & mask_0xff000000) == mask_0xff000000;
	const bool a_0x00ff0000 = (a_mask & mask_0x00ff0000) == mask_0x00ff0000;
	const bool a_0x0000ff00 = (a_mask & mask_0x0000ff00) == mask_0x0000ff00;
	const bool a_0x000000ff = (a_mask & mask_0x000000ff) == mask_0x000000ff;

	if (a_0x000000ff && r_0x0000ff00 && g_0x00ff0000 && b_0xff000000)
	{
		*format = TG_IMAGE_FORMAT_A8R8G8B8;
	}
	else if (a_0x000000ff && b_0x0000ff00 && g_0x00ff0000 && r_0xff000000)
	{
		*format = TG_IMAGE_FORMAT_A8B8G8R8;
	}
	else if (b_0x000000ff && g_0x0000ff00 && r_0x00ff0000 && a_0xff000000)
	{
		*format = TG_IMAGE_FORMAT_B8G8R8A8;
	}
	else if (r_0x000000ff && !g_0x0000ff00 && !b_0x00ff0000 && !a_0xff000000)
	{
		*format = TG_IMAGE_FORMAT_R8;
	}
	else if (r_0x000000ff && g_0x0000ff00 && !b_0x00ff0000 && !a_0xff000000)
	{
		*format = TG_IMAGE_FORMAT_R8G8;
	}
	else if (r_0x000000ff && g_0x0000ff00 && b_0x00ff0000 && !a_0xff000000)
	{
		*format = TG_IMAGE_FORMAT_R8G8B8;
	}
	else if (r_0x000000ff && g_0x0000ff00 && b_0x00ff0000 && a_0xff000000)
	{
		*format = TG_IMAGE_FORMAT_R8G8B8A8;
	}
	else
	{
		*format = TG_IMAGE_FORMAT_NONE;
		TG_ASSERT(false);
	}
}
void tg_image_convert_format_to_masks(tg_image_format format, ui32* r_mask, ui32* g_mask, ui32* b_mask, ui32* a_mask)
{
	switch (format)
	{
	case TG_IMAGE_FORMAT_A8R8G8B8:
	{
		*r_mask = 0x0000ff00;
		*g_mask = 0x00ff0000;
		*b_mask = 0xff000000;
		*a_mask = 0x000000ff;
	} break;
	case TG_IMAGE_FORMAT_A8B8G8R8:
	{
		*r_mask = 0xff000000;
		*g_mask = 0x00ff0000;
		*b_mask = 0x0000ff00;
		*a_mask = 0x000000ff;
	} break;
	case TG_IMAGE_FORMAT_R8:
	{
		*r_mask = 0x000000ff;
		*g_mask = 0x00000000;
		*b_mask = 0x00000000;
		*a_mask = 0x00000000;
	} break;
	case TG_IMAGE_FORMAT_R8G8:
	{
		*r_mask = 0x000000ff;
		*g_mask = 0x0000ff00;
		*b_mask = 0x00000000;
		*a_mask = 0x00000000;
	} break;
	case TG_IMAGE_FORMAT_R8G8B8:
	{
		*r_mask = 0x000000ff;
		*g_mask = 0x0000ff00;
		*b_mask = 0x00ff0000;
		*a_mask = 0x00000000;
	} break;
	case TG_IMAGE_FORMAT_R8G8B8A8:
	{
		*r_mask = 0x000000ff;
		*g_mask = 0x0000ff00;
		*b_mask = 0x00ff0000;
		*a_mask = 0xff000000;
	} break;
	case TG_IMAGE_FORMAT_NONE:
	{
		*r_mask = 0x00000000;
		*g_mask = 0x00000000;
		*b_mask = 0x00000000;
		*a_mask = 0x00000000;
		TG_ASSERT(false);
	} break;
	}
}
void tg_image_load_bmp_from_memory(tg_image_h* p_handle, ui64 size, const char* memory)
{
	TG_ASSERT(p_handle && memory);
	TG_ASSERT(size >= TG_BMP_MIN_SIZE && *(ui16*)memory == TG_BMP_IDENTIFIER);

	const char* bitmapfileheader_ptr = memory + TG_BMP_BITMAPFILEHEADER_OFFSET;
	tg_bmp_bitmapfileheader bitmapfileheader = { 0 };
	bitmapfileheader.type = *(ui16*)(bitmapfileheader_ptr + 0);
	bitmapfileheader.size = *(ui32*)(bitmapfileheader_ptr + 2);
	bitmapfileheader.reserved1 = *(ui16*)(bitmapfileheader_ptr + 6);
	bitmapfileheader.reserved2 = *(ui16*)(bitmapfileheader_ptr + 8);
	bitmapfileheader.offset_bits = *(ui32*)(bitmapfileheader_ptr + 10);

	const char* bitmapinfoheader_ptr = memory + TG_BMP_BITMAPINFOHEADER_OFFSET;
	tg_bitmapinfoheader bitmapinfoheader = { 0 };
	bitmapinfoheader.size = *(ui32*)(bitmapinfoheader_ptr + 0);
	bitmapinfoheader.width = *(i32*)(bitmapinfoheader_ptr + 4);
	bitmapinfoheader.height = *(i32*)(bitmapinfoheader_ptr + 8);
	bitmapinfoheader.planes = *(ui16*)(bitmapinfoheader_ptr + 12);
	bitmapinfoheader.bit_count = *(ui16*)(bitmapinfoheader_ptr + 14);
	bitmapinfoheader.compression = *(ui32*)(bitmapinfoheader_ptr + 16);
	bitmapinfoheader.size_image = *(ui32*)(bitmapinfoheader_ptr + 20);
	bitmapinfoheader.x_pels_per_meter = *(i32*)(bitmapinfoheader_ptr + 24);
	bitmapinfoheader.y_pels_per_meter = *(i32*)(bitmapinfoheader_ptr + 28);
	bitmapinfoheader.clr_used = *(ui32*)(bitmapinfoheader_ptr + 32);
	bitmapinfoheader.clr_important = *(ui32*)(bitmapinfoheader_ptr + 36);

	TG_ASSERT(size >= (ui64)bitmapfileheader.offset_bits + ((ui64)bitmapinfoheader.width * (ui64)bitmapinfoheader.height * (ui64)sizeof(ui32)));
	TG_ASSERT((tg_bmp_bitmapinfoheader_type)bitmapinfoheader.size == TG_BMP_BITMAPINFOHEADER_TYPE_BITMAPV5HEADER);
	TG_ASSERT(bitmapinfoheader.compression == TG_BMP_COMPRESSION_BI_BITFIELDS);

	tg_bitmapv5header bitmapv5header = { 0 };
	bitmapv5header.bitmapinfoheader = bitmapinfoheader;
	bitmapv5header.r_mask = *(ui32*)(bitmapinfoheader_ptr + 40);
	bitmapv5header.g_mask = *(ui32*)(bitmapinfoheader_ptr + 44);
	bitmapv5header.b_mask = *(ui32*)(bitmapinfoheader_ptr + 48);
	bitmapv5header.a_mask = *(ui32*)(bitmapinfoheader_ptr + 52);
	bitmapv5header.cs_type = *(ui32*)(bitmapinfoheader_ptr + 56);
	bitmapv5header.endpoints = *(tg_ciexyz_triple*)(bitmapinfoheader_ptr + 60);
	bitmapv5header.gamma_red = *(ui32*)(bitmapinfoheader_ptr + 96);
	bitmapv5header.gamma_green = *(ui32*)(bitmapinfoheader_ptr + 100);
	bitmapv5header.gamma_blue = *(ui32*)(bitmapinfoheader_ptr + 104);
	bitmapv5header.intent = *(ui32*)(bitmapinfoheader_ptr + 108);
	bitmapv5header.profile_data = *(ui32*)(bitmapinfoheader_ptr + 112);
	bitmapv5header.profile_size = *(ui32*)(bitmapinfoheader_ptr + 116);
	bitmapv5header.reserved = *(ui32*)(bitmapinfoheader_ptr + 120);

	TG_ASSERT(bitmapv5header.cs_type == TG_BMP_CS_TYPE_LCS_WINDOWS_COLOR_SPACE);

	const ui64 data_size = (ui64)bitmapv5header.width * (ui64)bitmapv5header.height * (ui64)sizeof(*(*p_handle)->data);
	const ui64 image_size = (ui64)sizeof(**p_handle) + data_size;
	*p_handle = tg_malloc(image_size);

	(*p_handle)->width = bitmapv5header.width;
	(*p_handle)->height = bitmapv5header.height;
	(*p_handle)->r_mask = bitmapv5header.r_mask;
	(*p_handle)->g_mask = bitmapv5header.g_mask;
	(*p_handle)->b_mask = bitmapv5header.b_mask;
	(*p_handle)->a_mask = bitmapv5header.a_mask;
	tg_image_convert_masks_to_format(bitmapv5header.r_mask, bitmapv5header.g_mask, bitmapv5header.b_mask, bitmapv5header.a_mask, &(*p_handle)->format);
	memcpy((*p_handle)->data, (ui32*)(memory + bitmapfileheader.offset_bits), data_size);
}
