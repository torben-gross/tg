#include "graphics/vulkan/tgvk_mesh.h"

#ifdef TG_VULKAN

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

static void tg__copy(VkDeviceSize offset, VkDeviceSize size, tgvk_buffer* p_src, void* p_dst)
{
    tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(size);

    tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);

    VkBufferCopy buffer_copy = { 0 };
    buffer_copy.srcOffset = offset;
    buffer_copy.dstOffset = 0;
    buffer_copy.size = size;

    vkCmdCopyBuffer(p_command_buffer->buffer, p_src->buffer, p_staging_buffer->buffer, 1, &buffer_copy);
    tgvk_command_buffer_end_and_submit(p_command_buffer);
    tg_memcpy(size, p_staging_buffer->memory.p_mapped_device_memory, p_dst);

    tgvk_global_staging_buffer_release();
}

static void tg__set_buffer_data(tgvk_buffer* p_buffer, tg_size size, const void* p_data, VkBufferUsageFlagBits buffer_type_flag)
{
    if (size && p_data)
    {
        if (size > p_buffer->memory.size)
        {
            if (p_buffer->buffer)
            {
                tgvk_buffer_destroy(p_buffer);
            }
            *p_buffer = TGVK_BUFFER_CREATE(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | buffer_type_flag, TGVK_MEMORY_DEVICE);
        }

        tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(size);
        tg_memcpy(size, p_data, p_staging_buffer->memory.p_mapped_device_memory);
        tgvk_buffer_copy(size, p_staging_buffer, p_buffer);
        tgvk_global_staging_buffer_release();
    }
    else
    {
        tgvk_buffer_destroy(p_buffer);
        *p_buffer = (tgvk_buffer){ 0 };
    }
}

static void tg__set_buffer_data2(tgvk_buffer* p_buffer, tg_size size, tgvk_buffer* p_storage_buffer, VkBufferUsageFlagBits buffer_type_flag)
{
    if (size && p_storage_buffer)
    {
        if (size > p_buffer->memory.size)
        {
            if (p_buffer->buffer)
            {
                tgvk_buffer_destroy(p_buffer);
            }
            *p_buffer = TGVK_BUFFER_CREATE(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | buffer_type_flag, TGVK_MEMORY_DEVICE);
        }
        tgvk_buffer_copy(size, p_storage_buffer, p_buffer);
    }
    else
    {
        tgvk_buffer_destroy(p_buffer);
        *p_buffer = (tgvk_buffer){ 0 };
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



void tgvk_mesh_create(const char* p_filename, v3 scale, TG_OUT tg_mesh* p_mesh) // TODO: scale is temporary
{
    TG_ASSERT(p_filename);
    TG_ASSERT(tgm_v3_magsqr(scale) > 0.0f);

    tg_file_properties file_properties = { 0 };
    tgp_file_get_properties(p_filename, &file_properties);
    char* p_data = TG_MALLOC_STACK(file_properties.size);
    tgp_file_load(p_filename, file_properties.size, p_data);

    if (tg_string_equal(file_properties.p_extension, "obj"))
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

        v3* p_unique_positions = TG_MALLOC_STACK(unique_vertex_count * sizeof(v3));
        v3* p_unique_normals = TG_MALLOC_STACK(unique_normal_count * sizeof(v3));
        v2* p_unique_uvs = unique_uv_count == 0 ? TG_NULL : TG_MALLOC_STACK(unique_uv_count * sizeof(v2));

        v3i* p_triangle_positions = TG_MALLOC_STACK(total_triangle_count * sizeof(v3i));
        v3i* p_triangle_normals = TG_MALLOC_STACK(total_triangle_count * sizeof(v3i));
        v3i* p_triangle_uvs = TG_MALLOC_STACK(total_triangle_count * sizeof(v3i));

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

        v3* p_positions = TG_MALLOC_STACK(3LL * (tg_size)total_triangle_count * sizeof(v3));
        v3* p_normals = TG_MALLOC_STACK(3LL * (tg_size)total_triangle_count * sizeof(v3));
        v2* p_uvs = TG_MALLOC_STACK(3LL * (tg_size)total_triangle_count * sizeof(v2));

        for (u32 i = 0; i < total_triangle_count; i++)
        {
            p_positions[3 * i + 0] = p_unique_positions[p_triangle_positions[i].p_data[0] - 1];
            p_positions[3 * i + 1] = p_unique_positions[p_triangle_positions[i].p_data[1] - 1];
            p_positions[3 * i + 2] = p_unique_positions[p_triangle_positions[i].p_data[2] - 1];
            p_normals[3 * i + 0] = p_triangle_normals[i].p_data[0] == 0 ? (v3) { 0.0f, 0.0f, 0.0f } : p_unique_normals[p_triangle_normals[i].p_data[0] - 1]; // TODO: these should only be included if actually present in the given mesh, otherwise, mesh should initially be creates without them
            p_normals[3 * i + 1] = p_triangle_normals[i].p_data[1] == 0 ? (v3) { 0.0f, 0.0f, 0.0f } : p_unique_normals[p_triangle_normals[i].p_data[1] - 1]; // TODO: same goes for uvs
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

        tgvk_mesh_set_positions(p_mesh, 3 * total_triangle_count, p_positions);
        tgvk_mesh_set_normals(p_mesh, 3 * total_triangle_count, p_normals);
        tgvk_mesh_set_uvs(p_mesh, 3 * total_triangle_count, p_uvs);
        // TODO: i feel like the normals are wrong, if included here! e.g. sponza.obj
        //tgvk_mesh_set_indices(&mesh, ..., ...);

        TG_FREE_STACK(3LL * (tg_size)total_triangle_count * sizeof(v2));
        TG_FREE_STACK(3LL * (tg_size)total_triangle_count * sizeof(v3));
        TG_FREE_STACK(3LL * (tg_size)total_triangle_count * sizeof(v3));

        TG_FREE_STACK(total_triangle_count * sizeof(v3i));
        TG_FREE_STACK(total_triangle_count * sizeof(v3i));
        TG_FREE_STACK(total_triangle_count * sizeof(v3i));

        if (unique_uv_count != 0)
        {
            TG_FREE_STACK(unique_uv_count * sizeof(v2));
        }
        TG_FREE_STACK(unique_normal_count * sizeof(v3));
        TG_FREE_STACK(unique_vertex_count * sizeof(v3));
    }

    TG_FREE_STACK(file_properties.size);
}

void tgvk_mesh_copy_indices(tg_mesh* p_mesh, u32 first, u32 count, u16* p_buffer)
{
    TG_ASSERT(p_mesh && p_mesh->index_buffer.buffer);

    const VkDeviceSize offset = first * sizeof(u16);
    const VkDeviceSize size = tgm_u32_min(count, p_mesh->index_count) * sizeof(u16);
    tg__copy(offset, size, &p_mesh->index_buffer, p_buffer);
}

void tgvk_mesh_copy_positions(tg_mesh* p_mesh, u32 first, u32 count, v3* p_buffer)
{
    TG_ASSERT(p_mesh && p_mesh->position_buffer.buffer);

    const VkDeviceSize offset = first * sizeof(v3);
    const VkDeviceSize size = tgm_u32_min(count, p_mesh->vertex_count) * sizeof(v3);
    tg__copy(offset, size, &p_mesh->position_buffer, p_buffer);
}

void tgvk_mesh_copy_normals(tg_mesh* p_mesh, u32 first, u32 count, v3* p_buffer)
{
    TG_ASSERT(p_mesh && p_mesh->normal_buffer.buffer);

    const VkDeviceSize offset = first * sizeof(v3);
    const VkDeviceSize size = tgm_u32_min(count, p_mesh->vertex_count) * sizeof(v3);
    tg__copy(offset, size, &p_mesh->normal_buffer, p_buffer);
}

void tgvk_mesh_copy_uvs(tg_mesh* p_mesh, u32 first, u32 count, v2* p_buffer)
{
    TG_ASSERT(p_mesh && p_mesh->uv_buffer.buffer);

    const VkDeviceSize offset = first * sizeof(v2);
    const VkDeviceSize size = tgm_u32_min(count, p_mesh->vertex_count) * sizeof(v2);
    tg__copy(offset, size, &p_mesh->uv_buffer, p_buffer);
}

void tgvk_mesh_copy_tangents(tg_mesh* p_mesh, u32 first, u32 count, v3* p_buffer)
{
    TG_ASSERT(p_mesh && p_mesh->tangent_buffer.buffer);

    const VkDeviceSize offset = first * sizeof(v3);
    const VkDeviceSize size = tgm_u32_min(count, p_mesh->vertex_count) * sizeof(v3);
    tg__copy(offset, size, &p_mesh->tangent_buffer, p_buffer);
}

void tgvk_mesh_copy_bitangents(tg_mesh* p_mesh, u32 first, u32 count, v3* p_buffer)
{
    TG_ASSERT(p_mesh && p_mesh->bitangent_buffer.buffer);

    const VkDeviceSize offset = first * sizeof(v3);
    const VkDeviceSize size = tgm_u32_min(count, p_mesh->vertex_count) * sizeof(v3);
    tg__copy(offset, size, &p_mesh->bitangent_buffer, p_buffer);
}

void tgvk_mesh_set_indices(tg_mesh* p_mesh, u32 count, const u16* p_indices)
{
    TG_ASSERT(p_mesh);
    TG_ASSERT((!count && !p_indices) || (count && p_indices));

    p_mesh->index_count = p_indices ? count : 0;
    tg__set_buffer_data(&p_mesh->index_buffer, count * sizeof(*p_indices), (void*)p_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void tgvk_mesh_set_positions(tg_mesh* p_mesh, u32 count, const v3* p_positions) // TODO: all of these setters must work AFTER a render command has been created!
{
    TG_ASSERT(p_mesh);
    TG_ASSERT((!count && !p_positions) || (count && p_positions));

    p_mesh->vertex_count = p_positions ? count : 0;
    tg__set_buffer_data(&p_mesh->position_buffer, count * sizeof(*p_positions), (const void*)p_positions, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void tgvk_mesh_set_positions2(tg_mesh* p_mesh, u32 count, tgvk_buffer* p_storage_buffer)
{
    TG_ASSERT(p_mesh);
    TG_ASSERT(count);
    TG_ASSERT(p_storage_buffer);
    TG_ASSERT(p_storage_buffer->memory.size >= count * sizeof(v3));

    p_mesh->vertex_count = count;
    tg__set_buffer_data2(&p_mesh->position_buffer, count * sizeof(v3), p_storage_buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void tgvk_mesh_set_normals(tg_mesh* p_mesh, u32 count, const v3* p_normals)
{
    TG_ASSERT(p_mesh);
    TG_ASSERT((!count && !p_normals) || (count && p_normals));

    tg__set_buffer_data(&p_mesh->normal_buffer, count * sizeof(*p_normals), (void*)p_normals, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void tgvk_mesh_set_normals2(tg_mesh* p_mesh, u32 count, tgvk_buffer* p_storage_buffer)
{
    TG_ASSERT(p_mesh);
    TG_ASSERT(count);
    TG_ASSERT(p_storage_buffer);
    TG_ASSERT(p_storage_buffer->memory.size >= count * sizeof(v3));

    tg__set_buffer_data2(&p_mesh->normal_buffer, count * sizeof(v3), p_storage_buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void tgvk_mesh_set_uvs(tg_mesh* p_mesh, u32 count, const v2* p_uvs)
{
    TG_ASSERT(p_mesh);
    TG_ASSERT((!count && !p_uvs) || (count && p_uvs));

    tg__set_buffer_data(&p_mesh->uv_buffer, count * sizeof(*p_uvs), (void*)p_uvs, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void tgvk_mesh_set_tangents(tg_mesh* p_mesh, u32 count, const v3* p_tangents)
{
    TG_ASSERT(p_mesh);
    TG_ASSERT((!count && !p_tangents) || (count && p_tangents));

    tg__set_buffer_data(&p_mesh->tangent_buffer, count * sizeof(*p_tangents), (void*)p_tangents, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void tgvk_mesh_set_bitangents(tg_mesh* p_mesh, u32 count, const v3* p_bitangents)
{
    TG_ASSERT(p_mesh);
    TG_ASSERT((!count && !p_bitangents) || (count && p_bitangents));

    tg__set_buffer_data(&p_mesh->bitangent_buffer, count * sizeof(*p_bitangents), (void*)p_bitangents, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void tgvk_mesh_regenerate_normals(tg_mesh* p_mesh)
{
    TG_ASSERT(p_mesh);

    TG_INVALID_CODEPATH(); // TODO: create tgvk_mesh_generate_normals(u32 count, const v3* p_positions, v3* p_normals_buffer); instead!

    v3* p_positions = TG_MALLOC_STACK(p_mesh->vertex_count * sizeof(*p_positions));
    tgvk_mesh_copy_positions(p_mesh, 0, p_mesh->vertex_count, p_positions);

    v3* p_normals = TG_MALLOC_STACK(p_mesh->vertex_count * sizeof(*p_normals));

    if (p_mesh->index_count != 0)
    {
        u16* p_indices = TG_MALLOC_STACK(p_mesh->index_count * sizeof(*p_indices));
        tgvk_mesh_copy_indices(p_mesh, 0, p_mesh->index_count, p_indices);

        for (u32 i = 0; i < p_mesh->index_count; i += 3)
        {
            // TODO: these will override normals, that have been set before. this should be interpolated (only relevant if indices exits).
            const v3 normal = tg__calculate_normal(p_positions[p_indices[i]], p_positions[p_indices[i + 1]], p_positions[p_indices[i + 2]]);
            p_normals[p_indices[i    ]] = normal;
            p_normals[p_indices[i + 1]] = normal;
            p_normals[p_indices[i + 2]] = normal;
        }

        TG_FREE_STACK(p_mesh->index_count * sizeof(*p_indices));
    }
    else
    {
#if 0 // TODO: reimplement this!
        // TODO: most of this can be created only once instead of per mesh
        tgvk_shader compute_shader = { 0 };
        tgvk_pipeline compute_pipeline = { 0 };



        compute_shader = ((tg_compute_shader_h)tg_assets_get_asset("shaders/normals_vbo.comp"))->shader;

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

        compute_pipeline = tgvk_pipeline_create_compute(&compute_shader);
        uniform_buffer = tgvk_uniform_buffer_create(sizeof(tg_normals_compute_shader_uniform_buffer));

        tgvk_descriptor_set_update_storage_buffer(compute_pipeline.descriptor_set, p_staging_buffer->buffer, 0);
        tgvk_descriptor_set_update_uniform_buffer(compute_pipeline.descriptor_set, uniform_buffer.buffer, 1);

        tgvk_command_buffer_begin(global_compute_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, TG_NULL);
        {
            vkCmdBindPipeline(global_compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline.pipeline);
            vkCmdBindDescriptorSets(global_compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline.pipeline_layout, 0, 1, &compute_pipeline.descriptor_set, 0, TG_NULL);
            vkCmdDispatch(global_compute_command_buffer, vertex_count, 1, 1);
        }
        tgvk_command_buffer_end_and_submit(global_compute_command_buffer, TGVK_QUEUE_TYPE_COMPUTE);



        tgvk_buffer_destroy(&uniform_buffer);
        tgvk_pipeline_destroy(&compute_pipeline);
#else
        for (u32 i = 0; i < p_mesh->vertex_count; i += 3)
        {
            const v3 normal = tg__calculate_normal(p_positions[i], p_positions[i + 1], p_positions[i + 2]);
            p_normals[i] = normal;
            p_normals[i + 1] = normal;
            p_normals[i + 2] = normal;
        }
#endif
    }

    tgvk_mesh_set_normals(p_mesh, p_mesh->vertex_count, p_normals);

    TG_FREE_STACK(p_mesh->vertex_count * sizeof(*p_normals));
    TG_FREE_STACK(p_mesh->vertex_count * sizeof(*p_positions));
}

void tgvk_mesh_regenerate_tangents_bitangents(tg_mesh* p_mesh)
{
    TG_INVALID_CODEPATH();
    TG_ASSERT(p_mesh->index_count == 0);

    TG_ASSERT(p_mesh);

    for (u32 i = 0; i < p_mesh->vertex_count; i += 3)
    {
        // TODO: this
        //tg__calculate_tangent_bitangent(&p_vertices[i + 0], &p_vertices[i + 1], &p_vertices[i + 2]);
    }
}

void tgvk_mesh_destroy(tg_mesh* p_mesh)
{
    TG_ASSERT(p_mesh);

    if (p_mesh->index_buffer.buffer)
    {
        tgvk_buffer_destroy(&p_mesh->index_buffer);
    }
    if (p_mesh->position_buffer.buffer)
    {
        tgvk_buffer_destroy(&p_mesh->position_buffer);
    }
    if (p_mesh->normal_buffer.buffer)
    {
        tgvk_buffer_destroy(&p_mesh->normal_buffer);
    }
    if (p_mesh->uv_buffer.buffer)
    {
        tgvk_buffer_destroy(&p_mesh->uv_buffer);
    }
    if (p_mesh->tangent_buffer.buffer)
    {
        tgvk_buffer_destroy(&p_mesh->tangent_buffer);
    }
    if (p_mesh->bitangent_buffer.buffer)
    {
        tgvk_buffer_destroy(&p_mesh->bitangent_buffer);
    }
}

#endif
