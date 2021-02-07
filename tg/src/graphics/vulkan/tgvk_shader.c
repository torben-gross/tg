#include "graphics/vulkan/tgvk_core.h"

#ifdef TG_VULKAN

#include "graphics/tg_shader_library.h"
#include "util/tg_string.h"



void tg_compute_shader_bind_input(tg_compute_shader_h h_compute_shader, u32 first_handle_index, u32 handle_count, tg_handle* p_handles)
{
	for (u32 i = first_handle_index; i < first_handle_index + handle_count; i++)
	{
		tgvk_descriptor_set_update(h_compute_shader->descriptor_set.descriptor_set, p_handles[i], i);
	}
}

tg_compute_shader_h tg_compute_shader_create(const char* p_filename)
{
	TG_ASSERT(p_filename);

	tg_compute_shader_h h_compute_shader = tg_shader_library_get_compute_shader(p_filename);
	if (!h_compute_shader)
	{
		h_compute_shader = tgvk_handle_take(TG_STRUCTURE_TYPE_COMPUTE_SHADER);
		h_compute_shader->shader = tgvk_shader_create(p_filename);
		h_compute_shader->compute_pipeline = tgvk_pipeline_create_compute(&h_compute_shader->shader);
		h_compute_shader->descriptor_set = tgvk_descriptor_set_create(&h_compute_shader->compute_pipeline);
	}

	return h_compute_shader;
}

void tg_compute_shader_dispatch(tg_compute_shader_h h_compute_shader, u32 group_count_x, u32 group_count_y, u32 group_count_z)
{
	TG_ASSERT(h_compute_shader && group_count_x && group_count_y && group_count_z);

	tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_COMPUTE);
	vkCmdBindPipeline(p_command_buffer->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_compute_shader->compute_pipeline.pipeline);
	vkCmdBindDescriptorSets(p_command_buffer->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_compute_shader->compute_pipeline.layout.pipeline_layout, 0, 1, &h_compute_shader->descriptor_set.descriptor_set, 0, TG_NULL);
	vkCmdDispatch(p_command_buffer->command_buffer, group_count_x, group_count_y, group_count_z);
	tgvk_command_buffer_end_and_submit(p_command_buffer);
}

void tg_compute_shader_destroy(tg_compute_shader_h h_compute_shader)
{
	TG_ASSERT(h_compute_shader);

	tgvk_descriptor_set_destroy(&h_compute_shader->descriptor_set);
	tgvk_pipeline_destroy(&h_compute_shader->compute_pipeline);
	tgvk_shader_destroy(&h_compute_shader->shader);
	tgvk_handle_release(h_compute_shader);
}



tg_vertex_shader_h tg_vertex_shader_create(const char* p_filename)
{
	TG_ASSERT(p_filename);

	tg_vertex_shader_h h_vertex_shader = tg_shader_library_get_vertex_shader(p_filename);
	if (!h_vertex_shader)
	{
		h_vertex_shader = tgvk_handle_take(TG_STRUCTURE_TYPE_VERTEX_SHADER);
		h_vertex_shader->shader = tgvk_shader_create(p_filename);
	}

	return h_vertex_shader;
}

void tg_vertex_shader_destroy(tg_vertex_shader_h h_vertex_shader)
{
	TG_ASSERT(h_vertex_shader);

	tgvk_shader_destroy(&h_vertex_shader->shader);
	tgvk_handle_release(h_vertex_shader);
}



tg_fragment_shader_h tg_fragment_shader_create(const char* p_filename)
{
	TG_ASSERT(p_filename);

	tg_fragment_shader_h h_fragment_shader = tg_shader_library_get_fragment_shader(p_filename);
	if (!h_fragment_shader)
	{
		h_fragment_shader = tgvk_handle_take(TG_STRUCTURE_TYPE_FRAGMENT_SHADER);
		h_fragment_shader->shader = tgvk_shader_create(p_filename);
	}

	return h_fragment_shader;
}

void tg_fragment_shader_destroy(tg_fragment_shader_h h_fragment_shader)
{
	TG_ASSERT(h_fragment_shader);

	tgvk_shader_destroy(&h_fragment_shader->shader);
	tgvk_handle_release(h_fragment_shader);
}

#endif
