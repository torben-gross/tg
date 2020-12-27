#include "graphics/font/tg_font.h"

#include "graphics/font/tg_open_type_types.h"
#include "graphics/font/tg_open_type_layout_common_table_formats.h"
#include "graphics/font/tg_open_type_font_variations_common_table_formats.h"
#include "graphics/font/tg_open_type_cmap.h"
#include "graphics/font/tg_open_type_dsig.h"
#include "graphics/font/tg_open_type_gdef.h"
#include "graphics/font/tg_open_type_glyf.h"
#include "graphics/font/tg_open_type_gpos.h"
#include "graphics/font/tg_open_type_head.h"
#include "graphics/font/tg_open_type_loca.h"
#include "graphics/font/tg_open_type_maxp.h"
#include "math/tg_math.h"
#include "memory/tg_memory.h"
#include "platform/tg_platform.h"
#include "util/tg_string.h"



// source: https://docs.microsoft.com/en-us/typography/opentype/spec/

#define TG_SFNT_VERSION_TRUE_TYPE          0x00010000
#define TG_SFNT_VERSION_OPEN_TYPE          'OTTO'

#define TG_OPEN_TYPE_I16(x)                ((((x) & 0x00ff) << 8) | (((x) & 0xff00) >> 8))
#define TG_OPEN_TYPE_I32(x)                ((((x) & 0x000000ff) << 24) | (((x) & 0x0000ff00) << 8) | (((x) & 0x00ff0000) >> 8) | ((x) & 0xff000000) >> 24))
#define TG_OPEN_TYPE_I64(x)                        \
    (                                              \
    (((*(i64*)&(x)) & 0x00000000000000ff) << 56) | \
    (((*(i64*)&(x)) & 0x000000000000ff00) << 40) | \
    (((*(i64*)&(x)) & 0x0000000000ff0000) << 24) | \
    (((*(i64*)&(x)) & 0x00000000ff000000) <<  8) | \
    (((*(i64*)&(x)) & 0x000000ff00000000) >>  8) | \
    (((*(i64*)&(x)) & 0x0000ff0000000000) >> 24) | \
    (((*(i64*)&(x)) & 0x00ff000000000000) >> 40) | \
    (((*(i64*)&(x)) & 0xff00000000000000) >> 56)   \
    )
#define TG_OPEN_TYPE_U16(x)                ((((x) & 0x00ff) << 8) | (((x) & 0xff00) >> 8))
#define TG_OPEN_TYPE_U32(x)                ((((x) & 0x000000ff) << 24) | (((x) & 0x0000ff00) << 8) | (((x) & 0x00ff0000) >> 8) | (((x) & 0xff000000) >> 24))
#define TG_OPEN_TYPE_U64(x)                        \
    (                                              \
    (((*(u64*)&(x)) & 0x00000000000000ff) << 56) | \
    (((*(u64*)&(x)) & 0x000000000000ff00) << 40) | \
    (((*(u64*)&(x)) & 0x0000000000ff0000) << 24) | \
    (((*(u64*)&(x)) & 0x00000000ff000000) <<  8) | \
    (((*(u64*)&(x)) & 0x000000ff00000000) >>  8) | \
    (((*(u64*)&(x)) & 0x0000ff0000000000) >> 24) | \
    (((*(u64*)&(x)) & 0x00ff000000000000) >> 40) | \
    (((*(u64*)&(x)) & 0xff00000000000000) >> 56)   \
    )
#define TG_OPEN_TYPE_F2DOT14               TG_OPEN_TYPE_I16
#define TG_OPEN_TYPE_FWORD                 TG_OPEN_TYPE_I16

#define TG_OPEN_TYPE_NEXT_I16(p_it)        TG_OPEN_TYPE_I16(*(i16*)p_it); p_it += 2
#define TG_OPEN_TYPE_NEXT_I32(p_it)        TG_OPEN_TYPE_I32(*(i32*)p_it); p_it += 4
#define TG_OPEN_TYPE_NEXT_I64(p_it)        TG_OPEN_TYPE_I64(*(i64*)p_it); p_it += 8
#define TG_OPEN_TYPE_NEXT_U16(p_it)        TG_OPEN_TYPE_U16(*(u16*)p_it); p_it += 2
#define TG_OPEN_TYPE_NEXT_U32(p_it)        TG_OPEN_TYPE_U32(*(u32*)p_it); p_it += 4
#define TG_OPEN_TYPE_NEXT_U64(p_it)        TG_OPEN_TYPE_U64(*(u64*)p_it); p_it += 8
#define TG_OPEN_TYPE_NEXT_F2DOT14(p_it)    TG_OPEN_TYPE_F2DOT14(*(tg_f2dot14*)p_it); p_it += 2
#define TG_OPEN_TYPE_NEXT_FWORD(p_it)      TG_OPEN_TYPE_FWORD(*(tg_fword*)p_it); p_it += 2



typedef enum tg_open_type__tag
{
	TG_OPEN_TYPE__TAG__AVAR       = 'avar',
	TG_OPEN_TYPE__TAG__BASE       = 'BASE',
	TG_OPEN_TYPE__TAG__CBDT       = 'CBDT',
	TG_OPEN_TYPE__TAG__CBLC       = 'CBLC',
	TG_OPEN_TYPE__TAG__CFF        = 'CFF ',
	TG_OPEN_TYPE__TAG__CFF2       = 'CFF2',
	TG_OPEN_TYPE__TAG__cmap       = 'cmap',
	TG_OPEN_TYPE__TAG__COLR       = 'COLR',
	TG_OPEN_TYPE__TAG__CPAL       = 'CPAL',
	TG_OPEN_TYPE__TAG__cvar       = 'cvar',
	TG_OPEN_TYPE__TAG__cvt        = 'cvt ',
	TG_OPEN_TYPE__TAG__DSIG       = 'DSIG',
	TG_OPEN_TYPE__TAG__EBDT       = 'EBDT',
	TG_OPEN_TYPE__TAG__EBLC       = 'EBLC',
	TG_OPEN_TYPE__TAG__EBSC       = 'EBSC',
	TG_OPEN_TYPE__TAG__fpgm       = 'fpgm',
	TG_OPEN_TYPE__TAG__fvar       = 'fvar',
	TG_OPEN_TYPE__TAG__gasp       = 'gasp',
	TG_OPEN_TYPE__TAG__GDEF       = 'GDEF',
	TG_OPEN_TYPE__TAG__glyf       = 'glyf',
	TG_OPEN_TYPE__TAG__GPOS       = 'GPOS',
	TG_OPEN_TYPE__TAG__GSUB       = 'GSUB',
	TG_OPEN_TYPE__TAG__gvar       = 'gvar',
	TG_OPEN_TYPE__TAG__hdmx       = 'hdmx',
	TG_OPEN_TYPE__TAG__head       = 'head',
	TG_OPEN_TYPE__TAG__hhea       = 'hhea',
	TG_OPEN_TYPE__TAG__hmtx       = 'hmtx',
	TG_OPEN_TYPE__TAG__HVAR       = 'HVAR',
	TG_OPEN_TYPE__TAG__JSTF       = 'JSTF',
	TG_OPEN_TYPE__TAG__kern       = 'kern',
	TG_OPEN_TYPE__TAG__loca       = 'loca',
	TG_OPEN_TYPE__TAG__LTSH       = 'LTSH',
	TG_OPEN_TYPE__TAG__MATH       = 'MATH',
	TG_OPEN_TYPE__TAG__maxp       = 'maxp',
	TG_OPEN_TYPE__TAG__MERG       = 'MERG',
	TG_OPEN_TYPE__TAG__meta       = 'meta',
	TG_OPEN_TYPE__TAG__MVAR       = 'MVAR',
	TG_OPEN_TYPE__TAG__NAME       = 'name',
	TG_OPEN_TYPE__TAG__OS2        = 'OS/2',
	TG_OPEN_TYPE__TAG__PCLT       = 'PCLT',
	TG_OPEN_TYPE__TAG__post       = 'post',
	TG_OPEN_TYPE__TAG__prep       = 'prep',
	TG_OPEN_TYPE__TAG__sbix       = 'sbix',
	TG_OPEN_TYPE__TAG__STAT       = 'STAT',
	TG_OPEN_TYPE__TAG__SVG        = 'SVG ',
	TG_OPEN_TYPE__TAG__VDMX       = 'VDMX',
	TG_OPEN_TYPE__TAG__vhea       = 'vhea',
	TG_OPEN_TYPE__TAG__vmtx       = 'vmtx',
	TG_OPEN_TYPE__TAG__VORG       = 'VORG',
	TG_OPEN_TYPE__TAG__VVAR       = 'VVAR',
} tg_open_type__tag;



typedef struct tg_open_type__offset_table
{
	u32    sfnt_version;
	u16    num_tables;
	u16    search_range;
	u16    entry_selector;
	u16    range_shift;
} tg_open_type__offset_table;

typedef struct tg_open_type__table_record
{
	char           p_table_tag[4];
	u32            checksum;
	tg_offset32    offset;
	u32            length;
} tg_open_type__table_record;

typedef struct tg_open_type
{
	tg_open_type__offset_table    offset_table;
	tg_open_type__table_record    p_table_records[0];
} tg_open_type;



static void tg__open_type__convert_class_def_format(const void* p_src, void* p_dst)
{
	*(u16*)p_dst = TG_OPEN_TYPE_U16(*(u16*)p_src);

	if (*(u16*)p_dst == 1)
	{
		TG_INVALID_CODEPATH(); // TODO: untested

		const tg_open_type__class_def_format_1__table* p_src_class_def_format_1 = (tg_open_type__class_def_format_1__table*)p_src;
		tg_open_type__class_def_format_1__table* p_dst_class_def_format_1 = (tg_open_type__class_def_format_1__table*)p_dst;

		p_dst_class_def_format_1->start_glyph_id = TG_OPEN_TYPE_U16(p_src_class_def_format_1->start_glyph_id);
		p_dst_class_def_format_1->glyph_count = TG_OPEN_TYPE_U16(p_src_class_def_format_1->glyph_count);
		for (u16 glyph_idx = 0; glyph_idx < p_dst_class_def_format_1->glyph_count; glyph_idx++)
		{
			p_dst_class_def_format_1->p_class_value_array[glyph_idx] = TG_OPEN_TYPE_U16(p_src_class_def_format_1->p_class_value_array[glyph_idx]);
		}
	}
	else
	{
		TG_ASSERT(*(u16*)p_dst == 2);

		const tg_open_type__class_def_format_2__table* p_src_class_def_format_2 = (tg_open_type__class_def_format_2__table*)p_src;
		tg_open_type__class_def_format_2__table* p_dst_class_def_format_2 = (tg_open_type__class_def_format_2__table*)p_dst;

		p_dst_class_def_format_2->class_range_count = TG_OPEN_TYPE_U16(p_src_class_def_format_2->class_range_count);
		for (u32 class_range_idx = 0; class_range_idx < p_dst_class_def_format_2->class_range_count; class_range_idx++)
		{
			const tg_open_type__class_range_record* p_src_class_range_record = &p_src_class_def_format_2->p_class_range_records[class_range_idx];
			tg_open_type__class_range_record* p_dst_class_range_record = &p_dst_class_def_format_2->p_class_range_records[class_range_idx];

			p_dst_class_range_record->start_glyph = TG_OPEN_TYPE_U16(p_src_class_range_record->start_glyph);
			p_dst_class_range_record->end_glyph = TG_OPEN_TYPE_U16(p_src_class_range_record->end_glyph);
			p_dst_class_range_record->class = TG_OPEN_TYPE_U16(p_src_class_range_record->class);
		}
	}
}

static void tg__open_type__fill_mapping(const tg_open_type__cmap* p_cmap, TG_OUT tg_open_type_font* p_font)
{
	TG_ASSERT(TG_OPEN_TYPE_U16(p_cmap->version) == 0);
	const u16 num_encoding_records = TG_OPEN_TYPE_U16(p_cmap->num_tables);
	for (u16 encoding_record_idx = 0; encoding_record_idx < num_encoding_records; encoding_record_idx++)
	{
		const tg_open_type__encoding_record* p_encoding_record = &p_cmap->p_encoding_records[encoding_record_idx];
		const tg_open_type__platform platform_id = (tg_open_type__platform)TG_OPEN_TYPE_U16(p_encoding_record->platform_id);
		if (platform_id == TG_OPEN_TYPE__PLATFORM__WINDOWS)
		{
			TG_ASSERT((tg_open_type__windows__encodings)TG_OPEN_TYPE_U16(p_encoding_record->encoding_id) == TG_OPEN_TYPE__WINDOWS__ENCODING__Unicode_BMP);
			const tg_offset32 offset = TG_OPEN_TYPE_U32(p_encoding_record->offset);

			const tg_open_type__cmap_subtable_format_4* p_cmap_subtable_format_4 = (tg_open_type__cmap_subtable_format_4*)&((u8*)p_cmap)[offset];
			TG_ASSERT(TG_OPEN_TYPE_U16(p_cmap_subtable_format_4->format) == 4);
			TG_ASSERT(TG_OPEN_TYPE_U16(p_cmap_subtable_format_4->language) == 0);

			const u16 subtable__seg_count_x_2 = TG_OPEN_TYPE_U16(p_cmap_subtable_format_4->seg_count_x_2);
			TG_ASSERT(subtable__seg_count_x_2 % 2 == 0);

			const u16 seg_count = subtable__seg_count_x_2 / 2;

			const u16* p_end_code = p_cmap_subtable_format_4->p_end_code;
			const u16* p_start_code = &p_end_code[seg_count + 1];
			const i16* p_id_delta = (i16*)&p_start_code[seg_count];
			const u16* p_id_range_offset = (u16*)&p_id_delta[seg_count];

			TG_ASSERT(p_end_code[seg_count - 1] == 0xffff);
			TG_ASSERT(p_end_code[seg_count] == 0);
			TG_ASSERT(p_start_code[seg_count - 1] == 0xffff);

			for (u16 i = 0; i < seg_count - 1; i++)
			{
				const u16 start_code = TG_OPEN_TYPE_U16(p_start_code[i]);
				if (start_code > 255)// TODO: support more than 256 characters
				{
					break;
				}
				const u16 end_code = TG_OPEN_TYPE_U16(p_end_code[i]);
				TG_ASSERT(start_code <= end_code); // TODO: return missingGlyph
				const u16 id_range_offset = TG_OPEN_TYPE_U16(p_id_range_offset[i]);

				if (id_range_offset != 0)
				{
					const u16 end = TG_MIN(end_code, 255);
					for (u16 c = start_code; c <= end; c++)
					{
						const u16 off = id_range_offset / 2 + (c - start_code);
						TG_ASSERT(((u32)id_range_offset / 2 + ((u32)c - (u32)start_code)) % 65536 == off);
						const u16 glyph_id = TG_OPEN_TYPE_U16(p_id_range_offset[(u64)i + (u64)off]);
#ifdef TG_DEBUG
						// TODO: keep this for testing for now. this indexing is described here:
						// https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6cmap.html
						// glyphIndex = *( &idRangeOffset[i] + idRangeOffset[i] / 2 + (c - startCode[i]) )
						const u16 alt_glyph_id = TG_OPEN_TYPE_U16(*(&p_id_range_offset[i] + id_range_offset / 2 + (c - start_code)));
						TG_ASSERT(glyph_id == alt_glyph_id);
#endif
						p_font->p_character_to_glyph[c] = glyph_id;
					}
				}
				else
				{
					const u16 end = TG_MIN(end_code, 255);
					for (u16 c = start_code; c <= end; c++)
					{
						const u16 id_delta = TG_OPEN_TYPE_U16(p_id_delta[i]);
						const u16 glyph_id = id_delta + c;
						TG_ASSERT((u16)(((u32)id_delta + (u32)c) % 65536) == glyph_id);
						p_font->p_character_to_glyph[c] = glyph_id;
					}
				}
			}

			break;
		}
	}
}

#define TG_LOCA(glyph_idx) \
    (index_to_loc_format == 0 \
        ? 2 * (tg_offset32)TG_OPEN_TYPE_U16(p_loca->short_version.p_offsets[glyph_idx]) \
        : TG_OPEN_TYPE_U32(p_loca->long_version.p_offsets[glyph_idx]))

static void tg__open_type__glyph_size(
	u16                          glyph_idx,
	i16                          index_to_loc_format,
	const tg_open_type__loca*    p_loca,
	const u8*                    p_glyf,
	TG_OUT i16*                  p_number_of_contours,
	TG_OUT u32*                  p_number_of_points,
	TG_OUT u64*                  p_glyph_size
)
{
	i16 alt_number_of_contours = 0;
	u32 alt_number_of_points = 0;
	u64 alt_glyph_size = 0;

	if (!p_number_of_contours)
	{
		p_number_of_contours = &alt_number_of_contours;
	}
	if (!p_number_of_points)
	{
		p_number_of_points = &alt_number_of_points;
	}
	if (!p_glyph_size)
	{
		p_glyph_size = &alt_glyph_size;
	}

	const tg_offset32 start_offset = TG_LOCA(glyph_idx);
	const tg_offset32 end_offset = TG_LOCA(glyph_idx + 1);
	const u32 size = end_offset - start_offset;

	if (size > 0)
	{
		const u8* p_src_it = &p_glyf[start_offset];
		const i16 number_of_contours = TG_OPEN_TYPE_I16(*(i16*)p_src_it);
		p_src_it += sizeof(tg_open_type__glyf);

		if (number_of_contours >= 0)
		{
			const u32 logical_point_count = (u32)TG_OPEN_TYPE_U16(((u16*)p_src_it)[number_of_contours - 1]) + 1;

			*p_number_of_contours += number_of_contours;
			*p_number_of_points += logical_point_count;
			*p_glyph_size += number_of_contours * sizeof(tg_open_type_contour) + logical_point_count * sizeof(tg_open_type_point);
		}
		else
		{
			b32 more_components = TG_TRUE;
			while (more_components)
			{
				const u16 flags = TG_OPEN_TYPE_U16(((u16*)p_src_it)[0]);
				const u16 glyph_index = TG_OPEN_TYPE_U16(((u16*)p_src_it)[1]);

				p_src_it += 4;
				p_src_it += flags & TG_OPEN_TYPE__COMPOSITE_GLYPH__ARG_1_AND_2_ARE_WORDS ? 4 : 2;
				if (flags & TG_OPEN_TYPE__COMPOSITE_GLYPH__WE_HAVE_A_SCALE)
				{
					p_src_it += 2;
				}
				else if (flags & TG_OPEN_TYPE__COMPOSITE_GLYPH__WE_HAVE_AN_X_AND_Y_SCALE)
				{
					p_src_it += 4;
				}
				else if (flags & TG_OPEN_TYPE__COMPOSITE_GLYPH__WE_HAVE_A_TWO_BY_TWO)
				{
					p_src_it += 8;
				}

				tg__open_type__glyph_size(glyph_index, index_to_loc_format, p_loca, p_glyf, p_number_of_contours, p_number_of_points, p_glyph_size);
				more_components = flags & TG_OPEN_TYPE__COMPOSITE_GLYPH__MORE_COMPONENTS;
			}
		}
	}
}

static void tg__open_type__allocate(
	tg_open_type_font*           p_font,
	i16                          index_to_loc_format,
	const u8*                    p_kern,
	const tg_open_type__loca*    p_loca,
	const u8*                    p_glyf,
	TG_OUT u64*                  p_size,
	TG_OUT void**                pp_memory
)
{
	TG_ASSERT(pp_memory);

	u64 size = (u64)p_font->glyph_count * sizeof(*p_font->p_glyphs);
	for (u16 glyph_idx = 0; glyph_idx < p_font->glyph_count; glyph_idx++)
	{
		u64 s = 0;
		tg__open_type__glyph_size(glyph_idx, index_to_loc_format, p_loca, (u8*)p_glyf, TG_NULL, TG_NULL, &s);
		size += s;
	}

	if (p_kern)
	{
		const u8* p_src_it = p_kern;

		const u16 header_version = TG_OPEN_TYPE_NEXT_U16(p_src_it);
		const u16 n_tables = TG_OPEN_TYPE_NEXT_U16(p_src_it);

		TG_ASSERT(header_version == 0);
		TG_ASSERT(n_tables == 1); // TODO: are tables packed tightly right behind each other? arial only has one table...

		for (u32 table_idx = 0; table_idx < n_tables; table_idx++)
		{
			const u8* p_temp_src_it = p_src_it;

			const u16 subtable_version = TG_OPEN_TYPE_NEXT_U16(p_src_it);
			const u16 length = TG_OPEN_TYPE_NEXT_U16(p_src_it);
			p_src_it += 2;

			if (subtable_version == 0)
			{
				const u16 n_pairs = TG_OPEN_TYPE_NEXT_U16(p_src_it);
				size += n_pairs * sizeof(tg_open_type_kerning);
			}
			else
			{
				TG_INVALID_CODEPATH();
			}

			p_src_it = &p_temp_src_it[length];
		}
	}

	if (p_size)
	{
		*p_size = size;
	}
	*pp_memory = TG_MEMORY_ALLOC(size);
}

static void tg__open_type__fill_glyph_outline(
	u16                           glyph_idx,
	i16                           index_to_loc_format,
	const tg_open_type__loca*     p_loca,
	const u8*                     p_glyf,
	TG_INOUT u8**                 pp_data,
	TG_OUT tg_open_type_glyph*    p_glyph
)
{
	const tg_offset32 start_offset = TG_LOCA(glyph_idx);
	const tg_offset32 end_offset = TG_LOCA(glyph_idx + 1);
	const u32 size = end_offset - start_offset;

	if (size > 0)
	{
		const tg_open_type__glyf* p_glyf_entry = (tg_open_type__glyf*)&p_glyf[start_offset];
		const i16 number_of_contours = TG_OPEN_TYPE_I16(p_glyf_entry->number_of_contours);

		p_glyph->x_min = TG_OPEN_TYPE_I16(p_glyf_entry->x_min);
		p_glyph->y_min = TG_OPEN_TYPE_I16(p_glyf_entry->y_min);
		p_glyph->x_max = TG_OPEN_TYPE_I16(p_glyf_entry->x_max);
		p_glyph->y_max = TG_OPEN_TYPE_I16(p_glyf_entry->y_max);

		if (number_of_contours >= 0)
		{
			TG_ASSERT(number_of_contours > 0);

			const u16* p_end_pts_of_contours = (u16*)&p_glyf_entry[1];
			const u16* p_instruction_length = (u16*)&p_end_pts_of_contours[number_of_contours];
			const u16 instruction_length = TG_OPEN_TYPE_U16(*p_instruction_length);
			const u8* p_instructions = (u8*)&p_instruction_length[1];

			const u32 logical_point_count = (u32)TG_OPEN_TYPE_U16(p_end_pts_of_contours[number_of_contours - 1]) + 1;

			u8* p_flags = TG_NULL;
			f32* p_x_coordinates = TG_NULL;
			f32* p_y_coordinates = TG_NULL;

			const u32 flags_capacity = logical_point_count * sizeof(*p_flags);
			const u32 x_coordinates_capacity = logical_point_count * sizeof(*p_x_coordinates);
			const u32 y_coordinates_capacity = logical_point_count * sizeof(*p_y_coordinates);

			p_flags = TG_MEMORY_STACK_ALLOC(flags_capacity);
			p_x_coordinates = TG_MEMORY_STACK_ALLOC(x_coordinates_capacity);
			p_y_coordinates = TG_MEMORY_STACK_ALLOC(y_coordinates_capacity);

			// Unpack flags
			const u8* p_it = &p_instructions[instruction_length];
			u32 logical_point_idx = 0;
			while (logical_point_idx < logical_point_count)
			{
				const u8 flags = *p_it++;
				TG_ASSERT(!(flags & TG_OPEN_TYPE__SIMPLE_GLYPH__Reserved));
				p_flags[logical_point_idx++] = flags;
				if (flags & TG_OPEN_TYPE__SIMPLE_GLYPH__REPEAT_FLAG)
				{
					const u8 repeat_count = *p_it++;
					for (u8 i = 0; i < repeat_count; i++)
					{
						p_flags[logical_point_idx++] = flags;
					}
				}
			}

			// Unpack x coordinates
			logical_point_idx = 0;
			while (logical_point_idx < logical_point_count)
			{
				const u8 flags = p_flags[logical_point_idx];

				const b32 x_is_same_or_positive_x_short_vector = flags & TG_OPEN_TYPE__SIMPLE_GLYPH__X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR;
				i16 relative_x_coordinate = 0;
				if (flags & TG_OPEN_TYPE__SIMPLE_GLYPH__X_SHORT_VECTOR)
				{
					relative_x_coordinate = (i16)*p_it++;
					if (!x_is_same_or_positive_x_short_vector)
					{
						relative_x_coordinate = -relative_x_coordinate;
					}
				}
				else
				{
					if (!x_is_same_or_positive_x_short_vector)
					{
						relative_x_coordinate = 256 * (i16)p_it[0] + (i16)p_it[1];
						p_it = &p_it[2];
					}
				}

				const f32 absolute_x_coordinate = (logical_point_idx == 0 ? 0.0f : p_x_coordinates[logical_point_idx - 1]) + (f32)relative_x_coordinate;
				TG_ASSERT(absolute_x_coordinate >= (f32)p_glyph->x_min && absolute_x_coordinate <= (f32)p_glyph->x_max);
				p_x_coordinates[logical_point_idx++] = absolute_x_coordinate;
			}

			// Unpack y coordinates
			logical_point_idx = 0;
			while (logical_point_idx < logical_point_count)
			{
				const u8 flags = p_flags[logical_point_idx];

				const b32 y_is_same_or_positive_y_short_vector = flags & TG_OPEN_TYPE__SIMPLE_GLYPH__Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR;
				i16 relative_y_coordinate = 0;
				if (flags & TG_OPEN_TYPE__SIMPLE_GLYPH__Y_SHORT_VECTOR)
				{
					relative_y_coordinate = (i16)*p_it++;
					if (!y_is_same_or_positive_y_short_vector)
					{
						relative_y_coordinate = -relative_y_coordinate;
					}
				}
				else
				{
					if (!y_is_same_or_positive_y_short_vector)
					{
						relative_y_coordinate = 256 * (i16)p_it[0] + (i16)p_it[1];
						p_it = &p_it[2];
					}
				}

				const f32 absolute_y_coordinate = (logical_point_idx == 0 ? 0.0f : p_y_coordinates[logical_point_idx - 1]) + (f32)relative_y_coordinate;
				TG_ASSERT(absolute_y_coordinate >= (f32)p_glyph->y_min && absolute_y_coordinate <= (f32)p_glyph->y_max);
				p_y_coordinates[logical_point_idx++] = absolute_y_coordinate;
			}

			p_glyph->contour_count = (u16)number_of_contours;
			p_glyph->p_contours = (tg_open_type_contour*)*pp_data;
			*pp_data += p_glyph->contour_count * sizeof(*p_glyph->p_contours);

			u32 first_point_idx = 0;
			for (u16 contour_idx = 0; contour_idx < p_glyph->contour_count; contour_idx++)
			{
				const u32 one_past_last_point_idx = (u32)TG_OPEN_TYPE_U16(p_end_pts_of_contours[contour_idx]) + 1;

				tg_open_type_contour* p_contour = &p_glyph->p_contours[contour_idx];
				p_contour->point_count = one_past_last_point_idx - first_point_idx;
				p_contour->p_points = (tg_open_type_point*)*pp_data;
				*pp_data += p_contour->point_count * sizeof(*p_contour->p_points);

				for (u32 point_idx = 0; point_idx < p_contour->point_count; point_idx++)
				{
					tg_open_type_point* p_point = &p_contour->p_points[point_idx];
					p_point->x = p_x_coordinates[first_point_idx + point_idx];
					p_point->y = p_y_coordinates[first_point_idx + point_idx];
					p_point->flags = p_flags[first_point_idx + point_idx];
				}

				first_point_idx = one_past_last_point_idx;
			}

			TG_MEMORY_STACK_FREE(y_coordinates_capacity);
			TG_MEMORY_STACK_FREE(x_coordinates_capacity);
			TG_MEMORY_STACK_FREE(flags_capacity);
		}
		else
		{
			TG_ASSERT(number_of_contours == -1);

			p_glyph->contour_count = 0;

			const u8* p_src_it = (u8*)&p_glyf_entry[1];
			b32 more_components = TG_TRUE;
			while (more_components)
			{
				const u16 flags = TG_OPEN_TYPE_U16(((u16*)p_src_it)[0]);
				const u16 glyph_index = TG_OPEN_TYPE_U16(((u16*)p_src_it)[1]);

				p_src_it += 4;
				p_src_it += flags & TG_OPEN_TYPE__COMPOSITE_GLYPH__ARG_1_AND_2_ARE_WORDS ? 4 : 2;
				if (flags & TG_OPEN_TYPE__COMPOSITE_GLYPH__WE_HAVE_A_SCALE)
				{
					p_src_it += 2;
				}
				else if (flags & TG_OPEN_TYPE__COMPOSITE_GLYPH__WE_HAVE_AN_X_AND_Y_SCALE)
				{
					p_src_it += 4;
				}
				else if (flags & TG_OPEN_TYPE__COMPOSITE_GLYPH__WE_HAVE_A_TWO_BY_TWO)
				{
					p_src_it += 8;
				}

				i16 num_of_contours = 0;
				tg__open_type__glyph_size(glyph_index, index_to_loc_format, p_loca, p_glyf, &num_of_contours, TG_NULL, TG_NULL);

				TG_ASSERT(num_of_contours > 0);
				p_glyph->contour_count += (u16)num_of_contours;
				more_components = flags & TG_OPEN_TYPE__COMPOSITE_GLYPH__MORE_COMPONENTS;
			}

			p_glyph->p_contours = (tg_open_type_contour*)*pp_data;
			*pp_data += p_glyph->contour_count * sizeof(*p_glyph->p_contours);

			p_src_it = (u8*)&p_glyf_entry[1];
			more_components = TG_TRUE;
			u32 contour_idx = 0;
			while (more_components)
			{
				const u16 flags = TG_OPEN_TYPE_U16(((u16*)p_src_it)[0]);
				const u16 glyph_index = TG_OPEN_TYPE_U16(((u16*)p_src_it)[1]);
				p_src_it += 4;

				m3 matrix = tgm_m3_identity();
				if (flags & TG_OPEN_TYPE__COMPOSITE_GLYPH__ARGS_ARE_XY_VALUES)
				{
					if (flags & TG_OPEN_TYPE__COMPOSITE_GLYPH__ARG_1_AND_2_ARE_WORDS)
					{
						matrix.m02 = (f32)TG_OPEN_TYPE_I16(((i16*)p_src_it)[0]);
						matrix.m12 = (f32)TG_OPEN_TYPE_I16(((i16*)p_src_it)[1]);
						p_src_it += 4;
					}
					else
					{
						matrix.m02 = (f32)((i8*)p_src_it)[0];
						matrix.m12 = (f32)((i8*)p_src_it)[1];
						p_src_it += 2;
					}
				}
				else
				{
					TG_NOT_IMPLEMENTED();
				}

				if (flags & TG_OPEN_TYPE__COMPOSITE_GLYPH__WE_HAVE_A_SCALE)
				{
					matrix.m00 = TG_OPEN_TYPE_F2DOT14_2_F32(TG_OPEN_TYPE_F2DOT14(((tg_f2dot14*)p_src_it)[0]));
					matrix.m11 = matrix.m00;
					p_src_it += 2;
				}
				else if (flags & TG_OPEN_TYPE__COMPOSITE_GLYPH__WE_HAVE_AN_X_AND_Y_SCALE)
				{
					matrix.m00 = TG_OPEN_TYPE_F2DOT14_2_F32(TG_OPEN_TYPE_F2DOT14(((tg_f2dot14*)p_src_it)[0]));
					matrix.m11 = TG_OPEN_TYPE_F2DOT14_2_F32(TG_OPEN_TYPE_F2DOT14(((tg_f2dot14*)p_src_it)[1]));
					p_src_it += 4;
				}
				else if (flags & TG_OPEN_TYPE__COMPOSITE_GLYPH__WE_HAVE_A_TWO_BY_TWO)
				{
					matrix.m00 = TG_OPEN_TYPE_F2DOT14_2_F32(TG_OPEN_TYPE_F2DOT14(((tg_f2dot14*)p_src_it)[0]));
					matrix.m01 = TG_OPEN_TYPE_F2DOT14_2_F32(TG_OPEN_TYPE_F2DOT14(((tg_f2dot14*)p_src_it)[1]));
					matrix.m10 = TG_OPEN_TYPE_F2DOT14_2_F32(TG_OPEN_TYPE_F2DOT14(((tg_f2dot14*)p_src_it)[2]));
					matrix.m11 = TG_OPEN_TYPE_F2DOT14_2_F32(TG_OPEN_TYPE_F2DOT14(((tg_f2dot14*)p_src_it)[3]));
					p_src_it += 8;
				}

				u64 temp_glyph_size = 0;
				tg__open_type__glyph_size(glyph_index, index_to_loc_format, p_loca, p_glyf, TG_NULL, TG_NULL, &temp_glyph_size);

				tg_open_type_glyph temp_glyph = { 0 };
				u8* p_temp_data = TG_MEMORY_STACK_ALLOC(temp_glyph_size);

				tg__open_type__fill_glyph_outline(glyph_index, index_to_loc_format, p_loca, p_glyf, &p_temp_data, &temp_glyph);

				for (u16 cont_idx = 0; cont_idx < temp_glyph.contour_count; cont_idx++)
				{
					tg_open_type_contour* p_contour = &p_glyph->p_contours[contour_idx++];
					const tg_open_type_contour* p_temp_contour = &temp_glyph.p_contours[cont_idx];
					TG_ASSERT(p_temp_contour->point_count > 0);

					p_contour->point_count = p_temp_contour->point_count;
					p_contour->p_points = (tg_open_type_point*)*pp_data;
					*pp_data += p_contour->point_count * sizeof(*p_contour->p_points);

					for (u32 point_idx = 0; point_idx < p_temp_contour->point_count; point_idx++)
					{
						const tg_open_type_point* p_temp_point = &p_temp_contour->p_points[point_idx];
						const v3 c0 = { p_temp_point->x, p_temp_point->y, 1.0f };
						const v3 c1 = tgm_m3_mulv3(matrix, c0);

						p_contour->p_points[point_idx].x = c1.x; // TODO: rounding? iirc, there's a flag for that...
						p_contour->p_points[point_idx].y = c1.y;
						p_contour->p_points[point_idx].flags = p_temp_point->flags;
					}
				}

				TG_MEMORY_STACK_FREE(temp_glyph_size);

				more_components = flags & TG_OPEN_TYPE__COMPOSITE_GLYPH__MORE_COMPONENTS;
			}
		}
	}
}

static void tg__open_type__fill_glyph_outlines(i16 index_to_loc_format, const tg_open_type__loca* p_loca, const tg_open_type__glyf* p_glyf, TG_INOUT u8** pp_memory, TG_INOUT tg_open_type_font* p_font)
{
	p_font->p_glyphs = (tg_open_type_glyph*)*pp_memory;
	*pp_memory += p_font->glyph_count * sizeof(*p_font->p_glyphs);

	for (u16 glyph_idx = 0; glyph_idx < p_font->glyph_count; glyph_idx++)
	{
		tg__open_type__fill_glyph_outline(glyph_idx, index_to_loc_format, p_loca, (u8*)p_glyf, pp_memory, &p_font->p_glyphs[glyph_idx]);
	}
}

static void tg__open_type__fill_metrics(const u8* p_hhea, const u8* p_hmtx, TG_INOUT tg_open_type_font* p_font)
{
	TG_ASSERT(TG_OPEN_TYPE_U16(((u16*)p_hhea)[0]) == 1); // Major version number of the horizontal header table — set to 1.
	TG_ASSERT(TG_OPEN_TYPE_U16(((u16*)p_hhea)[1]) == 0); // Minor version number of the horizontal header table — set to 0.
	const u16 number_of_h_metrics = TG_OPEN_TYPE_U16(*(u16*)&p_hhea[34]);

	const u8* p_it = p_hmtx;
	for (u16 glyph_idx = 0; glyph_idx < number_of_h_metrics; glyph_idx++)
	{
		p_font->p_glyphs[glyph_idx].advance_width = TG_OPEN_TYPE_U16(*(u16*)p_it);
		p_it += 2;

		p_font->p_glyphs[glyph_idx].left_side_bearing = TG_OPEN_TYPE_I16(*(i16*)p_it);
		p_it += 2;
	}

	if (p_font->glyph_count > number_of_h_metrics)
	{
		p_it -= 4;
		const u16 advance_width = TG_OPEN_TYPE_U16(*(u16*)p_it);
		p_it += 4;

		for (u16 glyph_idx = number_of_h_metrics; glyph_idx < p_font->glyph_count; glyph_idx++)
		{
			p_font->p_glyphs[glyph_idx].advance_width = advance_width;

			p_font->p_glyphs[glyph_idx].left_side_bearing = TG_OPEN_TYPE_I16(*(i16*)p_it);
			p_it += 2;
		}
	}
}

static void tg__open_type__fill_kerning(const u8* p_kern, u8** pp_memory, TG_INOUT tg_open_type_font* p_font)
{
	// TODO: this probably can be removed, if i decide to nullify all allocated memory
	for (u16 glyph_idx = 0; glyph_idx < p_font->glyph_count; glyph_idx++)
	{
		tg_open_type_glyph* p_glyph = &p_font->p_glyphs[glyph_idx];
		p_glyph->kerning_count = 0;
		p_glyph->p_kernings = TG_NULL;
	}

	if (p_kern)
	{
		const u8* p_src_it = p_kern;

		const u16 header_version = TG_OPEN_TYPE_NEXT_U16(p_src_it);
		const u16 n_tables = TG_OPEN_TYPE_NEXT_U16(p_src_it);

		TG_ASSERT(header_version == 0);
		TG_ASSERT(n_tables == 1); // TODO: are tables packed tightly right behind each other? arial only has one table...

		for (u32 table_idx = 0; table_idx < n_tables; table_idx++)
		{
			const u16 subtable_version = TG_OPEN_TYPE_NEXT_U16(p_src_it);
			p_src_it += 2;
			const u16 coverage = TG_OPEN_TYPE_NEXT_U16(p_src_it);

			TG_ASSERT(subtable_version == 0);
			TG_ASSERT(coverage & 1);

			if (subtable_version == 0)
			{
				const u16 n_pairs = TG_OPEN_TYPE_NEXT_U16(p_src_it);
				p_src_it += 6;

				u16 last_left = 0;
				u16 last_right = 0;
				tg_open_type_glyph* p_glyph = &p_font->p_glyphs[0];
				p_glyph->kerning_count = 0;
				p_glyph->p_kernings = *(tg_open_type_kerning**)pp_memory;
				for (u32 pair_idx = 0; pair_idx < n_pairs; pair_idx++)
				{
					const u16 left = TG_OPEN_TYPE_NEXT_U16(p_src_it);
					const u16 right = TG_OPEN_TYPE_NEXT_U16(p_src_it);
					const tg_fword value = TG_OPEN_TYPE_NEXT_FWORD(p_src_it);

					if (left != last_left)
					{
						*pp_memory += p_glyph->kerning_count * sizeof(*p_glyph->p_kernings);
						p_glyph = &p_font->p_glyphs[left];
						p_glyph->kerning_count = 0;
						p_glyph->p_kernings = *(tg_open_type_kerning**)pp_memory;
					}

					tg_open_type_kerning* p_kerning = &p_glyph->p_kernings[p_glyph->kerning_count++];
					p_kerning->right = right;
					p_kerning->value = value;

#ifdef TG_DEBUG
					TG_ASSERT(last_left <= left);
					if (last_left == left)
					{
						TG_ASSERT(last_right <= right);
					}
					last_left = left;
					last_right = right;
#endif
				}
				*pp_memory += p_glyph->kerning_count * sizeof(*p_glyph->p_kernings);
			}
		}
	}
}

#undef TG_LOCA

static void* tg__open_type__find_table(const tg_open_type* p_open_type, tg_open_type__tag tag)
{
	void* p_result = TG_NULL;

	const u16 offset_table__num_tables = TG_OPEN_TYPE_U16(p_open_type->offset_table.num_tables);
	for (u32 table_idx = 0; table_idx < offset_table__num_tables; table_idx++)
	{
		const tg_open_type__table_record* p_table_record = &p_open_type->p_table_records[table_idx];
		const tg_open_type__tag t = (tg_open_type__tag)TG_OPEN_TYPE_U32(*(u32*)p_table_record->p_table_tag);
		if (t == tag)
		{
			const tg_offset32 offset = TG_OPEN_TYPE_U32(p_table_record->offset);
			p_result = (void*)&((u8*)p_open_type)[offset];
			break;
		}
	}

	return p_result;
}

static void tg__open_type__load(char* p_buffer, TG_OUT tg_open_type_font* p_font)
{
	const tg_open_type* p_open_type = (tg_open_type*)p_buffer;

	const tg_open_type__cmap* p_cmap = tg__open_type__find_table(p_open_type, TG_OPEN_TYPE__TAG__cmap);
	const tg_open_type__glyf* p_glyf = tg__open_type__find_table(p_open_type, TG_OPEN_TYPE__TAG__glyf);
	const tg_open_type__head* p_head = tg__open_type__find_table(p_open_type, TG_OPEN_TYPE__TAG__head);
	const u8* p_hhea = (u8*)tg__open_type__find_table(p_open_type, TG_OPEN_TYPE__TAG__hhea);
	const u8* p_hmtx = (u8*)tg__open_type__find_table(p_open_type, TG_OPEN_TYPE__TAG__hmtx);
	const u8* p_kern = (u8*)tg__open_type__find_table(p_open_type, TG_OPEN_TYPE__TAG__kern);
	const tg_open_type__loca* p_loca = tg__open_type__find_table(p_open_type, TG_OPEN_TYPE__TAG__loca);
	const tg_open_type__maxp* p_maxp = tg__open_type__find_table(p_open_type, TG_OPEN_TYPE__TAG__maxp);

	p_font->x_min = TG_OPEN_TYPE_I16(p_head->x_min);
	p_font->x_max = TG_OPEN_TYPE_I16(p_head->x_max);
	p_font->y_min = TG_OPEN_TYPE_I16(p_head->y_min);
	p_font->y_max = TG_OPEN_TYPE_I16(p_head->y_max);
	p_font->units_per_em = TG_OPEN_TYPE_U16(p_head->units_per_em);
	p_font->glyph_count = TG_OPEN_TYPE_U16(p_maxp->version_05.num_glyphs);
	TG_ASSERT(TG_OPEN_TYPE_U16(p_head->flags) & 1); // Left sidebearing point at x=0 (relevant only for TrueType rasterizers) — see the note below regarding variable fonts.

	const i16 index_to_loc_format = TG_OPEN_TYPE_I16(p_head->index_to_loc_format);

	u64 size = 0;
	void* p_memory = TG_NULL;
	tg__open_type__allocate(p_font, index_to_loc_format, p_kern, p_loca, (u8*)p_glyf, &size, &p_memory);
#ifdef TG_DEBUG
	const u8* p_memory_start = (u8*)p_memory;
#endif

	tg__open_type__fill_mapping(p_cmap, p_font);
	tg__open_type__fill_glyph_outlines(index_to_loc_format, p_loca, p_glyf, (u8**)&p_memory, p_font);
	tg__open_type__fill_metrics(p_hhea, p_hmtx, p_font);
	tg__open_type__fill_kerning(p_kern, (u8**)&p_memory, p_font);

#ifdef TG_DEBUG
	const u64 used_size = (u8*)p_memory - p_memory_start;
	TG_ASSERT(used_size == size);
#endif
}



void tg_font_load(const char* p_filename, TG_OUT tg_open_type_font* p_font)
{
	tg_file_properties file_properties = { 0 };
	const b32 get_file_properties_result = tgp_file_get_properties(p_filename, &file_properties);
	TG_ASSERT(get_file_properties_result);
	
	char* p_buffer = TG_MEMORY_STACK_ALLOC(file_properties.size);
	tgp_file_load(p_filename, file_properties.size, p_buffer);

	if (tg_string_equal(file_properties.p_extension, "ttf"))
	{
		tg__open_type__load(p_buffer, p_font);
	}
	else
	{
		TG_INVALID_CODEPATH();
	}

	TG_MEMORY_STACK_FREE(file_properties.size);
}

void tg_font_free(tg_open_type_font* p_font)
{
	TG_ASSERT(p_font && p_font->p_glyphs);

	TG_MEMORY_FREE(p_font->p_glyphs);
}

const tg_open_type_glyph* tg_font_get_glyph(const tg_open_type_font* p_font, unsigned char c)
{
	const u16 glyph_idx = p_font->p_character_to_glyph[c];
	const tg_open_type_glyph* p_glyph = &p_font->p_glyphs[glyph_idx];
	return p_glyph;
}

i16 tg_font_get_kerning(const tg_open_type_font* p_font, unsigned char left_c, unsigned char right_c)
{
	i16 result = 0;

	const tg_open_type_glyph* p_left_glyph = tg_font_get_glyph(p_font, left_c);
	const u16 right_glyph_idx = p_font->p_character_to_glyph[right_c];
	for (u16 kerning_idx = 0; kerning_idx < p_left_glyph->kerning_count; kerning_idx++) // TODO: binary search
	{
		const tg_open_type_kerning* p_kerning = &p_left_glyph->p_kernings[kerning_idx];
		if (p_kerning->right == right_glyph_idx)
		{
			result = p_kerning->value;
			break;
		}
	}

	return result;
}

void tg_font_rasterize_pt(const tg_open_type_font* p_font, const tg_open_type_glyph* p_glyph, u32 glyph_off_x, u32 glyph_off_y, u32 size_pt, u32 img_w, u32 img_h, TG_OUT u8* p_image_data)
{
	const f32 glyph_w = TG_FONT_GRID2PX(*p_font, size_pt, p_glyph->x_max - p_glyph->x_min);
	const f32 glyph_h = TG_FONT_GRID2PX(*p_font, size_pt, p_glyph->y_max - p_glyph->y_min);
	const f32 add_off_x = TG_FONT_GRID2PX(*p_font, size_pt, p_glyph->left_side_bearing);
	const f32 add_off_y = TG_FONT_GRID2PX(*p_font, size_pt, p_glyph->y_min - p_font->y_min);
	tg_font_rasterize_wh(p_glyph, glyph_off_x + (u32)add_off_x, glyph_off_y + (u32)add_off_y, (u32)glyph_w, (u32)glyph_h, img_w, img_h, p_image_data);
}

void tg_font_rasterize_wh(const tg_open_type_glyph* p_glyph, u32 glyph_off_x, u32 glyph_off_y, u32 glyph_w, u32 glyph_h, u32 img_w, u32 img_h, TG_OUT u8* p_image_data)
{
	TG_ASSERT(glyph_off_x + glyph_w <= img_w);
	TG_ASSERT(glyph_off_y + glyph_h <= img_h);

	for (u32 y = 0; y < glyph_h; y++)
	{
		const f32 normalized_y = ((f32)y + 0.5f) / (f32)glyph_h;
		const f32 glyph_y = (f32)p_glyph->y_min + normalized_y * (f32)(p_glyph->y_max - p_glyph->y_min);

		for (u32 x = 0; x < glyph_w; x++)
		{
			const f32 normalized_x = ((f32)x + 0.5f) / (f32)glyph_w;
			const f32 glyph_x = (f32)p_glyph->x_min + normalized_x * (f32)(p_glyph->x_max - p_glyph->x_min);

			u32 hit_count = 0;

			for (u16 contour_idx = 0; contour_idx < p_glyph->contour_count; contour_idx++)
			{
				const tg_open_type_contour* p_contour = &p_glyph->p_contours[contour_idx];
				u32 point_idx = 0;
				tg_open_type_point p0 = p_contour->p_points[point_idx++];
				while (point_idx < p_contour->point_count + 1)
				{
					TG_ASSERT(p0.flags & TG_OPEN_TYPE__SIMPLE_GLYPH__ON_CURVE_POINT);
					const tg_open_type_point p1 = p_contour->p_points[point_idx % p_contour->point_count];
					point_idx++;

					if (p1.flags & TG_OPEN_TYPE__SIMPLE_GLYPH__ON_CURVE_POINT) // straight line
					{
						const f32 denom_y = p1.y - p0.y;
						if (denom_y != 0.0f)
						{
							const f32 t = (glyph_y - p0.y) / denom_y;
							if (t >= 0.0f && t <= 1.0f)
							{
								const f32 x0 = p0.x + t * (p1.x - p0.x);
								if (x0 >= glyph_x)
								{
									hit_count++;
								}
							}
						}

						p0 = p1;
					}
					else // quadratic second-order bezier splines
					{
						tg_open_type_point p2 = p_contour->p_points[point_idx % p_contour->point_count];

						if (p2.flags & TG_OPEN_TYPE__SIMPLE_GLYPH__ON_CURVE_POINT)
						{
							point_idx++;
						}
						else
						{
							p2.x = (p1.x + p2.x) / 2.0f;
							p2.y = (p1.y + p2.y) / 2.0f;
							p2.flags |= TG_OPEN_TYPE__SIMPLE_GLYPH__ON_CURVE_POINT;
						}

						const f32 dy = p0.y - 2.0f * p1.y + p2.y;
						if (dy != 0.0f)
						{
							const f32 n0 = p0.y - p1.y;
							const f32 n1 = p1.y - p0.y;
							const f32 n2 = p0.y - glyph_y;
							const f32 a = n0 / dy;
							const f32 b0 = n1 / dy;
							const f32 b = b0 * b0;
							const f32 c = n2 / dy;
							const f32 s = b - c;
							if (s >= 0.0f)
							{
								const f32 r = tgm_f32_sqrt(s);
								const f32 t0 = a + r;
								const f32 t1 = a - r;
								if (t0 >= 0.0f && t0 <= 1.0f)
								{
									const f32 omt0 = 1.0f - t0;
									const f32 x0 = omt0 * omt0 * p0.x + 2.0f * omt0 * t0 * p1.x + t0 * t0 * p2.x;
									if (x0 >= glyph_x)
									{
										hit_count++;
									}
								}
								if (t1 != t0 && t1 >= 0.0f && t1 <= 1.0f)
								{
									const f32 omt1 = 1.0f - t1;
									const f32 x1 = omt1 * omt1 * p0.x + 2.0f * omt1 * t1 * p1.x + t1 * t1 * p2.x;
									if (x1 >= glyph_x)
									{
										hit_count++;
									}
								}
							}
						}

						p0 = p2;
					}
				}
			}

			if (hit_count & 1)
			{
				p_image_data[(img_h - 1 - y - glyph_off_y) * img_w + (x + glyph_off_x)] = (hit_count & 1) * 0xff;
			}
		}
	}
}
