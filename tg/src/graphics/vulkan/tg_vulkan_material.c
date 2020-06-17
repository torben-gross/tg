#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"



tg_material_h tg_material_internal_create(tg_vulkan_material_type vulkan_material_type, tg_vertex_shader_h h_vertex_shader, tg_fragment_shader_h h_fragment_shader, u32 handle_count, tg_handle* p_handles)
{
    tg_material_h h_material = TG_MEMORY_ALLOC(sizeof(*h_material));

    h_material->type = TG_HANDLE_TYPE_MATERIAL;
    h_material->material_type = vulkan_material_type;
    h_material->h_vertex_shader = h_vertex_shader;
    h_material->h_fragment_shader = h_fragment_shader;

    if (handle_count != 0)
    {
        const u64 descriptor_set_layout_bindings_size = handle_count * sizeof(VkDescriptorSetLayoutBinding);
        VkDescriptorSetLayoutBinding* p_descriptor_set_layout_bindings = TG_MEMORY_STACK_ALLOC(descriptor_set_layout_bindings_size);
        for (u32 i = 0; i < handle_count; i++)
        {
            p_descriptor_set_layout_bindings[i].binding = i;
            p_descriptor_set_layout_bindings[i].descriptorType = tg_vulkan_handle_type_convert_to_descriptor_type(*(tg_handle_type*)p_handles[i]);
            p_descriptor_set_layout_bindings[i].descriptorCount = 1;// TODO: arrays
            p_descriptor_set_layout_bindings[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            p_descriptor_set_layout_bindings[i].pImmutableSamplers = TG_NULL;
        }
        h_material->descriptor = tg_vulkan_descriptor_create(handle_count, p_descriptor_set_layout_bindings);
        TG_MEMORY_STACK_FREE(descriptor_set_layout_bindings_size);

        for (u32 i = 0; i < handle_count; i++)
        {
            tg_vulkan_descriptor_set_update(h_material->descriptor.descriptor_set, p_handles[i], i);
        }
    }
    else
    {
        h_material->descriptor.descriptor_pool = VK_NULL_HANDLE;
        h_material->descriptor.descriptor_set_layout = VK_NULL_HANDLE;
        h_material->descriptor.descriptor_set = VK_NULL_HANDLE;
    }

    return h_material;
}



tg_material_h tg_material_create_deferred(tg_vertex_shader_h h_vertex_shader, tg_fragment_shader_h h_fragment_shader, u32 handle_count, tg_handle* p_handles)
{
    TG_ASSERT(h_vertex_shader && h_fragment_shader);

    tg_material_h h_material = tg_material_internal_create(TG_VULKAN_MATERIAL_TYPE_DEFERRED, h_vertex_shader, h_fragment_shader, handle_count, p_handles);
    return h_material;
}

tg_material_h tg_material_create_forward(tg_vertex_shader_h h_vertex_shader, tg_fragment_shader_h h_fragment_shader, u32 handle_count, tg_handle* p_handles)
{
    TG_ASSERT(h_vertex_shader && h_fragment_shader);

    tg_material_h h_material = tg_material_internal_create(TG_VULKAN_MATERIAL_TYPE_FORWARD, h_vertex_shader, h_fragment_shader, handle_count, p_handles);
    return h_material;
}

void tg_material_destroy(tg_material_h h_material)
{
    TG_ASSERT(h_material);

    tg_vulkan_descriptor_destroy(&h_material->descriptor);
	TG_MEMORY_FREE(h_material);
}

b32 tg_material_is_deferred(tg_material_h h_material)
{
    TG_ASSERT(h_material);

    const b32 result = h_material->material_type == TG_VULKAN_MATERIAL_TYPE_DEFERRED;
    return result;
}

b32 tg_material_is_forward(tg_material_h h_material)
{
    TG_ASSERT(h_material);

    const b32 result = h_material->material_type == TG_VULKAN_MATERIAL_TYPE_FORWARD;
    return result;
}

#endif
