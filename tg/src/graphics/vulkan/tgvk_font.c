#include "graphics/vulkan/tgvk_core.h"

#ifdef TG_VULKAN

#include "graphics/font/tg_font.h"

#include "graphics/tg_image_io.h"

tg_font_h tg_font_create(const char* p_filename)
{
	TG_ASSERT(p_filename);

	tg_font_h h_font = tgvk_handle_take(TG_STRUCTURE_TYPE_FONT);

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
    u8* p_bitmap = TG_MEMORY_STACK_ALLOC(bitmap_size);
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
    tgvk_cmd_copy_buffer_to_color_image(p_command_buffer, p_staging_buffer, &h_font->texture_atlas);
    tgvk_cmd_transition_image_layout(
        p_command_buffer,
        &h_font->texture_atlas,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    );
    tgvk_command_buffer_end_and_submit(p_command_buffer);
    tgvk_global_staging_buffer_release();

    // TODO: generate this only once and read the next time
    // i'll also have to generate uv's etc. and store/load them
    tg_image_store_to_disc("fonts/arial.bmp", bitmap_width, bitmap_height, TG_COLOR_IMAGE_FORMAT_R8_UNORM, p_bitmap, TG_TRUE, TG_TRUE);

    TG_MEMORY_STACK_FREE(bitmap_size);



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
    u8* p_it = TG_MEMORY_ALLOC(allocation_size);
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



	tg_font_free(&font);

	return h_font;
}

void tg_font_destroy(tg_font_h h_font)
{
	TG_ASSERT(h_font);

    TG_MEMORY_FREE(h_font->p_glyphs);
    tgvk_image_destroy(&h_font->texture_atlas);
	tgvk_handle_release(h_font);
}

#endif
