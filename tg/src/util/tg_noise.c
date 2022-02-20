#include "util/tg_noise.h"

#include "memory/tg_memory.h"

void tg_poisson_disk_sampling_2d(v2 extent, f32 r, u32 k, TG_INOUT u32* p_buffer_point_count, TG_OUT v2* p_point_buffer)
{
	TG_ASSERT(extent.x >= 0.0f);
	TG_ASSERT(extent.y >= 0.0f);
	TG_ASSERT(r > 0.0f);
	TG_ASSERT(k > 0);

	if (*p_buffer_point_count == 0)
	{
		TG_ASSERT(p_point_buffer == TG_NULL);

		// The optimal pattern to fill a rectangle with disks is to align them in an equilateral
		// (60° angles) triangular pattern. The centers of two points of the triangles are either
		// aligned horizontally or vertically in the optimal solution. To inspect both solutions,
		// the width and height are simply swapped.
		// 
		// Let three disks be aligned in a triangle, such that two disks touch horizontally and the
		// third touches both below in the middle. Then, as defined in the paper, the distance
		// between all their centers is r. Therefore, we can compute the distance 'a' of the
		// centers between the disks of the first row and the disk of the second row projected onto
		// the y-axis by:
		// 
		// r^2 = (r/2)^2 + a^2
		// 
		// a^2 = r^2 - (r/2)^2
		//     = r^2 - (1/4)r^2
		//     = (3/4)r^2
		// 
		// a   = sqrt((3/4)r^2)
		//     = sqrt(3/4)sqrt(r^2)
		//     = sqrt(3/4)r
		// 
		// Doubling that distance gives the required offset, until the first of disks can be
		// repeated again (same offset for second row):
		//
		// 2a  = 2sqrt(3/4)r
		//     = 2sqrt(3)sqrt(1/4)r
		//     = 2sqrt(3)0.5r
		//     = sqrt(3)r
		// 
		// This offset may also be used for the 90° rotated disk setup.

		const f32 y_offset      = 1.73205080757f * r; // sqrt(3)r
		const f32 half_y_offset = 0.5f * y_offset;
		const f32 half_r        = 0.5f * r;

		// We simply repeat the procedure for switched width and height to compute all results
		for (u32 i = 0; i < 2; i++)
		{
			const v2 e = {
				extent.p_data[i],
				extent.p_data[(i + 1) & 1]
			};

			const u32 fst_row_n_points_per_rep = 1 + (u32)((e.x + TG_F32_EPSILON) / r);
			u32 snd_row_n_points_per_rep = 0;
			// Note: Without this if-statement, we're going negative (same below). Alternative, we
			// could use tg_f32_floor, which rounds towards zero instead.
			if (half_r <= e.x)
			{
				snd_row_n_points_per_rep = 1 + (u32)((e.x - half_r + TG_F32_EPSILON) / r);
			}

			const u32 fst_row_n_reps = 1 + (u32)((e.y + TG_F32_EPSILON) / y_offset);
			u32 snd_row_n_reps = 0;
			if (half_y_offset <= e.y)
			{
				snd_row_n_reps = 1 + (u32)((e.y - half_y_offset + TG_F32_EPSILON) / y_offset);
			}

			const u32 fst_row_n_disks = fst_row_n_points_per_rep * fst_row_n_reps;
			const u32 snd_row_n_disks = snd_row_n_points_per_rep * snd_row_n_reps;
			const u32 num_disks = fst_row_n_disks + snd_row_n_disks;

			*p_buffer_point_count = TG_MAX(*p_buffer_point_count, num_disks);

			if (e.x == e.y)
			{
				break;
			}
		}
	}
	else
	{
		TG_ASSERT(p_point_buffer != TG_NULL);

		const f32 d  = r + r;
		const f32 rr = r * r;
		const u32 point_buffer_capacity = *p_buffer_point_count;
		*p_buffer_point_count = 0;

		// Step 0.  Initialize an n-dimensional background grid for storing samples and
		// accelerating spatial searches. We pick the cell size to be bounded by r / sqrt(n), so
		// that each grid cell will contain at most one sample, and thus the grid can be
		// implemented as a simple n-dimensional array of integers: the default -1 indicates no
		// sample, a non-negative integer gives the index of the sample located in a cell.

		const f32 sqrt_n = 1.41421356237f; // sqrt(n) (Square root of dimension)
		const f32 cell_size = (r - TG_F32_EPSILON) / sqrt_n;
		const u32 n_cells_x = (u32)tgm_f32_ceil((extent.x + TG_F32_EPSILON) / cell_size);
		const u32 n_cells_y = (u32)tgm_f32_ceil((extent.y + TG_F32_EPSILON) / cell_size);
		const u32 n_cells = n_cells_x * n_cells_y;

		i32* p_background_grid;
		const tg_size background_grid_size = (tg_size)n_cells_x * (tg_size)n_cells_y * sizeof(*p_background_grid);
		p_background_grid = TG_MALLOC_STACK(background_grid_size);

		for (u32 i = 0; i < n_cells; i++)
		{
			p_background_grid[i] = -1;
		}

		// Step 1.  Select the initial sample, x0, randomly chosen uniformly from the domain.
		// Insert it into the background grid, and initialize the "active list" (an array of sample
		// indices) with this index (zero).

		i32* p_active_list;
		const tg_size active_list_size = point_buffer_capacity * sizeof(*p_active_list);
		p_active_list = TG_MALLOC_STACK(active_list_size);
		u32 active_list_n = 0;

		tg_rand_xorshift32 rand;
		tgm_rand_xorshift32_init(13931995, &rand);
		
		f32 x0_x = tgm_rand_xorshift32_next_f32(&rand) * extent.x;
		f32 x0_y = tgm_rand_xorshift32_next_f32(&rand) * extent.y;

		if (x0_x == extent.x) x0_x = 0.0f;
		if (x0_y == extent.y) x0_y = 0.0f;

		u32 x0_x_bggrid = (u32)(x0_x / cell_size);
		u32 x0_y_bggrid = (u32)(x0_y / cell_size);

		if (x0_x_bggrid == (f32)n_cells_x) x0_x_bggrid = 0;
		if (x0_y_bggrid == (f32)n_cells_y) x0_y_bggrid = 0;

		const u32 x0_bggrid_idx = n_cells_x * x0_y_bggrid + x0_x_bggrid;
		
		TG_ASSERT(active_list_n < point_buffer_capacity);
		p_active_list[active_list_n++] = x0_bggrid_idx;

		TG_ASSERT(x0_bggrid_idx < n_cells);
		p_background_grid[x0_bggrid_idx] = *p_buffer_point_count;

		TG_ASSERT(*p_buffer_point_count == 0);
		p_point_buffer[(*p_buffer_point_count)++] = (v2){ x0_x, x0_y };

		// Step 2.  While the active list is not empty, choose a random index from it (say i).
		// Generate up to k points chosen uniformly from the spherical annulus between radius r and
		// 2r around xi. For each point in turn, check if it is within distance r of existing
		// samples (using the background grid to only test nearby samples). If a point is
		// adequately far from existing samples, emit it as the next sample and add it to the
		// active list. If after k attempts no such point is found, instead remove i from the
		// active list.

		while (active_list_n > 0)
		{
			const u32 i = (u32)(tgm_rand_xorshift32_next_f32(&rand) * (f32)(active_list_n - 1));

			TG_ASSERT(i < active_list_n);
			const u32 xi_bggrid_idx = p_active_list[i];

			TG_ASSERT(xi_bggrid_idx < n_cells);
			const u32 xi_pntbuf_idx = p_background_grid[xi_bggrid_idx];

			TG_ASSERT(xi_pntbuf_idx < *p_buffer_point_count);
			const v2 xi = p_point_buffer[xi_pntbuf_idx];

			b32 point_found = TG_FALSE;
			for (u32 k_idx = 0; k_idx < k; k_idx++)
			{
				const f32 rotation = tgm_rand_xorshift32_next_f32(&rand) * TG_TAU;
				const f32 x_dir = tgm_f32_cos(rotation);
				const f32 y_dir = tgm_f32_sin(rotation);
				const f32 distance = tgm_rand_xorshift32_next_f32_inclusive_range(&rand, r, d);

				f32 xk_x = xi.x + distance * x_dir;
				f32 xk_y = xi.y + distance * y_dir;

				if (xk_x < 0.0f)           xk_x += extent.x;
				else if (xk_x >= extent.x) xk_x -= extent.x;
				TG_ASSERT(xk_x >= 0.0f && xk_x < extent.x);

				if (xk_y < 0.0f)           xk_y += extent.y;
				else if (xk_y >= extent.y) xk_y -= extent.y;
				TG_ASSERT(xk_y >= 0.0f && xk_y < extent.y);
				
				const u32 xk_x_bggrid = (u32)(xk_x / cell_size);
				const u32 xk_y_bggrid = (u32)(xk_y / cell_size);
				const u32 xk_bggrid_idx = n_cells_x * xk_y_bggrid + xk_x_bggrid;

				const f32 min_x = xk_x - r;
				const f32 min_y = xk_y - r;
				const f32 max_x = xk_x + r;
				const f32 max_y = xk_y + r;

				const i32 min_x_bggrid = (i32)tgm_f32_floor(min_x / cell_size);
				const i32 min_y_bggrid = (i32)tgm_f32_floor(min_y / cell_size);
				const i32 max_x_bggrid = (i32)tgm_f32_floor(max_x / cell_size);
				const i32 max_y_bggrid = (i32)tgm_f32_floor(max_y / cell_size);

				b32 adequately_far = TG_TRUE;
				for (i32 y_bggrid = min_y_bggrid; y_bggrid <= max_y_bggrid; y_bggrid++)
				{
					i32 y = y_bggrid;
					if (y < 0)                    y += n_cells_y;
					else if (y >= (i32)n_cells_y) y -= n_cells_y;
					TG_ASSERT(y >= 0 && y < (i32)n_cells_y);

					for (i32 x_bggrid = min_x_bggrid; x_bggrid <= max_x_bggrid; x_bggrid++)
					{
						i32 x = x_bggrid;
						if (x < 0)                    x += n_cells_x;
						else if (x >= (i32)n_cells_x) x -= n_cells_x;
						TG_ASSERT(x >= 0 && x < (i32)n_cells_x);

						const u32 bggrid_idx = n_cells_x * (u32)y + (u32)x;

						TG_ASSERT(bggrid_idx < n_cells);
						const i32 pntbuf_idx = p_background_grid[bggrid_idx];

						// Note: We can not uncomment this, because we may produce a point outside
						// of the bounds, which then is shifted to the other side. Then, it can be
						// too close to the original point.
						if (pntbuf_idx != -1/* && bggrid_idx != xi_bggrid_idx*/)
						{
							TG_ASSERT((u32)pntbuf_idx < *p_buffer_point_count);
							const v2 unshifted = p_point_buffer[pntbuf_idx];

							for (i32 y_shift_dir = -1; y_shift_dir <= 1; y_shift_dir++)
							{
								const f32 y_shift = (f32)y_shift_dir * extent.y;

								for (i32 x_shift_dir = -1; x_shift_dir <= 1; x_shift_dir++)
								{
									const f32 x_shift = (f32)x_shift_dir * extent.x;
									const f32 shifted_x = unshifted.x + x_shift;
									const f32 shifted_y = unshifted.y + y_shift;
									const f32 xk2xj_x = shifted_x - xk_x;
									const f32 xk2xj_y = shifted_y - xk_y;
									const f32 magsqr = xk2xj_x * xk2xj_x + xk2xj_y * xk2xj_y;

									if (magsqr < rr)
									{
										adequately_far = TG_FALSE;
										break;
									}
								}
								if (!adequately_far) break;
							}
							if (!adequately_far) break;
						}
					}
					if (!adequately_far) break;
				}

				if (adequately_far)
				{
					point_found = TG_TRUE;

					TG_ASSERT(active_list_n < point_buffer_capacity);
					p_active_list[active_list_n++] = xk_bggrid_idx;

					TG_ASSERT(xk_bggrid_idx < n_cells);
					TG_ASSERT(p_background_grid[xk_bggrid_idx] == -1);
					p_background_grid[xk_bggrid_idx] = *p_buffer_point_count;

					TG_ASSERT(*p_buffer_point_count < point_buffer_capacity);
					p_point_buffer[(*p_buffer_point_count)++] = (v2){ xk_x, xk_y };
				}
			}

			if (!point_found)
			{
				TG_ASSERT(i < active_list_n);
				TG_ASSERT(active_list_n > 0);
				p_active_list[i] = p_active_list[active_list_n - 1];
				active_list_n--;
			}
		}

		TG_FREE_STACK(active_list_size);
		TG_FREE_STACK(background_grid_size);
	}
}

void tg_poisson_disk_sampling_3d(v3 extent, f32 r, u32 k, TG_INOUT u32* p_buffer_point_count, TG_OUT v3* p_point_buffer)
{
	TG_ASSERT(extent.x > 0.0f);
	TG_ASSERT(extent.y > 0.0f);
	TG_ASSERT(extent.z > 0.0f);
	TG_ASSERT(r > 0.0f);
	TG_ASSERT(k > 0);

	TG_UNUSED(p_buffer_point_count);
	TG_UNUSED(p_point_buffer);
	TG_INVALID_CODEPATH();
}
