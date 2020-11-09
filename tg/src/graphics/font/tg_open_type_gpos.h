#ifndef TG_OPEN_TYPE_GPOS_H
#define TG_OPEN_TYPE_GPOS_H

#include "graphics/font/tg_open_type_types.h"

typedef struct tg_open_type__mark_record
{
	u16            mark_class;         // markClass: Class defined for the associated mark.
	tg_offset16    mark_anchor_offset; // markAnchorOffset: Offset to Anchor table, from beginning of MarkArray table.
} tg_open_type__mark_record;

typedef struct tg_open_type__mark_array__table
{
	u16                          mark_count;        // markCount: Number of MarkRecords
	tg_open_type__mark_record    p_mark_records[0]; // markRecords[markCount]: Array of MarkRecords, ordered by corresponding glyphs in the associated mark Coverage table.
} tg_open_type__mark_array__table;

typedef struct tg_open_type__anchor_format_3__table
{
	u16            anchor_format;   // anchorFormat: Format identifier, = 3
	i16            x_coordinate;    // xCoordinate: Horizontal value, in design units
	i16            y_coordinate;    // yCoordinate: Vertical value, in design units
	tg_offset16    x_device_offset; // xDeviceOffset: Offset to Device table (non-variable font) / VariationIndex table (variable font) for X coordinate, from beginning of Anchor table (may be NULL)
	tg_offset16    y_device_offset; // yDeviceOffset: Offset to Device table (non-variable font) / VariationIndex table (variable font) for Y coordinate, from beginning of Anchor table (may be NULL)
} tg_open_type__anchor_format_3__table;

typedef struct tg_open_type__anchor_format_2__table
{
	u16    anchor_format; // anchorFormat: Format identifier, = 2
	i16    x_coordinate;  // xCoordinate: Horizontal value, in design units
	i16    y_coordinate;  // yCoordinate: Vertical value, in design units
	u16    anchor_point;  // anchorPoint: Index to glyph contour point
} tg_open_type__anchor_format_2__table;

typedef struct tg_open_type__anchor_format_1__table
{
	u16    anchor_format; // anchorFormat: Format identifier, = 1
	i16    x_coordinate;  // xCoordinate: Horizontal value, in design units
	i16    y_coordinate;  // yCoordinate: Vertical value, in design units
} tg_open_type__anchor_format_1__table;

typedef enum tg_open_type__value_format__flags
{
	TG_OPEN_TYPE__VALUE_FORMAT__X_PLACEMENT           = 0x0001, // X_PLACEMENT: Includes horizontal adjustment for placement
	TG_OPEN_TYPE__VALUE_FORMAT__Y_PLACEMENT           = 0x0002, // Y_PLACEMENT: Includes vertical adjustment for placement
	TG_OPEN_TYPE__VALUE_FORMAT__X_ADVANCE             = 0x0004, // X_ADVANCE: Includes horizontal adjustment for advance
	TG_OPEN_TYPE__VALUE_FORMAT__Y_ADVANCE             = 0x0008, // Y_ADVANCE: Includes vertical adjustment for advance
	TG_OPEN_TYPE__VALUE_FORMAT__X_PLACEMENT_DEVICE    = 0x0010, // X_PLACEMENT_DEVICE: Includes Device table (non-variable font) / VariationIndex table (variable font) for horizontal placement
	TG_OPEN_TYPE__VALUE_FORMAT__Y_PLACEMENT_DEVICE    = 0x0020, // Y_PLACEMENT_DEVICE: Includes Device table (non-variable font) / VariationIndex table (variable font) for vertical placement
	TG_OPEN_TYPE__VALUE_FORMAT__X_ADVANCE_DEVICE      = 0x0040, // X_ADVANCE_DEVICE: Includes Device table (non-variable font) / VariationIndex table (variable font) for horizontal advance
	TG_OPEN_TYPE__VALUE_FORMAT__Y_ADVANCE_DEVICE      = 0x0080, // Y_ADVANCE_DEVICE: Includes Device table (non-variable font) / VariationIndex table (variable font) for vertical advance
	TG_OPEN_TYPE__VALUE_FORMAT__RESERVED              = 0xFF00  // Reserved: For future use (set to zero)
} tg_open_type__value_format__flags;

typedef struct tg_open_type__value_record
{
	i16            x_placement;         // xPlacement: Horizontal adjustment for placement, in design units.
	i16            y_placement;         // yPlacement: Vertical adjustment for placement, in design units.
	i16            x_advance;           // xAdvance: Horizontal adjustment for advance, in design units — only used for horizontal layout.
	i16            y_advance;           // yAdvance: Vertical adjustment for advance, in design units — only used for vertical layout.
	tg_offset16    x_pla_device_offset; // xPlaDeviceOffset: Offset to Device table (non-variable font) / VariationIndex table (variable font) for horizontal placement, from beginning of the immediate parent table (SinglePos or PairPosFormat2 lookup subtable, PairSet table within a PairPosFormat1 lookup subtable) — may be NULL.
	tg_offset16    y_pla_device_offset; // yPlaDeviceOffset: Offset to Device table (non-variable font) / VariationIndex table (variable font) for vertical placement, from beginning of the immediate parent table (SinglePos or PairPosFormat2 lookup subtable, PairSet table within a PairPosFormat1 lookup subtable) — may be NULL.
	tg_offset16    x_adv_device_offset; // xAdvDeviceOffset: Offset to Device table (non-variable font) / VariationIndex table (variable font) for horizontal advance, from beginning of the immediate parent table (SinglePos or PairPosFormat2 lookup subtable, PairSet table within a PairPosFormat1 lookup subtable) — may be NULL.
	tg_offset16    y_adv_device_offset; // yAdvDeviceOffset: Offset to Device table (non-variable font) / VariationIndex table (variable font) for vertical advance, from beginning of the immediate parent table (SinglePos or PairPosFormat2 lookup subtable, PairSet table within a PairPosFormat1 lookup subtable) — may be NULL.
} tg_open_type__value_record;

typedef struct tg_open_type__extension_pos_format_1__subtable
{
	u16            pos_format;            // posFormat: Format identifier: format = 1
	u16            extension_lookup_type; // extensionLookupType: Lookup type of subtable referenced by extensionOffset (i.e. the extension subtable).
	tg_offset32    extension_offset;      // extensionOffset: Offset to the extension subtable, of lookup type extensionLookupType, relative to the start of the ExtensionPosFormat1 subtable.
} tg_open_type__extension_pos_format_1__subtable;

typedef struct tg_open_type__pos_lookup_record tg_open_type__pos_lookup_record;

typedef struct tg_open_type__chain_context_pos_format_3__subtable
{
	u16                                 pos_format;                   // posFormat: Format identifier: format = 3
	u16                                 backtrack_glyph_count;        // backtrackGlyphCount: Number of glyphs in the backtrack sequence
	tg_offset16*                        p_backtrack_coverage_offsets; // backtrackCoverageOffsets[backtrackGlyphCount]: Array of offsets to coverage tables for the backtrack sequence, in glyph sequence order.
	u16                                 input_glyph_count;            // inputGlyphCount: Number of glyphs in input sequence
	tg_offset16*                        p_input_coverage_offsets;     // inputCoverageOffsets[inputGlyphCount]: Array of offsets to coverage tables for the input sequence, in glyph sequence order.
	u16                                 lookahead_glyph_count;        // lookaheadGlyphCount: Number of glyphs in lookahead sequence
	tg_offset16*                        p_lookahead_coverage_offsets; // lookaheadCoverageOffsets[lookaheadGlyphCount]: Array of offsets to coverage tables for the lookahead sequence, in glyph sequence order.
	u16                                 pos_count;                    // posCount: Number of PosLookupRecords
	tg_open_type__pos_lookup_record*    p_pos_lookup_records;         // posLookupRecords[posCount]: Array of PosLookupRecords, in design order.
} tg_open_type__chain_context_pos_format_3__subtable;

typedef struct tg_open_type__chain_pos_class_rule__table
{
	u16                                 backtrack_glyph_count; // backtrackGlyphCount: Total number of glyphs in the backtrack sequence.
	u16*                                p_backtrack_sequence;  // backtrackSequence[backtrackGlyphCount]: Array of backtrack-sequence classes.
	u16                                 input_glyph_count;     // inputGlyphCount: Total number of classes in the input sequence — includes the first class.
	u16*                                p_input_sequence;      // inputSequence[inputGlyphCount - 1]: Array of input classes to be matched to the input glyph sequence, beginning with the second glyph position.
	u16                                 lookahead_glyph_count; // lookaheadGlyphCount: Total number of classes in the lookahead sequence.
	u16*                                p_lookahead_sequence;  // lookAheadSequence[lookAheadGlyphCount]: Array of lookahead-sequence classes.
	u16                                 pos_count;             // posCount: Number of PosLookupRecords
	tg_open_type__pos_lookup_record*    p_pos_lookup_records;  // posLookupRecords[posCount]: Array of PosLookupRecords, in design order.
} tg_open_type__chain_pos_class_rule__table;

typedef struct tg_open_type__chain_pos_class_set__table
{
	u16            chain_pos_class_rule_count;        // chainPosClassRuleCount: Number of ChainPosClassRule tables
	tg_offset16    p_chain_pos_class_rule_offsets[0]; // chainPosClassRuleOffsets[chainPosClassRuleCount]: Array of offsets to ChainPosClassRule tables. Offsets are from beginning of ChainPosClassSet, ordered by preference.
} tg_open_type__chain_pos_class_set__table;

typedef struct tg_open_type__chain_context_pos_format_2__subtable
{
	u16            pos_format;                       // posFormat: Format identifier: format = 2
	tg_offset16    coverage_offset;                  // coverageOffset: Offset to Coverage table, from beginning of ChainContextPos subtable.
	tg_offset16    backtrack_class_def_offset;       // backtrackClassDefOffset: Offset to ClassDef table containing backtrack sequence context, from beginning of ChainContextPos subtable.
	tg_offset16    input_class_def_offset;           // inputClassDefOffset: Offset to ClassDef table containing input sequence context, from beginning of ChainContextPos subtable.
	tg_offset16    lookahead_class_def_offset;       // lookaheadClassDefOffset: Offset to ClassDef table containing lookahead sequence context, from beginning of ChainContextPos subtable.
	u16            chain_pos_class_set_count;        // chainPosClassSetCount: Number of ChainPosClassSet tables
	tg_offset16    p_chain_pos_class_set_offsets[0]; // chainPosClassSetOffsets[chainPosClassSetCount]: Array of offsets to ChainPosClassSet tables. Offsets are from beginning of ChainContextPos subtable, ordered by input class (may be NULL).
} tg_open_type__chain_context_pos_format_2__subtable;

typedef struct tg_open_type__chain_pos_rule__table
{
	u16                                 backtrack_glyph_count; // backtrackGlyphCount: Total number of glyphs in the backtrack sequence.
	u16*                                p_backtrack_sequence;  // backtrackSequence[backtrackGlyphCount]: Array of backtrack glyph IDs.
	u16                                 input_glyph_count;     // inputGlyphCount: Total number of glyphs in the input sequence — includes the first glyph.
	u16*                                p_input_sequence;      // inputSequence[inputGlyphCount - 1]: Array of input glyph IDs — start with second glyph.
	u16                                 lookahead_glyph_count; // lookaheadGlyphCount: Total number of glyphs in the lookahead sequence.
	u16*                                p_lookahead_sequence;  // lookAheadSequence[lookAheadGlyphCount]: Array of lookahead glyph IDs.
	u16                                 pos_count;             // posCount: Number of PosLookupRecords
	tg_open_type__pos_lookup_record*    p_pos_lookup_records;  // posLookupRecords[posCount]: Array of PosLookupRecords, in design order.
} tg_open_type__chain_pos_rule__table;

typedef struct tg_open_type__chain_pos_rule_set__table
{
	u16            chain_pos_rule_count;        // chainPosRuleCount: Number of ChainPosRule tables
	tg_offset16    p_chain_pos_rule_offsets[0]; // chainPosRuleOffsets[chainPosRuleCount]: Array of offsets to ChainPosRule tables. Offsets are from beginning of ChainPosRuleSet, ordered by preference.
} tg_open_type__chain_pos_rule_set__table;

typedef struct tg_open_type__chain_context_pos_format_1__subtable
{
	u16            pos_format;                      // posFormat: Format identifier: format = 1
	tg_offset16    coverage_offset;                 // coverageOffset: Offset to Coverage table, from beginning of ChainContextPos subtable.
	u16            chain_pos_rule_set_count;        // chainPosRuleSetCount: Number of ChainPosRuleSet tables
	tg_offset16    p_chain_pos_rule_set_offsets[0]; // chainPosRuleSetOffsets[chainPosRuleSetCount]: Array of offsets to ChainPosRuleSet tables. Offsets are from beginning of ChainContextPos subtable, ordered by Coverage Index.
} tg_open_type__chain_context_pos_format_1__subtable;

typedef struct tg_open_type__context_pos_format_3__subtable
{
	u16                                 pos_format;           // posFormat: Format identifier: format = 3
	u16                                 glyph_count;          // glyphCount: Number of glyphs in the input sequence
	u16                                 pos_count;            // posCount: Number of PosLookupRecords
	tg_offset16*                        p_coverage_offsets;   // coverageOffsets[glyphCount]: Array of offsets to Coverage tables, from beginning of ContextPos subtable.
	tg_open_type__pos_lookup_record*    p_pos_lookup_records; // posLookupRecords[posCount]: Array of PosLookupRecords, in design order.
} tg_open_type__context_pos_format_3__subtable;

typedef struct tg_open_type__pos_class_rule__table
{
	u16                                 glyph_count;          // glyphCount: Number of glyphs to be matched
	u16                                 pos_count;            // posCount: Number of PosLookupRecords
	u16*                                p_classes;            // classes[glyphCount - 1]: Array of classes to be matched to the input glyph sequence, beginning with the second glyph position.
	tg_open_type__pos_lookup_record*    p_pos_lookup_records; // posLookupRecords[posCount]: Array of PosLookupRecords, in design order.
} tg_open_type__pos_class_rule__table;

typedef struct tg_open_type__pos_class_set__table
{
	u16            pos_class_rule_count;        // posClassRuleCount: Number of PosClassRule tables
	tg_offset16    p_pos_class_rule_offsets[0]; // posClassRuleOffsets[posClassRuleCount]: Array of offsets to PosClassRule tables. Offsets are from beginning of PosClassSet, ordered by preference.
} tg_open_type__pos_class_set__table;

typedef struct tg_open_type__context_pos_format_2__subtable
{
	u16            pos_format;                 // posFormat: Format identifier : format = 2
	tg_offset16    coverage_offset;            // coverageOffset: Offset to Coverage table, from beginning of ContextPos subtable.
	tg_offset16    class_def_offset;           // classDefOffset: Offset to ClassDef table, from beginning of ContextPos subtable.
	u16            pos_class_set_count;        // posClassSetCount: Number of PosClassSet tables
	tg_offset16    p_pos_class_set_offsets[0]; // posClassSetOffsets[posClassSetCount]: Array of offsets to PosClassSet tables. Offsets are from beginning of ContextPos subtable, ordered by class (may be NULL).
} tg_open_type__context_pos_format_2__subtable;

typedef struct tg_open_type__pos_file__table
{
	u16                                 glyph_count;          // glyphCount: Number of glyphs in the input glyph sequence
	u16                                 pos_count;            // posCount: Number of PosLookupRecords
	u16*                                p_input_sequence;     // inputSequence[glyphCount - 1]: Array of input glyph IDs — starting with the second glyph.
	tg_open_type__pos_lookup_record*    p_pos_lookup_records; // posLookupRecords[posCount]: Array of positioning lookups, in design order.
} tg_open_type__pos_file__table;

typedef struct tg_open_type__pos_rule_set__table
{
	u16            pos_rule_count;        // posRuleCount: Number of PosRule tables
	tg_offset16    p_pos_rule_offsets[0]; // posRuleOffsets[posRuleCount]: Array of offsets to PosRule tables. Offsets are from beginning of PosRuleSet, ordered by preference.
} tg_open_type__pos_rule_set__table;

typedef struct tg_open_type__context_pos_format_1__subtable
{
	u16            pos_format;                // posFormat: Format identifier: format = 1
	tg_offset16    coverage_offset;           // coverageOffset: Offset to Coverage table, from beginning of ContextPos subtable.
	u16            pos_rule_set_count;        // posRuleSetCount: Number of PosRuleSet tables
	tg_offset16    p_pos_rule_set_offsets[0]; // posRuleSetOffsets[posRuleSetCount]: Array of offsets to PosRuleSet tables. Offsets are from beginning of ContextPos subtable, ordered by Coverage Index.
} tg_open_type__context_pos_format_1__subtable;

typedef struct tg_open_type__pos_lookup_record
{
	u16    sequence_index;    // sequenceIndex: Index (zero-based) to input glyph sequence
	u16    lookup_list_index; // lookupListIndex: Index (zero-based) into the LookupList for the Lookup table to apply to that position in the glyph sequence.
} tg_open_type__pos_lookup_record;

typedef struct tg_open_type__mark_2_record
{
	tg_offset16 p_mark_2_anchor_offsets[0]; // mark2AnchorOffsets[markClassCount]: Array of offsets(one per class) to Anchor tables.Offsets are from beginning of Mark2Array table, in class order.
} tg_open_type__mark_2_record;

typedef struct tg_open_type__mark_2_array__table
{
	u16      mark_2_count;     // mark2Count: Number of Mark2 records
	void*    p_mark_2_records; // mark2Records[mark2Count]: Array of Mark2Records, in Coverage order.
} tg_open_type__mark_2_array__table;

typedef struct tg_open_type__mark_mark_pos_format_1__subtable
{
	u16            pos_format;             // posFormat: Format identifier: format = 1
	tg_offset16    mark_1_coverage_offset; // mark1CoverageOffset: Offset to Combining Mark Coverage table, from beginning of MarkMarkPos subtable.
	tg_offset16    mark_2_coverage_offset; // mark2CoverageOffset: Offset to Base Mark Coverage table, from beginning of MarkMarkPos subtable.
	u16            mark_class_count;       // markClassCount: Number of Combining Mark classes defined
	tg_offset16    mark_1_array_offset;    // mark1ArrayOffset: Offset to MarkArray table for mark1, from beginning of MarkMarkPos subtable.
	tg_offset16    mark_2_array_offset;    // mark2ArrayOffset: Offset to Mark2Array table for mark2, from beginning of MarkMarkPos subtable.
} tg_open_type__mark_mark_pos_format_1__subtable;

typedef struct tg_open_type__component_record
{
	tg_offset16    p_ligature_anchor_offsets[0]; // ligatureAnchorOffsets[markClassCount]: Array of offsets (one per class) to Anchor tables. Offsets are from beginning of LigatureAttach table, ordered by class (may be NULL).
} tg_open_type__component_record;

typedef struct tg_open_type__ligature_attach__table
{
	u16      component_count;     // componentCount: Number of ComponentRecords in this ligature
	void*    p_component_records; // componentRecords[componentCount]: Array of Component records, ordered in writing direction.
} tg_open_type__ligature_attach__table;

typedef struct tg_open_type__ligature_array__table
{
	u16            ligature_count;               // ligatureCount: Number of LigatureAttach table offsets
	tg_offset16    p_ligature_attach_offsets[0]; // ligatureAttachOffsets[ligatureCount]: Array of offsets to LigatureAttach tables. Offsets are from beginning of LigatureArray table, ordered by ligatureCoverage index.
} tg_open_type__ligature_array__table;

typedef struct tg_open_type__mark_lig_pos_format_1__subtable
{
	u16            pos_format;               // posFormat: Format identifier: format = 1
	tg_offset16    mark_coverage_offset;     // markCoverageOffset: Offset to markCoverage table, from beginning of MarkLigPos subtable.
	tg_offset16    ligature_coverage_offset; // ligatureCoverageOffset: Offset to ligatureCoverage table, from beginning of MarkLigPos subtable.
	u16            mark_class_count;         // markClassCount: Number of defined mark classes
	tg_offset16    mark_array_offst;         // markArrayOffset: Offset to MarkArray table, from beginning of MarkLigPos subtable.
	tg_offset16    ligature_array_offset;    // ligatureArrayOffset: Offset to LigatureArray table, from beginning of MarkLigPos subtable.
} tg_open_type__mark_lig_pos_format_1__subtable;

typedef struct tg_open_type__base_record
{
	tg_offset16    p_base_anchor_offset[0]; // baseAnchorOffset[markClassCount]: Array of offsets (one per mark class) to Anchor tables. Offsets are from beginning of BaseArray table, ordered by class.
} tg_open_type__base_record;

typedef struct tg_open_type__base_array__table
{
	u16      base_count;        // baseCount: Number of BaseRecords
	void*    p_base_records[0]; // baseRecords[baseCount]: Array of BaseRecords, in order of baseCoverage Index.
} tg_open_type__base_array__table;

typedef struct tg_open_type__mask_base_pos_format_1__subtable
{
	u16            pos_format;           // posFormat: Format identifier : format = 1
	tg_offset16    mark_coverage_offset; // markCoverageOffset: Offset to markCoverage table, from beginning of MarkBasePos subtable.
	tg_offset16    base_coverage_offset; // baseCoverageOffset: Offset to baseCoverage table, from beginning of MarkBasePos subtable.
	u16            mark_class_count;     // markClassCount: Number of classes defined for marks
	tg_offset16    mask_array_offset;    // markArrayOffset: Offset to MarkArray table, from beginning of MarkBasePos subtable.
	tg_offset16    base_array_offset;    // baseArrayOffset: Offset to BaseArray table, from beginning of MarkBasePos subtable.
} tg_open_type__mask_base_pos_format_1__subtable;

typedef struct tg_open_type__entry_exit_record
{
	tg_offset16    entry_anchor_offset; // entryAnchorOffset: Offset to entryAnchor table, from beginning of CursivePos subtable (may be NULL).
	tg_offset16    exit_anchor_offst;   // exitAnchorOffset: Offset to exitAnchor table, from beginning of CursivePos subtable (may be NULL).
} tg_open_type__entry_exit_record;

typedef struct tg_open_type__cursive_pos_format_1__subtable
{
	u16                                pos_format;              // posFormat: Format identifier : format = 1
	tg_offset16                        coverage_offset;         // coverageOffset: Offset to Coverage table, from beginning of CursivePos subtable.
	u16                                entry_exit_count;        // entryExitCount: Number of EntryExit records
	tg_open_type__entry_exit_record    p_entry_exit_records[0]; // entryExitRecord[entryExitCount]: Array of EntryExit records, in Coverage index order.
} tg_open_type__cursive_pos_format_1__subtable;

typedef struct tg_open_type__class_2_record
{
	tg_open_type__value_record    value_record_1; // valueRecord1: Positioning for first glyph — empty if valueFormat1 = 0.
	tg_open_type__value_record    value_record_2; // valueRecord2: Positioning for second glyph — empty if valueFormat2 = 0.
} tg_open_type__class_2_record;

typedef struct tg_open_type__class_1_record
{
	tg_open_type__class_2_record p_class_2_records[0]; // class2Records[class2Count]: Array of Class2 records, ordered by classes in classDef2.
} tg_open_type__class_1_record;

typedef struct tg_open_type__pair_pos_format_2__subtable
{
	u16                             pos_format;         // posFormat: Format identifier : format = 2
	tg_offset16                     coverage_offset;    // coverageOffset: Offset to Coverage table, from beginning of PairPos subtable.
	u16                             value_format_1;     // valueFormat1: ValueRecord definition — for the first glyph of the pair (may be zero).
	u16                             value_format_2;     // valueFormat2: ValueRecord definition — for the second glyph of the pair (may be zero).
	tg_offset16                     class_def_1_offset; // classDef1Offset: Offset to ClassDef table, from beginning of PairPos subtable — for the first glyph of the pair.
	tg_offset16                     class_def_2_offset; // classDef2Offset: Offset to ClassDef table, from beginning of PairPos subtable — for the second glyph of the pair.
	u16                             class_1_count;      // class1Count: Number of classes in classDef1 table — includes Class 0.
	u16                             class_2_count;      // class2Count: Number of classes in classDef2 table — includes Class 0.
	void*                           p_class_1_records;  // class1Records[class1Count]: Array of Class1 records, ordered by classes in classDef1.
} tg_open_type__pair_pos_format_2__subtable;

typedef struct tg_open_type__pair_value_record
{
	u16                           second_glyph;   // secondGlyph: Glyph ID of second glyph in the pair (first glyph is listed in the Coverage table).
	tg_open_type__value_record    value_record_1; // valueRecord1: Positioning data for the first glyph in the pair.
	tg_open_type__value_record    value_record_2; // valueRecord2: Positioning data for the second glyph in the pair.
} tg_open_type__pair_value_record;

typedef struct tg_open_type__pair_set__table
{
	u16                                pair_value_count;        // pairValueCount: Number of PairValueRecords
	tg_open_type__pair_value_record    p_pair_value_records[0]; // pairValueRecords[pairValueCount]: Array of PairValueRecords, ordered by glyph ID of the second glyph.
} tg_open_type__pair_set__table;

typedef struct tg_open_type__pair_pos_format_1__subtable
{
	u16            pos_format;            // posFormat: Format identifier : format = 1
	tg_offset16    coverage_offset;       // coverageOffset: Offset to Coverage table, from beginning of PairPos subtable.
	u16            value_format_1;        // valueFormat1: Defines the types of data in valueRecord1 — for the first glyph in the pair (may be zero).
	u16            value_format_2;        // valueFormat2: Defines the types of data in valueRecord2 — for the second glyph in the pair (may be zero).
	u16            pair_set_count;        // pairSetCount: Number of PairSet tables
	tg_offset16    p_pair_set_offsets[0]; // pairSetOffsets[pairSetCount]: Array of offsets to PairSet tables.Offsets are from beginning of PairPos subtable, ordered by Coverage Index.
} tg_open_type__pair_pos_format_1__subtable;

typedef struct tg_open_type__single_pos_format_2__subtable
{
	u16                           pos_format;         // posFormat: Format identifier : format = 2
	tg_offset16                   coverage_offset;    // coverageOffset: Offset to Coverage table, from beginning of SinglePos subtable.
	u16                           value_format;       // valueFormat: Defines the types of data in the ValueRecords.
	u16                           value_count;        // valueCount: Number of ValueRecords — must equal glyphCount in the Coverage table.
	tg_open_type__value_record    p_value_records[0]; // valueRecords[valueCount]: Array of ValueRecords — positioning values applied to glyphs.
} tg_open_type__single_pos_format_2__subtable;

typedef struct tg_open_type__single_pos_format_1__subtable
{
	u16                           pos_format;      // posFormat: Format identifier : format = 1
	tg_offset16                   coverage_offset; // coverageOffset: Offset to Coverage table, from beginning of SinglePos subtable.
	u16                           value_format;    // valueFormat: Defines the types of data in the ValueRecord.
	tg_open_type__value_record    value_record;    // valueRecord: Defines positioning value(s) — applied to all glyphs in the Coverage table.
} tg_open_type__single_pos_format_1__subtable;

typedef struct tg_open_type__gpos_header_v11
{
	u16            major_version;             // majorVersion: Major version of the GPOS table, = 1
	u16            minor_version;             // minorVersion: Minor version of the GPOS table, = 1
	tg_offset16    script_list_offset;        // scriptListOffset: Offset to ScriptList table, from beginning of GPOS table
	tg_offset16    feature_list_offset;       // featureListOffset: Offset to FeatureList table, from beginning of GPOS table
	tg_offset16    lookup_list_offset;        // lookupListOffset: Offset to LookupList table, from beginning of GPOS table
	tg_offset32    feature_variations_offset; // featureVariationsOffset: Offset to FeatureVariations table, from beginning of GPOS table (may be NULL)
} tg_open_type__gpos_header_v11;

typedef struct tg_open_type__gpos_header_v10
{
	u16            major_version;       // majorVersion: Major version of the GPOS table, = 1
	u16            minor_version;       // minorVersion: Minor version of the GPOS table, = 0
	tg_offset16    script_list_offset;  // scriptListOffset: Offset to ScriptList table, from beginning of GPOS table
	tg_offset16    feature_list_offset; // featureListOffset: Offset to FeatureList table, from beginning of GPOS table
	tg_offset16    lookup_list_offset;  // lookupListOffset: Offset to LookupList table, from beginning of GPOS table
} tg_open_type_gpos_header_v10;

typedef enum tg_open_type__gpos__lookup_type__enumeration
{
	TG_OPEN_TYPE__GPOS__LOOKUP_TYPE__SINGLE_ADJUSTMENT              = 1, // Single adjustment: Adjust position of a single glyph
	TG_OPEN_TYPE__GPOS__LOOKUP_TYPE__PAIR_ADJUSTMENT                = 2, // Pair adjustment: Adjust position of a pair of glyphs
	TG_OPEN_TYPE__GPOS__LOOKUP_TYPE__CURSIVE_ATTACHMENT             = 3, // Cursive attachment: Attach cursive glyphs
	TG_OPEN_TYPE__GPOS__LOOKUP_TYPE__MARK_TO_BASE                   = 4, // MarkToBase attachment: Attach a combining mark to a base glyph
	TG_OPEN_TYPE__GPOS__LOOKUP_TYPE__MARK_TO_LIGATURE               = 5, // MarkToLigature attachment: Attach a combining mark to a ligature
	TG_OPEN_TYPE__GPOS__LOOKUP_TYPE__MARK_TO_MARK                   = 6, // MarkToMark attachment: Attach a combining mark to another mark
	TG_OPEN_TYPE__GPOS__LOOKUP_TYPE__CONTEXT_POSITIONING            = 7, // Context positioning: Position one or more glyphs in context
	TG_OPEN_TYPE__GPOS__LOOKUP_TYPE__CHAINED_CONTEXT_POSITIONING    = 8, // Chained Context positioning: Position one or more glyphs in chained context
	TG_OPEN_TYPE__GPOS__LOOKUP_TYPE__EXTENSION_POSITIONING          = 9  // Extension positioning: Extension mechanism for other positionings
} tg_open_type__gpos__lookup_type__enumeration;

#endif
