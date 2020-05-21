#ifndef TG_TRANSVOXEL_H
#define TG_TRANSVOXEL_H

#include "graphics/tg_graphics.h"
#include "math/tg_math.h"
#include "tg_common.h"



#define TG_TRANSVOXEL_ISOLEVEL_AT(isolevels, x, y, z)                               ((isolevels).p_data[17 * 17 * (i8)(z) + 17 * (i8)(y) + (i8)(x)])
#define TG_TRANSVOXEL_ISOLEVEL_AT_V3I(isolevels, v)                                 (TG_TRANSVOXEL_ISOLEVEL_AT(isolevels, (v).x, (v).y, (v).z))
#define TG_TRANSVOXEL_TRANSITION_FACES(x_neg, x_pos, y_neg, y_pos, z_neg, z_pos)    ((x_neg) << 0 | (x_pos) << 1 | (y_neg) << 2 | (y_pos) << 3 | (z_neg) << 4 | (z_pos) << 5)
#define TG_TRANSVOXEL_MAX_TRIANGLES_PER_TRANSITION_CELL                             0xc
#define TG_TRANSVOXEL_MAX_TRIANGLES_PER_REGULAR_CELL                                0x5



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
	i8    p_data[4913]; // 17^3
} tg_transvoxel_isolevels;

typedef struct tg_transvoxel_triangle
{
	tg_vertex_3d    p_vertices[3];
} tg_transvoxel_triangle;


// TODO: these currently do not scale transition cells, which results in straight edges in some locations
u32     tg_transvoxel_create_chunk(i32 x, i32 y, i32 z, const tg_transvoxel_isolevels* p_isolevels, const tg_transvoxel_isolevels** pp_connecting_isolevels, u8 lod, u8 transition_faces, tg_transvoxel_triangle* p_triangles);

#endif
