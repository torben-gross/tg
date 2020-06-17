#include "graphics/vulkan/tg_graphics_vulkan.h"
#include "graphics/vulkan/tg_vulkan_deferred_renderer.h"
#include "graphics/vulkan/tg_vulkan_forward_renderer.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "tg_entity.h"

tg_entity_graphics_data_ptr_h tg_entity_graphics_data_ptr_create(tg_mesh_h h_mesh, tg_material_h h_material)
{
	TG_ASSERT(h_material);

	tg_entity_graphics_data_ptr_h h_entity_graphics_data_ptr = TG_MEMORY_ALLOC(sizeof(*h_entity_graphics_data_ptr));

    h_entity_graphics_data_ptr->type = TG_HANDLE_TYPE_ENTITY_GRAPHICS_DATA_PTR;
    h_entity_graphics_data_ptr->lod_count = 1;
    h_entity_graphics_data_ptr->ph_lod_meshes[0] = h_mesh;
    h_entity_graphics_data_ptr->h_material = h_material;
    h_entity_graphics_data_ptr->model_ubo = tg_vulkan_buffer_create(sizeof(m4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    TG_ENTITY_GRAPHICS_DATA_PTR_MODEL(h_entity_graphics_data_ptr) = tgm_m4_identity();
    h_entity_graphics_data_ptr->camera_info_count = 0;

	return h_entity_graphics_data_ptr;
}

void tg_entity_graphics_data_ptr_destroy(tg_entity_graphics_data_ptr_h h_entity_graphics_data_ptr)
{
	TG_ASSERT(h_entity_graphics_data_ptr);

    tg_vulkan_buffer_destroy(&h_entity_graphics_data_ptr->model_ubo);
    for (u32 i = 0; i < h_entity_graphics_data_ptr->camera_info_count; i++)
    {
        // TODO: i believe, the same content exists in the vulkan camera file
        tg_vulkan_command_buffers_free(graphics_command_pool, h_entity_graphics_data_ptr->lod_count, h_entity_graphics_data_ptr->p_camera_infos[i].p_command_buffers);
        tg_vulkan_graphics_pipeline_destroy(h_entity_graphics_data_ptr->p_camera_infos[i].graphics_pipeline);
        tg_vulkan_pipeline_layout_destroy(h_entity_graphics_data_ptr->p_camera_infos[i].pipeline_layout);
        tg_vulkan_descriptor_destroy(&h_entity_graphics_data_ptr->p_camera_infos[i].descriptor);
    }
	TG_MEMORY_FREE(h_entity_graphics_data_ptr);
}

void tg_entity_graphics_data_ptr_set_mesh(tg_entity_graphics_data_ptr_h h_entity_graphics_data_ptr, tg_mesh_h h_mesh, u8 lod)
{
    TG_ASSERT(lod < TG_VULKAN_MAX_LOD_COUNT);

    for (u32 i = 0; i < h_entity_graphics_data_ptr->camera_info_count; i++)
    {
        tg_vulkan_camera_info_clear(&h_entity_graphics_data_ptr->p_camera_infos[i]);
    }
    h_entity_graphics_data_ptr->camera_info_count = 0;
    h_entity_graphics_data_ptr->ph_lod_meshes[lod] = h_mesh;
}

void tg_entity_graphics_data_ptr_set_model_matrix(tg_entity_graphics_data_ptr_h h_entity_graphics_data_ptr, const m4* p_model_matrix)
{
    TG_ENTITY_GRAPHICS_DATA_PTR_MODEL(h_entity_graphics_data_ptr) = *p_model_matrix;
    tg_vulkan_buffer_flush_mapped_memory(&h_entity_graphics_data_ptr->model_ubo); // TODO: use HOST_CACHED and flush... currently, this flush doesnt do anything
}

#endif
