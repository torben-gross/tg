#include "tg/graphics/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "tg/platform/tg_allocator.h"
#include "tg/util/tg_file_io.h"

void tg_graphics_vertex_shader_create(const char* filename, ui32 layout_element_count, const tg_vertex_shader_layout_element* layout, tg_vertex_shader_h* p_vertex_shader_h)
{
    TG_ASSERT(p_vertex_shader_h);

    *p_vertex_shader_h = tg_allocator_allocate(sizeof(**p_vertex_shader_h));

    ui64 size = 0;
    char* content = NULL;
    tg_file_io_read(filename, &size, &content);

    VkShaderModuleCreateInfo shader_module_create_info = { 0 };
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.pNext = NULL;
    shader_module_create_info.flags = 0;
    shader_module_create_info.codeSize = size;
    shader_module_create_info.pCode = (const ui32*)content;

    VK_CALL(vkCreateShaderModule(device, &shader_module_create_info, NULL, &(**p_vertex_shader_h).shader_module));
    tg_file_io_free(content);

    if (layout_element_count == 0)
    {
        (**p_vertex_shader_h).layout_element_count = 0;
        (**p_vertex_shader_h).layout = NULL;
    }
    else
    {
        (**p_vertex_shader_h).layout_element_count = layout_element_count;
        const ui32 layout_size = layout_element_count * sizeof(*layout);
        (**p_vertex_shader_h).layout = tg_allocator_allocate(layout_size);
        memcpy((**p_vertex_shader_h).layout, layout, layout_size);
    }
}

void tg_graphics_vertex_shader_destroy(tg_vertex_shader_h vertex_shader_h)
{
    if (vertex_shader_h->layout_element_count != 0)
    {
        tg_allocator_free(vertex_shader_h->layout);
    }
    vkDestroyShaderModule(device, vertex_shader_h->shader_module, NULL);
    tg_allocator_free(vertex_shader_h);
}

void tg_graphics_fragment_shader_create(const char* filename, tg_fragment_shader_h* p_fragment_shader_h)
{
    TG_ASSERT(p_fragment_shader_h);

    *p_fragment_shader_h = tg_allocator_allocate(sizeof(**p_fragment_shader_h));

    ui64 size = 0;
    char* content = NULL;
    tg_file_io_read(filename, &size, &content);

    VkShaderModuleCreateInfo shader_module_create_info = { 0 };
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.pNext = NULL;
    shader_module_create_info.flags = 0;
    shader_module_create_info.codeSize = size;
    shader_module_create_info.pCode = (const ui32*)content;

    VK_CALL(vkCreateShaderModule(device, &shader_module_create_info, NULL, &(**p_fragment_shader_h).shader_module));
    tg_file_io_free(content);
}

void tg_graphics_fragment_shader_destroy(tg_fragment_shader_h fragment_shader_h)
{
    vkDestroyShaderModule(device, fragment_shader_h->shader_module, NULL);
    tg_allocator_free(fragment_shader_h);
}

#endif
