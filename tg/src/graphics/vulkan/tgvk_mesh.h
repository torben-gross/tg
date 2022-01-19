#ifndef TGVK_MESH_H
#define TGVK_MESH_H

#include "graphics/vulkan/tgvk_core.h"

#ifdef TG_VULKAN

typedef struct tg_mesh
{
    u32            index_count;
    u32            vertex_count;
    tgvk_buffer    index_buffer;
    tgvk_buffer    position_buffer;
    tgvk_buffer    normal_buffer;
    tgvk_buffer    uv_buffer;
    tgvk_buffer    tangent_buffer;
    tgvk_buffer    bitangent_buffer;
} tg_mesh;

#endif

void    tgvk_mesh_create(const char* p_filename, v3 scale, TG_OUT tg_mesh* p_mesh); // TODO: scale is temporary
void    tgvk_mesh_set_indices(tg_mesh* p_mesh, u32 count, const u16* p_indices);
void    tgvk_mesh_set_positions(tg_mesh* p_mesh, u32 count, const v3* p_positions);
void    tgvk_mesh_set_positions2(tg_mesh* p_mesh, u32 count, tgvk_buffer* p_storage_buffer);
void    tgvk_mesh_set_normals(tg_mesh* p_mesh, u32 count, const v3* p_normals);
void    tgvk_mesh_set_normals2(tg_mesh* p_mesh, u32 count, tgvk_buffer* p_storage_buffer);
void    tgvk_mesh_set_uvs(tg_mesh* p_mesh, u32 count, const v2* p_uvs);
void    tgvk_mesh_set_tangents(tg_mesh* p_mesh, u32 count, const v3* p_tangents);
void    tgvk_mesh_set_bitangents(tg_mesh* p_mesh, u32 count, const v3* p_bitangents);
void    tgvk_mesh_regenerate_normals(tg_mesh* p_mesh);
void    tgvk_mesh_regenerate_tangents_bitangents(tg_mesh* p_mesh);
//void    tgvk_mesh_voxelize(tg_mesh* p_mesh); <<< TODO
void    tgvk_mesh_destroy(tg_mesh* p_mesh);

#endif
