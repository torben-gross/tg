#include "graphics/vulkan/tgvk_core.h"

#ifdef TG_VULKAN

#include "graphics/font/tg_font.h"
#include "graphics/tg_image_io.h"
#include "platform/tg_platform.h"
#include "util/tg_string.h"



// The serialized font starts with the 'tg_sfont' struct.
// The 'tg_sfont_glyph' array 'p_g' follows immediately.
// All 'tg_sfont_kerning' entries 'p_k' follow after that in order as follows:
//   p_g[0] has its first entry at p_k[0].
//   p_g[N] has its first entry at p_k[first entry of p_g[N - 1] + entry count of p_g[N - 1]]

typedef struct tg_sfont_kerning
{
    u8    right_glyph_idx; // u8
    u8    p_kerning[4];    // f32
} tg_sfont_kerning;

typedef struct tg_sfont_glyph
{
    u8    p_size[8];                 // v2
    u8    p_uv_min[8];               // v2
    u8    p_uv_max[8];               // v2
    u8    p_advance_width[4];        // f32
    u8    p_left_side_bearing[4];    // f32
    u8    p_bottom_side_bearing[4];  // f32
    u8    p_kerning_count[2];        // u16
} tg_sfont_glyph;

typedef struct tg_sfont
{
    u8                p_max_glyph_height[4];                 // f32
    u8                p_glyph_count[2];                      // u16
    u8                p_char_to_glyph[256];                  // u8[256]
    u8                p_texture_atlas_filename[TG_MAX_PATH]; // char[TG_MAX_PATH]                       // TODO: own texture format?
    tg_sfont_glyph    p_glyphs[0];                           // tg_serialized_font_glyph[*(u16*)p_glyph_count]
} tg_sfont;



static void tg__serialize(tg_font_h h_font, const char* p_filename)
{
    char p_texture_atlas_filename[TG_MAX_PATH] = { 0 };
    const tg_size texture_atlas_filename_size = tg_strcpy_no_nul(TG_MAX_PATH, p_texture_atlas_filename, p_filename);
    p_texture_atlas_filename[texture_atlas_filename_size - 1] = 'i';
    const b32 serialize_texture_atlas_result = tgvk_image_serialize(&h_font->texture_atlas, p_texture_atlas_filename);
    TG_ASSERT(serialize_texture_atlas_result);

    tg_size size = sizeof(tg_sfont) + h_font->glyph_count * sizeof(tg_sfont_glyph);
    const tg_size kernings_offset = size;
    for (u16 i = 0; i < h_font->glyph_count; i++)
    {
        size += h_font->p_glyphs[i].kerning_count * sizeof(tg_sfont_kerning);
    }

    u8* p_memory = TG_MALLOC_STACK(size);
    tg_sfont* p_sfont = (tg_sfont*)p_memory;
    tg_sfont_kerning* p_skerning_it = (tg_sfont_kerning*)(p_memory + kernings_offset);

    *(f32*)p_sfont->p_max_glyph_height = h_font->max_glyph_height;
    *(u16*)p_sfont->p_glyph_count      = h_font->glyph_count;
    tg_memcpy(256, h_font->p_char_to_glyph, p_sfont->p_char_to_glyph);
    const tg_size filename_size = tg_strcpy(TG_MAX_PATH, (char*)p_sfont->p_texture_atlas_filename, p_texture_atlas_filename);
    tg_memory_nullify(TG_MAX_PATH - filename_size, p_sfont->p_texture_atlas_filename + filename_size);

    for (u32 i = 0; i < (u32)h_font->glyph_count; i++)
    {
        const struct tg_font_glyph* p_glyph = &h_font->p_glyphs[i];
        tg_sfont_glyph* p_sglyph = &p_sfont->p_glyphs[i];

        *(v2*) p_sglyph->p_size                = p_glyph->size;
        *(v2*) p_sglyph->p_uv_min              = p_glyph->uv_min;
        *(v2*) p_sglyph->p_uv_max              = p_glyph->uv_max;
        *(f32*)p_sglyph->p_advance_width       = p_glyph->advance_width;
        *(f32*)p_sglyph->p_left_side_bearing   = p_glyph->left_side_bearing;
        *(f32*)p_sglyph->p_bottom_side_bearing = p_glyph->bottom_side_bearing;
        *(u16*)p_sglyph->p_kerning_count       = p_glyph->kerning_count;

        for (u32 j = 0; j < (u32)p_glyph->kerning_count; j++)
        {
            const struct tg_font_glyph_kerning* p_kerning = &p_glyph->p_kernings[j];

            p_skerning_it->right_glyph_idx  = p_kerning->right_glyph_idx;
            *(f32*)p_skerning_it->p_kerning = p_kerning->kerning;
            p_skerning_it++;
        }
    }

    const b32 create_file_result = tgp_file_create(p_filename, size, p_memory, TG_TRUE);
    TG_ASSERT(create_file_result);

    TG_FREE_STACK(size);
}

static void tg__deserialize(const char* p_filename, TG_OUT tg_font_h h_font)
{
    tg_file_properties file_properties = { 0 };
    tgp_file_get_properties(p_filename, &file_properties);

    u8* p_buffer = TG_MALLOC_STACK(file_properties.size);
    tgp_file_load(p_filename, file_properties.size, p_buffer);

    const tg_sfont* p_sfont = (tg_sfont*)p_buffer;
    const tg_sfont_kerning* p_skerning_it = (tg_sfont_kerning*)(p_buffer + sizeof(tg_sfont) + (tg_size)*(u16*)p_sfont->p_glyph_count * sizeof(tg_sfont_glyph));

    h_font->max_glyph_height = *(f32*)p_sfont->p_max_glyph_height;
    h_font->glyph_count      = *(u16*)p_sfont->p_glyph_count;
    tg_memcpy(256, p_sfont->p_char_to_glyph, h_font->p_char_to_glyph);

    tg_sampler_create_info sampler_create_info = { 0 };
    sampler_create_info.min_filter = TG_IMAGE_FILTER_LINEAR;
    sampler_create_info.mag_filter = TG_IMAGE_FILTER_LINEAR;
    sampler_create_info.address_mode_u = TG_IMAGE_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_create_info.address_mode_v = TG_IMAGE_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_create_info.address_mode_w = TG_IMAGE_ADDRESS_MODE_CLAMP_TO_BORDER;

    const b32 deserialize_texture_atlas_result = tgvk_image_deserialize((const char*)p_sfont->p_texture_atlas_filename, &h_font->texture_atlas);
    TG_ASSERT(deserialize_texture_atlas_result);
    tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
    tgvk_cmd_transition_image_layout(
        p_command_buffer,
        &h_font->texture_atlas,
        VK_ACCESS_SHADER_READ_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    );
    tgvk_command_buffer_end_and_submit(p_command_buffer);

    tg_size size = (tg_size)h_font->glyph_count * sizeof(*h_font->p_glyphs);
    const tg_size kernings_offset = size;
    for (u32 i = 0; i < h_font->glyph_count; i++)
    {
        const tg_sfont_glyph* p_sfont_glyph = &p_sfont->p_glyphs[i];
        size += (tg_size)*(u16*)p_sfont_glyph->p_kerning_count * sizeof(*h_font->p_glyphs->p_kernings);
    }

    u8* p_memory = TG_MALLOC(size);
    h_font->p_glyphs = (struct tg_font_glyph*)p_memory;
    struct tg_font_glyph_kerning* p_kerning_it = (struct tg_font_glyph_kerning*)(p_memory + kernings_offset);
    
    for (u32 i = 0; i < h_font->glyph_count; i++)
    {
        const tg_sfont_glyph* p_sglyph = &p_sfont->p_glyphs[i];
        struct tg_font_glyph* p_glyph  = &h_font->p_glyphs[i];
    
        p_glyph->size                = *(v2*) p_sglyph->p_size;
        p_glyph->uv_min              = *(v2*) p_sglyph->p_uv_min;
        p_glyph->uv_max              = *(v2*) p_sglyph->p_uv_max;
        p_glyph->advance_width       = *(f32*)p_sglyph->p_advance_width;
        p_glyph->left_side_bearing   = *(f32*)p_sglyph->p_left_side_bearing;
        p_glyph->bottom_side_bearing = *(f32*)p_sglyph->p_bottom_side_bearing;
        p_glyph->kerning_count       = *(u16*)p_sglyph->p_kerning_count;
        p_glyph->p_kernings          = p_kerning_it;
    
        for (u32 j = 0; j < p_glyph->kerning_count; j++)
        {
            p_kerning_it->right_glyph_idx = p_skerning_it->right_glyph_idx;
            p_kerning_it->kerning         = *(f32*)p_skerning_it->p_kerning;
    
            p_skerning_it++;
            p_kerning_it++;
        }
    }
    TG_ASSERT((tg_size)((u8*)p_kerning_it - p_memory) == size);
    
    TG_FREE_STACK(file_properties.size);
}



tg_font_h tg_font_create(const char* p_filename)
{
	TG_ASSERT(p_filename);

	tg_font_h h_font = tgvk_handle_take(TG_STRUCTURE_TYPE_FONT);

    char p_internal_filename[TG_MAX_PATH] = { 0 };
    {
        u32 i = 0;
        while (p_filename[i] != '\0' && p_filename[i] != '.')
        {
            p_internal_filename[i] = p_filename[i];
            i++;
        }
        p_internal_filename[i++] = '.';
        p_internal_filename[i++] = 't';
        p_internal_filename[i++] = 'g';
        p_internal_filename[i] = 'f';
    }

    if (tgp_file_exists(p_internal_filename))
    {
        tg__deserialize(p_internal_filename, h_font);
    }
    else
    {
        tg_open_type_font font = { 0 };
        tg_font_load(p_filename, &font);



        // find/set necessary data

        u16 p_glyph_indices[256] = { 0 };
        u32 p_glyph_x_max[257] = { 0 };
        u32 p_glyph_heights[256] = { 0 };
        u32 max_glyph_height = 0;

        h_font->max_glyph_height = TG_FONT_GRID2PX(font, TG_MAX_FONT_SIZE, font.y_max - font.y_min);
        for (u32 char_idx = 0; char_idx < 256; char_idx++)
        {
            const u16 glyph_idx = font.p_character_to_glyph[char_idx];
            b32 found = TG_FALSE;
            for (u16 i = 0; i < h_font->glyph_count; i++)
            {
                if (p_glyph_indices[i] == glyph_idx)
                {
                    found = TG_TRUE;
                    h_font->p_char_to_glyph[char_idx] = (u8)i;
                    break;
                }
            }
            if (!found)
            {
                const tg_open_type_glyph* p_glyph = &font.p_glyphs[glyph_idx];
                const u32 h = (u32)TG_FONT_GRID2PX(font, TG_MAX_FONT_SIZE, p_glyph->y_max - p_glyph->y_min);

                h_font->p_char_to_glyph[char_idx] = (u8)h_font->glyph_count;

                p_glyph_indices[h_font->glyph_count] = glyph_idx;
                p_glyph_x_max[h_font->glyph_count + 1] = p_glyph_x_max[h_font->glyph_count] + (u32)TG_FONT_GRID2PX(font, TG_MAX_FONT_SIZE, p_glyph->x_max - p_glyph->x_min) + 1;
                p_glyph_heights[h_font->glyph_count] = h;
                max_glyph_height = TG_MAX(max_glyph_height, h);
                h_font->glyph_count++;
            }
        }



        // create bitmap

        const u32 bitmap_width = p_glyph_x_max[h_font->glyph_count] + 1;
        const u32 bitmap_height = max_glyph_height + 2;
        const tg_size bitmap_size = (tg_size)bitmap_width * (tg_size)bitmap_height;
        u8* p_bitmap = TG_MALLOC_STACK(bitmap_size);
        tg_memory_nullify(bitmap_size, p_bitmap);

        for (u32 i = 0; i < h_font->glyph_count; i++)
        {
            const tg_open_type_glyph* p_glyph = &font.p_glyphs[p_glyph_indices[i]];
            const u32 x_offset = p_glyph_x_max[i] + 1;
            const u32 y_offset = 1;
            const u32 w = p_glyph_x_max[i + 1] - p_glyph_x_max[i] - 1;
            const u32 h = p_glyph_heights[i];
            tg_font_rasterize_wh(p_glyph, x_offset, y_offset, w, h, bitmap_width, bitmap_height, p_bitmap);
        }

        tg_sampler_create_info sampler_create_info = { 0 };
        sampler_create_info.min_filter = TG_IMAGE_FILTER_LINEAR;
        sampler_create_info.mag_filter = TG_IMAGE_FILTER_LINEAR;
        sampler_create_info.address_mode_u = TG_IMAGE_ADDRESS_MODE_CLAMP_TO_BORDER;
        sampler_create_info.address_mode_v = TG_IMAGE_ADDRESS_MODE_CLAMP_TO_BORDER;
        sampler_create_info.address_mode_w = TG_IMAGE_ADDRESS_MODE_CLAMP_TO_BORDER;

        h_font->texture_atlas = tgvk_image_create(TGVK_IMAGE_TYPE_COLOR, bitmap_width, bitmap_height, VK_FORMAT_R8_UINT, &sampler_create_info);
        tgvk_command_buffer* p_command_buffer = tgvk_command_buffer_get_and_begin_global(TGVK_COMMAND_POOL_TYPE_GRAPHICS);
        tgvk_cmd_transition_image_layout(
            p_command_buffer,
            &h_font->texture_atlas,
            0,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        tgvk_buffer* p_staging_buffer = tgvk_global_staging_buffer_take(bitmap_size);
        tg_memcpy(bitmap_size, p_bitmap, p_staging_buffer->memory.p_mapped_device_memory);
        tgvk_cmd_copy_buffer_to_image(p_command_buffer, p_staging_buffer, &h_font->texture_atlas);
        tgvk_cmd_transition_image_layout(
            p_command_buffer,
            &h_font->texture_atlas,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT
        );
        tgvk_command_buffer_end_and_submit(p_command_buffer);
        tgvk_global_staging_buffer_release();

        // TODO: generate this only once and read the next time
        // i'll also have to generate uv's etc. and store/load them
        tg_image_store_to_disc("fonts/arial.bmp", bitmap_width, bitmap_height, TG_COLOR_IMAGE_FORMAT_R8_UNORM, p_bitmap, TG_TRUE, TG_TRUE);



        // determine allocation size

        tg_size allocation_size = 0;
        u16 p_kerning_counts[256] = { 0 };

        for (u32 i = 0; i < h_font->glyph_count; i++)
        {
            const u16 glyph_idx = p_glyph_indices[i];
            const tg_open_type_glyph* p_glyph = &font.p_glyphs[glyph_idx];
            const u32 max_kerning_count = (u32)p_glyph->kerning_count;
            u16 kerning_count = 0;
            for (u32 kerning_idx = 0; kerning_idx < max_kerning_count; kerning_idx++)
            {
                const u16 right_glyph_idx = p_glyph->p_kernings[kerning_idx].right;
                for (u32 j = 0; j < h_font->glyph_count; j++)
                {
                    if (p_glyph_indices[j] == right_glyph_idx)
                    {
                        kerning_count++;
                        break;
                    }
                }
            }

            allocation_size += sizeof(*h_font->p_glyphs) + (tg_size)kerning_count * sizeof(*h_font->p_glyphs->p_kernings);
            p_kerning_counts[i] = kerning_count;
        }



        // allocate and fill glyphs and kernings

        TG_ASSERT(allocation_size > 0);
        u8* p_it = TG_MALLOC(allocation_size);
#ifdef TG_DEBUG
        const u8* p_start = p_it;
#endif
        h_font->p_glyphs = (struct tg_font_glyph*)p_it; p_it += (tg_size)h_font->glyph_count * sizeof(*h_font->p_glyphs);

        for (u32 i = 0; i < (u32)h_font->glyph_count; i++)
        {
            const u16 glyph_idx = p_glyph_indices[i];
            const tg_open_type_glyph* p_glyph = &font.p_glyphs[glyph_idx];

            // fill glyph
            h_font->p_glyphs[i].size.x = TG_FONT_GRID2PX(font, TG_MAX_FONT_SIZE, p_glyph->x_max - p_glyph->x_min);
            h_font->p_glyphs[i].size.y = TG_FONT_GRID2PX(font, TG_MAX_FONT_SIZE, p_glyph->y_max - p_glyph->y_min);
            h_font->p_glyphs[i].uv_min.x = ((f32)p_glyph_x_max[i] + 0.5f) / (f32)bitmap_width;
            h_font->p_glyphs[i].uv_min.y = 0.5f / (f32)bitmap_height;
            h_font->p_glyphs[i].uv_max.x = ((f32)p_glyph_x_max[i + 1] + 0.5f) / (f32)bitmap_width;
            h_font->p_glyphs[i].uv_max.y = ((f32)p_glyph_heights[i] + 1.5f) / (f32)bitmap_height;
            h_font->p_glyphs[i].advance_width = TG_FONT_GRID2PX(font, TG_MAX_FONT_SIZE, p_glyph->advance_width);
            h_font->p_glyphs[i].left_side_bearing = TG_FONT_GRID2PX(font, TG_MAX_FONT_SIZE, p_glyph->left_side_bearing);
            h_font->p_glyphs[i].bottom_side_bearing = TG_FONT_GRID2PX(font, TG_MAX_FONT_SIZE, p_glyph->y_min - font.y_min);

            // fill glyph kernings
            const u32 kerning_count = (u32)p_kerning_counts[i];
            h_font->p_glyphs[i].kerning_count = (u16)kerning_count;
            if (kerning_count > 0)
            {
                h_font->p_glyphs[i].p_kernings = (struct tg_font_glyph_kerning*)p_it; p_it += (tg_size)kerning_count * sizeof(*h_font->p_glyphs->p_kernings);

                for (u32 kerning_idx = 0; kerning_idx < kerning_count; kerning_idx++)
                {
                    const tg_open_type_kerning kerning = p_glyph->p_kernings[kerning_idx];
                    for (u32 j = 0; j < (u32)h_font->glyph_count; j++)
                    {
                        if (p_glyph_indices[j] == kerning.right)
                        {
                            h_font->p_glyphs[i].p_kernings[kerning_idx].right_glyph_idx = (u8)j;
                            break;
                        }
                    }
                    h_font->p_glyphs[i].p_kernings[kerning_idx].kerning = TG_FONT_GRID2PX(font, TG_MAX_FONT_SIZE, kerning.value);
                }
            }
            else
            {
                h_font->p_glyphs[i].p_kernings = TG_NULL;
            }
        }

#ifdef TG_DEBUG
        const tg_size delta = (tg_size)(p_it - p_start);
        TG_ASSERT(delta == allocation_size);
#endif

        tg__serialize(h_font, p_internal_filename);
        tgvk_command_buffer_begin(p_command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        tgvk_cmd_transition_image_layout(
            p_command_buffer,
            &h_font->texture_atlas,
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );
        tgvk_command_buffer_end_and_submit(p_command_buffer);

        TG_FREE_STACK(bitmap_size);
        tg_font_free(&font);
    }

	return h_font;
}

void tg_font_destroy(tg_font_h h_font)
{
	TG_ASSERT(h_font);

    TG_FREE(h_font->p_glyphs);
    tgvk_image_destroy(&h_font->texture_atlas);
	tgvk_handle_release(h_font);
}

#endif