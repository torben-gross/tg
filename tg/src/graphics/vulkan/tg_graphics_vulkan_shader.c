#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"



tg_compute_shader_h tgg_compute_shader_create(const char* filename, u32 input_element_count, tg_shader_input_element* p_shader_input_elements)
{
	TG_ASSERT(filename && input_element_count && p_shader_input_elements);

	tg_compute_shader_h compute_shader_h = TG_MEMORY_ALLOC(sizeof(*compute_shader_h));

	compute_shader_h->input_element_count = input_element_count;

	compute_shader_h->p_input_elements = TG_MEMORY_ALLOC(input_element_count * sizeof(*compute_shader_h->p_input_elements));
	for (u32 i = 0; i < input_element_count; i++)
	{
		compute_shader_h->p_input_elements[i] = p_shader_input_elements[i];
	}

	VkDescriptorSetLayoutBinding* p_descriptor_set_layout_bindings = TG_MEMORY_ALLOC(input_element_count * sizeof(*p_descriptor_set_layout_bindings));
	for (u32 i = 0; i < input_element_count; i++)
	{
		p_descriptor_set_layout_bindings[i].binding = i;
		p_descriptor_set_layout_bindings[i].descriptorType = tgg_vulkan_shader_input_element_type_convert(p_shader_input_elements[i].type);
		p_descriptor_set_layout_bindings[i].descriptorCount = p_shader_input_elements[i].array_element_count;
		p_descriptor_set_layout_bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		p_descriptor_set_layout_bindings[i].pImmutableSamplers = TG_NULL;
	}

	compute_shader_h->compute_shader = tgg_vulkan_compute_shader_create(filename, input_element_count, p_descriptor_set_layout_bindings);
	compute_shader_h->command_buffer = tgg_vulkan_command_buffer_allocate(compute_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	TG_MEMORY_FREE(p_descriptor_set_layout_bindings);

	return compute_shader_h;
}

void tgg_compute_shader_bind_input(tg_compute_shader_h compute_shader_h, tg_handle* p_shader_input_element_handles)
{
	for (u32 i = 0; i < compute_shader_h->input_element_count; i++)
	{
		tgg_vulkan_descriptor_set_update(compute_shader_h->compute_shader.descriptor.descriptor_set, &compute_shader_h->p_input_elements[i], p_shader_input_element_handles[i], i);
	}
}

void tgg_compute_shader_dispatch(tg_compute_shader_h compute_shader_h, u32 group_count_x, u32 group_count_y, u32 group_count_z)
{
	TG_ASSERT(compute_shader_h && group_count_x && group_count_y && group_count_z);

	tgg_vulkan_command_buffer_begin(compute_shader_h->command_buffer, 0, TG_NULL);
	vkCmdBindPipeline(compute_shader_h->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_shader_h->compute_shader.pipeline);
	vkCmdBindDescriptorSets(compute_shader_h->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_shader_h->compute_shader.pipeline_layout, 0, 1, &compute_shader_h->compute_shader.descriptor.descriptor_set, 0, TG_NULL);
	vkCmdDispatch(compute_shader_h->command_buffer, group_count_x, group_count_y, group_count_z);
	VK_CALL(vkEndCommandBuffer(compute_shader_h->command_buffer));

	VkSubmitInfo submit_info = { 0 };// TODO: add fence to compute shader
	{
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.pNext = TG_NULL;
		submit_info.waitSemaphoreCount = 0;
		submit_info.pWaitSemaphores = TG_NULL;
		submit_info.pWaitDstStageMask = TG_NULL;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &compute_shader_h->command_buffer;
		submit_info.signalSemaphoreCount = 0;
		submit_info.pSignalSemaphores = TG_NULL;
	}
	VK_CALL(vkQueueSubmit(compute_queue.queue, 1, &submit_info, TG_NULL));
	VK_CALL(vkQueueWaitIdle(compute_queue.queue));
}

void tgg_compute_shader_destroy(tg_compute_shader_h compute_shader_h)
{
	TG_ASSERT(compute_shader_h);

	tgg_vulkan_command_buffer_free(compute_command_pool, compute_shader_h->command_buffer);
	tgg_vulkan_shader_module_destroy(compute_shader_h->compute_shader.shader_module);
	TG_MEMORY_FREE(compute_shader_h->p_input_elements);
	TG_MEMORY_FREE(compute_shader_h);
}



tg_vertex_shader_h tgg_vertex_shader_create(const char* p_filename)
{
    TG_ASSERT(p_filename);

    tg_vertex_shader_h vertex_shader_h = TG_MEMORY_ALLOC(sizeof(*vertex_shader_h));
    vertex_shader_h->shader_module = tgg_vulkan_shader_module_create(p_filename);
    return vertex_shader_h;
}

void tgg_vertex_shader_destroy(tg_vertex_shader_h vertex_shader_h)
{
    TG_ASSERT(vertex_shader_h);

    tgg_vulkan_shader_module_destroy(vertex_shader_h->shader_module);
    TG_MEMORY_FREE(vertex_shader_h);
}



tg_fragment_shader_h tgg_fragment_shader_create(const char* p_filename)
{
    TG_ASSERT(p_filename);

    tg_fragment_shader_h fragment_shader_h = TG_MEMORY_ALLOC(sizeof(*fragment_shader_h));
    fragment_shader_h->shader_module = tgg_vulkan_shader_module_create(p_filename);
    return fragment_shader_h;
}

void tgg_fragment_shader_destroy(tg_fragment_shader_h fragment_shader_h)
{
    TG_ASSERT(fragment_shader_h);

    tgg_vulkan_shader_module_destroy(fragment_shader_h->shader_module);
    TG_MEMORY_FREE(fragment_shader_h);
}

#endif
