#ifndef TG_OPEN_TYPE_CMAP_H
#define TG_OPEN_TYPE_CMAP_H

#include "graphics/font/tg_open_type_types.h"

typedef struct tg_open_type__uvs_mapping__record
{
	u24    unicode_value; // unicodeValue: Base Unicode value of the UVS
	u16    glyph_id;      // glyphID: Glyph ID of the UVS
} tg_open_type__uvs_mapping__record;

typedef struct tg_open_type__non_default_uvs__table
{
	u32                                  num_uvs_mappings;  // numUVSMappings: Number of UVS Mappings that follow
	tg_open_type__uvs_mapping__record    p_uvs_mappings[0]; // UVSMapping: uvsMappings[numUVSMappings]	Array of UVSMapping records.
} tg_open_type__non_default_uvs__table;

typedef struct tg_open_type__unicode_range__record
{
	u24    start_unicode_value; // startUnicodeValue: First value in this range
	u8     additional_count;    // additionalCount: Number of additional values in this range
} tg_open_type__unicode_range__record;

typedef struct tg_open_type__default_uvs__table
{
	u32                                    num_unicode_value_ranges; // numUnicodeValueRanges: Number of Unicode character ranges.
	tg_open_type__unicode_range__record    p_ranges[0];              // ranges[numUnicodeValueRanges]: Array of UnicodeRange records.
} tg_open_type__default_uvs__table;

typedef struct tg_open_type__variation_selector__record
{
	u24            var_selector;           // varSelector: Variation selector
	tg_offset32    default_uvs_offset;     // defaultUVSOffset: Offset from the start of the format 14 subtable to Default UVS Table. May be 0.
	tg_offset32    non_default_uvs_offset; // nonDefaultUVSOffset: Offset from the start of the format 14 subtable to Non-Default UVS Table. May be 0.
} tg_open_type__variation_selector__record;

typedef struct tg_open_type__cmap_subtable_format_14
{
	u16                                         format;                   // format: Subtable format. Set to 14.
	u32                                         length;                   // length: Byte length of this subtable (including this header)
	u32                                         num_var_selector_records; // numVarSelectorRecords: Number of variation Selector Records
	tg_open_type__variation_selector__record    p_var_selector[0];        // varSelector[numVarSelectorRecords]: Array of VariationSelector records.
} tg_open_type__cmap_subtable_format_14;

typedef struct tg_open_type__constant_map_group__record
{
	u32    start_char_code; // startCharCode: First character code in this group
	u32    end_char_code;   // endCharCode: Last character code in this group
	u32    glyph_id;        // glyphID: Glyph index to be used for all the characters in the group’s range.
} tg_open_type__constant_map_group__record;

typedef struct tg_open_type__cmap_subtable_format_13
{
	u16                                         format;      // uint16	format	Subtable format; set to 13.
	u16                                         reserved;    // uint16	reserved	Reserved; set to 0
	u32                                         length;      // uint32	length	Byte length of this subtable (including the header)
	u32                                         language;    // uint32	language	For requirements on use of the language field, see “Use of the language field in 'cmap' subtables” in this document.
	u32                                         num_groups;  // uint32	numGroups	Number of groupings which follow
	tg_open_type__constant_map_group__record    p_groups[0]; // ConstantMapGroup	groups[numGroups]	Array of ConstantMapGroup records.
} tg_open_type__cmap_subtable_format_13;

typedef struct tg_open_type__sequential_map_group_12__record
{
	u32    start_char_code; // startCharCode: First character code in this group
	u32    end_char_code;   // endCharCode: Last character code in this group
	u32    start_glyph_id;  // startGlyphID: Glyph index corresponding to the starting character code
} tg_open_type__sequential_map_group_12__record;

typedef struct tg_open_type__cmap_subtable_format_12
{
	u16                                              format;      // format: Subtable format; set to 12.
	u16                                              reserved;    // reserved: Reserved; set to 0
	u32                                              length;      // length: Byte length of this subtable (including the header)
	u32                                              language;    // language: For requirements on use of the language field, see “Use of the language field in 'cmap' subtables” in this document.
	u32                                              num_groups;  // numGroups: Number of groupings which follow
	tg_open_type__sequential_map_group_12__record    p_groups[0]; // groups[numGroups]: Array of SequentialMapGroup records.
} tg_open_type__cmap_subtable_format_12;

typedef struct tg_open_type__cmap_subtable_format_10
{
	u16    format;          // format: Subtable format; set to 10.
	u16    reserved;        // reserved: Reserved; set to 0
	u32    length;          // length: Byte length of this subtable (including the header)
	u32    language;        // language: For requirements on use of the language field, see “Use of the language field in 'cmap' subtables” in this document.
	u32    start_char_code; // startCharCode: First character code covered
	u32    num_chars;       // numChars: Number of character codes covered
	u16    p_glyphs[0];     // glyphs[]: Array of glyph indices for the character codes covered
} tg_open_type__cmap_subtable_format_10;

typedef struct tg_open_type__sequential_map_group_8__record
{
	u32    start_char_code; // startCharCode: First character code in this group; note that if this group is for one or more 16-bit character codes (which is determined from the is32 array), this 32-bit value will have the high 16-bits set to zero
	u32    end_char_code;   // endCharCode: Last character code in this group; same condition as listed above for the startCharCode
	u32    start_glyph_id;  // startGlyphID: Glyph index corresponding to the starting character code
} tg_open_type__sequential_map_group_8__record;

typedef struct tg_open_type__cmap_subtable_format_8
{
	u16                                             format;       // format: Subtable format; set to 8.
	u16                                             reserved;     // reserved: Reserved; set to 0
	u32                                             length;       // length: Byte length of this subtable (including the header)
	u32                                             language;     // language: For requirements on use of the language field, see “Use of the language field in 'cmap' subtables” in this document.
	u8                                              p_is32[8192]; // is32[8192]: Tightly packed array of bits (8K bytes total) indicating whether the particular 16-bit (index) value is the start of a 32-bit character code
	u32                                             num_groups;   // numGroups: Number of groupings which follow
	tg_open_type__sequential_map_group_8__record    p_groups[0];  // groups[numGroups]: Array of SequentialMapGroup records.
} tg_open_type__cmap_subtable_format_8;

typedef struct tg_open_type__cmap_subtable_format_6
{
	u16    format;              // format: Format number is set to 6.
	u16    length;              // length: This is the length in bytes of the subtable.
	u16    language;            // language: For requirements on use of the language field, see “Use of the language field in 'cmap' subtables” in this document.
	u16    first_code;          // firstCode: First character code of subrange.
	u16    entry_count;         // entryCount: Number of character codes in subrange.
	u16    p_glyph_id_array[0]; // glyphIdArray[entryCount]: Array of glyph index values for character codes in the range.
} tg_open_type__cmap_subtable_format_6;

typedef struct tg_open_type__cmap_subtable_format_4
{
	u16    format;               // format: Format number is set to 4.
	u16    length;               // length: This is the length in bytes of the subtable.
	u16    language;             // language: For requirements on use of the language field, see “Use of the language field in 'cmap' subtables” in this document.
	u16    seg_count_x_2;        // segCountX2: 2 × segCount.
	u16    search_range;         // searchRange: 2 × (2**floor(log2(segCount)))
	u16    entry_selector;       // entrySelector: log2(searchRange/2)
	u16    range_shift;          // rangeShift: 2 × segCount - searchRange
	u16    p_end_code[0];        // endCode[segCount]: End characterCode for each segment, last=0xFFFF.
	//u16    reserved_pad;         // reservedPad: Set to 0.
	//u16    p_start_code[0];      // startCode[segCount]: Start character code for each segment.
	//i16    p_id_delta[0];        // idDelta[segCount]: Delta for all character codes in segment.
	//u16    p_id_range_offset[0]; // idRangeOffset[segCount]: Offsets into glyphIdArray or 0
	//u16    p_glyph_id_array[0];  // glyphIdArray[ ]: Glyph index array (arbitrary length)
} tg_open_type__cmap_subtable_format_4;

typedef struct tg_open_type__sub_header_record
{
	u16    first_code;      // firstCode: First valid low byte for this SubHeader.
	u16    entry_count;     // entryCount: Number of valid low bytes for this SubHeader.
	i16    id_delta;        // idDelta: See text below.
	u16    id_range_offset; // idRangeOffset: See text below.
} tg_open_type__sub_header_record;

typedef struct tg_open_type__cmap_subtable_format_2
{
	u16                                 format;                 // format: Format number is set to 2.
	u16                                 length;                 // length: This is the length in bytes of the subtable.
	u16                                 language;               // language: For requirements on use of the language field, see “Use of the language field in 'cmap' subtables” in this document.
	u16                                 p_sub_header_keys[256]; // subHeaderKeys[256]: Array that maps high bytes to subHeaders: value is subHeader index × 8.
	//tg_open_type__sub_header__record    p_sub_headers[0];       // subHeaders[ ]: Variable-length array of SubHeader records.
	//u16                                 p_glyph_index_array[0]; // glyphIndexArray[ ]: Variable-length array containing subarrays used for mapping the low byte of 2-byte characters.
} tg_open_type__cmap_subtable_format_2;

typedef struct tg_open_type__cmap_subtable_format_0
{
	u16    format;             // format: Format number is set to 0.
	u16    length;             // length: This is the length in bytes of the subtable.
	u16    language;           // language: For requirements on use of the language field, see “Use of the language field in 'cmap' subtables” in this document.
	u8     p_glyph_array[256]; // glyphIdArray[256]: An array that maps character codes to glyph index values.
} tg_open_type__cmap_subtable_format_0;

typedef enum tg_open_type__windows__encodings
{
	TG_OPEN_TYPE__WINDOWS__ENCODING__Symbol                     = 0, // Symbol
	TG_OPEN_TYPE__WINDOWS__ENCODING__Unicode_BMP                = 1, // Unicode BMP
	TG_OPEN_TYPE__WINDOWS__ENCODING__ShiftJIS                   = 2, // ShiftJIS
	TG_OPEN_TYPE__WINDOWS__ENCODING__PRC                        = 3, // PRC
	TG_OPEN_TYPE__WINDOWS__ENCODING__Big5                       = 4, // Big5
	TG_OPEN_TYPE__WINDOWS__ENCODING__Wansung                    = 5, // Wansung
	TG_OPEN_TYPE__WINDOWS__ENCODING__Johab                      = 6, // Johab
	TG_OPEN_TYPE__WINDOWS__ENCODING__Reserved0                  = 7, // Reserved
	TG_OPEN_TYPE__WINDOWS__ENCODING__Reserved1                  = 8, // Reserved
	TG_OPEN_TYPE__WINDOWS__ENCODING__Reserved2                  = 9, // Reserved
	TG_OPEN_TYPE__WINDOWS__ENCODING__Unicode_full_repertoire    = 10 // Unicode full repertoire
} tg_open_type__windows__encodings;

typedef enum tg_open_type__platform
{
	TG_OPEN_TYPE__PLATFORM__UNICODE      = 0,
	TG_OPEN_TYPE__PLATFORM__MACINTOSH    = 1,
	TG_OPEN_TYPE__PLATFORM__WINDOWS      = 3,
	TG_OPEN_TYPE__PLATFORM__CUSTOM       = 4
} tg_open_type__platform;

typedef struct tg_open_type__encoding_record
{
	u16            platform_id; // platformID: Platform ID.
	u16            encoding_id; // encodingID: Platform-specific encoding ID.
	tg_offset32    offset;      // offset: Byte offset from beginning of table to the subtable for this encoding.
} tg_open_type__encoding_record;

typedef struct tg_open_type__cmap
{
	u16                              version;               // version: Table version number (0).
	u16                              num_tables;            // numTables: Number of encoding tables that follow.
	tg_open_type__encoding_record    p_encoding_records[0]; // encodingRecords[numTables]	
} tg_open_type__cmap;

#endif
