#include "graphics/tg_image_loader.h"

#include "memory/tg_memory.h"
#include "util/tg_file_io.h"
#include <string.h> // TODO: memcpy



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



typedef struct tg_bitmapfileheader
{
	u16    type;
	u32    size;
	u16    reserved1;
	u16    reserved2;
	u32    offset_bits;
} tg_bitmapfileheader;

typedef enum tg_bitmapinfoheader_type
{
	TG_BITMAPINFOHEADER_TYPE_BITMAPCOREHEADER      = 12,
	TG_BITMAPINFOHEADER_TYPE_OS21XBITMAPHEADER     = 12,
	TG_BITMAPINFOHEADER_TYPE_OS22XBITMAPHEADER     = 64,
	TG_BITMAPINFOHEADER_TYPE_BITMAPINFOHEADER      = 40,
	TG_BITMAPINFOHEADER_TYPE_BITMAPV2INFOHEADER    = 52,
	TG_BITMAPINFOHEADER_TYPE_BITMAPV3INFOHEADER    = 56,
	TG_BITMAPINFOHEADER_TYPE_BITMAPV4HEADER        = 108,
	TG_BITMAPINFOHEADER_TYPE_BITMAPV5HEADER        = 124
} tg_bitmapinfoheader_type;

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
	u32    size;
	i32    width;
	i32    height;
	u16    planes;
	u16    bit_count;
	u32    compression;
	u32    size_image;
	i32    x_pels_per_meter;
	i32    y_pels_per_meter;
	u32    clr_used;
	u32    clr_important;
} tg_bitmapinfoheader;

typedef struct tg_bitmapv5header
{
	union
	{
		tg_bitmapinfoheader    bitmapinfoheader;
		struct
		{
			u32                size;
			i32                width;
			i32                height;
			u16                planes;
			u16                bit_count;
			u32                compression;
			u32                size_image;
			i32                x_pels_per_meter;
			i32                y_pels_per_meter;
			u32                clr_used;
			u32                clr_important;
		};
	};
	u32                        r_mask;
	u32                        g_mask;
	u32                        b_mask;
	u32                        a_mask;
	u32                        cs_type;
	tg_ciexyz_triple           endpoints;
	u32                        gamma_red;
	u32                        gamma_green;
	u32                        gamma_blue;
	u32                        intent;
	u32                        profile_data;
	u32                        profile_size;
	u32                        reserved;
} tg_bitmapv5header;



void tg_image_convert_format_to_masks(tg_color_image_format format, u32* r_mask, u32* g_mask, u32* b_mask, u32* a_mask);
void tg_image_convert_masks_to_format(u32 r_mask, u32 g_mask, u32 b_mask, u32 a_mask, tg_color_image_format* format);
void tg_image_load_bmp_from_memory(u64 file_size, const char* file_memory, u32* p_width, u32* p_height, tg_color_image_format* p_format, u32** p_data);



void tg_image_load(const char* p_filename, u32* p_width, u32* p_height, tg_color_image_format* p_format, u32** pp_data)
{
	TG_ASSERT(p_filename && p_width && p_height && p_format && pp_data);

	u64 size = 0;
	char* memory = TG_NULL;
	tg_file_io_read(p_filename, &size, &memory);

	if (size >= 2 && *(u16*)memory == TG_BMP_IDENTIFIER)
	{
		tg_image_load_bmp_from_memory(size, memory, p_width, p_height, p_format, pp_data);
	}

	tg_file_io_free(memory);
}
void tg_image_free(u32* p_data)
{
	TG_MEMORY_FREE(p_data);
}

void tg_image_convert_format(u32* p_data, u32 width, u32 height, tg_color_image_format old_format, tg_color_image_format new_format)
{
	u32 old_r_mask = 0;
	u32 old_g_mask = 0;
	u32 old_b_mask = 0;
	u32 old_a_mask = 0;
	tg_image_convert_format_to_masks(old_format, &old_r_mask, &old_g_mask, &old_b_mask, &old_a_mask);

	u32 new_r_mask = 0;
	u32 new_g_mask = 0;
	u32 new_b_mask = 0;
	u32 new_a_mask = 0;
	tg_image_convert_format_to_masks(new_format, &new_r_mask, &new_g_mask, &new_b_mask, &new_a_mask);

	for (u32 col = 0; col < width; col++)
	{
		for (u32 row = 0; row < height; row++)
		{
			u32 r = 0x00000000;
			u32 g = 0x00000000;
			u32 b = 0x00000000;
			u32 a = 0x00000000;
			
			u32 r_bit = 0x00000000;
			u32 g_bit = 0x00000000;
			u32 b_bit = 0x00000000;
			u32 a_bit = 0x00000000;

			for (u8 i = 0; i < 32; i++)
			{
				if (old_r_mask & (1 << i))
				{
					r |= (*p_data & (1 << i)) >> (i - r_bit++);
				}
				if (old_g_mask & (1 << i))
				{
					g |= (*p_data & (1 << i)) >> (i - g_bit++);
				}
				if (old_b_mask & (1 << i))
				{
					b |= (*p_data & (1 << i)) >> (i - b_bit++);
				}
				if (old_a_mask & (1 << i))
				{
					a |= (*p_data & (1 << i)) >> (i - a_bit++);
				}
			}

			*p_data = 0x00000000;

			for (u8 i = 0; i < 32; i++)
			{
				if (new_r_mask & (1 << i))
				{
					u32 bit = (r & 0x00000001) << i;
					r = r >> 0x00000001;
					*p_data |= bit;
				}
				if (new_g_mask & (1 << i))
				{
					u32 bit = (g & 0x00000001) << i;
					g = g >> 0x00000001;
					*p_data |= bit;
				}
				if (new_b_mask & (1 << i))
				{
					u32 bit = (b & 0x00000001) << i;
					b = b >> 0x00000001;
					*p_data |= bit;
				}
				if (new_a_mask & (1 << i))
				{
					u32 bit = (a & 0x00000001) << i;
					a = a >> 0x00000001;
					*p_data |= bit;
				}
			}

			p_data++;
		}
	}
}

void tg_image_convert_masks_to_format(u32 r_mask, u32 g_mask, u32 b_mask, u32 a_mask, tg_color_image_format* format)
{
	TG_ASSERT(r_mask | g_mask | b_mask | a_mask);

	const u32 mask_0x000000ff = 0x000000ff;
	const u32 mask_0x0000ff00 = 0x0000ff00;
	const u32 mask_0x00ff0000 = 0x00ff0000;
	const u32 mask_0xff000000 = 0xff000000;

	const b32 r_0xff000000 = (r_mask & mask_0xff000000) == mask_0xff000000;
	const b32 r_0x00ff0000 = (r_mask & mask_0x00ff0000) == mask_0x00ff0000;
	const b32 r_0x0000ff00 = (r_mask & mask_0x0000ff00) == mask_0x0000ff00;
	const b32 r_0x000000ff = (r_mask & mask_0x000000ff) == mask_0x000000ff;

	const b32 g_0xff000000 = (g_mask & mask_0xff000000) == mask_0xff000000;
	const b32 g_0x00ff0000 = (g_mask & mask_0x00ff0000) == mask_0x00ff0000;
	const b32 g_0x0000ff00 = (g_mask & mask_0x0000ff00) == mask_0x0000ff00;
	const b32 g_0x000000ff = (g_mask & mask_0x000000ff) == mask_0x000000ff;

	const b32 b_0xff000000 = (b_mask & mask_0xff000000) == mask_0xff000000;
	const b32 b_0x00ff0000 = (b_mask & mask_0x00ff0000) == mask_0x00ff0000;
	const b32 b_0x0000ff00 = (b_mask & mask_0x0000ff00) == mask_0x0000ff00;
	const b32 b_0x000000ff = (b_mask & mask_0x000000ff) == mask_0x000000ff;

	const b32 a_0xff000000 = (a_mask & mask_0xff000000) == mask_0xff000000;
	const b32 a_0x00ff0000 = (a_mask & mask_0x00ff0000) == mask_0x00ff0000;
	const b32 a_0x0000ff00 = (a_mask & mask_0x0000ff00) == mask_0x0000ff00;
	const b32 a_0x000000ff = (a_mask & mask_0x000000ff) == mask_0x000000ff;

	if (a_0x000000ff && r_0x0000ff00 && g_0x00ff0000 && b_0xff000000)
	{
		*format = TG_COLOR_IMAGE_FORMAT_A8R8G8B8;
	}
	else if (a_0x000000ff && b_0x0000ff00 && g_0x00ff0000 && r_0xff000000)
	{
		*format = TG_COLOR_IMAGE_FORMAT_A8B8G8R8;
	}
	else if (b_0x000000ff && g_0x0000ff00 && r_0x00ff0000 && a_0xff000000)
	{
		*format = TG_COLOR_IMAGE_FORMAT_B8G8R8A8;
	}
	else if (r_0x000000ff && !g_0x0000ff00 && !b_0x00ff0000 && !a_0xff000000)
	{
		*format = TG_COLOR_IMAGE_FORMAT_R8;
	}
	else if (r_0x000000ff && g_0x0000ff00 && !b_0x00ff0000 && !a_0xff000000)
	{
		*format = TG_COLOR_IMAGE_FORMAT_R8G8;
	}
	else if (r_0x000000ff && g_0x0000ff00 && b_0x00ff0000 && !a_0xff000000)
	{
		*format = TG_COLOR_IMAGE_FORMAT_R8G8B8;
	}
	else if (r_0x000000ff && g_0x0000ff00 && b_0x00ff0000 && a_0xff000000)
	{
		*format = TG_COLOR_IMAGE_FORMAT_R8G8B8A8;
	}
	else
	{
		TG_ASSERT(TG_FALSE);
	}
}
void tg_image_convert_format_to_masks(tg_color_image_format format, u32* r_mask, u32* g_mask, u32* b_mask, u32* a_mask)
{
	switch (format)
	{
	case TG_COLOR_IMAGE_FORMAT_A8R8G8B8:
	{
		*r_mask = 0x0000ff00;
		*g_mask = 0x00ff0000;
		*b_mask = 0xff000000;
		*a_mask = 0x000000ff;
	} break;
	case TG_COLOR_IMAGE_FORMAT_A8B8G8R8:
	{
		*r_mask = 0xff000000;
		*g_mask = 0x00ff0000;
		*b_mask = 0x0000ff00;
		*a_mask = 0x000000ff;
	} break;
	case TG_COLOR_IMAGE_FORMAT_B8G8R8A8:
	{
		*r_mask = 0x00ff0000;
		*g_mask = 0x0000ff00;
		*b_mask = 0x000000ff;
		*a_mask = 0xff000000;
	} break;
	case TG_COLOR_IMAGE_FORMAT_R8:
	{
		*r_mask = 0x000000ff;
		*g_mask = 0x00000000;
		*b_mask = 0x00000000;
		*a_mask = 0x00000000;
	} break;
	case TG_COLOR_IMAGE_FORMAT_R8G8:
	{
		*r_mask = 0x000000ff;
		*g_mask = 0x0000ff00;
		*b_mask = 0x00000000;
		*a_mask = 0x00000000;
	} break;
	case TG_COLOR_IMAGE_FORMAT_R8G8B8:
	{
		*r_mask = 0x000000ff;
		*g_mask = 0x0000ff00;
		*b_mask = 0x00ff0000;
		*a_mask = 0x00000000;
	} break;
	case TG_COLOR_IMAGE_FORMAT_R8G8B8A8:
	{
		*r_mask = 0x000000ff;
		*g_mask = 0x0000ff00;
		*b_mask = 0x00ff0000;
		*a_mask = 0xff000000;
	} break;
	default:
	{
		TG_ASSERT(TG_FALSE);
	} break;
	}
}
void tg_image_load_bmp_from_memory(u64 file_size, const char* file_memory, u32* p_width, u32* p_height, tg_color_image_format* p_format, u32** p_data)
{
	TG_ASSERT(file_memory && p_width && p_height && p_format && p_data);
	TG_ASSERT(file_size >= TG_BMP_MIN_SIZE && *(u16*)file_memory == TG_BMP_IDENTIFIER);

	const char* bitmapfileheader_ptr = file_memory + TG_BMP_BITMAPFILEHEADER_OFFSET;
	tg_bitmapfileheader bitmapfileheader = { 0 };
	bitmapfileheader.type = *(u16*)(bitmapfileheader_ptr + 0);
	bitmapfileheader.size = *(u32*)(bitmapfileheader_ptr + 2);
	bitmapfileheader.reserved1 = *(u16*)(bitmapfileheader_ptr + 6);
	bitmapfileheader.reserved2 = *(u16*)(bitmapfileheader_ptr + 8);
	bitmapfileheader.offset_bits = *(u32*)(bitmapfileheader_ptr + 10);

	const char* bitmapinfoheader_ptr = file_memory + TG_BMP_BITMAPINFOHEADER_OFFSET;
	tg_bitmapinfoheader bitmapinfoheader = { 0 };
	bitmapinfoheader.size = *(u32*)(bitmapinfoheader_ptr + 0);
	bitmapinfoheader.width = *(i32*)(bitmapinfoheader_ptr + 4);
	bitmapinfoheader.height = *(i32*)(bitmapinfoheader_ptr + 8);
	bitmapinfoheader.planes = *(u16*)(bitmapinfoheader_ptr + 12);
	bitmapinfoheader.bit_count = *(u16*)(bitmapinfoheader_ptr + 14);
	bitmapinfoheader.compression = *(u32*)(bitmapinfoheader_ptr + 16);
	bitmapinfoheader.size_image = *(u32*)(bitmapinfoheader_ptr + 20);
	bitmapinfoheader.x_pels_per_meter = *(i32*)(bitmapinfoheader_ptr + 24);
	bitmapinfoheader.y_pels_per_meter = *(i32*)(bitmapinfoheader_ptr + 28);
	bitmapinfoheader.clr_used = *(u32*)(bitmapinfoheader_ptr + 32);
	bitmapinfoheader.clr_important = *(u32*)(bitmapinfoheader_ptr + 36);

	TG_ASSERT(file_size >= (u64)bitmapfileheader.offset_bits + ((u64)bitmapinfoheader.width * (u64)bitmapinfoheader.height * (u64)sizeof(u32)));
	TG_ASSERT((tg_bitmapinfoheader_type)bitmapinfoheader.size == TG_BITMAPINFOHEADER_TYPE_BITMAPV5HEADER);
	TG_ASSERT(bitmapinfoheader.compression == TG_BMP_COMPRESSION_BI_BITFIELDS);

	tg_bitmapv5header bitmapv5header = { 0 };
	bitmapv5header.bitmapinfoheader = bitmapinfoheader;
	bitmapv5header.r_mask = *(u32*)(bitmapinfoheader_ptr + 40);
	bitmapv5header.g_mask = *(u32*)(bitmapinfoheader_ptr + 44);
	bitmapv5header.b_mask = *(u32*)(bitmapinfoheader_ptr + 48);
	bitmapv5header.a_mask = *(u32*)(bitmapinfoheader_ptr + 52);
	bitmapv5header.cs_type = *(u32*)(bitmapinfoheader_ptr + 56);
	bitmapv5header.endpoints = *(tg_ciexyz_triple*)(bitmapinfoheader_ptr + 60);
	bitmapv5header.gamma_red = *(u32*)(bitmapinfoheader_ptr + 96);
	bitmapv5header.gamma_green = *(u32*)(bitmapinfoheader_ptr + 100);
	bitmapv5header.gamma_blue = *(u32*)(bitmapinfoheader_ptr + 104);
	bitmapv5header.intent = *(u32*)(bitmapinfoheader_ptr + 108);
	bitmapv5header.profile_data = *(u32*)(bitmapinfoheader_ptr + 112);
	bitmapv5header.profile_size = *(u32*)(bitmapinfoheader_ptr + 116);
	bitmapv5header.reserved = *(u32*)(bitmapinfoheader_ptr + 120);

	TG_ASSERT(bitmapv5header.cs_type == TG_BMP_CS_TYPE_LCS_WINDOWS_COLOR_SPACE);

	*p_width = bitmapv5header.width;
	*p_height = bitmapv5header.height;
	tg_image_convert_masks_to_format(bitmapv5header.r_mask, bitmapv5header.g_mask, bitmapv5header.b_mask, bitmapv5header.a_mask, p_format);
	const u64 size = (u64)bitmapv5header.width * (u64)bitmapv5header.height * (u64)sizeof(**p_data);
	*p_data = TG_MEMORY_ALLOC(size);
	memcpy(*p_data, (u32*)(file_memory + bitmapfileheader.offset_bits), size);
}
