#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"

tg_material_h tgg_material_create(tg_vertex_shader_h vertex_shader_h, tg_fragment_shader_h fragment_shader_h, u32 input_element_count, tg_shader_input_element* p_shader_input_elements, tg_handle* p_shader_input_element_handles)
{
	TG_ASSERT(vertex_shader_h && fragment_shader_h);

	tg_material_h material_h = TG_MEMORY_ALLOC(sizeof(*material_h));

    material_h->vertex_shader_h = vertex_shader_h;
    material_h->fragment_shader_h = fragment_shader_h;

    if (input_element_count != 0)
    {
        VkDescriptorSetLayoutBinding* p_descriptor_set_layout_bindings = TG_MEMORY_ALLOC(input_element_count * sizeof(*p_descriptor_set_layout_bindings));
        for (u32 i = 0; i < input_element_count; i++)
        {
            p_descriptor_set_layout_bindings[i].binding = i;
            p_descriptor_set_layout_bindings[i].descriptorType = tgg_vulkan_shader_input_element_type_convert(p_shader_input_elements[i].type);
            p_descriptor_set_layout_bindings[i].descriptorCount = p_shader_input_elements[i].array_element_count;
            p_descriptor_set_layout_bindings[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            p_descriptor_set_layout_bindings[i].pImmutableSamplers = TG_NULL;
        }
        material_h->descriptor = tgg_vulkan_descriptor_create(input_element_count, p_descriptor_set_layout_bindings);
        TG_MEMORY_FREE(p_descriptor_set_layout_bindings);

        for (u32 i = 0; i < input_element_count; i++)
        {
            tgg_vulkan_descriptor_set_update(material_h->descriptor.descriptor_set, &p_shader_input_elements[i], p_shader_input_element_handles[i], i);
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

void tgg_material_destroy(tg_material_h material_h)
{
    TG_ASSERT(material_h);

    tgg_vulkan_descriptor_destroy(&material_h->descriptor);
	TG_MEMORY_FREE(material_h);
}

#endif
