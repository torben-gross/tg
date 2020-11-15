#include "graphics/font/tg_font_io.h"

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
#include "memory/tg_memory.h"
#include "platform/tg_platform.h"
#include "util/tg_string.h"



// source: https://docs.microsoft.com/en-us/typography/opentype/spec/

#define TG_SFNT_VERSION_TRUE_TYPE        0x00010000
#define TG_SFNT_VERSION_OPEN_TYPE        'OTTO'

#define TG_SWITCH_ENDIANNESS_16(x)       ((((*(u16*)&(x)) & 0x00ff) << 8) | (((*(u16*)&(x)) & 0xff00) >> 8))
#define TG_SWITCH_ENDIANNESS_32(x)       ((((*(u32*)&(x)) & 0x000000ff) << 24) | (((*(u32*)&(x)) & 0x0000ff00) << 8) | (((*(u32*)&(x)) & 0x00ff0000) >> 8) | (((*(u32*)&(x)) & 0xff000000) >> 24))



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



void tg__convert_class_def_format(const void* p_src, void* p_dst)
{
	*(u16*)p_dst = TG_SWITCH_ENDIANNESS_16(*(const u16*)p_src);

	if (*(u16*)p_dst == 1)
	{
		TG_INVALID_CODEPATH(); // TODO: untested

		const tg_open_type__class_def_format_1__table* p_src_class_def_format_1 = (tg_open_type__class_def_format_1__table*)p_src;
		tg_open_type__class_def_format_1__table* p_dst_class_def_format_1 = (tg_open_type__class_def_format_1__table*)p_dst;

		p_dst_class_def_format_1->start_glyph_id = TG_SWITCH_ENDIANNESS_16(p_src_class_def_format_1->start_glyph_id);
		p_dst_class_def_format_1->glyph_count = TG_SWITCH_ENDIANNESS_16(p_src_class_def_format_1->glyph_count);
		for (u32 glyph_idx = 0; glyph_idx < p_dst_class_def_format_1->glyph_count; glyph_idx++)
		{
			p_dst_class_def_format_1->p_class_value_array[glyph_idx] = TG_SWITCH_ENDIANNESS_16(p_src_class_def_format_1->p_class_value_array[glyph_idx]);
		}
	}
	else
	{
		TG_ASSERT(*(u16*)p_dst == 2);

		const tg_open_type__class_def_format_2__table* p_src_class_def_format_2 = (tg_open_type__class_def_format_2__table*)p_src;
		tg_open_type__class_def_format_2__table* p_dst_class_def_format_2 = (tg_open_type__class_def_format_2__table*)p_dst;

		p_dst_class_def_format_2->class_range_count = TG_SWITCH_ENDIANNESS_16(p_src_class_def_format_2->class_range_count);
		for (u32 class_range_idx = 0; class_range_idx < p_dst_class_def_format_2->class_range_count; class_range_idx++)
		{
			const tg_open_type__class_range_record* p_src_class_range_record = &p_src_class_def_format_2->p_class_range_records[class_range_idx];
			tg_open_type__class_range_record* p_dst_class_range_record = &p_dst_class_def_format_2->p_class_range_records[class_range_idx];

			p_dst_class_range_record->start_glyph = TG_SWITCH_ENDIANNESS_16(p_src_class_range_record->start_glyph);
			p_dst_class_range_record->end_glyph = TG_SWITCH_ENDIANNESS_16(p_src_class_range_record->end_glyph);
			p_dst_class_range_record->class = TG_SWITCH_ENDIANNESS_16(p_src_class_range_record->class);
		}
	}
}



void tg_font_load(const char* p_filename, TG_OUT tg_font* p_font)
{
	tg_file_properties file_properties = { 0 };
	const b32 get_file_properties_result = tgp_file_get_properties(p_filename, &file_properties);
	TG_ASSERT(get_file_properties_result);
	
	char* p_buffer = TG_MEMORY_STACK_ALLOC(file_properties.size);
	tgp_file_load(p_filename, file_properties.size, p_buffer);

	if (tg_string_equal(file_properties.p_extension, "ttf"))
	{
		const tg_open_type* p_open_type = (tg_open_type*)p_buffer;
		const u16 offset_table__num_tables = TG_SWITCH_ENDIANNESS_16(p_open_type->offset_table.num_tables);

		for (u32 table_idx = 0; table_idx < offset_table__num_tables; table_idx++)
		{
			const tg_open_type__table_record* p_table_record = &p_open_type->p_table_records[table_idx];

			const tg_open_type__tag table_record__table_tag = (tg_open_type__tag)TG_SWITCH_ENDIANNESS_32(p_table_record->p_table_tag);
			const tg_offset32 table_record__offset = TG_SWITCH_ENDIANNESS_32(p_table_record->offset);

			switch (table_record__table_tag)
			{
			case TG_OPEN_TYPE__TAG__AVAR: break;
			case TG_OPEN_TYPE__TAG__BASE: break;
			case TG_OPEN_TYPE__TAG__CBDT: break;
			case TG_OPEN_TYPE__TAG__CBLC: break;
			case TG_OPEN_TYPE__TAG__CFF:  break;
			case TG_OPEN_TYPE__TAG__CFF2: break;
			case TG_OPEN_TYPE__TAG__cmap:
			{
				const tg_open_type__cmap* p_cmap = (tg_open_type__cmap*)&p_buffer[table_record__offset];
				TG_ASSERT(TG_SWITCH_ENDIANNESS_16(p_cmap->version) == 0);
				const u16 num_encoding_records = TG_SWITCH_ENDIANNESS_16(p_cmap->num_tables);
				for (u16 encoding_record_idx = 0; encoding_record_idx < num_encoding_records; encoding_record_idx++)
				{
					const tg_open_type__encoding_record* p_encoding_record = &p_cmap->p_encoding_records[encoding_record_idx];
					const tg_open_type__platform platform_id = (tg_open_type__platform)TG_SWITCH_ENDIANNESS_16(p_encoding_record->platform_id);
					if (platform_id == TG_OPEN_TYPE__PLATFORM__WINDOWS)
					{
						const tg_open_type__windows__encodings encoding_id = (tg_open_type__windows__encodings)TG_SWITCH_ENDIANNESS_16(p_encoding_record->encoding_id);
						TG_ASSERT(encoding_id == TG_OPEN_TYPE__WINDOWS__ENCODING__Unicode_BMP);
						const tg_offset32 offset = TG_SWITCH_ENDIANNESS_32(p_encoding_record->offset);
						
						const tg_open_type__cmap_subtable_format_4* p_cmap_subtable_format_4 = (tg_open_type__cmap_subtable_format_4*)&((u8*)p_cmap)[offset];
						TG_ASSERT(TG_SWITCH_ENDIANNESS_16(p_cmap_subtable_format_4->format) == 4);
						TG_ASSERT(TG_SWITCH_ENDIANNESS_16(p_cmap_subtable_format_4->language) == 0);

						const u16 subtable__seg_count_x_2 = TG_SWITCH_ENDIANNESS_16(p_cmap_subtable_format_4->seg_count_x_2);
						TG_ASSERT(subtable__seg_count_x_2 % 2 == 0);

						const u16 seg_count = subtable__seg_count_x_2 / 2;

						const u16* p_end_code = p_cmap_subtable_format_4->p_end_code;
						const u16* p_start_code = &p_cmap_subtable_format_4->p_end_code[seg_count + 1];
						const i16* p_id_delta = (i16*)&p_start_code[seg_count];
						const u16* p_id_range_offset = (u16*)&p_id_delta[seg_count];

						// TODO: An array that maps character codes to glyph index values.
						// This contains the first 256 mappings and can be used if the given character is within that range
						//const u16* p_glyph_id_array = &p_id_range_offset[seg_count];

						TG_ASSERT(p_end_code[seg_count - 1] == 0xffff);
						TG_ASSERT(p_end_code[seg_count] == 0);
						TG_ASSERT(p_start_code[seg_count - 1] == 0xffff);

						for (u16 seg_idx = 0; seg_idx < seg_count - 1; seg_idx++)
						{
							const u16 start_code = TG_SWITCH_ENDIANNESS_16(p_start_code[seg_idx]);
							const u16 end_code = TG_SWITCH_ENDIANNESS_16(p_end_code[seg_idx]);
							TG_ASSERT(start_code <= end_code); // TODO: return missingGlyph
							const u16 id_range_offset = TG_SWITCH_ENDIANNESS_16(p_id_range_offset[seg_idx]);

							u16 glyph_id;
							if (id_range_offset != 0)
							{
								// glyphId = *(idRangeOffset[i] / 2 + (c - startCode[i]) + &idRangeOffset[i])

								const u16 c = start_code; // TODO: this character is the input and this is an example
								glyph_id = TG_SWITCH_ENDIANNESS_16(p_id_range_offset[seg_idx + (id_range_offset / 2 + (c - start_code))]);
							}
							else
							{
								const u16 c = start_code; // TODO: this character is the input and this is an example
								const u16 id_delta = TG_SWITCH_ENDIANNESS_16(p_id_delta[seg_idx]);
								glyph_id = id_delta + c;
							}
						}
					}
				}
			} break;
			case TG_OPEN_TYPE__TAG__COLR: break;
			case TG_OPEN_TYPE__TAG__CPAL: break;
			case TG_OPEN_TYPE__TAG__cvar: break;
			case TG_OPEN_TYPE__TAG__cvt:  break;
			case TG_OPEN_TYPE__TAG__DSIG:
			{
#if 0
				const tg_open_type__dsig_header* p_dsig = (tg_open_type__dsig_header*)&p_buffer[table_record__offset];
				p_dsig->version = TG_SWITCH_ENDIANNESS_32(p_dsig->version);
				p_dsig->num_signatures = TG_SWITCH_ENDIANNESS_16(p_dsig->num_signatures);
				p_dsig->flags = TG_SWITCH_ENDIANNESS_16(p_dsig->flags);

				TG_ASSERT(p_dsig->version == 0x00000001);
				for (u32 signature_idx = 0; signature_idx < p_dsig->num_signatures; signature_idx++)
				{
					const tg_open_type__signature_record* p_signature_record = &p_dsig->p_signature_records[signature_idx];
					p_signature_record->format = TG_SWITCH_ENDIANNESS_32(p_signature_record->format);
					p_signature_record->length = TG_SWITCH_ENDIANNESS_32(p_signature_record->length);
					p_signature_record->offset = TG_SWITCH_ENDIANNESS_32(p_signature_record->offset);

					TG_ASSERT(p_signature_record->format == 1);
					const tg_open_type__signature_block_format_1* p_signature_block = (tg_open_type__signature_block_format_1*)&((u8*)p_dsig)[p_signature_record->offset];
					p_signature_block->reserved1 = TG_SWITCH_ENDIANNESS_16(p_signature_block->reserved1);
					p_signature_block->reserved2 = TG_SWITCH_ENDIANNESS_16(p_signature_block->reserved2);
					p_signature_block->signature_length = TG_SWITCH_ENDIANNESS_32(p_signature_block->signature_length);

					TG_ASSERT(p_signature_block->reserved1 == 0);
					TG_ASSERT(p_signature_block->reserved2 == 0);
				}
#endif
			} break;
			case TG_OPEN_TYPE__TAG__EBDT: break;
			case TG_OPEN_TYPE__TAG__EBLC: break;
			case TG_OPEN_TYPE__TAG__EBSC: break;
			case TG_OPEN_TYPE__TAG__fpgm: break;
			case TG_OPEN_TYPE__TAG__fvar: break;
			case TG_OPEN_TYPE__TAG__gasp: break;
			case TG_OPEN_TYPE__TAG__GDEF:
			{
#ifdef TG_DEBUG
				const tg_open_type__gdef_header_v10* p_gdef_header = (tg_open_type__gdef_header_v10*)&p_buffer[table_record__offset];
				const u16 gdef_header__major_version = TG_SWITCH_ENDIANNESS_16(p_gdef_header->major_version);
				const u16 gdef_header__minor_version = TG_SWITCH_ENDIANNESS_16(p_gdef_header->minor_version);
				const tg_offset16 gdef_header__glyph_class_def_offset = TG_SWITCH_ENDIANNESS_16(p_gdef_header->glyph_class_def_offset);
				//const tg_offset16 gdef_header__attach_list_offset = TG_SWITCH_ENDIANNESS_16(p_gdef_header->attach_list_offset);
				//const tg_offset16 gdef_header__lig_caret_list_offset = TG_SWITCH_ENDIANNESS_16(p_gdef_header->lig_caret_list_offset);
				//const tg_offset16 gdef_header__mark_attach_class_def_offset = TG_SWITCH_ENDIANNESS_16(p_gdef_header->mark_attach_class_def_offset);

				// TODO: only version 1.0 is supported at the moment
				TG_ASSERT(gdef_header__major_version == 1);
				TG_ASSERT(gdef_header__minor_version == 0);

				tg_open_type__class_def_format_1__table* p_glyph_class_def = TG_NULL;
				//tg_open_type__attach_list__table* p_attach_list = TG_NULL;
				//tg_open_type__lig_caret_list__table* p_lig_caret_list = TG_NULL;
				//tg_open_type__class_def_format_1__table* p_mark_attach_class_def = TG_NULL;

				if (gdef_header__glyph_class_def_offset)
				{
					p_glyph_class_def = (tg_open_type__class_def_format_1__table*)&((u8*)p_gdef_header)[gdef_header__glyph_class_def_offset];
					tg__convert_class_def_format(p_glyph_class_def, p_glyph_class_def);
					if (p_glyph_class_def->class_format == 1)
					{
						TG_NOT_IMPLEMENTED();
					}
					else
					{
						TG_ASSERT(p_glyph_class_def->class_format == 2);
						const tg_open_type__class_def_format_2__table* p_glyph_class_def_format_2 = (const tg_open_type__class_def_format_2__table*)p_glyph_class_def;

						u16 glyph = 0;
						for (u16 i = 0; i < p_glyph_class_def_format_2->class_range_count; i++)
						{
							const tg_open_type__class_range_record* p_class_range_record = &p_glyph_class_def_format_2->p_class_range_records[i];
							if (p_class_range_record->class == TG_OPEN_TYPE__GLYPH_CLASS_DEF__BASE)
							{
								TG_ASSERT(p_class_range_record->start_glyph == glyph);
								glyph = p_class_range_record->end_glyph;
								if (glyph >= 255)
								{
									break;
								}
							}
						}
						TG_ASSERT(glyph >= 255);
					}
				}
#if 0
				if (gdef_header__attach_list_offset)
				{
					TG_INVALID_CODEPATH(); // TODO: untested

					p_attach_list = (tg_open_type__attach_list__table*)&((u8*)p_gdef_header)[gdef_header__attach_list_offset];
					p_attach_list->coverage_offset = TG_SWITCH_ENDIANNESS_16(p_attach_list->coverage_offset);
					p_attach_list->glyph_count = TG_SWITCH_ENDIANNESS_16(p_attach_list->glyph_count);
					for (u32 glyph_idx = 0; glyph_idx < p_attach_list->glyph_count; glyph_idx++)
					{
						p_attach_list->p_attach_point_offsets[glyph_idx] = TG_SWITCH_ENDIANNESS_16(p_attach_list->p_attach_point_offsets[glyph_idx]);
					}
				}
				if (gdef_header__lig_caret_list_offset)
				{
					p_lig_caret_list = (tg_open_type__lig_caret_list__table*)&((u8*)p_gdef_header)[gdef_header__lig_caret_list_offset];
					p_lig_caret_list->coverage_offset = TG_SWITCH_ENDIANNESS_16(p_lig_caret_list->coverage_offset);
					p_lig_caret_list->lig_glyph_count = TG_SWITCH_ENDIANNESS_16(p_lig_caret_list->lig_glyph_count);
					for (u32 lig_glyph_idx = 0; lig_glyph_idx < p_lig_caret_list->lig_glyph_count; lig_glyph_idx++)
					{
						p_lig_caret_list->p_lig_glyph_offsets[lig_glyph_idx] = TG_SWITCH_ENDIANNESS_16(p_lig_caret_list->p_lig_glyph_offsets[lig_glyph_idx]);
					}

					tg_open_type__coverage_format_1__table* p_coverage_format_1 = (tg_open_type__coverage_format_1__table*)&((u8*)p_lig_caret_list)[p_lig_caret_list->coverage_offset];
					p_coverage_format_1->coverage_format = TG_SWITCH_ENDIANNESS_16(p_coverage_format_1->coverage_format);
					if (p_coverage_format_1->coverage_format == 1)
					{
						p_coverage_format_1->glyph_count = TG_SWITCH_ENDIANNESS_16(p_coverage_format_1->glyph_count);
						for (u32 glyph_idx = 0; glyph_idx < p_coverage_format_1->glyph_count; glyph_idx++)
						{
							p_coverage_format_1->p_glyph_attay[glyph_idx] = TG_SWITCH_ENDIANNESS_16(p_coverage_format_1->p_glyph_attay[glyph_idx]);
						}
					}
					else
					{
						TG_INVALID_CODEPATH(); // TODO: 2 not yet supported
					}
				}
				if (gdef_header__mark_attach_class_def_offset)
				{
					p_mark_attach_class_def = (tg_open_type__class_def_format_1__table*)&((u8*)p_gdef_header)[gdef_header__mark_attach_class_def_offset];
					tg__convert_class_def_format(p_mark_attach_class_def, p_mark_attach_class_def);
				}
#endif
#endif
			} break;
			case TG_OPEN_TYPE__TAG__glyf:
			{
				const tg_open_type__glyf* p_glyf = (tg_open_type__glyf*)&p_buffer[table_record__offset];
				int bh = 0;
			} break;
			case TG_OPEN_TYPE__TAG__GPOS:
			{
#if 0
				u16* p_major = &((u16*)&p_buffer[table_record__offset])[0];
				u16* p_minor = &((u16*)&p_buffer[table_record__offset])[1];
				*p_major = TG_SWITCH_ENDIANNESS_16(*p_major);
				*p_minor = TG_SWITCH_ENDIANNESS_16(*p_minor);

				if (*p_major == 1)
				{
					if (*p_minor == 0)
					{
						tg_open_type__gpos_header_v10* p_gpos_header = (tg_open_type__gpos_header_v10*)&p_buffer[table_record__offset];
						p_gpos_header->script_list_offset = TG_SWITCH_ENDIANNESS_16(p_gpos_header->script_list_offset);
						p_gpos_header->feature_list_offset = TG_SWITCH_ENDIANNESS_16(p_gpos_header->feature_list_offset);
						p_gpos_header->lookup_list_offset = TG_SWITCH_ENDIANNESS_16(p_gpos_header->lookup_list_offset);

						const tg_open_type__script_list__table* p_script_list = (tg_open_type__script_list__table*)&((u8*)p_gpos_header)[p_gpos_header->script_list_offset];
						//const tg_open_type__feature_list__table* p_feature_list = (tg_open_type__feature_list__table*)&((u8*)p_gpos_header)[p_gpos_header->feature_list_offset];
						const tg_open_type__lookup_list__table* p_lookup_list = (tg_open_type__lookup_list__table*)&((u8*)p_gpos_header)[p_gpos_header->lookup_list_offset];

#if 0
						p_feature_list->feature_count = TG_SWITCH_ENDIANNESS_16(p_feature_list->feature_count);
						for (u32 feature_idx = 0; feature_idx < p_feature_list->feature_count; feature_idx++)
						{
							tg_open_type__feature_record* p_feature_record = &p_feature_list->p_feature_records[feature_idx];
							*(u32*)p_feature_record->p_feature_tag = TG_SWITCH_ENDIANNESS_32(p_feature_record->p_feature_tag);
							p_feature_record->feature_offset = TG_SWITCH_ENDIANNESS_16(p_feature_record->feature_offset);
						}
#endif

#ifdef TG_DEBUG
						const u16 script_list__script_count = TG_SWITCH_ENDIANNESS_16(p_script_list->script_count);
						b32 latin_supported = TG_FALSE;
						for (u16 script_record_idx = 0; script_record_idx < script_list__script_count; script_record_idx++)
						{
							const tg_open_type__script_record* p_script_record = &p_script_list->p_script_records[script_record_idx];
							const u32 script_record__script_tag = TG_SWITCH_ENDIANNESS_32(p_script_record->p_script_tag);
							
							if (script_record__script_tag == 'latn')
							{
								latin_supported = TG_TRUE;
								const u16 script_record__script_offset = TG_SWITCH_ENDIANNESS_16(p_script_record->script_offset);

								tg_open_type__script__table* p_script = (tg_open_type__script__table*)&((u8*)p_script_list)[script_record__script_offset];
								const tg_offset16 script__default_lang_sys = TG_SWITCH_ENDIANNESS_16(p_script->default_lang_sys);

								if (script__default_lang_sys)
								{
									tg_open_type__lang_sys__table* p_lang_sys = (tg_open_type__lang_sys__table*)&((u8*)p_script)[script__default_lang_sys];
									TG_ASSERT(TG_SWITCH_ENDIANNESS_16(p_lang_sys->lookup_order) == 0);
									TG_ASSERT(TG_SWITCH_ENDIANNESS_16(p_lang_sys->required_feature_index) == 0xffff);
									break;
								}
							}
						}
						TG_ASSERT(latin_supported);
#endif

						const u16 lookup_list__lookup_count = TG_SWITCH_ENDIANNESS_16(p_lookup_list->lookup_count);
						for (u16 lookup_idx = 0; lookup_idx < lookup_list__lookup_count; lookup_idx++)
						{
							const tg_offset16 lookup = TG_SWITCH_ENDIANNESS_16(p_lookup_list->p_lookups[lookup_idx]);
							const tg_open_type__lookup__table* p_lookup = (tg_open_type__lookup__table*)&((u8*)p_lookup_list)[lookup];
							const tg_open_type__gpos__lookup_type__enumeration lookup__lookup_type = (tg_open_type__gpos__lookup_type__enumeration)TG_SWITCH_ENDIANNESS_16(p_lookup->lookup_type);

#ifdef TG_DEBUG
							const tg_open_type__lookup_flag__bit__enumeration lookup_flags = (tg_open_type__lookup_flag__bit__enumeration)TG_SWITCH_ENDIANNESS_16(p_lookup->lookup_flag);
							TG_ASSERT((lookup_flags & TG_OPEN_TYPE__LOOKUP_FLAG__IGNORE_BASE_GLYPHS) == 0);
#endif

							if (lookup__lookup_type == TG_OPEN_TYPE__GPOS__LOOKUP_TYPE__SINGLE_ADJUSTMENT)
							{
								TG_NOT_IMPLEMENTED();
							}
							else if (lookup__lookup_type == TG_OPEN_TYPE__GPOS__LOOKUP_TYPE__EXTENSION_POSITIONING)
							{
								const u16 lookup__subtable_count = TG_SWITCH_ENDIANNESS_16(p_lookup->subtable_count);

								for (u16 subtable_idx = 0; subtable_idx < lookup__subtable_count; subtable_idx++)
								{
									const u16 lookup__subtable_offset = TG_SWITCH_ENDIANNESS_16(((u16*)&((u8*)p_lookup)[6])[subtable_idx]);
									const tg_open_type__extension_pos_format_1__subtable* p_extension_pos_format_1 = (const tg_open_type__extension_pos_format_1__subtable*)&((u8*)p_lookup)[lookup__subtable_offset];

									TG_ASSERT(TG_SWITCH_ENDIANNESS_16(p_extension_pos_format_1->pos_format) == 1);

									const tg_open_type__gpos__lookup_type__enumeration extension_pos_format_1__extension_lookup_type = (tg_open_type__gpos__lookup_type__enumeration)TG_SWITCH_ENDIANNESS_16(p_extension_pos_format_1->extension_lookup_type);
									TG_ASSERT(extension_pos_format_1__extension_lookup_type != TG_OPEN_TYPE__GPOS__LOOKUP_TYPE__EXTENSION_POSITIONING);

									if (extension_pos_format_1__extension_lookup_type == TG_OPEN_TYPE__GPOS__LOOKUP_TYPE__SINGLE_ADJUSTMENT)
									{
										const tg_offset32 extension_pos_format_1__extension_offset = TG_SWITCH_ENDIANNESS_32(p_extension_pos_format_1->extension_offset);
										const u16 single_pos__pos_format = TG_SWITCH_ENDIANNESS_16(((u8*)p_extension_pos_format_1)[extension_pos_format_1__extension_offset]);
										if (single_pos__pos_format == 1)
										{
											break;
											TG_NOT_IMPLEMENTED();
										}
										else if (single_pos__pos_format == 2)
										{
											const tg_open_type__single_pos_format_2__subtable* p_single_pos_format_2 = (tg_open_type__single_pos_format_2__subtable*)&((u8*)p_extension_pos_format_1)[extension_pos_format_1__extension_offset];

											const tg_offset16 single_pos_format_2__coverage_offset = TG_SWITCH_ENDIANNESS_16(p_single_pos_format_2->coverage_offset);
											const u16 coverage_format = TG_SWITCH_ENDIANNESS_16(*(u16*)&((u8*)p_single_pos_format_2)[single_pos_format_2__coverage_offset]);

											if (coverage_format == 1)
											{
												break;
												TG_NOT_IMPLEMENTED();
											}
											else if (coverage_format == 2)
											{
												const tg_open_type__coverage_format_2__table* p_coverage_format_2 = (tg_open_type__coverage_format_2__table*)&((u8*)p_single_pos_format_2)[single_pos_format_2__coverage_offset];

												const u16 coverage_format_2__range_count = TG_SWITCH_ENDIANNESS_16(p_coverage_format_2->range_count);
												for (u16 range_idx = 0; range_idx < coverage_format_2__range_count; range_idx++)
												{
													const tg_open_type__range_record* p_range_record = &p_coverage_format_2->p_range_records[range_idx];
													const u16 range_record__start_glyph_id = TG_SWITCH_ENDIANNESS_16(p_range_record->start_glyph_id);
													const u16 range_record__end_glyph_id = TG_SWITCH_ENDIANNESS_16(p_range_record->end_glyph_id);
													const u16 range_record__start_coverage_index = TG_SWITCH_ENDIANNESS_16(p_range_record->start_coverage_index);
												}
											}

#ifdef TG_DEBUG
											const tg_open_type__value_format__flags single_pos_format_2__value_format = (tg_open_type__value_format__flags)TG_SWITCH_ENDIANNESS_16(p_single_pos_format_2->value_format);
											const tg_open_type__value_format__flags required_flags = TG_OPEN_TYPE__VALUE_FORMAT__X_PLACEMENT | TG_OPEN_TYPE__VALUE_FORMAT__X_ADVANCE;
											TG_ASSERT((single_pos_format_2__value_format & required_flags) == required_flags);
#endif

											const u16 single_pos_format_2__value_count = TG_SWITCH_ENDIANNESS_16(p_single_pos_format_2->value_count);
											for (u16 value_idx = 0; value_idx < single_pos_format_2__value_count; value_idx++)
											{
												const tg_open_type__value_record* p_value_record = &p_single_pos_format_2->p_value_records[value_idx];
												const i16 value_record__x_placement = TG_SWITCH_ENDIANNESS_16(p_value_record->x_placement);
												const i16 value_record__x_advance = TG_SWITCH_ENDIANNESS_16(p_value_record->x_advance);
											}
										}
										else
										{
											TG_INVALID_CODEPATH();
										}
									}
								}
							}
							else
							{
								TG_INVALID_CODEPATH();
							}
						}
					}
					else if (*p_minor == 1)
					{
						TG_NOT_IMPLEMENTED();
					}
					else
					{
						TG_INVALID_CODEPATH();
					}
				}
				else
				{
					TG_INVALID_CODEPATH();
				}
#endif
			} break;
			case TG_OPEN_TYPE__TAG__GSUB: break;
			case TG_OPEN_TYPE__TAG__gvar: break;
			case TG_OPEN_TYPE__TAG__hdmx: break;
			case TG_OPEN_TYPE__TAG__head: break;
			case TG_OPEN_TYPE__TAG__hhea: break;
			case TG_OPEN_TYPE__TAG__hmtx: break;
			case TG_OPEN_TYPE__TAG__HVAR: break;
			case TG_OPEN_TYPE__TAG__JSTF: break;
			case TG_OPEN_TYPE__TAG__kern: break;
			case TG_OPEN_TYPE__TAG__loca: break;
			case TG_OPEN_TYPE__TAG__LTSH: break;
			case TG_OPEN_TYPE__TAG__MATH: break;
			case TG_OPEN_TYPE__TAG__maxp: break;
			case TG_OPEN_TYPE__TAG__MERG: break;
			case TG_OPEN_TYPE__TAG__meta: break;
			case TG_OPEN_TYPE__TAG__MVAR: break;
			case TG_OPEN_TYPE__TAG__NAME: break;
			case TG_OPEN_TYPE__TAG__OS2:  break;
			case TG_OPEN_TYPE__TAG__PCLT: break;
			case TG_OPEN_TYPE__TAG__post: break;
			case TG_OPEN_TYPE__TAG__prep: break;
			case TG_OPEN_TYPE__TAG__sbix: break;
			case TG_OPEN_TYPE__TAG__STAT: break;
			case TG_OPEN_TYPE__TAG__SVG:  break;
			case TG_OPEN_TYPE__TAG__VDMX: break;
			case TG_OPEN_TYPE__TAG__vhea: break;
			case TG_OPEN_TYPE__TAG__vmtx: break;
			case TG_OPEN_TYPE__TAG__VORG: break;
			case TG_OPEN_TYPE__TAG__VVAR: break;

			default: TG_INVALID_CODEPATH(); break;
			}
		}
	}

	TG_MEMORY_STACK_FREE(file_properties.size);
}

void tg_font_free(tg_font* p_font)
{

}
