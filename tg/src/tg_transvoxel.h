#ifndef TG_TRANSVOXEL_H
#define TG_TRANSVOXEL_H

#include "math/tg_math.h"
#include "tg_common.h"



#define TG_TRANSVOXEL_ISOLEVEL_AT(isolevels, x, y, z)                               ((isolevels).data[19 * 19 * ((z) + 1) + 19 * ((y) + 1) + ((x) + 1)])
#define TG_TRANSVOXEL_TRANSITION_FACES(x_neg, x_pos, y_neg, y_pos, z_neg, z_pos)    ((x_neg) << 0 | (x_pos) << 1 | (y_neg) << 2 | (y_pos) << 3 | (z_neg) << 4 | (z_pos) << 5)



typedef struct tg_transvoxel_isolevels
{
	i8    data[6859]; // 19^3
} tg_transvoxel_isolevels;

typedef struct tg_transvoxel_triangle
{
	v3    positions[3];
} tg_transvoxel_triangle;



u32     tg_transvoxel_create_chunk(i32 x, i32 y, i32 z, const tg_transvoxel_isolevels* p_isolevels, u8 lod, u8 transition_faces, tg_transvoxel_triangle* p_triangles);
void    tg_transvoxel_fill_isolevels(i32 x, i32 y, i32 z, tg_transvoxel_isolevels* p_isolevels);

#endif
