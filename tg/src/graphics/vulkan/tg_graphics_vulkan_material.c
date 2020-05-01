#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"



tg_material_h tg_material_internal_create(tg_vulkan_material_type vulkan_material_type, tg_vertex_shader_h vertex_shader_h, tg_fragment_shader_h fragment_shader_h, u32 handle_count, tg_handle* p_handles)
{
    tg_material_h material_h = TG_MEMORY_ALLOC(sizeof(*material_h));

    material_h->type = TG_HANDLE_TYPE_MATERIAL;
    material_h->material_type = vulkan_material_type;
    material_h->vertex_shader_h = vertex_shader_h;
    material_h->fragment_shader_h = fragment_shader_h;

    if (handle_count != 0)
    {
        VkDescriptorSetLayoutBinding* p_descriptor_set_layout_bindings = TG_MEMORY_ALLOC(handle_count * sizeof(*p_descriptor_set_layout_bindings));
        for (u32 i = 0; i < handle_count; i++)
        {
            p_descriptor_set_layout_bindings[i].binding = i;
            p_descriptor_set_layout_bindings[i].descriptorType = tg_vulkan_handle_type_convert_to_descriptor_type(*(tg_handle_type*)p_handles[i]);
            p_descriptor_set_layout_bindings[i].descriptorCount = 1;// TODO: arrays
            p_descriptor_set_layout_bindings[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            p_descriptor_set_layout_bindings[i].pImmutableSamplers = TG_NULL;
        }
        material_h->descriptor = tg_vulkan_descriptor_create(handle_count, p_descriptor_set_layout_bindings);
        TG_MEMORY_FREE(p_descriptor_set_layout_bindings);

        for (u32 i = 0; i < handle_count; i++)
        {
            tg_vulkan_descriptor_set_update(material_h->descriptor.descriptor_set, p_handles[i], i);
        }
    }
    else
    {
        material_h->descriptor.descriptor_pool = VK_NULL_HANDLE;
        material_h->descriptor.descriptor_set_layout = VK_NULL_HANDLE;
        material_h->descriptor.descriptor_set = VK_NULL_HANDLE;
    }

    return material_h;
}



tg_material_h tg_material_create_deferred(tg_vertex_shader_h vertex_shader_h, tg_fragment_shader_h fragment_shader_h, u32 handle_count, tg_handle* p_handles)
{
    TG_ASSERT(vertex_shader_h && fragment_shader_h);

    tg_material_h material_h = tg_material_internal_create(TG_VULKAN_MATERIAL_TYPE_DEFERRED, vertex_shader_h, fragment_shader_h, handle_count, p_handles);
    return material_h;
}

tg_material_h tg_material_create_forward(tg_vertex_shader_h vertex_shader_h, tg_fragment_shader_h fragment_shader_h, u32 handle_count, tg_handle* p_handles)
{
    TG_ASSERT(vertex_shader_h && fragment_shader_h);

    tg_material_h material_h = tg_material_internal_create(TG_VULKAN_MATERIAL_TYPE_FORWARD, vertex_shader_h, fragment_shader_h, handle_count, p_handles);
    return material_h;
}

void tg_material_destroy(tg_material_h material_h)
{
    TG_ASSERT(material_h);

    tg_vulkan_descriptor_destroy(&material_h->descriptor);
	TG_MEMORY_FREE(material_h);
}

b32 tg_material_is_deferred(tg_material_h material_h)
{
    TG_ASSERT(material_h);

    const b32 result = material_h->material_type == TG_VULKAN_MATERIAL_TYPE_DEFERRED;
    return result;
}

b32 tg_material_is_forward(tg_material_h material_h)
{
    TG_ASSERT(material_h);

    const b32 result = material_h->material_type == TG_VULKAN_MATERIAL_TYPE_FORWARD;
    return result;
}

#endif
