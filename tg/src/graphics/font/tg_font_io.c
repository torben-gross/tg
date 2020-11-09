#include "graphics/font/tg_font_io.h"

#include "graphics/font/tg_open_type_types.h"
#include "graphics/font/tg_open_type_layout_common_table_formats.h"
#include "graphics/font/tg_open_type_font_variations_common_table_formats.h"
#include "graphics/font/tg_open_type_dsig.h"
#include "graphics/font/tg_open_type_gdef.h"
#include "graphics/font/tg_open_type_gpos.h"
#include "memory/tg_memory.h"
#include "platform/tg_platform.h"
#include "util/tg_string.h"



// source: https://docs.microsoft.com/en-us/typography/opentype/spec/

#define TG_SFNT_VERSION_TRUE_TYPE     0x00010000
#define TG_SFNT_VERSION_OPEN_TYPE     'OTTO'

#define TG_SWITCH_ENDIANNESS_16(x)    ((((*(u16*)&(x)) & 0x00ff) << 8) | (((*(u16*)&(x)) & 0xff00) >> 8))
#define TG_SWITCH_ENDIANNESS_32(x)    ((((*(u32*)&(x)) & 0x000000ff) << 24) | (((*(u32*)&(x)) & 0x0000ff00) << 8) | (((*(u32*)&(x)) & 0x00ff0000) >> 8) | (((*(u32*)&(x)) & 0xff000000) >> 24))



typedef enum tg_open_type_tag
{
	TG_OPEN_TYPE_TAG_AVAR       = 'avar',
	TG_OPEN_TYPE_TAG_BASE       = 'BASE',
	TG_OPEN_TYPE_TAG_CBDT       = 'CBDT',
	TG_OPEN_TYPE_TAG_CBLC       = 'CBLC',
	TG_OPEN_TYPE_TAG_CFF        = 'CFF',
	TG_OPEN_TYPE_TAG_CFF2       = 'CFF2',
	TG_OPEN_TYPE_TAG_cmap       = 'cmap',
	TG_OPEN_TYPE_TAG_COLR       = 'COLR',
	TG_OPEN_TYPE_TAG_CPAL       = 'CPAL',
	TG_OPEN_TYPE_TAG_cvar       = 'cvar',
	TG_OPEN_TYPE_TAG_cvt        = 'cvt',
	TG_OPEN_TYPE_TAG_DSIG       = 'DSIG',
	TG_OPEN_TYPE_TAG_EBDT       = 'EBDT',
	TG_OPEN_TYPE_TAG_EBLC       = 'EBLC',
	TG_OPEN_TYPE_TAG_EBSC       = 'EBSC',
	TG_OPEN_TYPE_TAG_fpgm       = 'fpgm',
	TG_OPEN_TYPE_TAG_fvar       = 'fvar',
	TG_OPEN_TYPE_TAG_gasp       = 'gasp',
	TG_OPEN_TYPE_TAG_GDEF       = 'GDEF',
	TG_OPEN_TYPE_TAG_glyf       = 'glyf',
	TG_OPEN_TYPE_TAG_GPOS       = 'GPOS',
	TG_OPEN_TYPE_TAG_GSUB       = 'GSUB',
	TG_OPEN_TYPE_TAG_gvar       = 'gvar',
	TG_OPEN_TYPE_TAG_hdmx       = 'hdmx',
	TG_OPEN_TYPE_TAG_head       = 'head',
	TG_OPEN_TYPE_TAG_hhea       = 'hhea',
	TG_OPEN_TYPE_TAG_hmtx       = 'hmtx',
	TG_OPEN_TYPE_TAG_HVAR       = 'HVAR',
	TG_OPEN_TYPE_TAG_JSTF       = 'JSTF',
	TG_OPEN_TYPE_TAG_kern       = 'kern',
	TG_OPEN_TYPE_TAG_loca       = 'loca',
	TG_OPEN_TYPE_TAG_LTSH       = 'LTSH',
	TG_OPEN_TYPE_TAG_MATH       = 'MATH',
	TG_OPEN_TYPE_TAG_maxp       = 'maxp',
	TG_OPEN_TYPE_TAG_MERG       = 'MERG',
	TG_OPEN_TYPE_TAG_meta       = 'meta',
	TG_OPEN_TYPE_TAG_MVAR       = 'MVAR',
	TG_OPEN_TYPE_TAG_PCLT       = 'PCLT',
	TG_OPEN_TYPE_TAG_post       = 'post',
	TG_OPEN_TYPE_TAG_prep       = 'prep',
	TG_OPEN_TYPE_TAG_sbix       = 'sbix',
	TG_OPEN_TYPE_TAG_STAT       = 'STAT',
	TG_OPEN_TYPE_TAG_SVG        = 'SVG',
	TG_OPEN_TYPE_TAG_VDMX       = 'VDMX',
	TG_OPEN_TYPE_TAG_vhea       = 'vhea',
	TG_OPEN_TYPE_TAG_vmtx       = 'vmtx',
	TG_OPEN_TYPE_TAG_VORG       = 'VORG',
	TG_OPEN_TYPE_TAG_VVAR       = 'VVAR',
} tg_open_type_tag;





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
	//p_glyph_class_def = (tg_open_type_class_def_format_1*)&((u8*)p_gdef_header)[p_gdef_header->glyph_class_def_offset];
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
		tg_open_type* p_open_type = (tg_open_type*)p_buffer;

		for (u32 table_idx = 0; table_idx < p_open_type->offset_table.num_tables; table_idx++)
		{
			tg_open_type__table_record* p_table_record = &p_open_type->p_table_records[table_idx];

			*(u32*)p_table_record->p_table_tag = TG_SWITCH_ENDIANNESS_32(p_table_record->p_table_tag);
			p_table_record->checksum = TG_SWITCH_ENDIANNESS_32(p_table_record->checksum);
			p_table_record->offset = TG_SWITCH_ENDIANNESS_32(p_table_record->offset);
			p_table_record->length = TG_SWITCH_ENDIANNESS_32(p_table_record->length);

			switch (*(tg_open_type_tag*)p_table_record->p_table_tag)
			{
			case TG_OPEN_TYPE_TAG_AVAR:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_BASE:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_CBDT:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_CBLC:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_CFF:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_CFF2:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_cmap:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_COLR:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_CPAL:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_cvar:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_cvt:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_DSIG:
			{
#if 0
				tg_open_type__dsig_header* p_dsig = (tg_open_type__dsig_header*)&p_buffer[p_table_record->offset];
				p_dsig->version = TG_SWITCH_ENDIANNESS_32(p_dsig->version);
				p_dsig->num_signatures = TG_SWITCH_ENDIANNESS_16(p_dsig->num_signatures);
				p_dsig->flags = TG_SWITCH_ENDIANNESS_16(p_dsig->flags);

				TG_ASSERT(p_dsig->version == 0x00000001);
				for (u32 signature_idx = 0; signature_idx < p_dsig->num_signatures; signature_idx++)
				{
					tg_open_type__signature_record* p_signature_record = &p_dsig->p_signature_records[signature_idx];
					p_signature_record->format = TG_SWITCH_ENDIANNESS_32(p_signature_record->format);
					p_signature_record->length = TG_SWITCH_ENDIANNESS_32(p_signature_record->length);
					p_signature_record->offset = TG_SWITCH_ENDIANNESS_32(p_signature_record->offset);

					TG_ASSERT(p_signature_record->format == 1);
					tg_open_type__signature_block_format_1* p_signature_block = (tg_open_type__signature_block_format_1*)&((u8*)p_dsig)[p_signature_record->offset];
					p_signature_block->reserved1 = TG_SWITCH_ENDIANNESS_16(p_signature_block->reserved1);
					p_signature_block->reserved2 = TG_SWITCH_ENDIANNESS_16(p_signature_block->reserved2);
					p_signature_block->signature_length = TG_SWITCH_ENDIANNESS_32(p_signature_block->signature_length);

					TG_ASSERT(p_signature_block->reserved1 == 0);
					TG_ASSERT(p_signature_block->reserved2 == 0);
				}
#endif
			} break;
			case TG_OPEN_TYPE_TAG_EBDT:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_EBLC:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_EBSC:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_fpgm:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_fvar:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_gasp:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_GDEF:
			{
				tg_open_type__gdef_header_v10* p_gdef_header = (tg_open_type__gdef_header_v10*)&p_buffer[p_table_record->offset];
				p_gdef_header->major_version = TG_SWITCH_ENDIANNESS_16(p_gdef_header->major_version);
				p_gdef_header->minor_version = TG_SWITCH_ENDIANNESS_16(p_gdef_header->minor_version);
				p_gdef_header->glyph_class_def_offset = TG_SWITCH_ENDIANNESS_16(p_gdef_header->glyph_class_def_offset);
				p_gdef_header->attach_list_offset = TG_SWITCH_ENDIANNESS_16(p_gdef_header->attach_list_offset);
				p_gdef_header->lig_caret_list_offset = TG_SWITCH_ENDIANNESS_16(p_gdef_header->lig_caret_list_offset);
				p_gdef_header->mark_attach_class_def_offset = TG_SWITCH_ENDIANNESS_16(p_gdef_header->mark_attach_class_def_offset);

				// TODO: only version 1.0 is supported at the moment
				TG_ASSERT(p_gdef_header->major_version == 1);
				TG_ASSERT(p_gdef_header->minor_version == 0);

				tg_open_type__class_def_format_1__table* p_glyph_class_def = TG_NULL;
				tg_open_type__attach_list__table* p_attach_list = TG_NULL;
				tg_open_type__lig_caret_list__table* p_lig_caret_list = TG_NULL;
				tg_open_type__class_def_format_1__table* p_mark_attach_class_def = TG_NULL;

				if (p_gdef_header->glyph_class_def_offset)
				{
					p_glyph_class_def = (tg_open_type__class_def_format_1__table*)&((u8*)p_gdef_header)[p_gdef_header->glyph_class_def_offset];
					tg__convert_class_def_format(p_glyph_class_def, p_glyph_class_def);
				}
				if (p_gdef_header->attach_list_offset)
				{
					TG_INVALID_CODEPATH(); // TODO: untested

					p_attach_list = (tg_open_type__attach_list__table*)&((u8*)p_gdef_header)[p_gdef_header->attach_list_offset];
					p_attach_list->coverage_offset = TG_SWITCH_ENDIANNESS_16(p_attach_list->coverage_offset);
					p_attach_list->glyph_count = TG_SWITCH_ENDIANNESS_16(p_attach_list->glyph_count);
					for (u32 glyph_idx = 0; glyph_idx < p_attach_list->glyph_count; glyph_idx++)
					{
						p_attach_list->p_attach_point_offsets[glyph_idx] = TG_SWITCH_ENDIANNESS_16(p_attach_list->p_attach_point_offsets[glyph_idx]);
					}
				}
				if (p_gdef_header->lig_caret_list_offset)
				{
					p_lig_caret_list = (tg_open_type__lig_caret_list__table*)&((u8*)p_gdef_header)[p_gdef_header->lig_caret_list_offset];
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
				if (p_gdef_header->mark_attach_class_def_offset)
				{
					p_mark_attach_class_def = (tg_open_type__class_def_format_1__table*)&((u8*)p_gdef_header)[p_gdef_header->mark_attach_class_def_offset];
					tg__convert_class_def_format(p_mark_attach_class_def, p_mark_attach_class_def);
				}

				int bh = 0;
			} break;
			case TG_OPEN_TYPE_TAG_glyf:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_GPOS:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_GSUB:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_gvar:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_hdmx:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_head:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_hhea:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_hmtx:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_HVAR:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_JSTF:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_kern:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_loca:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_LTSH:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_MATH:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_maxp:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_MERG:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_meta:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_MVAR:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_PCLT:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_post:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_prep:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_sbix:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_STAT:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_SVG:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_VDMX:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_vhea:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_vmtx:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_VORG:
			{
				
			} break;
			case TG_OPEN_TYPE_TAG_VVAR:
			{
				
			} break;

			default: TG_INVALID_CODEPATH(); break;
			}
		}
		int bh = 0;
	}

	TG_MEMORY_STACK_FREE(file_properties.size);
}

void tg_font_free(tg_font* p_font)
{

}
