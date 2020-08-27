#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#if defined(TG_DEBUG) || !defined(TG_DISTRIBUTE)
#define TG_RECOMPILE_SHADERS_ON_STARTUP 1
#else
#define TG_RECOMPILE_SHADERS_ON_STARTUP 0
#endif

#include "memory/tg_memory.h"
#include "platform/tg_platform.h"
#include "tg_assets.h"
#include "util/tg_string.h"

#if TG_RECOMPILE_SHADERS_ON_STARTUP
static void tg__recompile(const char* p_filename)
{
	tg_file_properties uncompiled_properties = { 0 };
	tg_platform_file_get_properties(p_filename, &uncompiled_properties);

	char p_filename_buffer_spv[TG_MAX_PATH] = { 0 };
	tg_string_format(TG_MAX_PATH, p_filename_buffer_spv, "%s.spv", p_filename);

	tg_file_properties compiled_properties = { 0 };
	const b32 spv_exists = tg_platform_file_get_properties(p_filename_buffer_spv, &compiled_properties);

	b32 recompile = TG_FALSE;
	if (spv_exists)
	{
		recompile = tg_platform_system_time_compare(&compiled_properties.last_write_time, &uncompiled_properties.last_write_time) == -1;
	}

	if (!spv_exists || recompile)
	{
		char p_filename_buffer[TG_MAX_PATH] = { 0 };
		tg_string_format(TG_MAX_PATH, p_filename_buffer, "%s%c%s", uncompiled_properties.p_directory, TG_FILE_SEPERATOR, uncompiled_properties.p_short_filename);

		char p_full_filename_buffer[TG_MAX_PATH] = { 0 };
		tg_platform_prepend_asset_directory(p_filename_buffer, TG_MAX_PATH, p_full_filename_buffer);

		char p_delete_buffer[10 + 2 * TG_MAX_PATH] = { 0 };
		tg_string_format(
			sizeof(p_delete_buffer), p_delete_buffer,
			"del /s /q %s.spv",
			p_full_filename_buffer
		);
		const i32 result0 = system(p_delete_buffer);
		TG_ASSERT(result0 != -1);

		char p_compile_buffer[38 + 4 + 2 * TG_MAX_PATH] = { 0 };
		tg_string_format(
			sizeof(p_compile_buffer), p_compile_buffer,
			"C:/VulkanSDK/1.2.141.2/Bin/glslc.exe %s -o %s.spv",
			p_full_filename_buffer,
			p_full_filename_buffer
		);
		const i32 result1 = system(p_compile_buffer);
		TG_ASSERT(result1 != -1);
	}
}
#endif



void tg_compute_shader_bind_input(tg_compute_shader_h h_compute_shader, u32 first_handle_index, u32 handle_count, tg_handle* p_handles)
{
	for (u32 i = first_handle_index; i < first_handle_index + handle_count; i++)
	{
		tg_vulkan_descriptor_set_update(h_compute_shader->descriptor_set.descriptor_set, p_handles[i], i);
	}
}

tg_compute_shader_h tg_compute_shader_create(const char* p_filename)
{
	TG_ASSERT(p_filename);

	TG_ASSERT(tg_assets_get_asset(p_filename) == TG_NULL);
#if TG_RECOMPILE_SHADERS_ON_STARTUP
	tg__recompile(p_filename);
#endif

	tg_compute_shader_h h_compute_shader = TG_NULL;
	TG_VULKAN_TAKE_HANDLE(p_compute_shaders, h_compute_shader);

	h_compute_shader->type = TG_STRUCTURE_TYPE_COMPUTE_SHADER;
	h_compute_shader->vulkan_shader = tg_vulkan_shader_create(p_filename);
	h_compute_shader->compute_pipeline = tg_vulkan_pipeline_create_compute(&h_compute_shader->vulkan_shader);
	h_compute_shader->descriptor_set = tg_vulkan_descriptor_set_create(&h_compute_shader->compute_pipeline);

	return h_compute_shader;
}

void tg_compute_shader_dispatch(tg_compute_shader_h h_compute_shader, u32 group_count_x, u32 group_count_y, u32 group_count_z)
{
	TG_ASSERT(h_compute_shader && group_count_x && group_count_y && group_count_z);

	tg_vulkan_command_buffer_begin(global_compute_command_buffer, 0, TG_NULL);
	vkCmdBindPipeline(global_compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_compute_shader->compute_pipeline.pipeline);
	vkCmdBindDescriptorSets(global_compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, h_compute_shader->compute_pipeline.layout.pipeline_layout, 0, 1, &h_compute_shader->descriptor_set.descriptor_set, 0, TG_NULL);
	vkCmdDispatch(global_compute_command_buffer, group_count_x, group_count_y, group_count_z);
	VK_CALL(vkEndCommandBuffer(global_compute_command_buffer));

	VkSubmitInfo submit_info = { 0 };// TODO: add fence to compute shader
	{
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.pNext = TG_NULL;
		submit_info.waitSemaphoreCount = 0;
		submit_info.pWaitSemaphores = TG_NULL;
		submit_info.pWaitDstStageMask = TG_NULL;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &global_compute_command_buffer;
		submit_info.signalSemaphoreCount = 0;
		submit_info.pSignalSemaphores = TG_NULL;
	}
	tg_vulkan_queue_submit(TG_VULKAN_QUEUE_TYPE_COMPUTE, 1, &submit_info, VK_NULL_HANDLE);
	tg_vulkan_queue_wait_idle(TG_VULKAN_QUEUE_TYPE_COMPUTE);
}

void tg_compute_shader_destroy(tg_compute_shader_h h_compute_shader)
{
	TG_ASSERT(h_compute_shader);

	tg_vulkan_pipeline_destroy(&h_compute_shader->compute_pipeline);
	tg_vulkan_shader_destroy(&h_compute_shader->vulkan_shader);
	TG_VULKAN_RELEASE_HANDLE(h_compute_shader);
}

tg_compute_shader_h tg_compute_shader_get(const char* p_filename)
{
	TG_ASSERT(p_filename);

	tg_compute_shader_h h_compute_shader = tg_assets_get_asset(p_filename);
	return h_compute_shader;
}



tg_vertex_shader_h tg_vertex_shader_create(const char* p_filename)
{
	TG_ASSERT(p_filename);

	TG_ASSERT(tg_assets_get_asset(p_filename) == TG_NULL);
#if TG_RECOMPILE_SHADERS_ON_STARTUP
	tg__recompile(p_filename);
#endif

	tg_vertex_shader_h h_vertex_shader = TG_NULL;
	TG_VULKAN_TAKE_HANDLE(p_vertex_shaders, h_vertex_shader);

	h_vertex_shader->type = TG_STRUCTURE_TYPE_VERTEX_SHADER;
	h_vertex_shader->vulkan_shader = tg_vulkan_shader_create(p_filename);
	return h_vertex_shader;
}

void tg_vertex_shader_destroy(tg_vertex_shader_h h_vertex_shader)
{
	TG_ASSERT(h_vertex_shader);

	tg_vulkan_shader_destroy(&h_vertex_shader->vulkan_shader);
	TG_VULKAN_RELEASE_HANDLE(h_vertex_shader);
}

tg_vertex_shader_h tg_vertex_shader_get(const char* p_filename)
{
    TG_ASSERT(p_filename);

	tg_vertex_shader_h h_vertex_shader = tg_assets_get_asset(p_filename);
	return h_vertex_shader;
}



tg_fragment_shader_h tg_fragment_shader_create(const char* p_filename)
{
	TG_ASSERT(p_filename);

	TG_ASSERT(tg_assets_get_asset(p_filename) == TG_NULL);
#if TG_RECOMPILE_SHADERS_ON_STARTUP
	tg__recompile(p_filename);
#endif

	tg_fragment_shader_h h_fragment_shader = TG_NULL;
	TG_VULKAN_TAKE_HANDLE(p_fragment_shaders, h_fragment_shader);

	h_fragment_shader->type = TG_STRUCTURE_TYPE_FRAGMENT_SHADER;
	h_fragment_shader->vulkan_shader = tg_vulkan_shader_create(p_filename);
	return h_fragment_shader;
}

void tg_fragment_shader_destroy(tg_fragment_shader_h h_fragment_shader)
{
	TG_ASSERT(h_fragment_shader);

	tg_vulkan_shader_destroy(&h_fragment_shader->vulkan_shader);
	TG_VULKAN_RELEASE_HANDLE(h_fragment_shader);
}

tg_fragment_shader_h tg_fragment_shader_get(const char* p_filename)
{
    TG_ASSERT(p_filename);

	tg_fragment_shader_h h_fragment_shader = tg_assets_get_asset(p_filename);
    return h_fragment_shader;
}

#endif
