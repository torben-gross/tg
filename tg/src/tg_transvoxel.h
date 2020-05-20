#ifndef TG_TRANSVOXEL_H
#define TG_TRANSVOXEL_H

#include "math/tg_math.h"
#include "tg_common.h"



#define TG_TRANSVOXEL_ISOLEVEL_AT(isolevels, x, y, z)                               ((isolevels).data[19 * 19 * ((i8)(z) + 1) + 19 * ((i8)(y) + 1) + ((i8)(x) + 1)])
#define TG_TRANSVOXEL_ISOLEVEL_AT_V3I(isolevels, v)                                 (TG_TRANSVOXEL_ISOLEVEL_AT(isolevels, (v).x, (v).y, (v).z))
#define TG_TRANSVOXEL_TRANSITION_FACES(x_neg, x_pos, y_neg, y_pos, z_neg, z_pos)    ((x_neg) << 0 | (x_pos) << 1 | (y_neg) << 2 | (y_pos) << 3 | (z_neg) << 4 | (z_pos) << 5)



typedef enum tg_transvoxel_face
{
	TG_TRANSVOXEL_FACE_X_NEG = (1 << 0),
	TG_TRANSVOXEL_FACE_X_POS = (1 << 1),
	TG_TRANSVOXEL_FACE_Y_NEG = (1 << 2),
	TG_TRANSVOXEL_FACE_Y_POS = (1 << 3),
	TG_TRANSVOXEL_FACE_Z_NEG = (1 << 4),
	TG_TRANSVOXEL_FACE_Z_POS = (1 << 5)
} tg_transvoxel_face;

typedef struct tg_transvoxel_isolevels
{
	i8    data[6859]; // 19^3
} tg_transvoxel_isolevels;

typedef struct tg_transvoxel_triangle
{
	v3    positions[3];
} tg_transvoxel_triangle;


// TODO: these currently do not scale transition cells, which results in straight edges in some locations
u32     tg_transvoxel_create_chunk(i32 x, i32 y, i32 z, const tg_transvoxel_isolevels* p_isolevels, u8 lod, u8 transition_faces, tg_transvoxel_triangle* p_triangles);

#endif
