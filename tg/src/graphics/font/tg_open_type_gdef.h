#ifndef TG_OPEN_TYPE_GDEF_H
#define TG_OPEN_TYPE_GDEF_H

#include "graphics/font/tg_open_type_types.h"

typedef struct tg_open_type__mark_glyph_sets_table
{
	u16            mark_glyph_set_table_format; // markGlyphSetTableFormat: Format identifier == 1
	u16            mark_glyph_set_count;        // markGlyphSetCount: Number of mark glyph sets defined
	tg_offset32    p_coverage_offsets[0];       // coverageOffsets[markGlyphSetCount]: Array of offsets to mark glyph set coverage tables.
} tg_open_type__mark_glyph_sets_table;

typedef struct tg_open_type__varet_value_format_3__table // Design units plus Device or VariationIndex table
{
	u16            caret_value_format; // caretValueFormat: Format identifier : format = 3
	i16            coordinate;         // coordinate: X or Y value, in design units
	tg_offset16    device_offset;      // deviceOffset: Offset to Device table(non - variable font) / Variation Index table(variable font) for X or Y value - from beginning of CaretValue table
} tg_open_type__varet_value_format_3__table;

typedef struct tg_open_type__varet_value_format_2__table // Contour point
{
	u16    caret_value_format;      // caretValueFormat: Format identifier : format = 2
	u16    caret_value_point_index; // caretValuePointIndex: Contour point index on glyph
} tg_open_type__varet_value_format_2__table;

typedef struct tg_open_type__varet_value_format_1__table // Design units only
{
	u16    caret_value_format; // caretValueFormat: Format identifier : format = 1
	i16    coordinate;         // coordinate: X or Y value, in design units
} tg_open_type__varet_value_format_1__table;

typedef struct tg_open_type__lig_glyph__table
{
	u16            caret_count;              // caretCount: Number of CaretValue tables for this ligature(components - 1)
	tg_offset16    p_caret_value_offsets[0]; // caretValueOffsets[caretCount]: Array of offsets to CaretValue tables, from beginning of LigGlyph table — in increasing coordinate order
} tg_open_type__lig_glyph__table;

typedef struct tg_open_type__lig_caret_list__table
{
	tg_offset16    coverage_offset;        // coverageOffset: Offset to Coverage table - from beginning of LigCaretList table
	u16            lig_glyph_count;        // ligGlyphCount: Number of ligature glyphs
	tg_offset16    p_lig_glyph_offsets[0]; // ligGlyphOffsets[ligGlyphCount]: Array of offsets to LigGlyph tables, from beginning of LigCaretList table —in Coverage Index order
} tg_open_type__lig_caret_list__table;

typedef struct tg_open_type__attach_point__table
{
	u16    point_count;        // pointCount: Number of attachment points on this glyph
	u16    p_point_indices[0]; // pointIndices[pointCount]: Array of contour point indices -in increasing numerical order
} tg_open_type__attach_point__table;

typedef struct tg_open_type__attach_list__table
{
	tg_offset16    coverage_offset;           // coverageOffset: Offset to Coverage table - from beginning of AttachList table
	u16            glyph_count;               // glyphCount: Number of glyphs with attachment points
	tg_offset16    p_attach_point_offsets[0]; // attachPointOffsets[glyphCount]: Array of offsets to AttachPoint tables-from beginning of AttachList table-in Coverage Index order
} tg_open_type__attach_list__table;

typedef enum tg_open_type__gylph_class_def__enumeration__list
{
	TG_OPEN_TYPE__GLYPH_CLASS_DEF__BASE         = 1, // Base glyph (single character, spacing glyph)
	TG_OPEN_TYPE__GLYPH_CLASS_DEF__LIGATURE     = 2, // Ligature glyph (multiple character, spacing glyph)
	TG_OPEN_TYPE__GLYPH_CLASS_DEF__MARK         = 3, // Mark glyph (non-spacing combining glyph)
	TG_OPEN_TYPE__GLYPH_CLASS_DEF__COMPONENT    = 4  // Component glyph (part of single character, spacing glyph)
} tg_open_type__gylph_class_def__enumeration__list;

typedef struct tg_open_type__gdef_header_v13
{
	u16            major_version;                // majorVersion: Major version of the GDEF table, = 1
	u16            minor_version;                // minorVersion: Minor version of the GDEF table, = 3
	tg_offset16    glyph_class_def_offset;       // glyphClassDefOffset: Offset to class definition table for glyph type, from beginning of GDEF header (may be NULL)
	tg_offset16    attach_list_offset;           // attachListOffset: Offset to attachment point list table, from beginning of GDEF header (may be NULL)
	tg_offset16    lig_caret_list_offset;        // ligCaretListOffset: Offset to ligature caret list table, from beginning of GDEF header (may be NULL)
	tg_offset16    mark_attach_class_def_offset; // markAttachClassDefOffset: Offset to class definition table for mark attachment type, from beginning of GDEF header (may be NULL)
	tg_offset16    mark_glyph_sets_def_offset;   // markGlyphSetsDefOffset: Offset to the table of mark glyph set definitions, from beginning of GDEF header (may be NULL)
	tg_offset32    item_var_store_offset;        // itemVarStoreOffset: Offset to the Item Variation Store table, from beginning of GDEF header (may be NULL)
} tg_open_type__gdef_header_v13;

typedef struct tg_open_type__gdef_header_v12
{
	u16            major_version;                // majorVersion: Major version of the GDEF table, = 1
	u16            minor_version;                // minorVersion: Minor version of the GDEF table, = 2
	tg_offset16    glyph_class_def_offset;       // glyphClassDefOffset: Offset to class definition table for glyph type, from beginning of GDEF header (may be NULL)
	tg_offset16    attach_list_offset;           // attachListOffset: Offset to attachment point list table, from beginning of GDEF header (may be NULL)
	tg_offset16    lig_caret_list_offset;        // ligCaretListOffset: Offset to ligature caret list table, from beginning of GDEF header (may be NULL)
	tg_offset16    mark_attach_class_def_offset; // markAttachClassDefOffset: Offset to class definition table for mark attachment type, from beginning of GDEF header (may be NULL)
	tg_offset16    mark_glyph_sets_def_offset;   // markGlyphSetsDefOffset: Offset to the table of mark glyph set definitions, from beginning of GDEF header (may be NULL)
} tg_open_type__gdef_header_v12;

typedef struct tg_open_type__gdef_header_v10
{
	u16            major_version;                // majorVersion: Major version of the GDEF table, = 1
	u16            minor_version;                // minorVersion: Minor version of the GDEF table, = 0
	tg_offset16    glyph_class_def_offset;       // glyphClassDefOffset: Offset to class definition table for glyph type, from beginning of GDEF header (may be NULL)
	tg_offset16    attach_list_offset;           // attachListOffset: Offset to attachment point list table, from beginning of GDEF header (may be NULL)
	tg_offset16    lig_caret_list_offset;        // ligCaretListOffset: Offset to ligature caret list table, from beginning of GDEF header (may be NULL)
	tg_offset16    mark_attach_class_def_offset; // markAttachClassDefOffset: Offset to class definition table for mark attachment type, from beginning of GDEF header (may be NULL)
} tg_open_type__gdef_header_v10;

#endif
