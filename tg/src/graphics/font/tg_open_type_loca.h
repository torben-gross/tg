#ifndef TG_OPEN_TYPE_LOCA_H
#define TG_OPEN_TYPE_LOCA_H

#include "graphics/font/tg_open_type_types.h"

typedef struct tg_open_type__loca__long_version
{
	tg_offset32    p_offsets[0]; // offsets[n]: The actual local offset is stored. The value of n is numGlyphs + 1. The value for numGlyphs is found in the 'maxp' table.
} tg_open_type__loca__long_version;

typedef struct tg_open_type__loca__short_version
{
	tg_offset16    p_offsets[0]; // offsets[n]: The actual local offset divided by 2 is stored.The value of n is numGlyphs + 1. The value for numGlyphs is found in the 'maxp' table.
} tg_open_type__loca__short_version;

typedef union tg_open_type__loca
{
	tg_open_type__loca__short_version    short_version;
	tg_open_type__loca__long_version     long_version;
} tg_open_type__loca;

#endif
