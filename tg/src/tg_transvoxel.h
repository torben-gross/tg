#ifndef TG_TRANSVOXEL_H
#define TG_TRANSVOXEL_H

#include "math/tg_math.h"
#include "tg_common.h"

#define TG_ISOLEVEL_AT(isolevels, x, y, z)        ((isolevels).data[19 * 19* (z) + 19 * (y) + (x)])

typedef struct tg_transvoxel_isolevels
{
	i8    data[6859];
} tg_transvoxel_isolevels;

typedef struct tg_transvoxel_triangle
{
	v3    positions[3];
} tg_transvoxel_triangle;

u32     tg_transvoxel_create_chunk(const tg_transvoxel_isolevels* p_isolevels, i32 x, i32 y, i32 z, tg_transvoxel_triangle* p_triangles);
void    tg_transvoxel_fill_isolevels(tg_transvoxel_isolevels* p_isolevels, i32 x, i32 y, i32 z);

#endif
