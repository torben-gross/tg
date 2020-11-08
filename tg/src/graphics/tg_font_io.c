#include "graphics/tg_font_io.h"

#include "memory/tg_memory.h"
#include "platform/tg_platform.h"
#include "util/tg_string.h"



// source: https://docs.microsoft.com/en-us/typography/opentype/spec/

#define TG_SFNT_VERSION_TRUE_TYPE 0x00010000
#define TG_SFNT_VERSION_OPEN_TYPE 'OTTO'

#define TG_SWITCH_ENDIANNESS_16(x)    ((((*(u16*)&(x)) & 0x0000ff00) << 8) | (((*(u16*)&(x)) & 0x00ff0000) >> 8))
#define TG_SWITCH_ENDIANNESS_32(x)    ((((*(u32*)&(x)) & 0x000000ff) << 24) | (((*(u32*)&(x)) & 0x0000ff00) << 8) | (((*(u32*)&(x)) & 0x00ff0000) >> 8) | (((*(u32*)&(x)) & 0xff000000) >> 24))



typedef i16 tg_fword;
typedef u16 tg_ufword;
typedef i16 tg_f2dot14;
typedef i64 tg_longdatetime;
typedef u16 tg_offset16;
typedef u32 tg_offset32;



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

// OpenType Layout common table formats

typedef enum tg_open_type_delta_format_value
{
	TG_OPEN_TYPE_DELTA_FORMAT_VALUE_LOCAL_2_BIT_DELTAS    = 0x0001, // LOCAL_2_BIT_DELTAS: Signed 2 - bit value, 8 values per uint16
	TG_OPEN_TYPE_DELTA_FORMAT_VALUE_LOCAL_4_BIT_DELTAS    = 0x0002, // LOCAL_4_BIT_DELTAS: Signed 4 - bit value, 4 values per uint16
	TG_OPEN_TYPE_DELTA_FORMAT_VALUE_LOCAL_8_BIT_DELTAS    = 0x0003, // LOCAL_8_BIT_DELTAS: Signed 8 - bit value, 2 values per uint16
	TG_OPEN_TYPE_DELTA_FORMAT_VALUE_VARIATION_INDEX       = 0x8000, // VARIATION_INDEX: VariationIndex table, contains a delta - set index pair.
	TG_OPEN_TYPE_DELTA_FORMAT_VALUE_RESERVED              = 0x7FFC  // Reserved: For future use — set to 0
} tg_open_type_delta_format_value;

typedef enum tg_open_type_lookup_flag_bit_enumeration
{
	TG_OPEN_TYPE_LOOKUP_FLAG_BIT_RIGHT_TO_LEFT             = 0x0001, // rightToLeft: This bit relates only to the correct processing of the cursive attachment lookup type(GPOS lookup type 3). When this bit is set, the last glyph in a given sequence to which the cursive attachment lookup is applied, will be positioned on the baseline. Note: Setting of this bit is not intended to be used by operating systems or applications to determine text direction.
	TG_OPEN_TYPE_LOOKUP_FLAG_BIT_IGNORE_BASE_GLYPHS        = 0x0002, // ignoreBaseGlyphs: If set, skips over base glyphs
	TG_OPEN_TYPE_LOOKUP_FLAG_BIT_IGNORE_LIGATURES          = 0x0004, // ignoreLigatures: If set, skips over ligatures
	TG_OPEN_TYPE_LOOKUP_FLAG_BIT_IGNORE_MARKS              = 0x0008, // ignoreMarks: If set, skips over all combining marks
	TG_OPEN_TYPE_LOOKUP_FLAG_BIT_USE_MARK_FILTERING_SET    = 0x0010, // useMarkFilteringSet: If set, indicates that the lookup table structure is followed by a MarkFilteringSet field. The layout engine skips over all mark glyphs not in the mark filtering set indicated.
	TG_OPEN_TYPE_LOOKUP_FLAG_BIT_RESERVED                  = 0x00E0, // reserved: For future use(Set to zero)
	TG_OPEN_TYPE_LOOKUP_FLAG_BIT_MARK_ATTACHMENT_TYPE      = 0xFF00  // markAttachmentType: If not zero, skips over all marks of attachment type different from specified.
} tg_open_type_lookup_flag_bit_enumeration;

// OpenType Font Variations common table formats

typedef enum tg_open_type_packet_delta_flag
{
	TG_OPEN_TYPE_PACKET_DELTA_FLAG_DELTAS_ARE_ZERO         = 0x80, // DELTAS_ARE_ZERO:  Flag indicating that this run contains no data(no explicit delta values are stored), and that all of the deltas for this run are zero.
	TG_OPEN_TYPE_PACKET_DELTA_FLAG_DELTAS_ARE_WORDS        = 0x40, // DELTAS_ARE_WORDS: Flag indicating the data type for delta values in the run. If set, the run contains 16 - bit signed deltas(int16); if clear, the run contains 8 - bit signed deltas(int8).
	TG_OPEN_TYPE_PACKET_DELTA_FLAG_DELTA_RUN_COUNT_MASK    = 0x3F  // DELTA_RUN_COUNT_MASK: Mask for the low 6 bits to provide the number of delta values in the run, minus one.
} tg_open_type_packet_delta_flag;

typedef enum tg_open_type_packed_point_number_flag
{
	TG_OPEN_TYPE_PACKET_POINT_NUMBER_FLAG_POINTS_ARE_WORDS        = 0x80, // POINTS_ARE_WORDS: Flag indicating the data type used for point numbers in this run. If set, the point numbers are stored as unsigned 16 - bit values(uint16); if clear, the point numbers are stored as unsigned bytes(uint8).
	TG_OPEN_TYPE_PACKET_POINT_NUMBER_FLAG_POINT_RUN_COUNT_MASK    = 0x7F  // POINT_RUN_COUNT_MASK: Mask for the low 7 bits of the control byte to give the number of point number elements, minus 1.
} tg_open_type_packed_point_number_flag;

typedef enum tg_open_type_tuple_index_format
{
	TG_OPEN_TYPE_TUPLE_INDEX_FORMAT_EMBEDDED_PEAK_TUPLE      = 0x8000, // EMBEDDED_PEAK_TUPLE: Flag indicating that this tuple variation header includes an embedded peak tuple record, immediately after the tupleIndex field. If set, the low 12 bits of the tupleIndex value are ignored. Note that this must always be set within the 'cvar' table.
	TG_OPEN_TYPE_TUPLE_INDEX_FORMAT_INTERMEDIATE_REGION      = 0x4000, // INTERMEDIATE_REGION: Flag indicating that this tuple variation table applies to an intermediate region within the variation space. If set, the header includes the two intermediate-region, start and end tuple records, immediately after the peak tuple record (if present).
	TG_OPEN_TYPE_TUPLE_INDEX_FORMAT_PRIVATE_POINT_NUMBERS    = 0x2000, // PRIVATE_POINT_NUMBERS: Flag indicating that the serialized data for this tuple variation table includes packed “point” number data. If set, this tuple variation table uses that number data; if clear, this tuple variation table uses shared number data found at the start of the serialized data for this glyph variation data or 'cvar' table.
	TG_OPEN_TYPE_TUPLE_INDEX_FORMAT_RESERVED                 = 0x1000, // Reserved: Reserved for future use — set to 0.
	TG_OPEN_TYPE_TUPLE_INDEX_FORMAT_TUPLE_INDEX_MASK         = 0x0FFF  // TUPLE_INDEX_MASK: Mask for the low 12 bits to give the shared tuple records index.
} tg_open_type_tuple_index_format;

typedef enum tg_open_type_tuple_variation_count_flag
{
	TG_OPEN_TYPE_TUPLE_VARIATION_COUNT_FLAG_SHARED_POINT_NUMBERS    = 0x8000, // SHARED_POINT_NUMBERS: Flag indicating that some or all tuple variation tables reference a shared set of “point” numbers. These shared numbers are represented as packed point number data at the start of the serialized data.
	TG_OPEN_TYPE_TUPLE_VARIATION_COUNT_FLAG_RESERVED                = 0x7000, // Reserved: Reserved for future use — set to 0.
	TG_OPEN_TYPE_TUPLE_VARIATION_COUNT_FLAG_COUNT_MASK              = 0x0FFF  // COUNT_MASK: Mask for the low bits to give the number of tuple variation tables.
} tg_open_type_tuple_variation_count_flag;



typedef enum tg_open_type_gylph_class_def_enumeration_list
{
	TG_GLYPH_CLASS_DEF_BASE         = 1, // Base glyph (single character, spacing glyph)
	TG_GLYPH_CLASS_DEF_LIGATURE     = 2, // Ligature glyph (multiple character, spacing glyph)
	TG_GLYPH_CLASS_DEF_MARK         = 3, // Mark glyph (non-spacing combining glyph)
	TG_GLYPH_CLASS_DEF_COMPONENT    = 4  // Component glyph (part of single character, spacing glyph)
} tg_open_type_gylph_class_def_enumeration_list;



// OpenType Layout common table formats

typedef struct tg_open_type_feature_table_substitution_record
{
	u16            feature_index;           // featureIndex: The feature table index to match.
	tg_offset32    alternate_feature_table; // alternateFeatureTable: Offset to an alternate feature table, from start of the FeatureTableSubstitution table.
} tg_open_type_feature_table_substitution_record;

typedef struct tg_open_type_feature_table_substitution_table
{
	u16                                               major_version; // majorVersion: Major version of the feature table substitution table — set to 1
	u16                                               minor_version; // minorVersion: Minor version of the feature table substitution table — set to 0.
	u16                                               substitution_count; // substitutionCount: Number of feature table substitution records.
	tg_open_type_feature_table_substitution_record    p_substitutions[0]; // FeatureTableSubstitutionRecord: substitutions[substitutionCount]	Array of feature table substitution records.
} tg_open_type_feature_table_substitution_table;

typedef struct tg_open_type_condition_table_format_1
{
	u16           format;                 // Format: Format, = 1
	u16           axis_index;             // AxisIndex: Index(zero - based) for the variation axis within the 'fvar' table.
	tg_f2dot14    filter_range_min_value; // FilterRangeMinValue: Minimum value of the font variation instances that satisfy this condition.
	tg_f2dot14    filter_range_max_value; // FilterRangeMaxValue: Maximum value of the font variation instances that satisfy this condition.
} tg_open_type_condition_table_format_1;

typedef struct tg_open_type_condition_set_table
{
	u16            condition_count; // conditionCount: Number of conditions for this condition set.
	tg_offset32    p_conditions[0]; // conditions[conditionCount]: Array of offsets to condition tables, from beginning of the ConditionSet table.
} tg_open_type_condition_set_table;

typedef struct tg_open_type_feature_variation_record
{
	tg_offset32    condition_set_offset;              // conditionSetOffset: Offset to a condition set table, from beginning of FeatureVariations table.
	tg_offset32    feature_table_substitution_offset; // featureTableSubstitutionOffset: Offset to a feature table substitution table, from beginning of the FeatureVariations table.
} tg_open_type_feature_variation_record;

typedef struct tg_open_type_feature_variations_table
{
	u16                                      major_version;                  // majorVersion: Major version of the FeatureVariations table — set to 1.
	u16                                      minor_version;                  // minorVersion: Minor version of the FeatureVariations table — set to 0.
	u32                                      feature_variation_record_count; // featureVariationRecordCount: Number of feature variation records.
	tg_open_type_feature_variation_record    p_feature_variation_records[0]; // featureVariationRecords[featureVariationRecordCount]: Array of feature variation records.
} tg_open_type_feature_variations_table;

typedef struct tg_open_type_variation_index_table
{
	u16    delta_set_outer_index; // deltaSetOuterIndex: A delta - set outer index — used to select an item variation data subtable within the item variation store.
	u16    delta_set_inner_index; // deltaSetInnerIndex: A delta - set inner index — used to select a delta - set row within an item variation data subtable.
	u16    delta_format;          // deltaFormat: Format, = 0x8000
} tg_open_type_variation_index_table;

typedef struct tg_open_type_device_table
{
	u16    start_size;    // startSize: Smallest size to correct, in ppem
	u16    end_size;      // endSize: Largest size to correct, in ppem
	u16    delta_format;  // deltaFormat: Format of deltaValue array data : 0x0001, 0x0002, or 0x0003
	u16    p_delta_value; // deltaValue[]: Array of compressed data
} tg_open_type_device_table;

typedef struct tg_open_type_class_range_record
{
	u16    start_glyph; // startGlyphID: First glyph ID in the range
	u16    end_glyph;   // endGlyphID: Last glyph ID in the range
	u16    class;       // class: Applied to all glyphs in the range
} tg_open_type_class_range_record;

typedef struct tg_open_type_class_def_format_2
{
	u16                                class_format;             // classFormat: Format identifier — format = 2
	u16                                class_range_count;        // classRangeCount: Number of ClassRangeRecords
	tg_open_type_class_range_record    p_class_range_records[0]; // classRangeRecords[classRangeCount]: Array of ClassRangeRecords — ordered by startGlyphID
} tg_open_type_class_def_format_2;

typedef struct tg_open_type_class_def_format_1
{
	u16    class_format;
	u16    start_glyph_id;
	u16    glyph_count;
	u16    p_class_value_array[0];
} tg_open_type_class_def_format_1;

typedef struct tg_open_type_range_record
{
	u16    start_glyph_id;       // startGlyphID: First glyph ID in the range
	u16    end_glyph_id;         // endGlyphID: Last glyph ID in the range
	u16    start_coverage_index; // startCoverageIndex: Coverage Index of first glyph ID in range
} tg_open_type_range_record;

typedef struct tg_open_type_coverage_format_2
{
	u16                          coverage_format; // coverageFormat: Format identifier — format = 2
	u16                          range_count;     // rangeCount: Number of RangeRecords
	tg_open_type_range_record    p_range_records; // rangeRecords[rangeCount]: Array of glyph ranges — ordered by startGlyphID.
} tg_open_type_coverage_format_2;

typedef struct tg_open_type_coverage_format_1
{
	u16    coverage_format;  // coverageFormat: Format identifier — format = 1
	u16    glyph_count;      // glyphCount: Number of glyphs in the glyph array
	u16    p_glyph_attay[0]; // glyphArray[glyphCount]: Array of glyph IDs — in numerical order
} tg_open_type_coverage_format_1;

typedef struct tg_open_type_lookp_table
{
	u16            lookup_type;           // lookupType: Different enumerations for GSUBand GPOS
	u16            lookup_flag;           // lookupFlag: Lookup qualifiers
	u16            subtable_count;        // subTableCount: Number of subtables for this lookup
	tg_offset16    p_subtable_offsets[0]; // subtableOffsets[subTableCount]: Array of offsets to lookup subtables, from beginning of Lookup table
	// u16            mark_filtering_set; // markFilteringSet: Index (base 0) into GDEF mark glyph sets structure. This field is only present if bit useMarkFilteringSet of lookup flags is set.
} tg_open_type_lookp_table;

typedef struct tg_open_type_lookup_list_table
{
	u16            lookup_count; // lookupCount: Number of lookups in this table
	tg_offset16    p_lookups[0]; // lookups[lookupCount]: Array of offsets to Lookup tables, from beginning of LookupList — zero based(first lookup is Lookup index = 0)
} tg_open_type_lookup_list_table;

typedef struct tg_open_type_feature_table
{
	tg_offset16    feature_params;           // featureParams: = NULL (reserved for offset to FeatureParams)
	u16            lookup_index_count;       // lookupIndexCount: Number of LookupList indices for this feature
	u16            p_lookup_list_indices[0]; // lookupListIndices[lookupIndexCount]: Array of indices into the LookupList — zero-based (first lookup is LookupListIndex = 0)
} tg_open_type_feature_table;

typedef struct tg_open_type_feature_record
{
	char           p_feature_tag[4]; // featureTag: 4-byte feature identification tag
	tg_offset16    feature_offset;   // featureOffset: Offset to Feature table, from beginning of FeatureList
} tg_open_type_feature_record;

typedef struct tg_open_type_feature_list_table
{
	u16                            feature_count;        // featureCount: Number of FeatureRecords in this table
	tg_open_type_feature_record    p_feature_records[0]; // featureRecords[featureCount]: Array of FeatureRecords — zero-based (first feature has FeatureIndex = 0), listed alphabetically by feature tag
} tg_open_type_feature_list_table;

typedef struct tg_open_type_lang_sys_table
{
	tg_offset16    lookup_order;           // lookupOrder: = NULL (reserved for an offset to a reordering table)
	u16            required_feature_index; // requiredFeatureIndex: Index of a feature required for this language system; if no required features = 0xFFFF
	u16            feature_index_count;    // featureIndexCount: Number of feature index values for this language system — excludes the required feature
	u16            p_feature_indices[0];   // featureIndices[featureIndexCount]: Array of indices into the FeatureList, in arbitrary order
} tg_open_type_lang_sys_table;

typedef struct tg_open_type_lang_sys_record
{
	char           p_lang_sys_tag[4]; // langSysTag: 4-byte LangSysTag identifier
	tg_offset16    lang_sys_offset;   // langSysOffset: Offset to LangSys table, from beginning of Script table
} tg_open_type_lang_sys_record;

typedef struct tg_open_type_script_table
{
	tg_offset16                     default_lang_sys;      // defaultLangSys: Offset to default LangSys table, from beginning of Script table — may be NULL
	u16                             lang_sys_count;        // langSysCount: Number of LangSysRecords for this script — excluding the default LangSys
	tg_open_type_lang_sys_record    p_lang_sys_records[0]; // langSysRecords[langSysCount]: Array of LangSysRecords, listed alphabetically by LangSys tag
} tg_open_type_script_table;

typedef struct tg_open_type_script_record
{
	char           p_script_tag[4]; // scriptTag: 4-byte script tag identifier
	tg_offset16    script_offset;   // scriptOffset: Offset to Script table, from beginning of ScriptList
} tg_open_type_script_record;

typedef struct tg_open_type_script_list_table
{
	u16                           script_count;        // scriptCount: Number of ScriptRecords
	tg_open_type_script_record    p_script_records[0]; // scriptRecords[scriptCount]: Array of ScriptRecords, listed alphabetically by script tag
} tg_open_type_script_list_table;

// OpenType Font Variations common table formats

typedef struct tg_open_type_delta_set
{
	i16* p_delta_data_i16; // DeltaData[shortDeltaCount + regionIndexCount]: Variation delta values.
	i8*  p_delta_data_i8;  // DeltaData[shortDeltaCount + regionIndexCount]: Variation delta values.
} tg_open_type_delta_set;

typedef struct tg_open_type_item_variation_data_subtable
{
	u16                        item_count;         // itemCount: The number of delta sets for distinct items.
	u16                        short_delta_count;  // shortDeltaCount: The number of deltas in each delta set that use a 16 - bit representation.Must be less than or equal to regionIndexCount.
	u16                        region_index_count; // regionIndexCount: The number of variation regions referenced.
	u16*                       p_region_indexes;   // regionIndexes[regionIndexCount]: Array of indices into the variation region list for the regions referenced by this item variation data table.
	tg_open_type_delta_set*    p_delta_sets;       // deltaSets[itemCount]: Delta - set rows.
} tg_open_type_item_variation_data_subtable;

typedef struct tg_open_type_item_variation_store_table
{
	u16            format;                        // format: Format — set to 1
	tg_offset32    variation_region_list_offset;  // variationRegionListOffset: Offset in bytes from the start of the item variation store to the variation region list.
	u16            item_variation_data_count;     // itemVariationDataCount: The number of item variation data subtables.
	tg_offset32    p_item_variation_data_offsets; // itemVariationDataOffsets[itemVariationDataCount]: Offsets in bytes from the start of the item variation store to each item variation data subtable.
} tg_open_type_item_variation_store_table;

typedef struct tg_open_type_region_axis_coordinates
{
	tg_f2dot14    start_coord; // startCoord: The region start coordinate value for the current axis.
	tg_f2dot14    peak_coord;  // peakCoord: The region peak coordinate value for the current axis.
	tg_f2dot14    end_coord;   // endCoord: The region end coordinate value for the current axis.
} tg_open_type_region_axis_coordinates;

typedef tg_open_type_region_axis_coordinates* tg_open_type_variation_region; // regionAxes[axisCount]: Array of region axis coordinates records, in the order of axes given in the 'fvar' table.

typedef struct tg_open_type_variation_region_list
{
	u16                              axis_count;             // axisCount: The number of variation axes for this font. This must be the same number as axisCount in the 'fvar' table.
	u16                              region_count;           // regionCount: The number of variation region tables in the variation region list.
	tg_open_type_variation_region    p_variation_regions[0]; // variationRegions[regionCount]: Array of variation regions.
} tg_open_type_variation_region_list;

typedef struct tg_open_type_tuple_variation_header
{
	u16                   variation_data_size;      // variationDataSize: The size in bytes of the serialized data for this tuple variation table.
	u16                   tuple_index;              // tupleIndex: A packed field. The high 4 bits are flags(see below). The low 12 bits are an index into a shared tuple records array.
	// TODO: what type is this?
	// tg_open_type_tuple    peak_tuple;               // peakTuple: Peak tuple record for this tuple variation table — optional, determined by flags in the tupleIndex value. Note that this must always be included in the 'cvar' table.
	// tg_open_type_tuple    intermediate_start_tuple; // intermediateStartTuple: Intermediate start tuple record for this tuple variation table — optional, determined by flags in the tupleIndex value.
	// tg_open_type_tuple    intermediate_end_tuple;   // intermediateEndTuple: Intermediate end tuple record for this tuple variation table — optional, determined by flags in the tupleIndex value.
} tg_open_type_tuple_variation_header;

typedef struct tg_open_type_cvar_table_header
{
	u16                                    major_version;                // majorVersion: Major version number of the 'cvar' table — set to 1.
	u16                                    minor_version;                // minorVersion: Minor version number of the 'cvar' table — set to 0.
	u16                                    tuple_variation_count;        // tupleVariationCount: A packed field. The high 4 bits are flags(see below), and the low 12 bits are the number of tuple variation tables. The count can be any number between 1 and 4095.
	tg_offset16                            data_offset;                  // dataOffset: Offset from the start of the 'cvar' table to the serialized data.
	tg_open_type_tuple_variation_header    p_tuple_variation_headers[0]; // tupleVariationHeaders[tupleVariationCount]: Array of tuple variation headers.
} tg_open_type_cvar_table_header;

typedef struct tg_open_type_glyph_variation_data_header
{
	u16                                    tuple_variation_count;        // tupleVariationCount: A packed field. The high 4 bits are flags (see below), and the low 12 bits are the number of tuple variation tables for this glyph. The count can be any number between 1 and 4095.
	tg_offset16                            data_offset;                  // dataOffset: Offset from the start of the GlyphVariationData table to the serialized data.
	tg_open_type_tuple_variation_header    p_tuple_variation_headers[0]; //tupleVariationHeaders[tupleVariationCount]: Array of tuple variation headers.
} tg_open_type_glyph_variation_data_header;

typedef struct tg_open_type_tuple_record
{
	tg_f2dot14    p_coordinates[0]; // coordinates[axisCount]: Coordinate array specifying a position within the font’s variation space. The number of elements must match the axisCount specified in the 'fvar' table.
} tg_open_type_tuple_record;



typedef struct tg_open_type_signature_block_format_1
{
	u16    reserved1;
	u16    reserved2;
	u32    signature_length;
	u8     p_signature[0];
} tg_open_type_signature_block_format_1;

typedef struct tg_open_type_signature_record
{
	u32    format;
	u32    length;
	u32    offset;
} tg_open_type_signature_record;

typedef struct tg_open_type_dsig_header
{
	u32                              version;
	u16                              num_signatures;
	u16                              flags;
	tg_open_type_signature_record    p_signature_records[0]; // num_signatures
} tg_open_type_dsig_header;



typedef struct tg_open_type_mark_glyph_sets_table
{
	u16            mark_glyph_set_table_format; // markGlyphSetTableFormat: Format identifier == 1
	u16            mark_glyph_set_count;        // markGlyphSetCount: Number of mark glyph sets defined
	tg_offset32    p_coverage_offsets[0];       // coverageOffsets[markGlyphSetCount]: Array of offsets to mark glyph set coverage tables.
} tg_open_type_mark_glyph_sets_table;

typedef struct tg_open_type_varet_value_format_3_table
{
	u16            caret_value_format; // caretValueFormat: Format identifier : format = 3
	i16            coordinate;         // coordinate: X or Y value, in design units
	tg_offset16    device_offset;      // deviceOffset: Offset to Device table(non - variable font) / Variation Index table(variable font) for X or Y value - from beginning of CaretValue table
} tg_open_type_varet_value_format_3_table;

typedef struct tg_open_type_varet_value_format_2_table
{
	u16    caret_value_format;      // caretValueFormat: Format identifier : format = 2
	u16    caret_value_point_index; // caretValuePointIndex: Contour point index on glyph
} tg_open_type_varet_value_format_2_table;

typedef struct tg_open_type_varet_value_format_1_table
{
	u16    caret_value_format; // caretValueFormat: Format identifier : format = 1
	i16    coordinate;         // coordinate: X or Y value, in design units
} tg_open_type_varet_value_format_1_table;

typedef struct tg_open_type_lig_glyph_table
{
	u16            caret_count;              // caretCount: Number of CaretValue tables for this ligature(components - 1)
	tg_offset16    p_caret_value_offsets[0]; // caretValueOffsets[caretCount]: Array of offsets to CaretValue tables, from beginning of LigGlyph table — in increasing coordinate order
} tg_open_type_lig_glyph_table;

typedef struct tg_open_type_lig_caret_list_table
{
	tg_offset16    coverage_offset;        // coverageOffset: Offset to Coverage table - from beginning of LigCaretList table
	u16            lig_glyph_count;        // ligGlyphCount: Number of ligature glyphs
	tg_offset16    p_lig_glyph_offsets[0]; // ligGlyphOffsets[ligGlyphCount]: Array of offsets to LigGlyph tables, from beginning of LigCaretList table —in Coverage Index order
} tg_open_type_lig_caret_list_table;

typedef struct tg_open_type_attach_point_table
{
	u16    point_count;        // pointCount: Number of attachment points on this glyph
	u16    p_point_indices[0]; // pointIndices[pointCount]: Array of contour point indices -in increasing numerical order
} tg_open_type_attach_point_table;

typedef struct tg_open_type_attach_list_table
{
	tg_offset16    coverage_offset;           // coverageOffset: Offset to Coverage table - from beginning of AttachList table
	u16            glyph_count;               // glyphCount: Number of glyphs with attachment points
	tg_offset16    p_attach_point_offsets[0]; // attachPointOffsets[glyphCount]: Array of offsets to AttachPoint tables-from beginning of AttachList table-in Coverage Index order
} tg_open_type_attach_list_table;

typedef struct tg_open_type_class_def_format_1 tg_open_type_glyph_class_def_table;

typedef struct tg_open_type_gdef_header_v10
{
	u16            major_version;                // majorVersion: Major version of the GDEF table, = 1
	u16            minor_version;                // minorVersion: Minor version of the GDEF table, = 0
	tg_offset16    glyph_class_def_offset;       // glyphClassDefOffset: Offset to class definition table for glyph type, from beginning of GDEF header (may be NULL)
	tg_offset16    attach_list_offset;           // attachListOffset: Offset to attachment point list table, from beginning of GDEF header (may be NULL)
	tg_offset16    lig_caret_list_offset;        // ligCaretListOffset: Offset to ligature caret list table, from beginning of GDEF header (may be NULL)
	tg_offset16    mark_attach_class_def_offset; // markAttachClassDefOffset: Offset to class definition table for mark attachment type, from beginning of GDEF header (may be NULL)
} tg_open_type_gdef_header_v10;

typedef struct tg_open_type_gdef_header_v12
{
	u16            major_version;                // majorVersion: Major version of the GDEF table, = 1
	u16            minor_version;                // minorVersion: Minor version of the GDEF table, = 2
	tg_offset16    glyph_class_def_offset;       // glyphClassDefOffset: Offset to class definition table for glyph type, from beginning of GDEF header (may be NULL)
	tg_offset16    attach_list_offset;           // attachListOffset: Offset to attachment point list table, from beginning of GDEF header (may be NULL)
	tg_offset16    lig_caret_list_offset;        // ligCaretListOffset: Offset to ligature caret list table, from beginning of GDEF header (may be NULL)
	tg_offset16    mark_attach_class_def_offset; // markAttachClassDefOffset: Offset to class definition table for mark attachment type, from beginning of GDEF header (may be NULL)
	tg_offset16    mark_glyph_sets_def_offset;   // markGlyphSetsDefOffset: Offset to the table of mark glyph set definitions, from beginning of GDEF header (may be NULL)
} tg_open_type_gdef_header_v12;

typedef struct tg_open_type_gdef_header_v13
{
	u16            major_version;                // majorVersion: Major version of the GDEF table, = 1
	u16            minor_version;                // minorVersion: Minor version of the GDEF table, = 3
	tg_offset16    glyph_class_def_offset;       // glyphClassDefOffset: Offset to class definition table for glyph type, from beginning of GDEF header (may be NULL)
	tg_offset16    attach_list_offset;           // attachListOffset: Offset to attachment point list table, from beginning of GDEF header (may be NULL)
	tg_offset16    lig_caret_list_offset;        // ligCaretListOffset: Offset to ligature caret list table, from beginning of GDEF header (may be NULL)
	tg_offset16    mark_attach_class_def_offset; // markAttachClassDefOffset: Offset to class definition table for mark attachment type, from beginning of GDEF header (may be NULL)
	tg_offset16    mark_glyph_sets_def_offset;   // markGlyphSetsDefOffset: Offset to the table of mark glyph set definitions, from beginning of GDEF header (may be NULL)
	tg_offset32    item_var_store_offset;        // itemVarStoreOffset: Offset to the Item Variation Store table, from beginning of GDEF header (may be NULL)
} tg_open_type_gdef_header_v13;



typedef struct tg_open_type_offset_table
{
	u32    sfnt_version;
	u16    num_tables;
	u16    search_range;
	u16    entry_selector;
	u16    range_shift;
} tg_open_type_offset_table;

typedef struct tg_open_type_table_record
{
	char           p_table_tag[4]; // tag
	u32            checksum;
	tg_offset32    offset;
	u32            length;
} tg_open_type_table_record;

typedef struct tg_open_type
{
	tg_open_type_offset_table    offset_table;
	tg_open_type_table_record    p_table_records[0];
} tg_open_type;



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
			tg_open_type_table_record* p_table_record = &p_open_type->p_table_records[table_idx];

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
				tg_open_type_dsig_header* p_dsig = (tg_open_type_dsig_header*)&p_buffer[p_table_record->offset];
				p_dsig->version = TG_SWITCH_ENDIANNESS_32(p_dsig->version);
				p_dsig->num_signatures = TG_SWITCH_ENDIANNESS_16(p_dsig->num_signatures);
				p_dsig->flags = TG_SWITCH_ENDIANNESS_16(p_dsig->flags);

				TG_ASSERT(p_dsig->version == 0x00000001);
				TG_ASSERT(p_dsig->num_signatures == 0); // TODO: signatures are not yet supported
				for (u32 signature_idx = 0; signature_idx < p_dsig->num_signatures; signature_idx++)
				{
					tg_open_type_signature_record* p_signature_record = &p_dsig->p_signature_records[signature_idx];
					p_signature_record->format = TG_SWITCH_ENDIANNESS_32(p_signature_record->format);
					p_signature_record->length = TG_SWITCH_ENDIANNESS_32(p_signature_record->length);
					p_signature_record->offset = TG_SWITCH_ENDIANNESS_32(p_signature_record->offset);

					TG_ASSERT(p_signature_record->format == 1);
					tg_open_type_signature_block_format_1* p_signature_block = (tg_open_type_signature_block_format_1*)&p_buffer[p_signature_record->offset];
					p_signature_block->reserved1 = TG_SWITCH_ENDIANNESS_16(p_signature_block->reserved1);
					p_signature_block->reserved2 = TG_SWITCH_ENDIANNESS_16(p_signature_block->reserved2);
					p_signature_block->signature_length = TG_SWITCH_ENDIANNESS_32(p_signature_block->signature_length);
				}
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
				tg_open_type_gdef_header_v10* p_gdef_header = (tg_open_type_gdef_header_v10*)&p_buffer[p_table_record->offset];
				p_gdef_header->major_version = TG_SWITCH_ENDIANNESS_16(p_gdef_header->major_version);
				p_gdef_header->minor_version = TG_SWITCH_ENDIANNESS_16(p_gdef_header->minor_version);
				p_gdef_header->glyph_class_def_offset = TG_SWITCH_ENDIANNESS_16(p_gdef_header->glyph_class_def_offset);
				p_gdef_header->attach_list_offset = TG_SWITCH_ENDIANNESS_16(p_gdef_header->attach_list_offset);
				p_gdef_header->lig_caret_list_offset = TG_SWITCH_ENDIANNESS_16(p_gdef_header->lig_caret_list_offset);
				p_gdef_header->mark_attach_class_def_offset = TG_SWITCH_ENDIANNESS_16(p_gdef_header->mark_attach_class_def_offset);

				TG_INVALID_CODEPATH();
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
