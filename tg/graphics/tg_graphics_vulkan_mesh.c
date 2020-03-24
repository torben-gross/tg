#include "tg_graphics_vulkan.h"

#ifdef TG_VULKAN

void tg_graphics_mesh_create(ui32 vertex_count, const tgm_vec3f* positions, const tgm_vec3f* normals, const tgm_vec2f* uvs, ui32 index_count, const ui16* indices, tg_mesh_h* p_mesh_h)
{
	TG_ASSERT(vertex_count && positions && normals && uvs && p_mesh_h);
	TG_ASSERT(index_count && indices); // TODO: if indices 0, dont use index buffer! remove assertion


}

void tg_graphics_mesh_destroy(tg_mesh_h mesh_h)
{

}

#endif
