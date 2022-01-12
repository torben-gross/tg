#include "graphics/vulkan/tgvk_core.h"

#ifdef TG_VULKAN

typedef struct tg_material_ubo
{
    v4     albedo;
    f32    metallic;
    f32    roughness;
} tg_material_ubo; // TODO: this also exists in tgvk_ray_tracer.c

tg_ray_trace_command_h tg_ray_trace_command_create(tg_material_h h_material, v3 position, tg_storage_image_3d_h h_voxels)
{
    TG_ASSERT(shared_render_resources.ray_tracer.initialized);

    // TODO: use material
    TG_ASSERT(h_voxels);

	tg_ray_trace_command_h h_command = tgvk_handle_take(TG_STRUCTURE_TYPE_RAY_TRACE_COMMAND);

    //h_command->descriptor_set = tgvk_descriptor_set_create(&shared_render_resources.ray_tracer.graphics_pipeline);
    h_command->model_ubo = TGVK_UNIFORM_BUFFER_CREATE(sizeof(m4));
    h_command->material_ubo = TGVK_UNIFORM_BUFFER_CREATE(sizeof(tg_material_ubo));

    m4* p_model = (m4*)h_command->model_ubo.memory.p_mapped_device_memory;
    const v3 scale = (v3){ (f32)h_voxels->image_3d.width, (f32)h_voxels->image_3d.height, (f32)h_voxels->image_3d.depth };
    const m4 translation_mat = tgm_m4_translate(position);
    const m4 rotation_mat = tgm_m4_identity();
    const m4 scale_mat = tgm_m4_scale(scale);
    const m4 model = tgm_m4_mul(translation_mat, tgm_m4_mul(rotation_mat, scale_mat));
    *p_model = model;

    tg_material_ubo* p_material = (tg_material_ubo*)h_command->material_ubo.memory.p_mapped_device_memory;
    p_material->albedo = (v4){ 0.2f, 0.5f, 1.0f, 1.0f };
    p_material->metallic = 0.0f;
    p_material->roughness = 1.0f;

    tgvk_descriptor_set_update_uniform_buffer(h_command->descriptor_set.set, &h_command->model_ubo, 0);
    //tgvk_descriptor_set_update_uniform_buffer(h_command->descriptor_set.descriptor_set, &shared_render_resources.ray_tracer.vis.view_projection_ubo, 1);
    tgvk_descriptor_set_update_uniform_buffer(h_command->descriptor_set.set, &h_command->material_ubo, 2);

	return h_command;
}

void tg_ray_trace_command_destroy(tg_ray_trace_command_h h_command)
{
    TG_ASSERT(h_command);

    TG_NOT_IMPLEMENTED();
}

#endif
