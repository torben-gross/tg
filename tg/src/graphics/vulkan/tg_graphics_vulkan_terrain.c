#include "graphics/vulkan/tg_graphics_vulkan_terrain.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"

tg_terrain_chunk_entity_h tg_terrain_chunk_entity_create(i32 x, i32 y, i32 z, tg_material_h material_h)
{
    tg_terrain_chunk_entity_h terrain_chunk_entity_h = TG_MEMORY_ALLOC(sizeof(*terrain_chunk_entity_h));



    tg_vulkan_buffer isolevel_ubo = { 0 };
    tg_vulkan_compute_shader isolevel_cs = { 0 };
    tg_vulkan_buffer marching_cubes_ubo = { 0 };
    tg_vulkan_compute_shader p_marching_cubes_cs[TG_VULKAN_TERRAIN_LOD_COUNT] = { 0 };
    tg_vulkan_compute_shader p_normals_cs[TG_VULKAN_TERRAIN_LOD_COUNT] = { 0 };
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;



    isolevel_ubo = tg_vulkan_buffer_create(sizeof(tg_terrain_chunk_entity_create_isolevel_ubo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT); // TODO: measure performance with staging buffer instead of flushing this one
    ((tg_terrain_chunk_entity_create_isolevel_ubo*)isolevel_ubo.p_mapped_device_memory)->chunk_index_x = x;
    ((tg_terrain_chunk_entity_create_isolevel_ubo*)isolevel_ubo.p_mapped_device_memory)->chunk_index_y = y;
    ((tg_terrain_chunk_entity_create_isolevel_ubo*)isolevel_ubo.p_mapped_device_memory)->chunk_index_z = z;
    tg_vulkan_buffer_flush_mapped_memory(&isolevel_ubo);
    
    terrain_chunk_entity_h->isolevel_si3d = tg_vulkan_storage_image_3d_create(TG_CHUNK_VERTEX_COUNT_X, TG_CHUNK_VERTEX_COUNT_Y, TG_CHUNK_VERTEX_COUNT_Z, VK_FORMAT_R32_SFLOAT);

    VkDescriptorSetLayoutBinding p_isolevel_descriptor_set_layout_bindings[2] = { 0 };

    p_isolevel_descriptor_set_layout_bindings[0].binding = 0;
    p_isolevel_descriptor_set_layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    p_isolevel_descriptor_set_layout_bindings[0].descriptorCount = 1;
    p_isolevel_descriptor_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    p_isolevel_descriptor_set_layout_bindings[0].pImmutableSamplers = TG_NULL;

    p_isolevel_descriptor_set_layout_bindings[1].binding = 1;
    p_isolevel_descriptor_set_layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    p_isolevel_descriptor_set_layout_bindings[1].descriptorCount = 1;
    p_isolevel_descriptor_set_layout_bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    p_isolevel_descriptor_set_layout_bindings[1].pImmutableSamplers = TG_NULL;

    isolevel_cs = tg_vulkan_compute_shader_create("shaders/terrain/isolevel.comp", 2, p_isolevel_descriptor_set_layout_bindings);
    tg_vulkan_descriptor_set_update_uniform_buffer(isolevel_cs.descriptor.descriptor_set, isolevel_ubo.buffer, 0);
    tg_vulkan_descriptor_set_update_storage_image_3d(isolevel_cs.descriptor.descriptor_set, &terrain_chunk_entity_h->isolevel_si3d, 1);

    marching_cubes_ubo = tg_vulkan_buffer_create(sizeof(tg_terrain_chunk_entity_create_marching_cubes_ubo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    ((tg_terrain_chunk_entity_create_marching_cubes_ubo*)marching_cubes_ubo.p_mapped_device_memory)->chunk_index_x = x;
    ((tg_terrain_chunk_entity_create_marching_cubes_ubo*)marching_cubes_ubo.p_mapped_device_memory)->chunk_index_y = y;
    ((tg_terrain_chunk_entity_create_marching_cubes_ubo*)marching_cubes_ubo.p_mapped_device_memory)->chunk_index_z = z;
    tg_vulkan_buffer_flush_mapped_memory(&marching_cubes_ubo);

    const u32 lod_count = 4;
    TG_ASSERT(lod_count <= TG_VULKAN_MAX_LOD_COUNT);
    for (u32 i = 0; i < lod_count; i++)
    {
        const u32 denominator = (1 << i);
        const u32 mesh_size = ((TG_CHUNK_VERTEX_COUNT_X - 1) / denominator) * ((TG_CHUNK_VERTEX_COUNT_Y - 1) / denominator) * ((TG_CHUNK_VERTEX_COUNT_Z - 1) / denominator) * 15 * sizeof(tg_vertex_3d);

        terrain_chunk_entity_h->p_lod_meshes[i].type = TG_HANDLE_TYPE_MESH;
        terrain_chunk_entity_h->p_lod_meshes[i].vbo = tg_vulkan_buffer_create(mesh_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        terrain_chunk_entity_h->p_lod_meshes[i].ibo.size = 0;
        terrain_chunk_entity_h->p_lod_meshes[i].ibo.buffer = VK_NULL_HANDLE;
        terrain_chunk_entity_h->p_lod_meshes[i].ibo.device_memory = VK_NULL_HANDLE;
        terrain_chunk_entity_h->p_lod_meshes[i].ibo.p_mapped_device_memory = TG_NULL;
    }

    VkDescriptorSetLayoutBinding p_marching_cubes_descriptor_set_layout_bindings[3] = { 0 };

    p_marching_cubes_descriptor_set_layout_bindings[0].binding = 0;
    p_marching_cubes_descriptor_set_layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    p_marching_cubes_descriptor_set_layout_bindings[0].descriptorCount = 1;
    p_marching_cubes_descriptor_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    p_marching_cubes_descriptor_set_layout_bindings[0].pImmutableSamplers = TG_NULL;

    p_marching_cubes_descriptor_set_layout_bindings[1].binding = 1;
    p_marching_cubes_descriptor_set_layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    p_marching_cubes_descriptor_set_layout_bindings[1].descriptorCount = 1;
    p_marching_cubes_descriptor_set_layout_bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    p_marching_cubes_descriptor_set_layout_bindings[1].pImmutableSamplers = TG_NULL;

    p_marching_cubes_descriptor_set_layout_bindings[2].binding = 2;
    p_marching_cubes_descriptor_set_layout_bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    p_marching_cubes_descriptor_set_layout_bindings[2].descriptorCount = 1;
    p_marching_cubes_descriptor_set_layout_bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    p_marching_cubes_descriptor_set_layout_bindings[2].pImmutableSamplers = TG_NULL;

    for (u32 i = 0; i < TG_VULKAN_TERRAIN_LOD_COUNT; i++)
    {
        p_marching_cubes_cs[i] = tg_vulkan_compute_shader_create("shaders/terrain/marching_cubes.comp", 3, p_marching_cubes_descriptor_set_layout_bindings);
        tg_vulkan_descriptor_set_update_uniform_buffer(p_marching_cubes_cs[i].descriptor.descriptor_set, marching_cubes_ubo.buffer, 0);
        tg_vulkan_descriptor_set_update_storage_image_3d(p_marching_cubes_cs[i].descriptor.descriptor_set, &terrain_chunk_entity_h->isolevel_si3d, 1);
        tg_vulkan_descriptor_set_update_storage_buffer(p_marching_cubes_cs[i].descriptor.descriptor_set, terrain_chunk_entity_h->p_lod_meshes[i].vbo.buffer, 2);
    }

    VkDescriptorSetLayoutBinding normals_descriptor_set_layout_binding = { 0 };
    normals_descriptor_set_layout_binding.binding = 0;
    normals_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    normals_descriptor_set_layout_binding.descriptorCount = 1;
    normals_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    normals_descriptor_set_layout_binding.pImmutableSamplers = TG_NULL;

    for (u32 i = 0; i < TG_VULKAN_TERRAIN_LOD_COUNT; i++)
    {
        p_normals_cs[i] = tg_vulkan_compute_shader_create("shaders/terrain/normals.comp", 1, &normals_descriptor_set_layout_binding);
        tg_vulkan_descriptor_set_update_storage_buffer(p_normals_cs[i].descriptor.descriptor_set, terrain_chunk_entity_h->p_lod_meshes[i].vbo.buffer, 0);
    }

    command_buffer = tg_vulkan_command_buffer_allocate(compute_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
    {
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, isolevel_cs.compute_pipeline);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, isolevel_cs.pipeline_layout, 0, 1, &isolevel_cs.descriptor.descriptor_set, 0, TG_NULL);
        vkCmdDispatch(command_buffer, TG_CHUNK_VERTEX_COUNT_X, TG_CHUNK_VERTEX_COUNT_Y, TG_CHUNK_VERTEX_COUNT_Z);

        VkMemoryBarrier isolevel_memory_barrier = { 0 };
        isolevel_memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        isolevel_memory_barrier.pNext = TG_NULL;
        isolevel_memory_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        isolevel_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &isolevel_memory_barrier, 0, TG_NULL, 0, TG_NULL);
        
        for (u32 i = 0; i < TG_VULKAN_TERRAIN_LOD_COUNT; i++)
        {
            const u32 denominator = (1 << i);

            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, p_marching_cubes_cs[i].compute_pipeline);
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, p_marching_cubes_cs[i].pipeline_layout, 0, 1, &p_marching_cubes_cs[i].descriptor.descriptor_set, 0, TG_NULL);
            vkCmdDispatch(command_buffer, (TG_CHUNK_VERTEX_COUNT_X - 1) / denominator, (TG_CHUNK_VERTEX_COUNT_Y - 1) / denominator, (TG_CHUNK_VERTEX_COUNT_Z - 1) / denominator); // TODO: update shaders to consider lod

            VkMemoryBarrier marching_cubes_memory_barrier = { 0 };
            marching_cubes_memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            marching_cubes_memory_barrier.pNext = TG_NULL;
            marching_cubes_memory_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            marching_cubes_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, &marching_cubes_memory_barrier, TG_NULL, 1, 0, TG_NULL);

            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, p_normals_cs[i].compute_pipeline);
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, p_normals_cs[i].pipeline_layout, 0, 1, &p_normals_cs[i].descriptor.descriptor_set, 0, TG_NULL);
            vkCmdDispatch(command_buffer, 5 * ((TG_CHUNK_VERTEX_COUNT_X - 1) / denominator) * ((TG_CHUNK_VERTEX_COUNT_X - 1) / denominator) * ((TG_CHUNK_VERTEX_COUNT_X - 1) / denominator), 1, 1);
        }
    }
    tg_vulkan_command_buffer_end_and_submit(command_buffer, &compute_queue);



    tg_vulkan_command_buffer_free(compute_command_pool, command_buffer);
    for (u32 i = 0; i < TG_VULKAN_TERRAIN_LOD_COUNT; i++)
    {
        tg_vulkan_compute_shader_destroy(&p_normals_cs[i]);
        tg_vulkan_compute_shader_destroy(&p_marching_cubes_cs[i]);
    }
    tg_vulkan_buffer_destroy(&marching_cubes_ubo);
    tg_vulkan_compute_shader_destroy(&isolevel_cs);
    tg_vulkan_buffer_destroy(&isolevel_ubo);



    terrain_chunk_entity_h->entity = tg_entity_create(TG_NULL, material_h);
    terrain_chunk_entity_h->entity.graphics_data_ptr_h->lod_count = TG_VULKAN_TERRAIN_LOD_COUNT; // TODO: find prettier solution
    for (u32 i = 0; i < TG_VULKAN_TERRAIN_LOD_COUNT; i++)
    {
        tg_entity_set_mesh(&terrain_chunk_entity_h->entity, &terrain_chunk_entity_h->p_lod_meshes[i], i);
    }



    return terrain_chunk_entity_h;
}

void tg_terrain_chunk_entity_destroy(tg_terrain_chunk_entity_h terrain_chunk_entity_h)
{
    TG_ASSERT(terrain_chunk_entity_h);

    TG_ASSERT(TG_FALSE);
}

#endif
