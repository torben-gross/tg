#ifndef TG_NOISE_H
#define TG_NOISE_H

#include "tg_common.h"
#include "math/tg_math.h"

// Source: https://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf
// 
// r := Minimum distance between samples
// k := Limit of samples to choose before rejection (typically k = 30)
// 
// If '*p_buffer_point_count' is zero, the maximum number of tightly packed points with 'r' as
// their minimal distance is written. Otherwise, '*p_buffer_point_count' is filled with the actual
// number of disks and 'p_point_buffer' is filled with the disks themselves.
void tg_poisson_disk_sampling_2d(v2 extent, f32 r, u32 k, TG_INOUT u32* p_buffer_point_count, TG_OUT v2* p_point_buffer);
void tg_poisson_disk_sampling_3d(v3 extent, f32 r, u32 k, TG_INOUT u32* p_buffer_point_count, TG_OUT v3* p_point_buffer);

#endif
