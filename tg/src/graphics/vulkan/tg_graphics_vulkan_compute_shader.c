#include "tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory_allocator.h"

tg_compute_shader_h tg_graphics_compute_shader_create(tg_compute_buffer_h b0, tg_compute_buffer_h b1, const char* filename)
{
	TG_ASSERT(filename);

	tg_compute_shader_h compute_shader_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(*compute_shader_h));
	tg_graphics_vulkan_shader_module_create(filename, &compute_shader_h->shader_module);

	VkDescriptorPoolSize descriptor_pool_size = { 0 };
	{
		descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_pool_size.descriptorCount = 1;
	}
	tg_graphics_vulkan_descriptor_pool_create(0, 1, 1, &descriptor_pool_size, &compute_shader_h->descriptor_pool);

	VkDescriptorSetLayoutBinding p_descriptor_set_layout_bindings[2] = { 0 };
	{
		p_descriptor_set_layout_bindings[0].binding = 0;
		p_descriptor_set_layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		p_descriptor_set_layout_bindings[0].descriptorCount = 1;
		p_descriptor_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		p_descriptor_set_layout_bindings[0].pImmutableSamplers = TG_NULL;

		p_descriptor_set_layout_bindings[1].binding = 1;
		p_descriptor_set_layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		p_descriptor_set_layout_bindings[1].descriptorCount = 1;
		p_descriptor_set_layout_bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		p_descriptor_set_layout_bindings[1].pImmutableSamplers = TG_NULL;
	}
	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = { 0 };
	{
		descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptor_set_layout_create_info.pNext = TG_NULL;
		descriptor_set_layout_create_info.flags = 0;
		descriptor_set_layout_create_info.bindingCount;
		descriptor_set_layout_create_info.pBindings = p_descriptor_set_layout_bindings;
	}
    tg_graphics_vulkan_descriptor_set_layout_create(0, sizeof(p_descriptor_set_layout_bindings) / sizeof(*p_descriptor_set_layout_bindings), p_descriptor_set_layout_bindings, &compute_shader_h->descriptor_set_layout);
	tg_graphics_vulkan_descriptor_set_allocate(compute_shader_h->descriptor_pool, compute_shader_h->descriptor_set_layout, &compute_shader_h->descriptor_set);

	VkDescriptorBufferInfo in_descriptor_buffer_info = { 0 };
	{
		in_descriptor_buffer_info.buffer = b0->buffer;
		in_descriptor_buffer_info.offset = 0;
		in_descriptor_buffer_info.range = VK_WHOLE_SIZE;
	}
	VkDescriptorBufferInfo out_descriptor_buffer_info = { 0 };
	{
		out_descriptor_buffer_info.buffer = b1->buffer;
		out_descriptor_buffer_info.offset = 0;
		out_descriptor_buffer_info.range = VK_WHOLE_SIZE;
	}
	VkWriteDescriptorSet p_write_descriptor_sets[2] = { 0 };
	{
		p_write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		p_write_descriptor_sets[0].pNext = TG_NULL;
		p_write_descriptor_sets[0].dstSet = compute_shader_h->descriptor_set;
		p_write_descriptor_sets[0].dstBinding = 0;
		p_write_descriptor_sets[0].dstArrayElement = 0;
		p_write_descriptor_sets[0].descriptorCount = 1;
		p_write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		p_write_descriptor_sets[0].pImageInfo = TG_NULL;
		p_write_descriptor_sets[0].pBufferInfo = &in_descriptor_buffer_info;
		p_write_descriptor_sets[0].pTexelBufferView;

		p_write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		p_write_descriptor_sets[1].pNext = TG_NULL;
		p_write_descriptor_sets[1].dstSet = compute_shader_h->descriptor_set;
		p_write_descriptor_sets[1].dstBinding = 1;
		p_write_descriptor_sets[1].dstArrayElement = 0;
		p_write_descriptor_sets[1].descriptorCount = 1;
		p_write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		p_write_descriptor_sets[1].pImageInfo = TG_NULL;
		p_write_descriptor_sets[1].pBufferInfo = &out_descriptor_buffer_info;
		p_write_descriptor_sets[1].pTexelBufferView;
	}
	vkUpdateDescriptorSets(device, 2, p_write_descriptor_sets, 0, TG_NULL);

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

	tg_graphics_vulkan_command_buffer_begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, compute_shader_h->command_buffer);
	vkCmdBindPipeline(compute_shader_h->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_shader_h->pipeline);
	vkCmdBindDescriptorSets(compute_shader_h->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_shader_h->pipeline_layout, 0, 1, &compute_shader_h->descriptor_set, 0, TG_NULL);
	vkCmdDispatch(compute_shader_h->command_buffer, 10, 1, 1);
	VK_CALL(vkEndCommandBuffer(compute_shader_h->command_buffer));

	return compute_shader_h;
}

void tg_graphics_compute_shader_dispatch(tg_compute_shader_h compute_shader_h)
{
	TG_ASSERT(compute_shader_h);

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
	TG_MEMORY_ALLOCATOR_FREE(compute_shader_h);
}

#endif
