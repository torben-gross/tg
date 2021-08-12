#include "graphics/vulkan/tgvk_core.h"

#ifdef TG_VULKAN

#include "math/tg_math.h"
#include "tg_rtvx_terrain.h"



tg_rtvx_terrain_h tg_rtvx_terrain_create(void)
{
	tg_rtvx_terrain_h h_terrain = TG_MALLOC(sizeof(*h_terrain)); // TODO: not malloc

	// TODO: gen on GPU
	const tg_size staging_buffer_size = TG_RTVX_TERRAIN_VX_STRIDE * TG_RTVX_TERRAIN_VX_STRIDE * TG_RTVX_TERRAIN_VX_STRIDE * sizeof(i8);
	tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(staging_buffer_size);
	i8* p_it = p_staging_buffer->memory.p_mapped_device_memory;
	for (i32 z = 0; z < TG_RTVX_TERRAIN_VX_STRIDE; z++)
	{
		for (i32 y = 0; y < TG_RTVX_TERRAIN_VX_STRIDE; y++)
		{
			for (i32 x = 0; x < TG_RTVX_TERRAIN_VX_STRIDE; x++)
			{
				const f32 xf = (f32)x;
				const f32 yf = (f32)y;
				const f32 zf = (f32)z;

				const f32 n_hills0 = tgm_noise(xf * 0.008f, 0.0f, zf * 0.008f);
				const f32 n_hills1 = tgm_noise(xf * 0.2f, 0.0f, zf * 0.2f);
				const f32 n_hills = n_hills0 + 0.005f * n_hills1;

				const f32 s_caves = 0.06f;
				const f32 c_caves_x = s_caves * xf;
				const f32 c_caves_y = s_caves * yf;
				const f32 c_caves_z = s_caves * zf;
				const f32 unclamped_noise_caves = tgm_noise(c_caves_x, c_caves_y, c_caves_z);
				const f32 n_caves = tgm_f32_clamp(unclamped_noise_caves, -1.0f, 0.0f);

				const f32 n = n_hills;
				f32 noise = (n * 64.0f) - (f32)(y - 8.0f);
				noise += 10.0f * n_caves;
				const f32 noise_clamped = tgm_f32_clamp(noise, -1.0f, 1.0f);
				const f32 f0 = (noise_clamped + 1.0f) * 0.5f;
				const f32 f1 = 254.0f * f0;
				const i8 f2 = -(i8)(tgm_f32_round_to_i32(f1) - 127);

				*p_it++ = f2;
			}
		}
	}

	h_terrain->voxels = TGVK_IMAGE_3D_CREATE(TGVK_IMAGE_TYPE_STORAGE, TG_RTVX_TERRAIN_VX_STRIDE, TG_RTVX_TERRAIN_VX_STRIDE, TG_RTVX_TERRAIN_VX_STRIDE, VK_FORMAT_R8_SINT, TG_NULL);

	tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
	tgvk_cmd_transition_image_3d_layout(p_command_buffer, &h_terrain->voxels, TGVK_LAYOUT_UNDEFINED, TGVK_LAYOUT_TRANSFER_WRITE);
	tgvk_cmd_copy_buffer_to_image_3d(p_command_buffer, p_staging_buffer, &h_terrain->voxels);
	tgvk_cmd_transition_image_3d_layout(p_command_buffer, &h_terrain->voxels, TGVK_LAYOUT_TRANSFER_WRITE, TGVK_LAYOUT_SHADER_READ_C);
	tgvk_command_buffer_end_and_submit(p_command_buffer);

	tgvk_global_staging_buffer_release();

	return h_terrain;
}

void tg_rtvx_terrain_destroy(tg_rtvx_terrain_h h_terrain)
{
	TG_ASSERT(h_terrain);

	TG_FREE(h_terrain);
}

#endif
