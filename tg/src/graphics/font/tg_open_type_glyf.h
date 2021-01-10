#ifndef TG_OPEN_TYPE_GLYF_H
#define TG_OPEN_TYPE_GLYF_H

#include "graphics/font/tg_open_type_types.h"

typedef enum tg_open_type__composite_glyph__flags
{
	TG_OPEN_TYPE__COMPOSITE_GLYPH__ARG_1_AND_2_ARE_WORDS        = 0x0001, // If this is set, the arguments are 16-bit (uint16 or int16); otherwise, they are bytes (uint8 or int8).
	TG_OPEN_TYPE__COMPOSITE_GLYPH__ARGS_ARE_XY_VALUES           = 0x0002, // If this is set, the arguments are signed xy values; otherwise, they are unsigned point numbers.
	TG_OPEN_TYPE__COMPOSITE_GLYPH__ROUND_XY_TO_GRID             = 0x0004, // For the xy values if the preceding is true.
	TG_OPEN_TYPE__COMPOSITE_GLYPH__WE_HAVE_A_SCALE              = 0x0008, // This indicates that there is a simple scale for the component. Otherwise, scale = 1.0.
	TG_OPEN_TYPE__COMPOSITE_GLYPH__MORE_COMPONENTS              = 0x0020, // Indicates at least one more glyph after this one.
	TG_OPEN_TYPE__COMPOSITE_GLYPH__WE_HAVE_AN_X_AND_Y_SCALE     = 0x0040, // The x direction will use a different scale from the y direction.
	TG_OPEN_TYPE__COMPOSITE_GLYPH__WE_HAVE_A_TWO_BY_TWO         = 0x0080, // There is a 2 by 2 transformation that will be used to scale the component.
	TG_OPEN_TYPE__COMPOSITE_GLYPH__WE_HAVE_INSTRUCTIONS         = 0x0100, // Following the last component are instructions for the composite character.
	TG_OPEN_TYPE__COMPOSITE_GLYPH__USE_MY_METRICS               = 0x0200, // If set, this forces the aw and lsb (and rsb) for the composite to be equal to those from this original glyph. This works for hinted and unhinted characters.
	TG_OPEN_TYPE__COMPOSITE_GLYPH__OVERLAP_COMPOUND             = 0x0400, // If set, the components of the compound glyph overlap. Use of this flag is not required in OpenType — that is, it is valid to have components overlap without having this flag set. It may affect behaviors in some platforms, however. (See Apple’s specification for details regarding behavior in Apple platforms.) When used, it must be set on the flag word for the first component. See additional remarks, above, for the similar OVERLAP_SIMPLE flag used in simple-glyph descriptions.
	TG_OPEN_TYPE__COMPOSITE_GLYPH__SCALED_COMPONENT_OFFSET      = 0x0800, // The composite is designed to have the component offset scaled.
	TG_OPEN_TYPE__COMPOSITE_GLYPH__UNSCALED_COMPONENT_OFFSET    = 0x1000, // The composite is designed not to have the component offset scaled.
	TG_OPEN_TYPE__COMPOSITE_GLYPH__Reserved                     = 0xe010  // Bits 4, 13, 14 and 15 are reserved: set to 0.
} tg_open_type__composite_glyph__flags;

typedef enum tg_open_type__simple_glyph__flags
{
	TG_OPEN_TYPE__SIMPLE_GLYPH__ON_CURVE_POINT                          = 0x01, // ON_CURVE_POINT: Bit 0: If set, the point is on the curve; otherwise, it is off the curve.
	TG_OPEN_TYPE__SIMPLE_GLYPH__X_SHORT_VECTOR                          = 0x02, // X_SHORT_VECTOR: Bit 1: If set, the corresponding x-coordinate is 1 byte long. If not set, it is two bytes long. For the sign of this value, see the description of the X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR flag.
	TG_OPEN_TYPE__SIMPLE_GLYPH__Y_SHORT_VECTOR                          = 0x04, // Y_SHORT_VECTOR: Bit 2: If set, the corresponding y-coordinate is 1 byte long. If not set, it is two bytes long. For the sign of this value, see the description of the Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR flag.
	TG_OPEN_TYPE__SIMPLE_GLYPH__REPEAT_FLAG                             = 0x08, // REPEAT_FLAG: Bit 3: If set, the next byte (read as unsigned) specifies the number of additional times this flag byte is to be repeated in the logical flags array — that is, the number of additional logical flag entries inserted after this entry. (In the expanded logical array, this bit is ignored.) In this way, the number of flags listed can be smaller than the number of points in the glyph description.
	TG_OPEN_TYPE__SIMPLE_GLYPH__X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR    = 0x10, // X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR: Bit 4: This flag has two meanings, depending on how the X_SHORT_VECTOR flag is set. If X_SHORT_VECTOR is set, this bit describes the sign of the value, with 1 equalling positive and 0 negative. If X_SHORT_VECTOR is not set and this bit is set, then the current x-coordinate is the same as the previous x-coordinate. If X_SHORT_VECTOR is not set and this bit is also not set, the current x-coordinate is a signed 16-bit delta vector.
	TG_OPEN_TYPE__SIMPLE_GLYPH__Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR    = 0x20, // Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR: Bit 5: This flag has two meanings, depending on how the Y_SHORT_VECTOR flag is set. If Y_SHORT_VECTOR is set, this bit describes the sign of the value, with 1 equalling positive and 0 negative. If Y_SHORT_VECTOR is not set and this bit is set, then the current y-coordinate is the same as the previous y-coordinate. If Y_SHORT_VECTOR is not set and this bit is also not set, the current y-coordinate is a signed 16-bit delta vector.
	TG_OPEN_TYPE__SIMPLE_GLYPH__OVERLAP_SIMPLE                          = 0x40, // OVERLAP_SIMPLE: Bit 6: If set, contours in the glyph description may overlap. Use of this flag is not required in OpenType — that is, it is valid to have contours overlap without having this flag set. It may affect behaviors in some platforms, however. (See the discussion of “Overlapping contours” in Apple’s specification for details regarding behavior in Apple platforms.) When used, it must be set on the first flag byte for the glyph. See additional details below.
	TG_OPEN_TYPE__SIMPLE_GLYPH__Reserved                                = 0x80  // Reserved: Bit 7 is reserved: set to zero.
} tg_open_type__simple_glyph__flags;

typedef struct tg_open_type__glyf
{
	i16    number_of_contours; // numberOfContours: If the number of contours is greater than or equal to zero, this is a simple glyph. If negative, this is a composite glyph — the value -1 should be used for composite glyphs.
	i16    x_min;              // xMin: Minimum x for coordinate data.
	i16    y_min;              // yMin: Minimum y for coordinate data.
	i16    x_max;              // xMax: Maximum x for coordinate data.
	i16    y_max;              // yMax: Maximum y for coordinate data.
} tg_open_type__glyf;

#endif
