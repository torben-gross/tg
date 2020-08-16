#include "graphics/tg_graphics.h"

#ifdef TG_VULKAN

#define TG_MESH_VERTEX_CAPACITY(h_mesh)    ((u32)((h_mesh)->vbo.memory.size / sizeof(tg_vertex)))
#define TG_MESH_INDEX_CAPACITY(h_mesh)     ((u32)((h_mesh)->ibo.memory.size / sizeof(u16)))

#include "memory/tg_memory.h"
#include "tg_assets.h"
#include "util/tg_string.h"



static v3 tg__calculate_normal(v3 p0, v3 p1, v3 p2)
{
    const v3 v01 = tgm_v3_sub(p1, p0);
    const v3 v02 = tgm_v3_sub(p2, p0);
    const v3 normal = tgm_v3_normalized_not_null(tgm_v3_cross(v01, v02), V3(0.0f));
    return normal;
}

static void tg__calculate_tangent_bitangent(v3 p0, v3 p1, v3 p2, v2 uv0, v2 uv1, v2 uv2, v3* p_tangent, v3* p_bitangent)
{
    const v3 delta_p_01 = tgm_v3_sub(p1, p0);
    const v3 delta_p_02 = tgm_v3_sub(p2, p0);

    const v2 delta_uv_01 = tgm_v2_sub(uv1, uv0);
    const v2 delta_uv_02 = tgm_v2_sub(uv2, uv0);

    const f32 f = 1.0f / (delta_uv_01.x * delta_uv_02.y - delta_uv_01.y * delta_uv_02.x);

    const v3 tangent_l_part = tgm_v3_mulf(delta_p_01, delta_uv_02.y);
    const v3 tangent_r_part = tgm_v3_mulf(delta_p_02, delta_uv_01.y);
    const v3 tlp_minus_trp = tgm_v3_sub(tangent_l_part, tangent_r_part);
    *p_tangent = tgm_v3_normalized(tgm_v3_mulf(tlp_minus_trp, f));

    const v3 bitangent_l_part = tgm_v3_mulf(delta_p_02, delta_uv_01.x);
    const v3 bitangent_r_part = tgm_v3_mulf(delta_p_01, delta_uv_02.x);
    const v3 blp_minus_brp = tgm_v3_sub(bitangent_l_part, bitangent_r_part);
    *p_bitangent = tgm_v3_normalized(tgm_v3_mulf(blp_minus_brp, f));
}

static v3 tg__calculate_bitangent(v3 normal, v3 tangent)
{
    const v3 bitangent = tgm_v3_normalized(tgm_v3_cross(normal, tangent));
    return bitangent;
}

void tg__set_buffer_data(tg_vulkan_buffer* p_vulkan_buffer, u64 size, void* p_data, VkBufferUsageFlagBits buffer_type_flag)
{
    if (size && p_data)
    {
        if (size > p_vulkan_buffer->memory.size)
        {
            if (p_vulkan_buffer->buffer)
            {
                tg_vulkan_buffer_destroy(p_vulkan_buffer);
            }
            *p_vulkan_buffer = tg_vulkan_buffer_create(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | buffer_type_flag, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        }

        // TODO: i could always save the last staging buffer and see if the required size
        // is larger and potentially recreate. then i could reuse this staging buffer for
        // everything! also, resizing would be nice. same applies for the case above.
        tg_vulkan_buffer staging_buffer = tg_vulkan_buffer_create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        v3* p_it = (v3*)staging_buffer.memory.p_mapped_device_memory;
        tg_memory_copy(size, p_data, staging_buffer.memory.p_mapped_device_memory);
        tg_vulkan_buffer_flush_mapped_memory(&staging_buffer);
        tg_vulkan_buffer_copy(size, staging_buffer.buffer, p_vulkan_buffer->buffer);
        tg_vulkan_buffer_destroy(&staging_buffer);
    }
    else
    {
        tg_vulkan_buffer_destroy(p_vulkan_buffer);
        *p_vulkan_buffer = (tg_vulkan_buffer){ 0 };
    }
}

static const char* tg__skip_line(const char* p_iterator, const char* const p_eof)
{
    while (p_iterator < p_eof && *p_iterator != '\n' && *p_iterator != '\r')
    {
        p_iterator++;
    }
    if (*p_iterator == '\r')
    {
        p_iterator++;
    }
    if (*p_iterator == '\n')
    {
        p_iterator++;
    }

    return p_iterator;
}

static const char* tg__skip_token(const char* p_iterator)
{
    while (*p_iterator != '\n' && *p_iterator != '\r' && *p_iterator != ' ')
    {
        p_iterator++;
    }

    return p_iterator;
}

static const char* tg__skip_spaces(const char* p_iterator, const char* const p_eof)
{
    while (p_iterator < p_eof && *p_iterator == ' ')
    {
        p_iterator++;
    }

    return p_iterator;
}

static const char* tg__skip_whitespace(const char* p_iterator, const char* const p_eof)
{
    while (p_iterator < p_eof && (*p_iterator == '\n' || *p_iterator == '\r' || *p_iterator == ' '))
    {
        p_iterator++;
    }

    return p_iterator;
}



tg_mesh tg_mesh_create()
{
    tg_mesh mesh = { 0 };
    mesh.type = TG_STRUCTURE_TYPE_MESH;
    return mesh;
}

tg_mesh tg_mesh_create2(const char* p_filename, v3 scale) // TODO: scale is temporary
{
    TG_ASSERT(p_filename && tgm_v3_magsqr(scale) != 0.0f);

    tg_mesh mesh = { 0 };
    mesh.type = TG_STRUCTURE_TYPE_MESH;

    tg_file_properties file_properties = { 0 };
    tg_platform_file_get_properties(p_filename, &file_properties);
    char* p_data = TG_MEMORY_STACK_ALLOC(file_properties.size);
    tg_platform_file_read(p_filename, file_properties.size, p_data);

    const char* p_extension = tg_string_extract_filename_extension(p_filename);
    if (tg_string_equal(p_extension, "obj"))
    {
        const char* p_it = p_data;
        const char* const p_eof = p_data + file_properties.size;

        u32 unique_vertex_count = 0;
        u32 unique_uv_count = 0;
        u32 unique_normal_count = 0;
        u32 total_triangle_count = 0;

        while (p_it < p_eof)
        {
            p_it = tg__skip_whitespace(p_it, p_eof);

            if (*p_it == 'v')
            {
                if (p_it[1] == 't')
                {
                    unique_uv_count++;
                }
                else if (p_it[1] == 'n')
                {
                    unique_normal_count++;
                }
                else
                {
                    unique_vertex_count++;
                }
            }
            else if (*p_it == 'f')
            {
                total_triangle_count++;
            }

            p_it = tg__skip_line(p_it, p_eof);
        }

        v3* p_unique_positions = TG_MEMORY_STACK_ALLOC(unique_vertex_count * sizeof(v3));
        v3* p_unique_normals = TG_MEMORY_STACK_ALLOC(unique_normal_count * sizeof(v3));
        v2* p_unique_uvs = TG_MEMORY_STACK_ALLOC(unique_uv_count * sizeof(v2));

        v3i* p_triangle_positions = TG_MEMORY_STACK_ALLOC(total_triangle_count * sizeof(v3i));
        v3i* p_triangle_normals = TG_MEMORY_STACK_ALLOC(total_triangle_count * sizeof(v3i));
        v3i* p_triangle_uvs = TG_MEMORY_STACK_ALLOC(total_triangle_count * sizeof(v3i));

        unique_vertex_count = 0;
        unique_uv_count = 0;
        unique_normal_count = 0;
        total_triangle_count = 0;

        p_it = p_data;
        while (p_it < p_eof)
        {
            p_it = tg__skip_whitespace(p_it, p_eof);

            if (*p_it == 'v')
            {
                if (p_it[1] == 't')
                {
                    p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    p_unique_uvs[unique_uv_count].x = tg_string_to_f32(p_it); p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    p_unique_uvs[unique_uv_count].y = 1.0f - tg_string_to_f32(p_it); p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    unique_uv_count++;
                }
                else if (p_it[1] == 'n')
                {
                    p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    p_unique_normals[unique_normal_count].x = tg_string_to_f32(p_it); p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    p_unique_normals[unique_normal_count].y = tg_string_to_f32(p_it); p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    p_unique_normals[unique_normal_count].z = tg_string_to_f32(p_it); p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    unique_normal_count++;
                }
                else
                {
                    p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    p_unique_positions[unique_vertex_count].x = tg_string_to_f32(p_it); p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    p_unique_positions[unique_vertex_count].y = tg_string_to_f32(p_it); p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    p_unique_positions[unique_vertex_count].z = tg_string_to_f32(p_it); p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);
                    unique_vertex_count++;
                }
            }
            else if (*p_it == 'f')
            {
                p_it = tg__skip_whitespace(tg__skip_token(p_it), p_eof);

                for (u8 i = 0; i < 3; i++)
                {
                    p_triangle_positions[total_triangle_count].p_data[i] = tg_string_to_i32(p_it);
                    TG_ASSERT(p_triangle_positions[total_triangle_count].p_data[i] >= 0);
                    while (*p_it++ != '/');
                    p_it = tg__skip_spaces(p_it, p_eof);

                    if (*p_it != '/')
                    {
                        p_triangle_uvs[total_triangle_count].p_data[i] = tg_string_to_i32(p_it);
                        TG_ASSERT(p_triangle_uvs[total_triangle_count].p_data[i] >= 0);
                        while (*p_it >= '0' && *p_it <= '9')
                        {
                            p_it++;
                        }
                        p_it = tg__skip_spaces(p_it, p_eof);
                    }
                    else
                    {
                        p_triangle_uvs[total_triangle_count].p_data[i] = 0;
                    }

                    if (*p_it == '/')
                    {
                        p_it++;
                        p_it = tg__skip_spaces(p_it, p_eof);
                        p_triangle_normals[total_triangle_count].p_data[i] = tg_string_to_i32(p_it);
                        TG_ASSERT(p_triangle_normals[total_triangle_count].p_data[i] >= 0);
                        p_it = tg__skip_token(p_it);
                    }
                    else
                    {
                        p_triangle_normals[total_triangle_count].p_data[i] = 0;
                    }
                    p_it = tg__skip_spaces(p_it, p_eof);
                }

#ifdef TG_DEBUG
                p_it = tg__skip_spaces(p_it, p_eof);
                TG_ASSERT(*p_it == '\n' || *p_it == '\r'); // TODO: only triangulated meshes are currently supported!
#endif

                total_triangle_count++;
            }
            else
            {
                p_it = tg__skip_line(p_it, p_eof);
            }
        }

        // TODO: u16 is not enough in some cases
        //TG_ASSERT(3 * total_triangle_count < TG_U16_MAX);

        v3* p_positions = TG_MEMORY_STACK_ALLOC(3 * (u64)total_triangle_count * sizeof(v3));
        v3* p_normals = TG_MEMORY_STACK_ALLOC(3 * (u64)total_triangle_count * sizeof(v3));
        v2* p_uvs = TG_MEMORY_STACK_ALLOC(3 * (u64)total_triangle_count * sizeof(v2));

        for (u32 i = 0; i < total_triangle_count; i++)
        {
            p_positions[3 * i + 0] = p_unique_positions[p_triangle_positions[i].p_data[0] - 1];
            p_positions[3 * i + 1] = p_unique_positions[p_triangle_positions[i].p_data[1] - 1];
            p_positions[3 * i + 2] = p_unique_positions[p_triangle_positions[i].p_data[2] - 1];
            p_normals[3 * i + 0] = p_triangle_normals[i].p_data[0] == 0 ? (v3) { 0.0f, 0.0f, 0.0f } : p_unique_normals[p_triangle_normals[i].p_data[0] - 1]; // TODO: these should only be included if actually present in the given mesh, otherwise, mesh should initially be creates without them
            p_normals[3 * i + 1] = p_triangle_normals[i].p_data[1] == 0 ? (v3) { 0.0f, 0.0f, 0.0f } : p_unique_normals[p_triangle_normals[i].p_data[1] - 1];
            p_normals[3 * i + 2] = p_triangle_normals[i].p_data[2] == 0 ? (v3) { 0.0f, 0.0f, 0.0f } : p_unique_normals[p_triangle_normals[i].p_data[2] - 1];
            p_uvs[3 * i + 0] = p_triangle_uvs[i].p_data[0] == 0 ? (v2) { 0.0f, 0.0f } : p_unique_uvs[p_triangle_uvs[i].p_data[0] - 1];
            p_uvs[3 * i + 1] = p_triangle_uvs[i].p_data[1] == 0 ? (v2) { 0.0f, 0.0f } : p_unique_uvs[p_triangle_uvs[i].p_data[1] - 1];
            p_uvs[3 * i + 2] = p_triangle_uvs[i].p_data[2] == 0 ? (v2) { 0.0f, 0.0f } : p_unique_uvs[p_triangle_uvs[i].p_data[2] - 1];
        }

        for (u32 i = 0; i < 3 * total_triangle_count; i++) // TODO: this must go
        {
            p_positions[i].x = p_positions[i].x * scale.x;
            p_positions[i].y = p_positions[i].y * scale.y;
            p_positions[i].z = p_positions[i].z * scale.z;
        }

        tg_mesh_set_positions(&mesh, 3 * total_triangle_count, p_positions);
        tg_mesh_set_normals(&mesh, 3 * total_triangle_count, p_normals);
        tg_mesh_set_uvs(&mesh, 3 * total_triangle_count, p_uvs);
        // TODO: i feel like the normals are wrong, if included here! e.g. sponza.obj
        //tg_mesh_set_indices(&mesh, ..., ...);

        TG_MEMORY_STACK_FREE(3 * (u64)total_triangle_count * sizeof(v2));
        TG_MEMORY_STACK_FREE(3 * (u64)total_triangle_count * sizeof(v3));
        TG_MEMORY_STACK_FREE(3 * (u64)total_triangle_count * sizeof(v3));

        TG_MEMORY_STACK_FREE(total_triangle_count * sizeof(v3i));
        TG_MEMORY_STACK_FREE(total_triangle_count * sizeof(v3i));
        TG_MEMORY_STACK_FREE(total_triangle_count * sizeof(v3i));

        TG_MEMORY_STACK_FREE(unique_uv_count * sizeof(v2));
        TG_MEMORY_STACK_FREE(unique_normal_count * sizeof(v3));
        TG_MEMORY_STACK_FREE(unique_vertex_count * sizeof(v3));
    }

    TG_MEMORY_STACK_FREE(file_properties.size);

    return mesh;
}

tg_mesh tg_mesh_create_sphere(f32 radius, u32 sector_count, u32 stack_count, b32 normals, b32 uvs, b32 tangents_bitangents) // TODO: i should not have a copied flat version of this!
{
    const u32 vertex_count = (sector_count + 1) * (stack_count + 1) - 2;
    const u32 index_count = 6 * sector_count * (stack_count - 1);

    v3* p_positions = TG_MEMORY_STACK_ALLOC(vertex_count * sizeof(*p_positions));
    v3* p_normals = normals ? TG_MEMORY_STACK_ALLOC(vertex_count * sizeof(*p_normals)) : TG_NULL;
    v2* p_uvs = uvs ? TG_MEMORY_STACK_ALLOC(vertex_count * sizeof(*p_uvs)) : TG_NULL;
    u16* p_indices = TG_MEMORY_STACK_ALLOC((u64)index_count * sizeof(*p_indices));

    const f32 sector_step = 2.0f * TG_PI / sector_count;
    const f32 stack_step = TG_PI / stack_count;

    u32 it = 0;
    for (u32 i = 0; i < stack_count + 1; i++)
    {
        TG_ASSERT(it < vertex_count);

        const f32 stack_angle = TG_PI / 2.0f - (f32)i * stack_step;
        const f32 y = radius * tgm_f32_sin(stack_angle);
        const f32 xz = radius * tgm_f32_cos(stack_angle);

        for (u32 j = 0; j < sector_count + 1; j++)
        {
            if ((i != 0 && i != stack_count) || j != sector_count) // we need once vertex less on the poles
            {
                const f32 sector_angle = (f32)j * sector_step;

                p_positions[it].x = xz * tgm_f32_cos(sector_angle);
                p_positions[it].y = y;
                p_positions[it].z = -xz * tgm_f32_sin(sector_angle);

                if (normals)
                {
                    p_normals[it] = tgm_v3_normalized(p_positions[it]);
                }

                if (uvs)
                {
                    if (i == 0 || i == stack_count)
                    {
                        p_uvs[it] = (v2){ ((f32)j + 0.5f) / sector_count, ((f32)i + 0.5f) / stack_count };
                    }
                    else
                    {
                        p_uvs[it] = (v2){ (f32)j / sector_count, (f32)i / stack_count };
                    }
                }

                it++;
            }
        }
    }

    it = 0;
    for (u32 i = 0; i < stack_count; i++)
    {
        for (u32 j = 0; j < sector_count; j++)
        {
            const u32 i0 = j + (sector_count + 1) * i;
            const u32 i1 = j + (sector_count + 1) * (i + 1);

            // v0 - v3
            // |     |
            // v1 - v2

            const u32 iv0 = i0;
            const u32 iv1 = i1;
            const u32 iv2 = i1 + 1;
            const u32 iv3 = i0 + 1;

            if (i == 0)
            {
                p_indices[it++] = iv0;
                p_indices[it++] = iv1 - 1;
                p_indices[it++] = iv2 - 1;
            }
            else if (i == stack_count - 1)
            {
                p_indices[it++] = iv0 - 1;
                p_indices[it++] = iv1 - 1;
                p_indices[it++] = iv3 - 1;
            }
            else
            {
                p_indices[it++] = iv0 - 1;
                p_indices[it++] = iv1 - 1;
                p_indices[it++] = iv2 - 1;
                p_indices[it++] = iv2 - 1;
                p_indices[it++] = iv3 - 1;
                p_indices[it++] = iv0 - 1;
            }
        }
    }

    tg_mesh mesh = tg_mesh_create(vertex_count, p_positions, p_normals, p_uvs, TG_NULL, index_count, p_indices);
    tg_mesh_set_indices(&mesh, index_count, p_indices);
    tg_mesh_set_positions(&mesh, vertex_count, p_positions);
    if (normals)
    {
        tg_mesh_set_normals(&mesh, vertex_count, p_normals);
    }
    if (uvs)
    {
        tg_mesh_set_uvs(&mesh, vertex_count, p_uvs);
    }
    if (tangents_bitangents)
    {
        tg_mesh_regenerate_tangents_bitangents(&mesh);
    }

    TG_MEMORY_STACK_FREE((u64)index_count * sizeof(*p_indices));
    if (uvs)
    {
        TG_MEMORY_STACK_FREE(vertex_count * sizeof(*p_uvs));
    }
    if (normals)
    {
        TG_MEMORY_STACK_FREE(vertex_count * sizeof(*p_normals));
    }
    TG_MEMORY_STACK_FREE(vertex_count * sizeof(*p_positions));

    return mesh;
}

tg_mesh tg_mesh_create_sphere_flat(f32 radius, u32 sector_count, u32 stack_count, b32 normals, b32 uvs, b32 tangents_bitangents)
{
    const u32 unique_vertex_count = (sector_count + 1) * (stack_count + 1);
    v3* p_unique_positions = TG_MEMORY_STACK_ALLOC(unique_vertex_count * sizeof(*p_unique_positions));
    v2* p_unique_uvs = uvs ? TG_MEMORY_STACK_ALLOC(unique_vertex_count * sizeof(*p_unique_uvs)) : TG_NULL;

    const f32 sector_step = 2.0f * TG_PI / sector_count;
    const f32 stack_step = TG_PI / stack_count;

    u32 it = 0;
    for (u32 i = 0; i < stack_count + 1; i++)
    {
        TG_ASSERT(it < unique_vertex_count);

        const f32 stack_angle = TG_PI / 2.0f - (f32)i * stack_step;
        const f32 y = radius * tgm_f32_sin(stack_angle);
        const f32 xz = radius * tgm_f32_cos(stack_angle);

        for (u32 j = 0; j < sector_count + 1; j++)
        {
            const f32 sector_angle = (f32)j * sector_step;

            p_unique_positions[it].x = xz * tgm_f32_cos(sector_angle);
            p_unique_positions[it].y = y;
            p_unique_positions[it].z = -xz * tgm_f32_sin(sector_angle);

            if (uvs)
            {
                p_unique_uvs[it].x = (f32)j / sector_count;
                p_unique_uvs[it].y = (f32)i / stack_count;
            }

            it++;
        }
    }

    const u32 max_vertex_count = 4 * unique_vertex_count;
    const u32 max_index_count = 6 * unique_vertex_count;
    v3* p_positions = TG_MEMORY_STACK_ALLOC((u64)max_vertex_count * sizeof(*p_positions));
    v2* p_uvs = uvs ? TG_MEMORY_STACK_ALLOC((u64)max_vertex_count * sizeof(*p_uvs)) : TG_NULL;
    u16* p_indices = TG_MEMORY_STACK_ALLOC((u64)max_index_count * sizeof(*p_indices));

    u32 vertex_count = 0;
    u32 index_count = 0;
    for (u32 i = 0; i < stack_count; i++)
    {
        for (u32 j = 0; j < sector_count; j++)
        {
            const u32 i0 = j + (sector_count + 1) * i;
            const u32 i1 = j + (sector_count + 1) * (i + 1);

            // v0 - v3
            // |     |
            // v1 - v2

            const v3 vert0 = p_unique_positions[i0];
            const v3 vert1 = p_unique_positions[i1];
            const v3 vert2 = p_unique_positions[i1 + 1];
            const v3 vert3 = p_unique_positions[i0 + 1];

            v2 uv0 = { 0 };
            v2 uv1 = { 0 };
            v2 uv2 = { 0 };
            v2 uv3 = { 0 };
            if (uvs)
            {
                p_unique_uvs[i0];
                p_unique_uvs[i1];
                p_unique_uvs[i1 + 1];
                p_unique_uvs[i0 + 1];
            }

            if (i == 0)
            {
                p_positions[vertex_count] = vert0;
                p_positions[vertex_count + 1] = vert1;
                p_positions[vertex_count + 2] = vert2;

                if (uvs)
                {
                    p_uvs[vertex_count] = tgm_v2_divf(tgm_v2_add(uv0, uv3), 2.0f);
                    p_uvs[vertex_count + 1] = uv1;
                    p_uvs[vertex_count + 2] = uv2;
                }

                p_indices[index_count] = vertex_count;
                p_indices[index_count + 1] = vertex_count + 1;
                p_indices[index_count + 2] = vertex_count + 2;

                vertex_count += 3;
                index_count += 3;
            }
            else if (i == stack_count - 1)
            {
                p_positions[vertex_count] = vert0;
                p_positions[vertex_count + 1] = vert1;
                p_positions[vertex_count + 2] = vert3;

                if (uvs)
                {
                    p_uvs[vertex_count] = uv0;
                    p_uvs[vertex_count + 1] = uv1;
                    p_uvs[vertex_count + 2] = tgm_v2_divf(tgm_v2_add(uv1, uv2), 2.0f);
                }

                p_indices[index_count] = vertex_count;
                p_indices[index_count + 1] = vertex_count + 1;
                p_indices[index_count + 2] = vertex_count + 2;

                vertex_count += 3;
                index_count += 3;
            }
            else
            {
                p_positions[vertex_count] = vert0;
                p_positions[vertex_count + 1] = vert1;
                p_positions[vertex_count + 2] = vert2;
                p_positions[vertex_count + 3] = vert3;

                if (uvs)
                {
                    p_uvs[vertex_count] = uv0;
                    p_uvs[vertex_count + 1] = uv1;
                    p_uvs[vertex_count + 2] = uv2;
                    p_uvs[vertex_count + 3] = uv3;
                }

                p_indices[index_count] = vertex_count;
                p_indices[index_count + 1] = vertex_count + 1;
                p_indices[index_count + 2] = vertex_count + 2;
                p_indices[index_count + 3] = vertex_count + 2;
                p_indices[index_count + 4] = vertex_count + 3;
                p_indices[index_count + 5] = vertex_count;

                vertex_count += 4;
                index_count += 6;
            }
        }
    }

    tg_mesh mesh = tg_mesh_create(vertex_count, p_positions, TG_NULL, p_uvs, TG_NULL, index_count, p_indices);
    tg_mesh_set_indices(&mesh, index_count, p_indices);
    tg_mesh_set_positions(&mesh, vertex_count, p_positions);
    if (normals)
    {
        tg_mesh_regenerate_normals(&mesh);
    }
    if (uvs)
    {
        tg_mesh_set_uvs(&mesh, vertex_count, p_uvs);
    }
    if (tangents_bitangents)
    {
        tg_mesh_regenerate_tangents_bitangents(&mesh);
    }

    TG_MEMORY_STACK_FREE((u64)max_index_count * sizeof(*p_indices));
    if (uvs)
    {
        TG_MEMORY_STACK_FREE((u64)max_vertex_count * sizeof(*p_uvs));
    }
    TG_MEMORY_STACK_FREE((u64)max_vertex_count * sizeof(*p_positions));

    if (uvs)
    {
        TG_MEMORY_STACK_FREE(unique_vertex_count * sizeof(*p_unique_uvs));
    }
    TG_MEMORY_STACK_FREE(unique_vertex_count * sizeof(*p_unique_positions));

    return mesh;
}

void tg_mesh_set_indices(tg_mesh* p_mesh, u32 count, const u16* p_indices)
{
    TG_ASSERT(p_mesh && ((!count && !p_indices) || (count && p_indices)));

    p_mesh->index_count = p_indices ? count : 0;
    tg__set_buffer_data(&p_mesh->index_buffer, count * sizeof(*p_indices), (void*)p_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    if (count && p_indices)
    {
        if (!p_mesh->p_indices)
        {
            p_mesh->index_capacity = count;
            p_mesh->p_indices = TG_MEMORY_ALLOC(count * sizeof(*p_indices));
        }
        else if (count > p_mesh->index_capacity)
        {
            p_mesh->index_capacity = count;
            p_mesh->p_indices = TG_MEMORY_REALLOC(count * sizeof(*p_indices), p_mesh->p_indices);
        }
        tg_memory_copy(count * sizeof(*p_indices), p_indices, p_mesh->p_indices);
    }
}

void tg_mesh_set_positions(tg_mesh* p_mesh, u32 count, const v3* p_positions) // TODO: all of these setters must work AFTER a render command has been created!
{
    TG_ASSERT(p_mesh && ((!count && !p_positions) || (count && p_positions)));

    p_mesh->position_count = p_positions ? count : 0;
    tg__set_buffer_data(&p_mesh->positions_buffer, count * sizeof(*p_positions), (void*)p_positions, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    if (count && p_positions)
    {
        p_mesh->bounds.min = (v3){ p_positions[0].x, p_positions[0].y, p_positions[0].z };
        p_mesh->bounds.max = (v3){ p_positions[0].x, p_positions[0].y, p_positions[0].z };
        for (u32 i = 1; i < count; i++)
        {
            p_mesh->bounds.min = tgm_v3_min(p_mesh->bounds.min, p_positions[i]);
            p_mesh->bounds.max = tgm_v3_max(p_mesh->bounds.max, p_positions[i]);
        }
        if (!p_mesh->p_positions)
        {
            p_mesh->position_capacity = count;
            p_mesh->p_positions = TG_MEMORY_ALLOC(count * sizeof(*p_positions));
        }
        else if (count > p_mesh->position_capacity)
        {
            p_mesh->position_capacity = count;
            p_mesh->p_positions = TG_MEMORY_REALLOC(count * sizeof(*p_positions), p_mesh->p_positions);
        }
        tg_memory_copy(count * sizeof(*p_positions), p_positions, p_mesh->p_positions);
    }
}

void tg_mesh_set_normals(tg_mesh* p_mesh, u32 count, const v3* p_normals)
{
    TG_ASSERT(p_mesh && ((!count && !p_normals) || (count && p_normals)));

    p_mesh->normal_count = p_normals ? count : 0;
    tg__set_buffer_data(&p_mesh->normals_buffer, count * sizeof(*p_normals), (void*)p_normals, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void tg_mesh_set_uvs(tg_mesh* p_mesh, u32 count, const v2* p_uvs)
{
    TG_ASSERT(p_mesh && ((!count && !p_uvs) || (count && p_uvs)));

    p_mesh->uv_count = p_uvs ? count : 0;
    tg__set_buffer_data(&p_mesh->uvs_buffer, count * sizeof(*p_uvs), (void*)p_uvs, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void tg_mesh_set_tangents_bitangents(tg_mesh* p_mesh, u32 count, const v3* p_tangents, const v3* p_bitangents)
{
    TG_ASSERT(p_mesh && ((!count && !p_tangents && !p_bitangents) || (count && p_tangents && p_bitangents)));

    p_mesh->tangent_count = p_tangents ? count : 0;
    p_mesh->bitangent_count = p_bitangents ? count : 0;
    tg__set_buffer_data(&p_mesh->tangents_buffer, count * sizeof(*p_tangents), (void*)p_tangents, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    tg__set_buffer_data(&p_mesh->bitangents_buffer, count * sizeof(*p_bitangents), (void*)p_bitangents, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void tg_mesh_regenerate_normals(tg_mesh* p_mesh)
{
    TG_ASSERT(p_mesh);

    v3* p_normals = TG_MEMORY_STACK_ALLOC(p_mesh->position_count * sizeof(*p_normals));

    if (p_mesh->index_count != 0)
    {
        for (u32 i = 0; i < p_mesh->index_count; i += 3)
        {
            // TODO: these will override normals, that have been set before. this should be interpolated (only relevant if indices exits).
            const v3 normal = tg__calculate_normal(p_mesh->p_positions[p_mesh->p_indices[i]], p_mesh->p_positions[p_mesh->p_indices[i + 1]], p_mesh->p_positions[p_mesh->p_indices[i + 2]]);
            p_normals[p_mesh->p_indices[i    ]] = normal;
            p_normals[p_mesh->p_indices[i + 1]] = normal;
            p_normals[p_mesh->p_indices[i + 2]] = normal;
        }
    }
    else
    {
#if 0
        // TODO: most of this can be created only once instead of per mesh
        tg_vulkan_shader compute_shader = { 0 };
        tg_vulkan_pipeline compute_pipeline = { 0 };
        VkCommandBuffer command_buffer = VK_NULL_HANDLE;



        compute_shader = ((tg_compute_shader_h)tg_assets_get_asset("shaders/normals_vbo.comp"))->vulkan_shader;

        VkDescriptorSetLayoutBinding p_descriptor_set_layout_bindings[2] = { 0 };

        p_descriptor_set_layout_bindings[0].binding = 0;
        p_descriptor_set_layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        p_descriptor_set_layout_bindings[0].descriptorCount = 1;
        p_descriptor_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        p_descriptor_set_layout_bindings[0].pImmutableSamplers = TG_NULL;

        p_descriptor_set_layout_bindings[1].binding = 1;
        p_descriptor_set_layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        p_descriptor_set_layout_bindings[1].descriptorCount = 1;
        p_descriptor_set_layout_bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        p_descriptor_set_layout_bindings[1].pImmutableSamplers = TG_NULL;

        compute_pipeline = tg_vulkan_pipeline_create_compute(&compute_shader);
        command_buffer = tg_vulkan_command_buffer_allocate(TG_VULKAN_COMMAND_POOL_TYPE_COMPUTE, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        uniform_buffer = tg_vulkan_buffer_create(sizeof(tg_normals_compute_shader_uniform_buffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        tg_vulkan_descriptor_set_update_storage_buffer(compute_pipeline.descriptor_set, p_staging_buffer->buffer, 0);
        tg_vulkan_descriptor_set_update_uniform_buffer(compute_pipeline.descriptor_set, uniform_buffer.buffer, 1);

        tg_vulkan_command_buffer_begin(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
        {
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline.pipeline);
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline.pipeline_layout, 0, 1, &compute_pipeline.descriptor_set, 0, TG_NULL);
            vkCmdDispatch(command_buffer, vertex_count, 1, 1);
        }
        tg_vulkan_command_buffer_end_and_submit(command_buffer, TG_VULKAN_QUEUE_TYPE_COMPUTE);



        tg_vulkan_buffer_destroy(&uniform_buffer);
        tg_vulkan_command_buffer_free(TG_VULKAN_COMMAND_POOL_TYPE_COMPUTE, command_buffer);
        tg_vulkan_pipeline_destroy(&compute_pipeline);
#else
        for (u32 i = 0; i < p_mesh->position_count; i += 3)
        {
            const v3 normal = tg__calculate_normal(p_mesh->p_positions[i], p_mesh->p_positions[i + 1], p_mesh->p_positions[i + 2]);
            p_normals[i] = normal;
            p_normals[i + 1] = normal;
            p_normals[i + 2] = normal;
        }
#endif
    }

    tg_mesh_set_normals(p_mesh, p_mesh->position_count, p_normals);

    TG_MEMORY_STACK_FREE(p_mesh->position_count * sizeof(*p_normals));
}

void tg_mesh_regenerate_tangents_bitangents(tg_mesh* p_mesh)
{
    TG_INVALID_CODEPATH();
    TG_ASSERT(p_mesh->index_count == 0);

    TG_ASSERT(p_mesh);

    for (u32 i = 0; i < p_mesh->position_count; i += 3)
    {
        // TODO: this
        //tg__calculate_tangent_bitangent(&p_vertices[i + 0], &p_vertices[i + 1], &p_vertices[i + 2]);
    }
}

void tg_mesh_destroy(tg_mesh* p_mesh)
{
    TG_ASSERT(p_mesh);

    if (p_mesh->positions_buffer.buffer)
    {
        tg_vulkan_buffer_destroy(&p_mesh->positions_buffer);
    }
    if (p_mesh->p_positions)
    {
        TG_MEMORY_FREE(p_mesh->p_positions);
    }
    if (p_mesh->normals_buffer.buffer)
    {
        tg_vulkan_buffer_destroy(&p_mesh->normals_buffer);
    }
    if (p_mesh->uvs_buffer.buffer)
    {
        tg_vulkan_buffer_destroy(&p_mesh->uvs_buffer);
    }
    if (p_mesh->tangents_buffer.buffer)
    {
        tg_vulkan_buffer_destroy(&p_mesh->tangents_buffer);
    }
    if (p_mesh->bitangents_buffer.buffer)
    {
        tg_vulkan_buffer_destroy(&p_mesh->bitangents_buffer);
    }
    if (p_mesh->p_indices)
    {
        TG_MEMORY_FREE(p_mesh->p_indices);
    }
    *p_mesh = (tg_mesh){ 0 };
}

#endif
