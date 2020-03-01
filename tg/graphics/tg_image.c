#include "tg_image.h"

#include "tg/platform/tg_allocator.h"
#include "tg/util/tg_file_io.h"
#include <string.h>

#define TGI_BMP_IDENTIFIER                      'MB'
#define TGI_BMP_MIN_SIZE                        (12 + 16)

#define TGI_BMP_BITMAPFILEHEADER_OFFSET         0
#define TGI_BMP_BITMAPINFOHEADER_OFFSET         14
#define TGI_BMP_BITMAPV5HEADER_OFFSET           TGI_BMP_BITMAPINFOHEADER_OFFSET

#define TGI_BMP_COMPRESSION_BI_RGB              0L
#define TGI_BMP_COMPRESSION_BI_RLE8             1L
#define TGI_BMP_COMPRESSION_BI_RLE4             2L
#define TGI_BMP_COMPRESSION_BI_BITFIELDS        3L
#define TGI_BMP_COMPRESSION_BI_JPEG             4L
#define TGI_BMP_COMPRESSION_BI_PNG              5L

#define TGI_BMP_CS_TYPE_LCS_CALIBRATED_RGB      0x00000000L
#define TGI_BMP_CS_TYPE_LCS_sRGB                'sRGB'
#define TGI_BMP_CS_TYPE_LCS_WINDOWS_COLOR_SPACE 'Win '
#define TGI_BMP_CS_TYPE_PROFILE_LINKED          'LINK'
#define TGI_BMP_CS_TYPE_PROFILE_EMBEDDED        'MBED'

#define TGI_BMP_INTENT_LCS_GM_BUSINESS          0x00000001L
#define TGI_BMP_INTENT_LCS_GM_GRAPHICS          0x00000002L
#define TGI_BMP_INTENT_LCS_GM_IMAGES            0x00000004L
#define TGI_BMP_INTENT_LCS_GM_ABS_COLORIMETRIC  0x00000008L

typedef struct tgi_bmp_bitmapfileheader
{
	ui16 type;
	ui32 size;
	ui16 reserved1;
	ui16 reserved2;
	ui32 offset_bits;
} tgi_bmp_bitmapfileheader;

typedef enum tgi_bmp_bitmapinfoheader_type
{
	TGI_BMP_BITMAPINFOHEADER_TYPE_BITMAPCOREHEADER   = 12,
	TGI_BMP_BITMAPINFOHEADER_TYPE_OS21XBITMAPHEADER  = 12,
	TGI_BMP_BITMAPINFOHEADER_TYPE_OS22XBITMAPHEADER  = 64,
	TGI_BMP_BITMAPINFOHEADER_TYPE_BITMAPINFOHEADER   = 40,
	TGI_BMP_BITMAPINFOHEADER_TYPE_BITMAPV2INFOHEADER = 52,
	TGI_BMP_BITMAPINFOHEADER_TYPE_BITMAPV3INFOHEADER = 56,
	TGI_BMP_BITMAPINFOHEADER_TYPE_BITMAPV4HEADER     = 108,
	TGI_BMP_BITMAPINFOHEADER_TYPE_BITMAPV5HEADER     = 124
} tgi_bmp_bitmapinfoheader_type;

typedef struct tgi_bmp_fxpt_2dot30
{
	i32 x;
	i32 y;
	i32 z;
} tgi_bmp_fxpt_2dot30;

typedef struct tgi_bmp_ciexyz
{
	tgi_bmp_fxpt_2dot30 ciexyz_x;
	tgi_bmp_fxpt_2dot30 ciexyz_y;
	tgi_bmp_fxpt_2dot30 ciexyz_z;
} tgi_bmp_ciexyz;

typedef struct tgi_ciexyz_triple
{
	tgi_bmp_ciexyz ciexyz_red;
	tgi_bmp_ciexyz ciexyz_green;
	tgi_bmp_ciexyz ciexyz_blue;
} tgi_ciexyz_triple;

typedef struct tgi_bmp_bitmapinfoheader
{
	ui32 size;
	i32  width;
	i32  height;
	ui16 planes;
	ui16 bit_count;
	ui32 compression;
	ui32 size_image;
	i32  x_pels_per_meter;
	i32  y_pels_per_meter;
	ui32 clr_used;
	ui32 clr_important;
} tgi_bmp_bitmapinfoheader;

typedef struct tgi_bmp_bitmapv5header
{
	union
	{
		tgi_bmp_bitmapinfoheader bitmapinfoheader;
		struct
		{
			ui32 size;
			i32  width;
			i32  height;
			ui16 planes;
			ui16 bit_count;
			ui32 compression;
			ui32 size_image;
			i32  x_pels_per_meter;
			i32  y_pels_per_meter;
			ui32 clr_used;
			ui32 clr_important;
		};
	};
	ui32              red_mask;
	ui32              green_mask;
	ui32              blue_mask;
	ui32              alpha_mask;
	ui32              cs_type;
	tgi_ciexyz_triple endpoints;
	ui32              gamma_red;
	ui32              gamma_green;
	ui32              gamma_blue;
	ui32              intent;
	ui32              profile_data;
	ui32              profile_size;
	ui32              reserved;
} tgi_bmp_bitmapv5header;

void tgi_load_bmp_from_memory(ui64 size, const char* memory, ui32* width, ui32* height, tgi_pixel** data);

void tgi_load(const char* filename, ui32* width, ui32* height, tgi_pixel** data)
{
	TG_ASSERT(filename && width && height && data);

	ui64 size = 0;
	char* memory = NULL;
	tg_file_io_read(filename, &size, &memory);

	if (size >= 2 && *(ui16*)memory == TGI_BMP_IDENTIFIER)
	{
		tgi_load_bmp_from_memory(size, memory, width, height, data);
	}

	tg_file_io_free(memory);
}

void tgi_load_bmp(const char* filename, ui32* width, ui32* height, tgi_pixel** data)
{
	TG_ASSERT(filename && width && height && data);

	ui64 size = 0;
	char* memory = NULL;
	tg_file_io_read(filename, &size, &memory);
	tgi_load_bmp_from_memory(size, memory, width, height, data);
	tg_file_io_free(memory);
}

void tgi_load_bmp_from_memory(ui64 size, const char* memory, ui32* width, ui32* height, tgi_pixel** data)
{
	TG_ASSERT(size >= TGI_BMP_MIN_SIZE && *(ui16*)memory == TGI_BMP_IDENTIFIER);
	
	const char* bitmapfileheader_ptr = memory + TGI_BMP_BITMAPFILEHEADER_OFFSET;
	tgi_bmp_bitmapfileheader bitmapfileheader = { 0 };
	bitmapfileheader.type                     = *(ui16*)(bitmapfileheader_ptr +  0);
	bitmapfileheader.size                     = *(ui32*)(bitmapfileheader_ptr +  2);
	bitmapfileheader.reserved1                = *(ui16*)(bitmapfileheader_ptr +  6);
	bitmapfileheader.reserved2                = *(ui16*)(bitmapfileheader_ptr +  8);
	bitmapfileheader.offset_bits              = *(ui32*)(bitmapfileheader_ptr + 10);

	const char* bitmapinfoheader_ptr = memory + TGI_BMP_BITMAPINFOHEADER_OFFSET;
	tgi_bmp_bitmapinfoheader bitmapinfoheader = { 0 };
	bitmapinfoheader.size                     = *(ui32*)(bitmapinfoheader_ptr +  0);
	bitmapinfoheader.width                    = *( i32*)(bitmapinfoheader_ptr +  4);
	bitmapinfoheader.height                   = *( i32*)(bitmapinfoheader_ptr +  8);
	bitmapinfoheader.planes                   = *(ui16*)(bitmapinfoheader_ptr + 12);
	bitmapinfoheader.bit_count                = *(ui16*)(bitmapinfoheader_ptr + 14);
	bitmapinfoheader.compression              = *(ui32*)(bitmapinfoheader_ptr + 16);
	bitmapinfoheader.size_image               = *(ui32*)(bitmapinfoheader_ptr + 20);
	bitmapinfoheader.x_pels_per_meter         = *( i32*)(bitmapinfoheader_ptr + 24);
	bitmapinfoheader.y_pels_per_meter         = *( i32*)(bitmapinfoheader_ptr + 28);
	bitmapinfoheader.clr_used                 = *(ui32*)(bitmapinfoheader_ptr + 32);
	bitmapinfoheader.clr_important            = *(ui32*)(bitmapinfoheader_ptr + 36);

	TG_ASSERT(size >= (ui64)bitmapfileheader.offset_bits + ((ui64)bitmapinfoheader.width * (ui64)bitmapinfoheader.height * (ui64)sizeof(ui32)));
	TG_ASSERT((tgi_bmp_bitmapinfoheader_type)bitmapinfoheader.size == TGI_BMP_BITMAPINFOHEADER_TYPE_BITMAPV5HEADER);
	TG_ASSERT(bitmapinfoheader.compression == TGI_BMP_COMPRESSION_BI_BITFIELDS);

	tgi_bmp_bitmapv5header bitmapv5header = { 0 };
	bitmapv5header.bitmapinfoheader       = bitmapinfoheader;
	bitmapv5header.red_mask               = *(ui32*)             (bitmapinfoheader_ptr +  40);
	bitmapv5header.green_mask             = *(ui32*)             (bitmapinfoheader_ptr +  44);
	bitmapv5header.blue_mask              = *(ui32*)             (bitmapinfoheader_ptr +  48);
	bitmapv5header.alpha_mask             = *(ui32*)             (bitmapinfoheader_ptr +  52);
	bitmapv5header.cs_type                = *(ui32*)             (bitmapinfoheader_ptr +  56);
	bitmapv5header.endpoints              = *(tgi_ciexyz_triple*)(bitmapinfoheader_ptr +  60);
	bitmapv5header.gamma_red              = *(ui32*)             (bitmapinfoheader_ptr +  96);
	bitmapv5header.gamma_green            = *(ui32*)             (bitmapinfoheader_ptr + 100);
	bitmapv5header.gamma_blue             = *(ui32*)             (bitmapinfoheader_ptr + 104);
	bitmapv5header.intent                 = *(ui32*)             (bitmapinfoheader_ptr + 108);
	bitmapv5header.profile_data           = *(ui32*)             (bitmapinfoheader_ptr + 112);
	bitmapv5header.profile_size           = *(ui32*)             (bitmapinfoheader_ptr + 116);
	bitmapv5header.reserved               = *(ui32*)             (bitmapinfoheader_ptr + 120);

	TG_ASSERT(bitmapv5header.cs_type == TGI_BMP_CS_TYPE_LCS_WINDOWS_COLOR_SPACE);

	*width = bitmapinfoheader.width;
	*height = bitmapinfoheader.height;

	const ui32* bmp_pixel_array = (ui32*)(memory + bitmapfileheader.offset_bits);
	const ui32 pixels_size = bitmapv5header.width * bitmapv5header.height * sizeof(tgi_pixel);
	*data = tg_malloc(pixels_size);
	memset(*data, 0, pixels_size);

	for (i32 col = 0; col < bitmapv5header.width; col++)
	{
		for (i32 row = 0; row < bitmapv5header.height; row++)
		{
			const ui32* bmp_pixel = &bmp_pixel_array[col * bitmapv5header.width + row];
			tgi_pixel* pixel = &(*data)[col * bitmapv5header.width + row];

			ui32 r_bit = 0;
			ui32 g_bit = 0;
			ui32 b_bit = 0;
			ui32 a_bit = 0;
			for (ui8 i = 0; i < 32; i++)
			{
				if (bitmapv5header.red_mask & (1 << i))
				{
					pixel->r |= (*bmp_pixel & (1 << i)) >> (i - r_bit++);
				}
				if (bitmapv5header.green_mask & (1 << i))
				{
					pixel->g |= (*bmp_pixel & (1 << i)) >> (i - g_bit++);
				}
				if (bitmapv5header.blue_mask & (1 << i))
				{
					pixel->b |= (*bmp_pixel & (1 << i)) >> (i - b_bit++);
				}
				if (bitmapv5header.alpha_mask & (1 << i))
				{
					pixel->a |= (*bmp_pixel & (1 << i)) >> (i - a_bit++);
				}
			}
		}
	}
}

void tgi_free(tgi_pixel* data)
{
	tg_free(data);
}
