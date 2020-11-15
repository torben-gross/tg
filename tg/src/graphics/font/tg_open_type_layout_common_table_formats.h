#ifndef TG_OPEN_TYPE_LAYOUT_COMMON_TABLE_FORMATS_H
#define TG_OPEN_TYPE_LAYOUT_COMMON_TABLE_FORMATS_H

#include "graphics/font/tg_open_type_types.h"

typedef struct tg_open_type__feature_table_substitution_record
{
	u16            feature_index;           // featureIndex: The feature table index to match.
	tg_offset32    alternate_feature_table; // alternateFeatureTable: Offset to an alternate feature table, from start of the FeatureTableSubstitution table.
} tg_open_type__feature_table_substitution_record;

typedef struct tg_open_type__feature_table_substitution__table
{
	u16                                                major_version; // majorVersion: Major version of the feature table substitution table — set to 1
	u16                                                minor_version; // minorVersion: Minor version of the feature table substitution table — set to 0.
	u16                                                substitution_count; // substitutionCount: Number of feature table substitution records.
	tg_open_type__feature_table_substitution_record    p_substitutions[0]; // FeatureTableSubstitutionRecord: substitutions[substitutionCount]	Array of feature table substitution records.
} tg_open_type__feature_table_substitution__table;

typedef struct tg_open_type__condition_table_format_1
{
	u16           format;                 // Format: Format, = 1
	u16           axis_index;             // AxisIndex: Index(zero - based) for the variation axis within the 'fvar' table.
	tg_f2dot14    filter_range_min_value; // FilterRangeMinValue: Minimum value of the font variation instances that satisfy this condition.
	tg_f2dot14    filter_range_max_value; // FilterRangeMaxValue: Maximum value of the font variation instances that satisfy this condition.
} tg_open_type__condition_table_format_1;

typedef struct tg_open_type__condition_set__table
{
	u16            condition_count; // conditionCount: Number of conditions for this condition set.
	tg_offset32    p_conditions[0]; // conditions[conditionCount]: Array of offsets to condition tables, from beginning of the ConditionSet table.
} tg_open_type__condition_set__table;

typedef struct tg_open_type__feature_variation_record
{
	tg_offset32    condition_set_offset;              // conditionSetOffset: Offset to a condition set table, from beginning of FeatureVariations table.
	tg_offset32    feature_table_substitution_offset; // featureTableSubstitutionOffset: Offset to a feature table substitution table, from beginning of the FeatureVariations table.
} tg_open_type__feature_variation_record;

typedef struct tg_open_type__feature_variations__table
{
	u16                                       major_version;                  // majorVersion: Major version of the FeatureVariations table — set to 1.
	u16                                       minor_version;                  // minorVersion: Minor version of the FeatureVariations table — set to 0.
	u32                                       feature_variation_record_count; // featureVariationRecordCount: Number of feature variation records.
	tg_open_type__feature_variation_record    p_feature_variation_records[0]; // featureVariationRecords[featureVariationRecordCount]: Array of feature variation records.
} tg_open_type__feature_variations__table;

typedef struct tg_open_type__variation_index__table
{
	u16    delta_set_outer_index; // deltaSetOuterIndex: A delta - set outer index — used to select an item variation data subtable within the item variation store.
	u16    delta_set_inner_index; // deltaSetInnerIndex: A delta - set inner index — used to select a delta - set row within an item variation data subtable.
	u16    delta_format;          // deltaFormat: Format, = 0x8000
} tg_open_type__variation_index__table;

typedef struct tg_open_type__device__table
{
	u16    start_size;    // startSize: Smallest size to correct, in ppem
	u16    end_size;      // endSize: Largest size to correct, in ppem
	u16    delta_format;  // deltaFormat: Format of deltaValue array data : 0x0001, 0x0002, or 0x0003
	u16    p_delta_value; // deltaValue[]: Array of compressed data
} tg_open_type__device__table;

typedef enum tg_open_type__delta_format__values
{
	TG_OPEN_TYPE__DELTA_FORMAT__LOCAL_2_BIT_DELTAS    = 0x0001, // LOCAL_2_BIT_DELTAS: Signed 2 - bit value, 8 values per uint16
	TG_OPEN_TYPE__DELTA_FORMAT__LOCAL_4_BIT_DELTAS    = 0x0002, // LOCAL_4_BIT_DELTAS: Signed 4 - bit value, 4 values per uint16
	TG_OPEN_TYPE__DELTA_FORMAT__LOCAL_8_BIT_DELTAS    = 0x0003, // LOCAL_8_BIT_DELTAS: Signed 8 - bit value, 2 values per uint16
	TG_OPEN_TYPE__DELTA_FORMAT__VARIATION_INDEX       = 0x8000, // VARIATION_INDEX: VariationIndex table, contains a delta - set index pair.
	TG_OPEN_TYPE__DELTA_FORMAT__RESERVED              = 0x7ffc  // Reserved: For future use — set to 0
} tg_open_type__delta_format__values;

typedef struct tg_open_type__class_range_record
{
	u16    start_glyph; // startGlyphID: First glyph ID in the range
	u16    end_glyph;   // endGlyphID: Last glyph ID in the range
	u16    class;       // class: Applied to all glyphs in the range
} tg_open_type__class_range_record;

typedef struct tg_open_type__class_def_format_2__table // Class ranges
{
	u16                                 class_format;             // classFormat: Format identifier — format = 2
	u16                                 class_range_count;        // classRangeCount: Number of ClassRangeRecords
	tg_open_type__class_range_record    p_class_range_records[0]; // classRangeRecords[classRangeCount]: Array of ClassRangeRecords — ordered by startGlyphID
} tg_open_type__class_def_format_2__table;

typedef struct tg_open_type__class_def_format_1__table // Class array
{
	u16    class_format;           // classFormat: Format identifier — format = 1
	u16    start_glyph_id;         // startGlyphID: First glyph ID of the classValueArray
	u16    glyph_count;            // glyphCount: Size of the classValueArray
	u16    p_class_value_array[0]; // classValueArray[glyphCount]: Array of Class Values — one per glyph ID
} tg_open_type__class_def_format_1__table;

typedef struct tg_open_type__range_record
{
	u16    start_glyph_id;       // startGlyphID: First glyph ID in the range
	u16    end_glyph_id;         // endGlyphID: Last glyph ID in the range
	u16    start_coverage_index; // startCoverageIndex: Coverage Index of first glyph ID in range
} tg_open_type__range_record;

typedef struct tg_open_type__coverage_format_2__table // Range of glyphs
{
	u16                           coverage_format;    // coverageFormat: Format identifier — format = 2
	u16                           range_count;        // rangeCount: Number of RangeRecords
	tg_open_type__range_record    p_range_records[0]; // rangeRecords[rangeCount]: Array of glyph ranges — ordered by startGlyphID.
} tg_open_type__coverage_format_2__table;

typedef struct tg_open_type__coverage_format_1__table // Individual glyph indices
{
	u16    coverage_format;  // coverageFormat: Format identifier — format = 1
	u16    glyph_count;      // glyphCount: Number of glyphs in the glyph array
	u16    p_glyph_attay[0]; // glyphArray[glyphCount]: Array of glyph IDs — in numerical order
} tg_open_type__coverage_format_1__table;

typedef enum tg_open_type__lookup_flag__bit__enumeration
{
	TG_OPEN_TYPE__LOOKUP_FLAG__RIGHT_TO_LEFT             = 0x0001, // rightToLeft: This bit relates only to the correct processing of the cursive attachment lookup type (GPOS lookup type 3). When this bit is set, the last glyph in a given sequence to which the cursive attachment lookup is applied, will be positioned on the baseline. Note: Setting of this bit is not intended to be used by operating systems or applications to determine text direction.
	TG_OPEN_TYPE__LOOKUP_FLAG__IGNORE_BASE_GLYPHS        = 0x0002, // ignoreBaseGlyphs: If set, skips over base glyphs
	TG_OPEN_TYPE__LOOKUP_FLAG__IGNORE_LIGATURES          = 0x0004, // ignoreLigatures: If set, skips over ligatures
	TG_OPEN_TYPE__LOOKUP_FLAG__IGNORE_MARKS              = 0x0008, // ignoreMarks: If set, skips over all combining marks
	TG_OPEN_TYPE__LOOKUP_FLAG__USE_MARK_FILTERING_SET    = 0x0010, // useMarkFilteringSet: If set, indicates that the lookup table structure is followed by a MarkFilteringSet field. The layout engine skips over all mark glyphs not in the mark filtering set indicated.
	TG_OPEN_TYPE__LOOKUP_FLAG__RESERVED                  = 0x00e0, // reserved: For future use (Set to zero)
	TG_OPEN_TYPE__LOOKUP_FLAG__MARK_ATTACHMENT_TYPE      = 0xff00  // markAttachmentType: If not zero, skips over all marks of attachment type different from specified.
} tg_open_type__lookup_flag__bit__enumeration;

typedef struct tg_open_type__lookup__table
{
	u16             lookup_type;        // lookupType: Different enumerations for GSUB and GPOS
	u16             lookup_flag;        // lookupFlag: Lookup qualifiers
	u16             subtable_count;     // subTableCount: Number of subtables for this lookup
	//tg_offset16*    p_subtable_offsets; // subtableOffsets[subTableCount]: Array of offsets to lookup subtables, from beginning of Lookup table
	//u16             mark_filtering_set; // markFilteringSet: Index (base 0) into GDEF mark glyph sets structure. This field is only present if bit useMarkFilteringSet of lookup flags is set.
} tg_open_type__lookup__table;

typedef struct tg_open_type__lookup_list__table
{
	u16            lookup_count; // lookupCount: Number of lookups in this table
	tg_offset16    p_lookups[0]; // lookups[lookupCount]: Array of offsets to Lookup tables, from beginning of LookupList — zero based (first lookup is Lookup index = 0)
} tg_open_type__lookup_list__table;

typedef struct tg_open_type__feature__table
{
	tg_offset16    feature_params;           // featureParams: = NULL (reserved for offset to FeatureParams)
	u16            lookup_index_count;       // lookupIndexCount: Number of LookupList indices for this feature
	u16            p_lookup_list_indices[0]; // lookupListIndices[lookupIndexCount]: Array of indices into the LookupList — zero-based (first lookup is LookupListIndex = 0)
} tg_open_type__feature__table;

typedef struct tg_open_type__feature_record
{
	char           p_feature_tag[4]; // featureTag: 4-byte feature identification tag
	tg_offset16    feature_offset;   // featureOffset: Offset to Feature table, from beginning of FeatureList
} tg_open_type__feature_record;

typedef struct tg_open_type__feature_list__table
{
	u16                             feature_count;        // featureCount: Number of FeatureRecords in this table
	tg_open_type__feature_record    p_feature_records[0]; // featureRecords[featureCount]: Array of FeatureRecords — zero-based (first feature has FeatureIndex = 0), listed alphabetically by feature tag
} tg_open_type__feature_list__table;

typedef struct tg_open_type__lang_sys__table
{
	tg_offset16    lookup_order;           // lookupOrder: = NULL (reserved for an offset to a reordering table)
	u16            required_feature_index; // requiredFeatureIndex: Index of a feature required for this language system; if no required features = 0xFFFF
	u16            feature_index_count;    // featureIndexCount: Number of feature index values for this language system — excludes the required feature
	u16            p_feature_indices[0];   // featureIndices[featureIndexCount]: Array of indices into the FeatureList, in arbitrary order
} tg_open_type__lang_sys__table;

typedef struct tg_open_type__lang_sys_record
{
	char           p_lang_sys_tag[4]; // langSysTag: 4-byte LangSysTag identifier
	tg_offset16    lang_sys_offset;   // langSysOffset: Offset to LangSys table, from beginning of Script table
} tg_open_type__lang_sys_record;

typedef struct tg_open_type__script__table
{
	tg_offset16                      default_lang_sys;      // defaultLangSys: Offset to default LangSys table, from beginning of Script table — may be NULL
	u16                              lang_sys_count;        // langSysCount: Number of LangSysRecords for this script — excluding the default LangSys
	tg_open_type__lang_sys_record    p_lang_sys_records[0]; // langSysRecords[langSysCount]: Array of LangSysRecords, listed alphabetically by LangSys tag
} tg_open_type__script__table;

typedef struct tg_open_type__script_record
{
	char           p_script_tag[4]; // scriptTag: 4-byte script tag identifier
	tg_offset16    script_offset;   // scriptOffset: Offset to Script table, from beginning of ScriptList
} tg_open_type__script_record;

typedef struct tg_open_type__script_list__table
{
	u16                            script_count;        // scriptCount: Number of ScriptRecords
	tg_open_type__script_record    p_script_records[0]; // scriptRecords[scriptCount]: Array of ScriptRecords, listed alphabetically by script tag
} tg_open_type__script_list__table;

#endif
