#include "graphics/tg_image_io.h"

#include "memory/tg_memory.h"
#include "platform/tg_platform.h"
#include "util/tg_string.h"



#define TG_BMP_IDENTIFIER                         'MB'

#define TG_BMP_BITMAPFILEHEADER_SIZE              14

#define TG_BMP_BITMAPCOREHEADER_SIZE              12
#define TG_BMP_OS21XBITMAPHEADER_SIZE             12
#define TG_BMP_OS22XBITMAPHEADER_SIZE             64
#define TG_BMP_BITMAPINFOHEADER_SIZE              40
#define TG_BMP_BITMAPV2INFOHEADER_SIZE            52
#define TG_BMP_BITMAPV3INFOHEADER_SIZE            56
#define TG_BMP_BITMAPV4HEADER_SIZE                108
#define TG_BMP_BITMAPV5HEADER_SIZE                124

#define TG_BMP_BITMAPFILEHEADER_OFFSET            0
#define TG_BMP_BITMAPINFOHEADER_OFFSET            TG_BMP_BITMAPFILEHEADER_SIZE

#define TG_BMP_COMPRESSION_BI_RGB                 0L
#define TG_BMP_COMPRESSION_BI_RLE8                1L
#define TG_BMP_COMPRESSION_BI_RLE4                2L
#define TG_BMP_COMPRESSION_BI_BITFIELDS           3L
#define TG_BMP_COMPRESSION_BI_JPEG                4L
#define TG_BMP_COMPRESSION_BI_PNG                 5L

#define TG_BMP_CS_TYPE_LCS_CALIBRATED_RGB         0x00000000L
#define TG_BMP_CS_TYPE_LCS_SRGB                   'sRGB'
#define TG_BMP_CS_TYPE_LCS_WINDOWS_COLOR_SPACE    'Win '
#define TG_BMP_CS_TYPE_PROFILE_LINKED             'LINK'
#define TG_BMP_CS_TYPE_PROFILE_EMBEDDED           'MBED'

#define TG_BMP_INTENT_LCS_GM_BUSINESS             0x00000001L
#define TG_BMP_INTENT_LCS_GM_GRAPHICS             0x00000002L
#define TG_BMP_INTENT_LCS_GM_IMAGES               0x00000004L
#define TG_BMP_INTENT_LCS_GM_ABS_COLORIMETRIC     0x00000008L



typedef enum tg_bitmapinfoheader_type
{
	TG_BITMAPINFOHEADER_TYPE_BITMAPCOREHEADER      = TG_BMP_BITMAPCOREHEADER_SIZE,
	TG_BITMAPINFOHEADER_TYPE_OS21XBITMAPHEADER     = TG_BMP_OS21XBITMAPHEADER_SIZE,
	TG_BITMAPINFOHEADER_TYPE_OS22XBITMAPHEADER     = TG_BMP_OS22XBITMAPHEADER_SIZE,
	TG_BITMAPINFOHEADER_TYPE_BITMAPINFOHEADER      = TG_BMP_BITMAPINFOHEADER_SIZE,
	TG_BITMAPINFOHEADER_TYPE_BITMAPV2INFOHEADER    = TG_BMP_BITMAPV2INFOHEADER_SIZE,
	TG_BITMAPINFOHEADER_TYPE_BITMAPV3INFOHEADER    = TG_BMP_BITMAPV3INFOHEADER_SIZE,
	TG_BITMAPINFOHEADER_TYPE_BITMAPV4HEADER        = TG_BMP_BITMAPV4HEADER_SIZE,
	TG_BITMAPINFOHEADER_TYPE_BITMAPV5HEADER        = TG_BMP_BITMAPV5HEADER_SIZE
} tg_bitmapinfoheader_type;



typedef struct tg_bitmapfileheader
{
	u16    type;
	u32    size;
	u16    reserved1;
	u16    reserved2;
	u32    offset_bits;
} tg_bitmapfileheader;

typedef struct tg_fxpt_2dot30
{
	i32    x;
	i32    y;
	i32    z;
} tg_fxpt_2dot30;

typedef struct tg_cie_xyz
{
	tg_fxpt_2dot30    cie_xyz_x;
	tg_fxpt_2dot30    cie_xyz_y;
	tg_fxpt_2dot30    cie_xyz_z;
} tg_cie_xyz;

typedef struct tg_cie_xyz_triple
{
	tg_cie_xyz    cie_xyz_red;
	tg_cie_xyz    cie_xyz_green;
	tg_cie_xyz    cie_xyz_blue;
} tg_cie_xyz_triple;

typedef struct tg_bitmap_info_header
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
} tg_bitmap_info_header;

typedef struct tg_bitmap_v5_header
{
	union
	{
		tg_bitmap_info_header    bitmap_info_header;
		struct
		{
			u32                  size;
			i32                  width;
			i32                  height;
			u16                  planes;
			u16                  bit_count;
			u32                  compression;
			u32                  size_image;
			i32                  x_pels_per_meter;
			i32                  y_pels_per_meter;
			u32                  clr_used;
			u32                  clr_important;
		};
	};
	u32                          r_mask;
	u32                          g_mask;
	u32                          b_mask;
	u32                          a_mask;
	u32                          cs_type;
	tg_cie_xyz_triple            endpoints;
	u32                          gamma_red;
	u32                          gamma_green;
	u32                          gamma_blue;
	u32                          intent;
	u32                          profile_data;
	u32                          profile_size;
	u32                          reserved;
} tg_bitmap_v5_header;

typedef struct tg_rgb_quad
{
	u8    rgb_blue;
	u8    rgb_green;
	u8    rgb_red;
	u8    rgb_reserved;
} tg_rgb_quad;



static void tg__convert_format_to_masks(tg_color_image_format format, TG_OUT u32* r_mask, TG_OUT u32* g_mask, TG_OUT u32* b_mask, TG_OUT u32* a_mask)
{
	switch (format)
	{
	case TG_COLOR_IMAGE_FORMAT_A8B8G8R8_UNORM:
	{
		*r_mask = 0xff000000;
		*g_mask = 0x00ff0000;
		*b_mask = 0x0000ff00;
		*a_mask = 0x000000ff;
	} break;
	case TG_COLOR_IMAGE_FORMAT_B8G8R8A8_UNORM:
	{
		*r_mask = 0x00ff0000;
		*g_mask = 0x0000ff00;
		*b_mask = 0x000000ff;
		*a_mask = 0xff000000;
	} break;
	case TG_COLOR_IMAGE_FORMAT_R8_UNORM:
	{
		*r_mask = 0x000000ff;
		*g_mask = 0x00000000;
		*b_mask = 0x00000000;
		*a_mask = 0x00000000;
	} break;
	case TG_COLOR_IMAGE_FORMAT_R8G8_UNORM:
	{
		*r_mask = 0x000000ff;
		*g_mask = 0x0000ff00;
		*b_mask = 0x00000000;
		*a_mask = 0x00000000;
	} break;
	case TG_COLOR_IMAGE_FORMAT_R8G8B8_UNORM:
	{
		*r_mask = 0x000000ff;
		*g_mask = 0x0000ff00;
		*b_mask = 0x00ff0000;
		*a_mask = 0x00000000;
	} break;
	case TG_COLOR_IMAGE_FORMAT_R8G8B8A8_UNORM:
	{
		*r_mask = 0x000000ff;
		*g_mask = 0x0000ff00;
		*b_mask = 0x00ff0000;
		*a_mask = 0xff000000;
	} break;
	default: TG_INVALID_CODEPATH(); break;
	}
}

static void tg__convert_masks_to_format(u32 r_mask, u32 g_mask, u32 b_mask, u32 a_mask, TG_OUT tg_color_image_format* format)
{
	TG_ASSERT(r_mask | g_mask | b_mask | a_mask);

	const u32 mask_0x000000ff = 0x000000ff;
	const u32 mask_0x0000ff00 = 0x0000ff00;
	const u32 mask_0x00ff0000 = 0x00ff0000;
	const u32 mask_0xff000000 = 0xff000000;

	const b32 r_0xff000000 = (r_mask & mask_0xff000000) == mask_0xff000000;
	const b32 r_0x00ff0000 = (r_mask & mask_0x00ff0000) == mask_0x00ff0000;
	//const b32 r_0x0000ff00 = (r_mask & mask_0x0000ff00) == mask_0x0000ff00;
	const b32 r_0x000000ff = (r_mask & mask_0x000000ff) == mask_0x000000ff;

	//const b32 g_0xff000000 = (g_mask & mask_0xff000000) == mask_0xff000000;
	const b32 g_0x00ff0000 = (g_mask & mask_0x00ff0000) == mask_0x00ff0000;
	const b32 g_0x0000ff00 = (g_mask & mask_0x0000ff00) == mask_0x0000ff00;
	//const b32 g_0x000000ff = (g_mask & mask_0x000000ff) == mask_0x000000ff;

	//const b32 b_0xff000000 = (b_mask & mask_0xff000000) == mask_0xff000000;
	const b32 b_0x00ff0000 = (b_mask & mask_0x00ff0000) == mask_0x00ff0000;
	const b32 b_0x0000ff00 = (b_mask & mask_0x0000ff00) == mask_0x0000ff00;
	const b32 b_0x000000ff = (b_mask & mask_0x000000ff) == mask_0x000000ff;

	const b32 a_0xff000000 = (a_mask & mask_0xff000000) == mask_0xff000000;
	//const b32 a_0x00ff0000 = (a_mask & mask_0x00ff0000) == mask_0x00ff0000;
	//const b32 a_0x0000ff00 = (a_mask & mask_0x0000ff00) == mask_0x0000ff00;
	const b32 a_0x000000ff = (a_mask & mask_0x000000ff) == mask_0x000000ff;

	if (a_0x000000ff && b_0x0000ff00 && g_0x00ff0000 && r_0xff000000)
	{
		*format = TG_COLOR_IMAGE_FORMAT_A8B8G8R8_UNORM;
	}
	else if (b_0x000000ff && g_0x0000ff00 && r_0x00ff0000 && a_0xff000000)
	{
		*format = TG_COLOR_IMAGE_FORMAT_B8G8R8A8_UNORM;
	}
	else if (r_0x000000ff && !g_0x0000ff00 && !b_0x00ff0000 && !a_0xff000000)
	{
		*format = TG_COLOR_IMAGE_FORMAT_R8_UNORM;
	}
	else if (r_0x000000ff && g_0x0000ff00 && !b_0x00ff0000 && !a_0xff000000)
	{
		*format = TG_COLOR_IMAGE_FORMAT_R8G8_UNORM;
	}
	else if (r_0x000000ff && g_0x0000ff00 && b_0x00ff0000 && !a_0xff000000)
	{
		*format = TG_COLOR_IMAGE_FORMAT_R8G8B8_UNORM;
	}
	else if (r_0x000000ff && g_0x0000ff00 && b_0x00ff0000 && a_0xff000000)
	{
		*format = TG_COLOR_IMAGE_FORMAT_R8G8B8A8_UNORM;
	}
	else
	{
		TG_INVALID_CODEPATH();
	}
}

static void tg__load_bmp_from_memory(tg_size file_size, const char* p_file_memory, TG_OUT u32* p_width, TG_OUT u32* p_height, TG_OUT tg_color_image_format* p_format, TG_OUT u32** pp_data)
{
	TG_ASSERT(p_file_memory && p_width && p_height && p_format && pp_data);
	TG_ASSERT(*(u16*)p_file_memory == TG_BMP_IDENTIFIER);

	const char* p_bitmap_file_header = &p_file_memory[TG_BMP_BITMAPFILEHEADER_OFFSET];
	
	tg_bitmapfileheader bitmap_file_header   = { 0 };
	bitmap_file_header.type                  = *(u16*)&p_bitmap_file_header[  0];
	bitmap_file_header.size                  = *(u32*)&p_bitmap_file_header[  2];
	bitmap_file_header.reserved1             = *(u16*)&p_bitmap_file_header[  6];
	bitmap_file_header.reserved2             = *(u16*)&p_bitmap_file_header[  8];
	bitmap_file_header.offset_bits           = *(u32*)&p_bitmap_file_header[ 10];

	const char* p_bitmap_info_header = &p_file_memory[TG_BMP_BITMAPINFOHEADER_OFFSET];

	tg_bitmap_info_header bitmap_info_header = { 0 };
	bitmap_info_header.size                  = *(u32*)&p_bitmap_info_header[  0];
	bitmap_info_header.width                 = *(i32*)&p_bitmap_info_header[  4];
	bitmap_info_header.height                = *(i32*)&p_bitmap_info_header[  8];
	bitmap_info_header.planes                = *(u16*)&p_bitmap_info_header[ 12];
	bitmap_info_header.bit_count             = *(u16*)&p_bitmap_info_header[ 14];
	bitmap_info_header.compression           = *(u32*)&p_bitmap_info_header[ 16];
	bitmap_info_header.size_image            = *(u32*)&p_bitmap_info_header[ 20];
	bitmap_info_header.x_pels_per_meter      = *(i32*)&p_bitmap_info_header[ 24];
	bitmap_info_header.y_pels_per_meter      = *(i32*)&p_bitmap_info_header[ 28];
	bitmap_info_header.clr_used              = *(u32*)&p_bitmap_info_header[ 32];
	bitmap_info_header.clr_important         = *(u32*)&p_bitmap_info_header[ 36];

	TG_ASSERT(file_size >= (tg_size)bitmap_file_header.offset_bits + ((tg_size)bitmap_info_header.width * (tg_size)bitmap_info_header.height * sizeof(u32)));
	TG_ASSERT((tg_bitmapinfoheader_type)bitmap_info_header.size == TG_BMP_BITMAPV5HEADER_SIZE);
	TG_ASSERT(bitmap_info_header.compression == TG_BMP_COMPRESSION_BI_BITFIELDS);

	tg_bitmap_v5_header bitmap_v5_header     = { 0 };
	bitmap_v5_header.bitmap_info_header      = bitmap_info_header;
	bitmap_v5_header.r_mask                  = *(u32*)&p_bitmap_info_header[ 40];
	bitmap_v5_header.g_mask                  = *(u32*)&p_bitmap_info_header[ 44];
	bitmap_v5_header.b_mask                  = *(u32*)&p_bitmap_info_header[ 48];
	bitmap_v5_header.a_mask                  = *(u32*)&p_bitmap_info_header[ 52];
	bitmap_v5_header.cs_type                 = *(u32*)&p_bitmap_info_header[ 56];
	bitmap_v5_header.endpoints               = *(tg_cie_xyz_triple*)&p_bitmap_info_header[60];
	bitmap_v5_header.gamma_red               = *(u32*)&p_bitmap_info_header[ 96];
	bitmap_v5_header.gamma_green             = *(u32*)&p_bitmap_info_header[100];
	bitmap_v5_header.gamma_blue              = *(u32*)&p_bitmap_info_header[104];
	bitmap_v5_header.intent                  = *(u32*)&p_bitmap_info_header[108];
	bitmap_v5_header.profile_data            = *(u32*)&p_bitmap_info_header[112];
	bitmap_v5_header.profile_size            = *(u32*)&p_bitmap_info_header[116];
	bitmap_v5_header.reserved                = *(u32*)&p_bitmap_info_header[120];

	TG_ASSERT(bitmap_v5_header.cs_type == TG_BMP_CS_TYPE_LCS_WINDOWS_COLOR_SPACE);

#ifdef TG_DEBUG
	const u32 rgb_quad_count = (bitmap_file_header.offset_bits - (bitmap_info_header.size + TG_BMP_BITMAPINFOHEADER_OFFSET)) / sizeof(tg_rgb_quad);
	TG_ASSERT(rgb_quad_count == 3);

	tg_rgb_quad* p_rgb_quad_r = (tg_rgb_quad*)&p_bitmap_info_header[bitmap_info_header.size];
	TG_ASSERT(*(u32*)p_rgb_quad_r == 0x000000ff);

	tg_rgb_quad* p_rgb_quad_g = &p_rgb_quad_r[1];
	TG_ASSERT(*(u32*)p_rgb_quad_g == 0x0000ff00);

	tg_rgb_quad* p_rgb_quad_b = &p_rgb_quad_g[1];
	TG_ASSERT(*(u32*)p_rgb_quad_b == 0x00ff0000);
#endif

	*p_width = bitmap_v5_header.width;
	*p_height = bitmap_v5_header.height;
	tg__convert_masks_to_format(bitmap_v5_header.r_mask, bitmap_v5_header.g_mask, bitmap_v5_header.b_mask, bitmap_v5_header.a_mask, p_format);
	const tg_size size = (tg_size)bitmap_v5_header.width * (tg_size)bitmap_v5_header.height * sizeof(**pp_data);
	*pp_data = TG_MALLOC(size);
	tg_memcpy(size, &p_file_memory[bitmap_file_header.offset_bits], *pp_data);
}



void tg_image_load(const char* p_filename, TG_OUT u32* p_width, TG_OUT u32* p_height, TG_OUT tg_color_image_format* p_format, TG_OUT u32** pp_data)
{
	TG_ASSERT(p_filename && p_width && p_height && p_format && pp_data);

	tg_file_properties file_properties = { 0 };
	tgp_file_get_properties(p_filename, &file_properties);
	char* p_memory = TG_MALLOC_STACK(file_properties.size);
	tgp_file_load(p_filename, file_properties.size, p_memory);

	if (file_properties.size >= 2 && *(u16*)p_memory == TG_BMP_IDENTIFIER)
	{
		tg__load_bmp_from_memory(file_properties.size, p_memory, p_width, p_height, p_format, pp_data);
	}

	TG_FREE_STACK(file_properties.size);
}

void tg_image_free(u32* p_data)
{
	TG_FREE(p_data);
}

void tg_image_convert_format(TG_INOUT u32* p_data, u32 width, u32 height, tg_color_image_format old_format, tg_color_image_format new_format)
{
	u32 old_r_mask = 0;
	u32 old_g_mask = 0;
	u32 old_b_mask = 0;
	u32 old_a_mask = 0;
	tg__convert_format_to_masks(old_format, &old_r_mask, &old_g_mask, &old_b_mask, &old_a_mask);

	u32 new_r_mask = 0;
	u32 new_g_mask = 0;
	u32 new_b_mask = 0;
	u32 new_a_mask = 0;
	tg__convert_format_to_masks(new_format, &new_r_mask, &new_g_mask, &new_b_mask, &new_a_mask);

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

b32 tg_image_store_to_disc(const char* p_filename, u32 width, u32 height, tg_color_image_format format, const u8* p_data, b32 force_alpha_one, b32 replace_existing)
{
	b32 result = TG_FALSE;

	const char* p_extension = TG_NULL;
	const char* p_it = p_filename;
	while (*p_it != '\0')
	{
		if (*p_it == '.')
		{
			p_extension = p_it + 1;
		}
		p_it++;
	}
	TG_ASSERT(p_extension);

	if (tg_string_equal(p_extension, "bmp"))
	{
		const u32 in_pixel_size = (u32)tg_color_image_format_size(format);
		const u32 in_channels = tg_color_image_format_channels(format);
		const u32 in_data_size = width * height * in_pixel_size;

		const u32 out_pixel_size = 4;//TG_MIN(sizeof(u32), in_pixel_size);
		const u32 out_channels = 4;
		const u32 out_data_size = width * height * out_pixel_size;

		const tg_color_image_format out_format = in_pixel_size == out_pixel_size && in_channels == out_channels ? format : TG_COLOR_IMAGE_FORMAT_R8G8B8A8_UNORM;

		const u32 bmp_size = TG_BMP_BITMAPFILEHEADER_SIZE + TG_BMP_BITMAPV5HEADER_SIZE + 3 * sizeof(tg_rgb_quad) + out_data_size;
		char* p_buffer = TG_MALLOC_STACK(bmp_size);

		const u32 offset_bits = 150;

		char* p_bitmap_file_header = &p_buffer[TG_BMP_BITMAPFILEHEADER_OFFSET];

		// bitmap file header
		*(u16*)&p_bitmap_file_header[0x00] = TG_BMP_IDENTIFIER;                      // type
		*(u32*)&p_bitmap_file_header[0x02] = bmp_size;                               // size
		*(u16*)&p_bitmap_file_header[0x06] = 0;                                      // reserved1
		*(u16*)&p_bitmap_file_header[0x08] = 0;                                      // reserved2
		*(u32*)&p_bitmap_file_header[0x0a] = offset_bits;                            // offset_bits
		
		char* p_bitmap_info_header = &p_buffer[TG_BMP_BITMAPINFOHEADER_OFFSET];
		
		// bitmap info header
		*(u32*)&p_bitmap_info_header[  0] = TG_BMP_BITMAPV5HEADER_SIZE;              // size
		*(i32*)&p_bitmap_info_header[  4] = (i32)width;                              // width
		*(i32*)&p_bitmap_info_header[  8] = -(i32)height;                            // height
		*(u16*)&p_bitmap_info_header[ 12] = 1;                                       // planes
		*(u16*)&p_bitmap_info_header[ 14] = 8 * (u16)out_pixel_size;                 // bit_count
		*(u32*)&p_bitmap_info_header[ 16] = TG_BMP_COMPRESSION_BI_BITFIELDS;         // compression
		*(u32*)&p_bitmap_info_header[ 20] = bmp_size;                                // size_image
		*(i32*)&p_bitmap_info_header[ 24] = 0;                                       // x_pels_per_meter
		*(i32*)&p_bitmap_info_header[ 28] = 0;                                       // y_pels_per_meter
		*(u32*)&p_bitmap_info_header[ 32] = 0;                                       // clr_used
		*(u32*)&p_bitmap_info_header[ 36] = 0;                                       // clr_important

		// bitmap v5 header
		tg__convert_format_to_masks(
			out_format,
			(u32*)&p_bitmap_info_header[40],
			(u32*)&p_bitmap_info_header[44],
			(u32*)&p_bitmap_info_header[48],
			(u32*)&p_bitmap_info_header[52]
		);
		*(u32*)&p_bitmap_info_header[ 56] = TG_BMP_CS_TYPE_LCS_WINDOWS_COLOR_SPACE;  // cs_type
		*(tg_cie_xyz_triple*)&p_bitmap_info_header[60] = (tg_cie_xyz_triple){ 0 };   // endpoints
		*(u32*)&p_bitmap_info_header[ 96] = 0;                                       // gamma_red
		*(u32*)&p_bitmap_info_header[100] = 0;                                       // gamma_green
		*(u32*)&p_bitmap_info_header[104] = 0;                                       // gamma_blue
		*(u32*)&p_bitmap_info_header[108] = 0;                                       // intent
		*(u32*)&p_bitmap_info_header[112] = 0;                                       // profile_data
		*(u32*)&p_bitmap_info_header[116] = 0;                                       // profile_size
		*(u32*)&p_bitmap_info_header[120] = 0;                                       // reserved

		// rgb quad array
		*(u32*)&p_buffer[138]             = 0x000000ff;                              // rgb_quads[0]
		*(u32*)&p_buffer[142]             = 0x0000ff00;                              // rgb_quads[1]
		*(u32*)&p_buffer[146]             = 0x00ff0000;                              // rgb_quads[2]

		// color-index array
		u32* p_pixels = (u32*)&p_buffer[offset_bits];
		if (in_data_size == out_data_size && in_pixel_size == out_pixel_size && in_channels == out_channels)
		{
			tg_memcpy(in_data_size, p_data, p_pixels);
		}
		else
		{
			if (in_pixel_size == 1 && in_channels == 1)
			{
				for (u32 i = 0; i < in_data_size; i++)
				{
					// TODO: format could vary!
					const u32 brightness = (u32)p_data[i];
					p_pixels[i] = brightness | brightness << 8 | brightness << 16;
				}
			}
			else
			{
				TG_NOT_IMPLEMENTED();
			}
		}

		if (force_alpha_one)
		{
			if (out_pixel_size == 4)
			{
				const u32 alpha_mask = *(u32*)&p_bitmap_info_header[52];
				for (u32 i = 0; i < in_data_size / 4; i++)
				{
					p_pixels[i] |= alpha_mask;
				}
			}
			else
			{
				TG_NOT_IMPLEMENTED();
			}
		}

		result = tgp_file_create(p_filename, (size_t)bmp_size, p_buffer, replace_existing);
		
		TG_FREE_STACK(bmp_size);
	}
	else
	{
		TG_INVALID_CODEPATH();
	}

	return result;
}
