#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "graphics/vulkan/tg_graphics_vulkan_shader.h"
#include "memory/tg_memory_allocator.h"

tg_material_h tgg_material_create(tg_vertex_shader_h vertex_shader_h, tg_fragment_shader_h fragment_shader_h, u32 input_element_count, tg_shader_input_element* p_input_elements, void** pp_input_element_handles)
{
	TG_ASSERT(vertex_shader_h && fragment_shader_h);

	tg_material_h material_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(*material_h));

    material_h->vertex_shader_h = vertex_shader_h;
    material_h->fragment_shader_h = fragment_shader_h;

    if (input_element_count != 0)
    {
        VkDescriptorPoolSize* p_descriptor_pool_sizes = TG_MEMORY_ALLOCATOR_ALLOCATE(input_element_count * sizeof(*p_descriptor_pool_sizes));
        for (u32 i = 0; i < input_element_count; i++)
        {
            p_descriptor_pool_sizes[i].type = tgg_vulkan_shader_input_element_type_convert(p_input_elements[i].type);
            p_descriptor_pool_sizes[i].descriptorCount = p_input_elements[i].array_element_count;
        }
        material_h->descriptor_pool = tgg_vulkan_descriptor_pool_create(0, 1, input_element_count, p_descriptor_pool_sizes);
        TG_MEMORY_ALLOCATOR_FREE(p_descriptor_pool_sizes);

        VkDescriptorSetLayoutBinding* p_descriptor_set_layout_bindings = TG_MEMORY_ALLOCATOR_ALLOCATE(input_element_count * sizeof(*p_descriptor_set_layout_bindings));
        for (u32 i = 0; i < input_element_count; i++)
        {
            p_descriptor_set_layout_bindings[i].binding = i;
            p_descriptor_set_layout_bindings[i].descriptorType = tgg_vulkan_shader_input_element_type_convert(p_input_elements[i].type);
            p_descriptor_set_layout_bindings[i].descriptorCount = p_input_elements[i].array_element_count;
            p_descriptor_set_layout_bindings[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            p_descriptor_set_layout_bindings[i].pImmutableSamplers = TG_NULL;
        }
        material_h->descriptor_set_layout = tgg_vulkan_descriptor_set_layout_create(0, input_element_count, p_descriptor_set_layout_bindings);
        TG_MEMORY_ALLOCATOR_FREE(p_descriptor_set_layout_bindings);

        material_h->descriptor_set = tgg_vulkan_descriptor_set_allocate(material_h->descriptor_pool, material_h->descriptor_set_layout);

        for (u32 i = 0; i < input_element_count; i++)
        {
            switch (p_input_elements[i].type)
            {
            case TG_SHADER_INPUT_ELEMENT_TYPE_COMPUTE_BUFFER:
            {
                tg_compute_buffer_h compute_buffer_h = ((tg_compute_buffer_h*)pp_input_element_handles)[i];
                tgg_vulkan_descriptor_set_update_storage_buffer(material_h->descriptor_set, compute_buffer_h->buffer.buffer, i, p_input_elements[i].array_element_count);
            } break;
            case TG_SHADER_INPUT_ELEMENT_TYPE_UNIFORM_BUFFER:
            {
                tg_uniform_buffer_h uniform_buffer_h = ((tg_uniform_buffer_h*)pp_input_element_handles)[i];
                tgg_vulkan_descriptor_set_update_uniform_buffer(material_h->descriptor_set, uniform_buffer_h->buffer.buffer, i, p_input_elements[i].array_element_count);
            } break;
            default: TG_ASSERT(TG_FALSE);
            }
        }
    }
    else
    {
        material_h->descriptor_pool = VK_NULL_HANDLE;
        material_h->descriptor_set_layout = VK_NULL_HANDLE;
        material_h->descriptor_set = VK_NULL_HANDLE;
    }

    return material_h;
}

void tgg_material_destroy(tg_material_h material_h)
{
    TG_ASSERT(material_h);

	TG_MEMORY_ALLOCATOR_FREE(material_h);
}

#endif
