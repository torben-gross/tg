#ifndef TG_OPEN_TYPE_HEAD_H
#define TG_OPEN_TYPE_HEAD_H

#include "graphics/font/tg_open_type_types.h"

typedef struct tg_open_type__head
{
	u16                major_version;       // majorVersion: Major version number of the font header table � set to 1.
	u16                minor_version;       // minorVersion: Minor version number of the font header table � set to 0.
	tg_fixed           font_revision;       // fontRevision: Set by font manufacturer.
	u32                checksum_adjustment; // checkSumAdjustment: To compute: set it to 0, sum the entire font as uint32, then store 0xB1B0AFBA - sum. If the font is used as a component in a font collection file, the value of this field will be invalidated by changes to the file structure and font table directory, and must be ignored.
	u32                magic_number;        // magicNumber: Set to 0x5F0F3CF5.

	// Bit 0:     Baseline for font at y=0;
	// Bit 1:     Left sidebearing point at x=0 (relevant only for TrueType rasterizers) � see the note below regarding variable fonts;
	// Bit 2:     Instructions may depend on point size;
	// Bit 3:     Force ppem to integer values for all internal scaler math; may use fractional ppem sizes if this bit is clear;
	// Bit 4:     Instructions may alter advance width (the advance widths might not scale linearly);
	// Bit 5:     This bit is not used in OpenType, and should not be set in order to ensure compatible behavior on all platforms. If set, it may result in different behavior for vertical layout in some platforms. (See Apple�s specification for details regarding behavior in Apple platforms.)
	// Bits 6�10: These bits are not used in Opentype and should always be cleared. (See Apple�s specification for details regarding legacy used in Apple platforms.)
	// Bit 11:    Font data is �lossless� as a result of having been subjected to optimizing transformation and/or compression (such as e.g. compression mechanisms defined by ISO/IEC 14496-18, MicroType Express, WOFF 2.0 or similar) where the original font functionality and features are retained but the binary compatibility between input and output font files is not guaranteed. As a result of the applied transform, the DSIG table may also be invalidated.
	// Bit 12:    Font converted (produce compatible metrics)
	// Bit 13:    Font optimized for ClearType�. Note, fonts that rely on embedded bitmaps (EBDT) for rendering should not be considered optimized for ClearType, and therefore should keep this bit cleared.
	// Bit 14:    Last Resort font. If set, indicates that the glyphs encoded in the 'cmap' subtables are simply generic symbolic representations of code point ranges and don�t truly represent support for those code points. If unset, indicates that the glyphs encoded in the 'cmap' subtables represent proper support for those code points.
	// Bit 15:    Reserved, set to 0
	u16                flags;               // flags

	u16                units_per_em;        // unitsPerEm: Set to a value from 16 to 16384. Any value in this range is valid. In fonts that have TrueType outlines, a power of 2 is recommended as this allows performance optimizations in some rasterizers.
	tg_longdatetime    created;             // created: Number of seconds since 12:00 midnight that started January 1st 1904 in GMT/UTC time zone. 64-bit integer
	tg_longdatetime    modified;            // modified: Number of seconds since 12:00 midnight that started January 1st 1904 in GMT/UTC time zone. 64-bit integer
	i16                x_min;               // xMin: For all glyph bounding boxes.
	i16                y_min;               // yMin: For all glyph bounding boxes.
	i16                x_max;               // xMax: For all glyph bounding boxes.
	i16                y_max;               // yMax: For all glyph bounding boxes.

	// Bit 0: Bold (if set to 1);
	// Bit 1: Italic (if set to 1)
	// Bit 2: Underline (if set to 1)
	// Bit 3: Outline (if set to 1)
	// Bit 4: Shadow (if set to 1)
	// Bit 5: Condensed (if set to 1)
	// Bit 6: Extended (if set to 1)
	// Bits 7�15: Reserved (set to 0).
	u16                mac_style;           // macStyle

	u16                lowest_rec_ppem;     // lowestRecPPEM: Smallest readable size in pixels.

	// Deprecated (Set to 2).
	//  0: Fully mixed directional glyphs;
	//  1: Only strongly left to right;
	//  2: Like 1 but also contains neutrals;
	// -1: Only strongly right to left;
	// -2: Like -1 but also contains neutrals.
	// (A neutral character has no inherent directionality; it is not a character with zero (0) width. Spaces and punctuation are examples of neutral characters. Non-neutral characters are those with inherent directionality. For example, Roman letters (left-to-right) and Arabic letters (right-to-left) have directionality. In a �normal� Roman font where spaces and punctuation are present, the font direction hints should be set to two (2).)
	i16                font_direction_hint; // fontDirectionHint

	i16                index_to_loc_format; // indexToLocFormat: 0 for short offsets (Offset16), 1 for long (Offset32).
	i16                glyph_data_format;   // glyphDataFormat: 0 for current format.
} tg_open_type__head;

#endif
