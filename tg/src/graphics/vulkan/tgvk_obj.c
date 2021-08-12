#include "graphics/vulkan/tgvk_core.h"

#ifdef TG_VULKAN

tg_obj_h tg_obj_create(u32 log2_w, u32 log2_h, u32 log2_d)
{
	TG_ASSERT(log2_w <= 32);
	TG_ASSERT(log2_h <= 32);
	TG_ASSERT(log2_d <= 32);

	tg_obj_h h_obj = tgvk_handle_take(TG_STRUCTURE_TYPE_OBJ);

	const tg_obj* p_obj_arr = tgvk_handle_array(TG_STRUCTURE_TYPE_OBJ);
	if (p_obj_arr == h_obj)
	{
		h_obj->first_voxel_id = 0;
	}
	else
	{
		const tg_obj* p_prev_obj = h_obj - 1;
		h_obj->first_voxel_id = p_prev_obj->first_voxel_id;
		const u32 mask  = (1 << 5) - 1;
		const u32 prev_log2_w =  p_prev_obj->packed_log2_whd        & mask;
		const u32 prev_log2_h = (p_prev_obj->packed_log2_whd >>  5) & mask;
		const u32 prev_log2_d = (p_prev_obj->packed_log2_whd >> 10) & mask;
		const u32 prev_w = 1 << prev_log2_w;
		const u32 prev_h = 1 << prev_log2_h;
		const u32 prev_d = 1 << prev_log2_d;
		const u32 prev_num_voxels = prev_w * prev_h * prev_d;
		h_obj->first_voxel_id += prev_num_voxels;
	}

	h_obj->ubo = TGVK_UNIFORM_BUFFER_CREATE(sizeof(u32));
	*((u32*)h_obj->ubo.memory.p_mapped_device_memory) = h_obj->first_voxel_id;

	h_obj->descriptor_set = tgvk_descriptor_set_create(&shared_render_resources.ray_tracer.vis.pipeline);
	tgvk_descriptor_set_update_uniform_buffer(h_obj->descriptor_set.descriptor_set, &h_obj->ubo, 0);

	u32 packed = log2_w;
	packed = packed | (log2_h << 5);
	packed = packed | (log2_d << 10);
	h_obj->packed_log2_whd = (u16)packed;

	// TODO: gen on GPU
	const u32 w = 1 << log2_w;
	const u32 h = 1 << log2_h;
	const u32 d = 1 << log2_d;
	const tg_size buffer_size = ((tg_size)w * (tg_size)h * (tg_size)d) / 8;
	tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(buffer_size);
	u8* p_it = p_staging_buffer->memory.p_mapped_device_memory;
	u32 solid_bits = 0;
	u32 bit_idx = 0;

	for (u32 z = 0; z < d; z++)
	{
		for (u32 y = 0; y < h; y++)
		{
			for (u32 x = 0; x < w; x++)
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
				const u32 solid = f2 <= 0;

				solid_bits |= solid << bit_idx;
				bit_idx++;

				// TODO: space filling z curve
				if (bit_idx == 8)
				{
					*p_it++ = (u8)solid_bits;
					bit_idx = 0;
					solid_bits = 0;
				}
			}
		}
	}

	// TODO: lods
	h_obj->voxels = TGVK_BUFFER_CREATE(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, TGVK_MEMORY_DEVICE);
	tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
	tgvk_cmd_copy_buffer(p_command_buffer, buffer_size, p_staging_buffer, &h_obj->voxels);
	tgvk_command_buffer_end_and_submit(p_command_buffer);
	tgvk_global_staging_buffer_release();

	return h_obj;
}

void tg_obj_destroy(tg_obj_h h_obj)
{
	TG_ASSERT(h_obj);

	TG_NOT_IMPLEMENTED();
}

#endif