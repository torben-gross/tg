#include "graphics/vulkan/tg_graphics_vulkan.h"
#include "graphics/vulkan/tg_graphics_vulkan_deferred_renderer.h"
#include "graphics/vulkan/tg_graphics_vulkan_forward_renderer.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "tg_entity.h"

tg_entity_graphics_data_ptr_h tg_entity_graphics_data_ptr_create(tg_mesh_h mesh_h, tg_material_h material_h)
{
	TG_ASSERT(mesh_h && material_h);

	tg_entity_graphics_data_ptr_h entity_graphics_data_ptr_h = TG_MEMORY_ALLOC(sizeof(*entity_graphics_data_ptr_h));

    entity_graphics_data_ptr_h->type = TG_HANDLE_TYPE_ENTITY_GRAPHICS_DATA_PTR;
    entity_graphics_data_ptr_h->mesh_h = mesh_h;
    entity_graphics_data_ptr_h->material_h = material_h;
    entity_graphics_data_ptr_h->model_ubo = tg_vulkan_buffer_create(sizeof(m4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    *((m4*)entity_graphics_data_ptr_h->model_ubo.p_mapped_device_memory) = tgm_m4_identity();
    entity_graphics_data_ptr_h->camera_info_count = 0;

	return entity_graphics_data_ptr_h;
}

void tg_entity_graphics_data_ptr_destroy(tg_entity_graphics_data_ptr_h entity_graphics_data_ptr_h)
{
	TG_ASSERT(entity_graphics_data_ptr_h);

    tg_vulkan_buffer_destroy(&entity_graphics_data_ptr_h->model_ubo);
    for (u32 i = 0; i < entity_graphics_data_ptr_h->camera_info_count; i++)
    {
        tg_vulkan_command_buffer_free(graphics_command_pool, entity_graphics_data_ptr_h->p_camera_infos[i].command_buffer);
        tg_vulkan_graphics_pipeline_destroy(entity_graphics_data_ptr_h->p_camera_infos[i].graphics_pipeline);
        tg_vulkan_pipeline_layout_destroy(entity_graphics_data_ptr_h->p_camera_infos[i].pipeline_layout);
        tg_vulkan_descriptor_destroy(&entity_graphics_data_ptr_h->p_camera_infos[i].descriptor);
    }
	TG_MEMORY_FREE(entity_graphics_data_ptr_h);
}

void tg_entity_graphics_data_ptr_set_model_matrix(tg_entity_graphics_data_ptr_h entity_graphics_data_ptr_h, const m4* p_model_matrix)
{
    *((m4*)entity_graphics_data_ptr_h->model_ubo.p_mapped_device_memory) = *p_model_matrix;
    tg_vulkan_buffer_flush_mapped_memory(&entity_graphics_data_ptr_h->model_ubo); // TODO: use HOST_CACHED and flush... currently, this flush doesnt do anything
}

#endif
