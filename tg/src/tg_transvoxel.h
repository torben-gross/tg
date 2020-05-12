#ifndef TG_TRANSVOXEL_H
#define TG_TRANSVOXEL_H

#include "math/tg_math.h"
#include "tg_common.h"

#define TG_ISOLEVEL_AT(isolevels, x, y, z)        ((isolevels).data[19 * 19 * ((z) + 1) + 19 * ((y) + 1) + ((x) + 1)])

typedef enum tg_transvoxel_face
{
	TG_TRANSVOXEL_FACE_X_NEG,
	TG_TRANSVOXEL_FACE_X_POS,
	TG_TRANSVOXEL_FACE_Y_NEG,
	TG_TRANSVOXEL_FACE_Y_POS,
	TG_TRANSVOXEL_FACE_Z_NEG,
	TG_TRANSVOXEL_FACE_Z_POS
} tg_transvoxel_face;

typedef struct tg_transvoxel_isolevels
{
	i8    data[6859];
} tg_transvoxel_isolevels;

typedef struct tg_transvoxel_triangle
{
	v3    positions[3];
} tg_transvoxel_triangle;

u32     tg_transvoxel_create_chunk(const tg_transvoxel_isolevels* p_isolevels, i32 x, i32 y, i32 z, u8 lod, tg_transvoxel_triangle* p_triangles);
u32     tg_transvoxel_create_transition_face(const tg_transvoxel_isolevels* p_isolevels, i32 x, i32 y, i32 z, tg_transvoxel_face face, tg_transvoxel_triangle* p_triangles);
void    tg_transvoxel_fill_isolevels(tg_transvoxel_isolevels* p_isolevels, i32 x, i32 y, i32 z);

#endif
