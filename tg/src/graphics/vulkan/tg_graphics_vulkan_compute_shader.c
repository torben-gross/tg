#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory_allocator.h"

VkDescriptorType tg_graphics_compute_shader_internal_convert_type(tg_compute_shader_input_element_type type)
{
	switch (type)
	{
	case TG_COMPUTE_SHADER_INPUT_ELEMENT_TYPE_COMPUTE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	case TG_COMPUTE_SHADER_INPUT_ELEMENT_TYPE_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	}

	TG_ASSERT(TG_FALSE);
	return 0;
}

tg_compute_shader_h tg_graphics_compute_shader_create(u32 input_element_count, tg_compute_shader_input_element_type* p_input_element_types, const char* filename)
{
	TG_ASSERT(input_element_count && p_input_element_types && filename);

	tg_compute_shader_h compute_shader_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(*compute_shader_h));

	compute_shader_h->input_element_count = input_element_count;
	compute_shader_h->p_input_element_types = TG_MEMORY_ALLOCATOR_ALLOCATE(input_element_count * sizeof(*compute_shader_h->p_input_element_types));
	for (u32 i = 0; i < input_element_count; i++)
	{
		compute_shader_h->p_input_element_types[i] = p_input_element_types[i];
	}
	
	tg_graphics_vulkan_shader_module_create(filename, &compute_shader_h->shader_module);

	VkDescriptorPoolSize descriptor_pool_size = { 0 };
	{
		descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_pool_size.descriptorCount = 1;
	}
	tg_graphics_vulkan_descriptor_pool_create(0, 1, 1, &descriptor_pool_size, &compute_shader_h->descriptor_pool);

	VkDescriptorSetLayoutBinding* p_descriptor_set_layout_bindings = TG_MEMORY_ALLOCATOR_ALLOCATE(input_element_count * sizeof(*p_descriptor_set_layout_bindings));
	for (u32 i = 0; i < input_element_count; i++)
	{
		p_descriptor_set_layout_bindings[i].binding = i;
		p_descriptor_set_layout_bindings[i].descriptorType = tg_graphics_compute_shader_internal_convert_type(p_input_element_types[i]);
		p_descriptor_set_layout_bindings[i].descriptorCount = 1;
		p_descriptor_set_layout_bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		p_descriptor_set_layout_bindings[i].pImmutableSamplers = TG_NULL;
	}
    tg_graphics_vulkan_descriptor_set_layout_create(0, input_element_count, p_descriptor_set_layout_bindings, &compute_shader_h->descriptor_set_layout);
	TG_MEMORY_ALLOCATOR_FREE(p_descriptor_set_layout_bindings);
	tg_graphics_vulkan_descriptor_set_allocate(compute_shader_h->descriptor_pool, compute_shader_h->descriptor_set_layout, &compute_shader_h->descriptor_set);

	tg_graphics_vulkan_pipeline_layout_create(1, &compute_shader_h->descriptor_set_layout, 0, TG_NULL, &compute_shader_h->pipeline_layout);

	VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info = { 0 };
	{
		pipeline_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipeline_shader_stage_create_info.pNext = TG_NULL;
		pipeline_shader_stage_create_info.flags = 0;
		pipeline_shader_stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		pipeline_shader_stage_create_info.module = compute_shader_h->shader_module;
		pipeline_shader_stage_create_info.pName = "main";
		pipeline_shader_stage_create_info.pSpecializationInfo = TG_NULL;
	}
	VkComputePipelineCreateInfo compute_pipeline_create_info = { 0 };
	{
		compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		compute_pipeline_create_info.pNext = TG_NULL;
		compute_pipeline_create_info.flags = 0;
		compute_pipeline_create_info.stage = pipeline_shader_stage_create_info;
		compute_pipeline_create_info.layout = compute_shader_h->pipeline_layout;
		compute_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
		compute_pipeline_create_info.basePipelineIndex = -1;
	}
	VK_CALL(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &compute_pipeline_create_info, TG_NULL, &compute_shader_h->pipeline)); // TODO: abstract

	compute_shader_h->command_pool = tg_graphics_vulkan_command_pool_create(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, compute_queue.index);
	tg_graphics_vulkan_command_buffer_allocate(compute_shader_h->command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &compute_shader_h->command_buffer);

	return compute_shader_h;
}

void tg_graphics_compute_shader_bind_input_elements(tg_compute_shader_h compute_shader_h, void** pp_handles)
{
	for (u32 i = 0; i < compute_shader_h->input_element_count; i++)
	{
		switch (compute_shader_h->p_input_element_types[i])
		{
		case TG_COMPUTE_SHADER_INPUT_ELEMENT_TYPE_COMPUTE_BUFFER:
		{
			tg_compute_buffer_h compute_buffer_h = *(tg_compute_buffer_h*)pp_handles[i];
			VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
			{
				descriptor_buffer_info.buffer = compute_buffer_h->buffer.buffer;
				descriptor_buffer_info.offset = 0;
				descriptor_buffer_info.range = VK_WHOLE_SIZE;
			}
			VkWriteDescriptorSet write_descriptor_set = { 0 };
			{
				write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write_descriptor_set.pNext = TG_NULL;
				write_descriptor_set.dstSet = compute_shader_h->descriptor_set;
				write_descriptor_set.dstBinding = i;
				write_descriptor_set.dstArrayElement = 0;
				write_descriptor_set.descriptorCount = 1;
				write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				write_descriptor_set.pImageInfo = TG_NULL;
				write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
				write_descriptor_set.pTexelBufferView = TG_NULL;
			}
			vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
		} break;
		case TG_COMPUTE_SHADER_INPUT_ELEMENT_TYPE_UNIFORM_BUFFER:
		{
			tg_uniform_buffer_h uniform_buffer_h = *(tg_uniform_buffer_h*)pp_handles[i];
			VkDescriptorBufferInfo descriptor_buffer_info = { 0 };
			{
				descriptor_buffer_info.buffer = uniform_buffer_h->buffer.buffer;
				descriptor_buffer_info.offset = 0;
				descriptor_buffer_info.range = VK_WHOLE_SIZE;
			}
			VkWriteDescriptorSet write_descriptor_set = { 0 };
			{
				write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write_descriptor_set.pNext = TG_NULL;
				write_descriptor_set.dstSet = compute_shader_h->descriptor_set;
				write_descriptor_set.dstBinding = i;
				write_descriptor_set.dstArrayElement = 0;
				write_descriptor_set.descriptorCount = 1;
				write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				write_descriptor_set.pImageInfo = TG_NULL;
				write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
				write_descriptor_set.pTexelBufferView = TG_NULL;
			}
			vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, TG_NULL);
		} break;
		default: TG_ASSERT(TG_FALSE);
		}
	}
}

void tg_graphics_compute_shader_dispatch(tg_compute_shader_h compute_shader_h, u32 group_count_x, u32 group_count_y, u32 group_count_z)
{
	TG_ASSERT(compute_shader_h && group_count_x && group_count_y && group_count_z);

	tg_graphics_vulkan_command_buffer_begin(0, compute_shader_h->command_buffer);
	vkCmdBindPipeline(compute_shader_h->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_shader_h->pipeline);
	vkCmdBindDescriptorSets(compute_shader_h->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_shader_h->pipeline_layout, 0, 1, &compute_shader_h->descriptor_set, 0, TG_NULL);
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

void tg_graphics_compute_shader_destroy(tg_compute_shader_h compute_shader_h)
{
	TG_ASSERT(compute_shader_h);

	tg_graphics_vulkan_shader_module_destroy(compute_shader_h->shader_module);
	TG_MEMORY_ALLOCATOR_FREE(compute_shader_h->p_input_element_types);
	TG_MEMORY_ALLOCATOR_FREE(compute_shader_h);
}

#endif
