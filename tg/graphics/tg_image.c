#include "tg_image.h"

#include "tg/platform/tg_allocator.h"
#include "tg/util/tg_file_io.h"
#include <string.h>
#include <Windows.h> // TODO: remove

#define TGI_BMP_BITMAPFILEHEADER_OFFSET 0
#define TGI_BMP_BITMAPINFOHEADER_OFFSET 14
#define TGI_BMP_BITMAPV5HEADER_OFFSET   TGI_BMP_BITMAPINFOHEADER_OFFSET

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

typedef struct tgi_bitmap_ciexyz_triple
{
	tgi_bmp_ciexyz ciexyz_red;
	tgi_bmp_ciexyz ciexyz_green;
	tgi_bmp_ciexyz ciexyz_blue;
} tgi_bitmap_ciexyz_triple;

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
			ui32                     size;
			i32                      width;
			i32                      height;
			ui16                     planes;
			ui16                     bit_count;
			ui32                     compression;
			ui32                     size_image;
			i32                      x_pels_per_meter;
			i32                      y_pels_per_meter;
			ui32                     clr_used;
			ui32                     clr_important;
		};
	};
	ui32                     red_mask;
	ui32                     green_mask;
	ui32                     blue_mask;
	ui32                     alpha_mask;
	ui32                     cs_type;
	tgi_bitmap_ciexyz_triple endpoints;
	ui32                     gamma_red;
	ui32                     gamma_green;
	ui32                     gamma_blue;
	ui32                     intent;
	ui32                     profile_data;
	ui32                     profile_size;
	ui32                     reserved;
} tgi_bmp_bitmapv5header;

#define TGI_BMP_COMPRESSION_BI_RGB        0L
#define TGI_BMP_COMPRESSION_BI_RLE8       1L
#define TGI_BMP_COMPRESSION_BI_RLE4       2L
#define TGI_BMP_COMPRESSION_BI_BITFIELDS  3L
#define TGI_BMP_COMPRESSION_BI_JPEG       4L
#define TGI_BMP_COMPRESSION_BI_PNG        5L

#define TGI_BMP_CS_TYPE_LCS_CALIBRATED_RGB      0x00000000L
#define TGI_BMP_CS_TYPE_LCS_sRGB                'sRGB'
#define TGI_BMP_CS_TYPE_LCS_WINDOWS_COLOR_SPACE 'Win '
#define TGI_BMP_CS_TYPE_PROFILE_LINKED          'LINK'
#define TGI_BMP_CS_TYPE_PROFILE_EMBEDDED        'MBED'

#define TGI_BMP_INTENT_LCS_GM_BUSINESS         0x00000001L
#define TGI_BMP_INTENT_LCS_GM_GRAPHICS         0x00000002L
#define TGI_BMP_INTENT_LCS_GM_IMAGES           0x00000004L
#define TGI_BMP_INTENT_LCS_GM_ABS_COLORIMETRIC 0x00000008L

typedef struct tgi_pixel
{
	f32 r;
	f32 g;
	f32 b;
	f32 a;
} tgi_pixel;

void tgi_bmp_load(const char* filename, ui32* width, ui32* height, f32* data)
{
	TG_ASSERT(filename && width && height && data);

	ui64 bmp_size = 0;
	char* bmp_content = NULL;
	tg_file_io_read(filename, &bmp_size, &bmp_content);
		
	TG_ASSERT(bmp_size > 1 && bmp_content[0] == 'B' && bmp_content[1] == 'M');
	
	char* bitmapfileheader_ptr = bmp_content + TGI_BMP_BITMAPFILEHEADER_OFFSET;
	tgi_bmp_bitmapfileheader bitmapfileheader = { 0 };
	bitmapfileheader.type                     = *(ui16*)(bitmapfileheader_ptr +  0);
	bitmapfileheader.size                     = *(ui32*)(bitmapfileheader_ptr +  2);
	bitmapfileheader.reserved1                = *(ui16*)(bitmapfileheader_ptr +  6);
	bitmapfileheader.reserved2                = *(ui16*)(bitmapfileheader_ptr +  8);
	bitmapfileheader.offset_bits              = *(ui32*)(bitmapfileheader_ptr + 10);

	char* bitmapinfoheader_ptr = bmp_content + TGI_BMP_BITMAPINFOHEADER_OFFSET;
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

	TG_ASSERT((tgi_bmp_bitmapinfoheader_type)bitmapinfoheader.size == TGI_BMP_BITMAPINFOHEADER_TYPE_BITMAPV5HEADER);
	TG_ASSERT(bitmapinfoheader.compression == TGI_BMP_COMPRESSION_BI_BITFIELDS);

	tgi_bmp_bitmapv5header bitmapv5header = { 0 };
	bitmapv5header.bitmapinfoheader       = bitmapinfoheader;
	bitmapv5header.red_mask               = *(ui32*)                    (bitmapinfoheader_ptr +  40);
	bitmapv5header.green_mask             = *(ui32*)                    (bitmapinfoheader_ptr +  44);
	bitmapv5header.blue_mask              = *(ui32*)                    (bitmapinfoheader_ptr +  48);
	bitmapv5header.alpha_mask             = *(ui32*)                    (bitmapinfoheader_ptr +  52);
	bitmapv5header.cs_type                = *(ui32*)                    (bitmapinfoheader_ptr +  56);
	bitmapv5header.endpoints              = *(tgi_bitmap_ciexyz_triple*)(bitmapinfoheader_ptr +  60);
	bitmapv5header.gamma_red              = *(ui32*)                    (bitmapinfoheader_ptr +  96);
	bitmapv5header.gamma_green            = *(ui32*)                    (bitmapinfoheader_ptr + 100);
	bitmapv5header.gamma_blue             = *(ui32*)                    (bitmapinfoheader_ptr + 104);
	bitmapv5header.intent                 = *(ui32*)                    (bitmapinfoheader_ptr + 108);
	bitmapv5header.profile_data           = *(ui32*)                    (bitmapinfoheader_ptr + 112);
	bitmapv5header.profile_size           = *(ui32*)                    (bitmapinfoheader_ptr + 116);
	bitmapv5header.reserved               = *(ui32*)                    (bitmapinfoheader_ptr + 120);

	TG_ASSERT(bitmapv5header.cs_type == TGI_BMP_CS_TYPE_LCS_WINDOWS_COLOR_SPACE);

	const ui32* bmp_pixel_array = (ui32*)(bmp_content + bitmapfileheader.offset_bits);
	const ui32 pixels_size = bitmapv5header.width * bitmapv5header.height * sizeof(tgi_pixel);
	tgi_pixel* pixels = tg_malloc(pixels_size);
	memset(pixels, 0, pixels_size);

	for (i32 col = 0; col < bitmapv5header.width; col++)
	{
		for (i32 row = 0; row < bitmapv5header.height; row++)
		{
			ui32* bmp_pixel = &bmp_pixel_array[col * bitmapv5header.width + row];

			ui32 r = 0, r_bit = 0;
			ui32 g = 0, g_bit = 0;
			ui32 b = 0, b_bit = 0;
			ui32 a = 0, a_bit = 0;
			for (ui8 i = 0; i < 32; i++)
			{
				if (bitmapv5header.red_mask & (1 << i))
				{
					r |= (*bmp_pixel & (1 << i)) >> (i - r_bit++);
				}
				if (bitmapv5header.green_mask & (1 << i))
				{
					g |= (*bmp_pixel & (1 << i)) >> (i - g_bit++);
				}
				if (bitmapv5header.blue_mask & (1 << i))
				{
					b |= (*bmp_pixel & (1 << i)) >> (i - b_bit++);
				}
				if (bitmapv5header.alpha_mask & (1 << i))
				{
					a |= (*bmp_pixel & (1 << i)) >> (i - a_bit++);
				}
			}

			tgi_pixel* pixel = &pixels[col * bitmapv5header.width + row];
			pixel->r = (float)r / 255.0f;
			pixel->g = (float)g / 255.0f;
			pixel->b = (float)b / 255.0f;
			pixel->a = (float)a / 255.0f;
		}
	}

	tg_file_io_free(bmp_content);
}
