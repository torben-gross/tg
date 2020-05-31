#ifndef TG_TRANSVOXEL_H
#define TG_TRANSVOXEL_H

#include "graphics/tg_graphics.h"
#include "math/tg_math.h"
#include "tg_common.h"



#define TG_TRANSVOXEL_VERTICES 17
#define TG_TRANSVOXEL_CELLS (TG_TRANSVOXEL_VERTICES - 1)

#define TG_TRANSVOXEL_ISOLEVEL_AT(isolevels, x, y, z) ((isolevels).p_data[TG_TRANSVOXEL_VERTICES * TG_TRANSVOXEL_VERTICES * (i8)(z) + TG_TRANSVOXEL_VERTICES * (i8)(y) + (i8)(x)])
#define TG_TRANSVOXEL_ISOLEVEL_AT_V3I(isolevels, v) (TG_TRANSVOXEL_ISOLEVEL_AT(isolevels, (v).x, (v).y, (v).z))
#define TG_TRANSVOXEL_TRANSITION_FACES(x_neg, x_pos, y_neg, y_pos, z_neg, z_pos) ((x_neg) << 0 | (x_pos) << 1 | (y_neg) << 2 | (y_pos) << 3 | (z_neg) << 4 | (z_pos) << 5)

#define TG_TRANSVOXEL_REGULAR_CELL_MAX_TRIANGLES 0x5
#define TG_TRANSVOXEL_CHUNK_REGULAR_CELL_COUNT (TG_TRANSVOXEL_CELLS * TG_TRANSVOXEL_CELLS * TG_TRANSVOXEL_CELLS)
#define TG_TRANSVOXEL_CHUNK_REGULAR_CELLS_MAX_TRIANGLES (TG_TRANSVOXEL_CHUNK_REGULAR_CELL_COUNT * TG_TRANSVOXEL_REGULAR_CELL_MAX_TRIANGLES)

#define TG_TRANSVOXEL_TRANSITION_CELL_MAX_TRIANGLES 0xc
#define TG_TRANSVOXEL_TRANSITION_X_CELL_COUNT (TG_TRANSVOXEL_CELLS * TG_TRANSVOXEL_CELLS)
#define TG_TRANSVOXEL_TRANSITION_Y_CELL_COUNT (TG_TRANSVOXEL_CELLS * TG_TRANSVOXEL_CELLS)
#define TG_TRANSVOXEL_TRANSITION_Z_CELL_COUNT (TG_TRANSVOXEL_CELLS * TG_TRANSVOXEL_CELLS)
#define TG_TRANSVOXEL_TRANSITIONS_CELL_COUNT (2 * (TG_TRANSVOXEL_TRANSITION_X_CELL_COUNT + TG_TRANSVOXEL_TRANSITION_Y_CELL_COUNT + TG_TRANSVOXEL_TRANSITION_Z_CELL_COUNT))
#define TG_TRANSVOXEL_TRANSITIONS_MAX_TRIANGLES (TG_TRANSVOXEL_TRANSITIONS_CELL_COUNT * TG_TRANSVOXEL_TRANSITION_CELL_MAX_TRIANGLES)

#define TG_TRANSVOXEL_CHUNK_MAX_TRIANGLES (TG_TRANSVOXEL_CHUNK_REGULAR_CELLS_MAX_TRIANGLES + TG_TRANSVOXEL_TRANSITIONS_MAX_TRIANGLES)

#define TG_TRANSVOXEL_REGULAR_CELL_AT(chunk, cx, cy, cz) ((chunk).p_regular_cells[(TG_TRANSVOXEL_CELLS / (1 << (chunk).lod)) * (TG_TRANSVOXEL_CELLS / (1 << (chunk).lod)) * (cz) + (TG_TRANSVOXEL_CELLS / (1 << (chunk).lod)) * (cy) + (cx)])



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
	i8    p_data[TG_TRANSVOXEL_VERTICES * TG_TRANSVOXEL_VERTICES * TG_TRANSVOXEL_VERTICES];
} tg_transvoxel_isolevels;

typedef struct tg_transvoxel_triangle
{
	tg_vertex    p_vertices[3];
} tg_transvoxel_triangle;

typedef struct tg_transvoxel_regular_cell
{
	u32                        triangle_count;
	tg_transvoxel_triangle*    p_triangles;
} tg_transvoxel_regular_cell;

typedef struct tg_transvoxel_transition_cell
{
	u16                        low_res_vertex_bitmap;
	u32                        triangle_count;
	tg_transvoxel_triangle*    p_triangles;
} tg_transvoxel_transition_cell;

typedef struct tg_transvoxel_chunk
{
	v3i                              coordinates;
	u8                               lod;
	tg_transvoxel_isolevels          isolevels;
	u8                               transition_faces_bitmap;
	u32                              triangle_count;
	tg_transvoxel_triangle*          p_triangles;

	tg_transvoxel_regular_cell       p_regular_cells[TG_TRANSVOXEL_CHUNK_REGULAR_CELL_COUNT];

	tg_transvoxel_transition_cell    p_transition_cells_x_neg[TG_TRANSVOXEL_TRANSITION_X_CELL_COUNT];
	tg_transvoxel_transition_cell    p_transition_cells_x_pos[TG_TRANSVOXEL_TRANSITION_X_CELL_COUNT];
	tg_transvoxel_transition_cell    p_transition_cells_y_neg[TG_TRANSVOXEL_TRANSITION_Y_CELL_COUNT];
	tg_transvoxel_transition_cell    p_transition_cells_y_pos[TG_TRANSVOXEL_TRANSITION_Y_CELL_COUNT];
	tg_transvoxel_transition_cell    p_transition_cells_z_neg[TG_TRANSVOXEL_TRANSITION_Z_CELL_COUNT];
	tg_transvoxel_transition_cell    p_transition_cells_z_pos[TG_TRANSVOXEL_TRANSITION_Z_CELL_COUNT];
} tg_transvoxel_chunk;

typedef struct tg_transvoxel_connecting_chunks
{
	union
	{
		tg_transvoxel_chunk*            pp_chunks[27];
		struct
		{
			tg_transvoxel_chunk*        p_xneg_yneg_zneg;
			tg_transvoxel_chunk*        p_xneu_yneg_zneg;
			tg_transvoxel_chunk*        p_xpos_yneg_zneg;
			tg_transvoxel_chunk*        p_xneg_yneu_zneg;
			tg_transvoxel_chunk*        p_xneu_yneu_zneg;
			tg_transvoxel_chunk*        p_xpos_yneu_zneg;
			tg_transvoxel_chunk*        p_xneg_ypos_zneg;
			tg_transvoxel_chunk*        p_xneu_ypos_zneg;
			tg_transvoxel_chunk*        p_xpos_ypos_zneg;
			
			tg_transvoxel_chunk*        p_xneg_yneg_zneu;
			tg_transvoxel_chunk*        p_xneu_yneg_zneu;
			tg_transvoxel_chunk*        p_xpos_yneg_zneu;
			tg_transvoxel_chunk*        p_xneg_yneu_zneu;

			union
			{
				tg_transvoxel_chunk*    p_xneu_yneu_zneu;
				tg_transvoxel_chunk*    p_central_chunk;
			};

			tg_transvoxel_chunk*        p_xpos_yneu_zneu;
			tg_transvoxel_chunk*        p_xneg_ypos_zneu;
			tg_transvoxel_chunk*        p_xneu_ypos_zneu;
			tg_transvoxel_chunk*        p_xpos_ypos_zneu;
			
			tg_transvoxel_chunk*        p_xneg_yneg_zpos;
			tg_transvoxel_chunk*        p_xneu_yneg_zpos;
			tg_transvoxel_chunk*        p_xpos_yneg_zpos;
			tg_transvoxel_chunk*        p_xneg_yneu_zpos;
			tg_transvoxel_chunk*        p_xneu_yneu_zpos;
			tg_transvoxel_chunk*        p_xpos_yneu_zpos;
			tg_transvoxel_chunk*        p_xneg_ypos_zpos;
			tg_transvoxel_chunk*        p_xneu_ypos_zpos;
			tg_transvoxel_chunk*        p_xpos_ypos_zpos;
		};
	};
} tg_transvoxel_connecting_chunks;



void     tg_transvoxel_fill_chunk(tg_transvoxel_chunk* p_chunk);
void     tg_transvoxel_fill_transitions(tg_transvoxel_connecting_chunks* p_connecting_chunks);

#endif
