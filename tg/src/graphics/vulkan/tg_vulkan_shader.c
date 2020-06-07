#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "platform/tg_platform.h"
#include "tg_assets.h"
#include "util/tg_string.h"

#ifdef TG_DEBUG
void tg_shader_internal_recompile(const char* p_filename)
{
	tg_file_properties uncompiled_properties = { 0 };
	tg_platform_get_file_properties(p_filename, &uncompiled_properties);

	char p_filename_buffer_spv[TG_MAX_PATH] = { 0 };
	tg_string_format(TG_MAX_PATH, p_filename_buffer_spv, "%s.spv", p_filename);

	tg_file_properties compiled_properties = { 0 };
	const b32 spv_exists = tg_platform_get_file_properties(p_filename_buffer_spv, &compiled_properties);

	b32 recompile = TG_FALSE;
	if (spv_exists)
	{
		recompile = tg_platform_system_time_compare(&compiled_properties.last_write_time, &uncompiled_properties.last_write_time) == -1;
	}

	if (!spv_exists || recompile)
	{
		char p_filename_buffer[TG_MAX_PATH] = { 0 };
		tg_string_format(TG_MAX_PATH, p_filename_buffer, "%s%c%s", uncompiled_properties.p_relative_directory, tg_platform_get_file_separator(), uncompiled_properties.p_filename);

		char p_delete_buffer[10 + 2 * TG_MAX_PATH] = { 0 };
		tg_string_format(
			sizeof(p_delete_buffer), p_delete_buffer,
			"del /s /q %s.spv",
			p_filename_buffer
		);
		TG_ASSERT(system(p_delete_buffer) != -1);

		char p_compile_buffer[38 + 4 + 2 * TG_MAX_PATH] = { 0 };
		tg_string_format(
			sizeof(p_compile_buffer), p_compile_buffer,
			"C:/VulkanSDK/1.2.131.2/Bin/glslc.exe %s -o %s.spv",
			p_filename_buffer,
			p_filename_buffer
		);
		TG_ASSERT(system(p_compile_buffer) != -1);
	}
}
#endif



void tg_compute_shader_bind_input(tg_compute_shader_h compute_shader_h, u32 first_handle_index, u32 handle_count, tg_handle* p_handles)
{
	for (u32 i = first_handle_index; i < first_handle_index + handle_count; i++)
	{
		tg_vulkan_descriptor_set_update(compute_shader_h->descriptor.descriptor_set, p_handles[i], i);
	}
}

tg_compute_shader_h tg_compute_shader_create(const char* p_filename)
{
	TG_ASSERT(p_filename);

	TG_ASSERT(tg_assets_get_asset(p_filename) == TG_NULL);
#ifdef TG_DEBUG
	tg_shader_internal_recompile(p_filename);
#endif

	tg_compute_shader_h compute_shader_h = TG_MEMORY_ALLOC(sizeof(*compute_shader_h));
	compute_shader_h->type = TG_HANDLE_TYPE_COMPUTE_SHADER;

	compute_shader_h->vulkan_shader = tg_vulkan_shader_create(p_filename);
	compute_shader_h->descriptor = tg_vulkan_descriptor_create_for_shader(&compute_shader_h->vulkan_shader);
	compute_shader_h->pipeline_layout = tg_vulkan_pipeline_layout_create(1, &compute_shader_h->descriptor.descriptor_set_layout, 0, TG_NULL);
	compute_shader_h->compute_pipeline = tg_vulkan_compute_pipeline_create(compute_shader_h->vulkan_shader.shader_module, compute_shader_h->pipeline_layout);
	// TODO: global command buffer in tg_graphics_vulkan.c
	compute_shader_h->command_buffer = tg_vulkan_command_buffer_allocate(compute_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	return compute_shader_h;
}

void tg_compute_shader_dispatch(tg_compute_shader_h compute_shader_h, u32 group_count_x, u32 group_count_y, u32 group_count_z)
{
	TG_ASSERT(compute_shader_h && group_count_x && group_count_y && group_count_z);

	tg_vulkan_command_buffer_begin(compute_shader_h->command_buffer, 0, TG_NULL);
	vkCmdBindPipeline(compute_shader_h->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_shader_h->compute_pipeline);
	vkCmdBindDescriptorSets(compute_shader_h->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_shader_h->pipeline_layout, 0, 1, &compute_shader_h->descriptor.descriptor_set, 0, TG_NULL);
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

void tg_compute_shader_destroy(tg_compute_shader_h compute_shader_h)
{
	TG_ASSERT(compute_shader_h);

	tg_vulkan_command_buffer_free(compute_command_pool, compute_shader_h->command_buffer);
	tg_vulkan_compute_pipeline_destroy(compute_shader_h->compute_pipeline);
	tg_vulkan_pipeline_layout_destroy(compute_shader_h->pipeline_layout);
	tg_vulkan_descriptor_destroy(&compute_shader_h->descriptor);
	tg_vulkan_shader_destroy(&compute_shader_h->vulkan_shader);
	TG_MEMORY_FREE(compute_shader_h);
}

tg_compute_shader_h tg_compute_shader_get(const char* p_filename)
{
	TG_ASSERT(p_filename);

	tg_compute_shader_h compute_shader_h = tg_assets_get_asset(p_filename);
	return compute_shader_h;
}



tg_vertex_shader_h tg_vertex_shader_create(const char* p_filename)
{
	TG_ASSERT(p_filename);

	TG_ASSERT(tg_assets_get_asset(p_filename) == TG_NULL);
#ifdef TG_DEBUG
	tg_shader_internal_recompile(p_filename);
#endif

	tg_vertex_shader_h vertex_shader_h = TG_MEMORY_ALLOC(sizeof(*vertex_shader_h));
	vertex_shader_h->type = TG_HANDLE_TYPE_VERTEX_SHADER;
	vertex_shader_h->vulkan_shader = tg_vulkan_shader_create(p_filename);
	return vertex_shader_h;
}

void tg_vertex_shader_destroy(tg_vertex_shader_h vertex_shader_h)
{
	TG_ASSERT(vertex_shader_h);

	tg_vulkan_shader_destroy(&vertex_shader_h->vulkan_shader);
	TG_MEMORY_FREE(vertex_shader_h);
}

tg_vertex_shader_h tg_vertex_shader_get(const char* p_filename)
{
    TG_ASSERT(p_filename);

	tg_vertex_shader_h vertex_shader_h = tg_assets_get_asset(p_filename);
	return vertex_shader_h;
}



tg_fragment_shader_h tg_fragment_shader_create(const char* p_filename)
{
	TG_ASSERT(p_filename);

	TG_ASSERT(tg_assets_get_asset(p_filename) == TG_NULL);
#ifdef TG_DEBUG
	tg_shader_internal_recompile(p_filename);
#endif

	tg_fragment_shader_h fragment_shader_h = TG_MEMORY_ALLOC(sizeof(*fragment_shader_h));
	fragment_shader_h->type = TG_HANDLE_TYPE_FRAGMENT_SHADER;
	fragment_shader_h->vulkan_shader = tg_vulkan_shader_create(p_filename);
	return fragment_shader_h;
}

void tg_fragment_shader_destroy(tg_fragment_shader_h fragment_shader_h)
{
	TG_ASSERT(fragment_shader_h);

	tg_vulkan_shader_destroy(&fragment_shader_h->vulkan_shader);
	TG_MEMORY_FREE(fragment_shader_h);
}

tg_fragment_shader_h tg_fragment_shader_get(const char* p_filename)
{
    TG_ASSERT(p_filename);

	tg_fragment_shader_h fragment_shader_h = tg_assets_get_asset(p_filename);
    return fragment_shader_h;
}

#endif
