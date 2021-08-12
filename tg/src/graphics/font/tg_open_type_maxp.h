#ifndef TG_OPEN_TYPE_MAXP_H
#define TG_OPEN_TYPE_MAXP_H

#include "graphics/font/tg_open_type_types.h"

#define TG_OPEN_TYPE__MAXP_VERSION_10 0x00010000
#define TG_OPEN_TYPE__MAXP_VERSION_05 0x00005000

typedef struct tg_open_type__maxp__version_10
{
	tg_fixed    version;                  // version: 0x00010000 for version 1.0.
	u16         num_glyphs;               // numGlyphs: The number of glyphs in the font.
	u16         max_points;               // maxPoints: Maximum points in a non-composite glyph.
	u16         max_contours;             // maxContours: Maximum contours in a non-composite glyph.
	u16         max_composite_points;     // maxCompositePoints: Maximum points in a composite glyph.
	u16         max_composite_contours;   // maxCompositeContours: Maximum contours in a composite glyph.
	u16         max_zones;                // maxZones: 1 if instructions do not use the twilight zone (Z0), or 2 if instructions do use Z0; should be set to 2 in most cases.
	u16         max_twilight_points;      // maxTwilightPoints: Maximum points used in Z0.
	u16         max_storage;              // maxStorage: Number of Storage Area locations.
	u16         max_function_defs;        // maxFunctionDefs: Number of FDEFs, equal to the highest function number + 1.
	u16         max_instruction_defs;     // maxInstructionDefs: Number of IDEFs.
	u16         max_stack_elements;       // maxStackElements: Maximum stack depth across Font Program ('fpgm' table), CVT Program ('prep' table) and all glyph instructions (in the 'glyf' table).
	u16         max_size_of_instructions; // maxSizeOfInstructions: Maximum byte count for glyph instructions.
	u16         max_component_elements;   // maxComponentElements: Maximum number of components referenced at “top level” for any composite glyph.
	u16         max_component_depth;      // maxComponentDepth: Maximum levels of recursion; 1 for simple components.
} tg_open_type__maxp__version_10;

typedef struct tg_open_type__maxp__version_05
{
	tg_fixed    version;    // version: 0x00005000 for version 0.5 (Note the difference in the representation of a non-zero fractional part, in Fixed numbers.)
	u16         num_glyphs; // numGlyphs: The number of glyphs in the font.
} tg_open_type__maxp__version_05;

typedef union tg_open_type__maxp
{
	tg_open_type__maxp__version_05    version_05;
	tg_open_type__maxp__version_10    version_10;
} tg_open_type__maxp;

#endif
