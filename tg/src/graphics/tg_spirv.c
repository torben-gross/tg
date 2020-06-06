#include "graphics/tg_spirv.h"

#include "memory/tg_memory.h"
#include "util/tg_string.h"



#define TG_SPIRV_MAGIC_NUMBER           0x07230203
#define TG_SPIRV_MIN_CAPACITY    4



b32 tg_spirv_internal_set_descriptor_set_and_binding(u32 word_count, const u32* p_words, u32 id, tg_spirv_structure_base* p_structure)
{
    b32 has_descriptor_set = TG_FALSE;
    b32 has_binding = TG_FALSE;

    u32 processed_word_count = 0;
    while (processed_word_count < word_count)
    {
        const u32 opcode = p_words[processed_word_count];
        const tg_spirv_op opcode_enumerant = opcode & 0xffff;
        const u16 op_word_count = opcode >> 16;

        if (opcode_enumerant == TG_SPIRV_OP_DECORATE && p_words[processed_word_count + 1] == id)
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                const tg_spirv_decoration decoration = p_words[processed_word_count + 2];
                switch (decoration)
                {
                case TG_SPIRV_DECORATION_BINDING:
                {
                    ((tg_spirv_structure*)p_structure)->binding = p_words[processed_word_count + 3];
                    has_binding = TG_TRUE;
                } break;
                case TG_SPIRV_DECORATION_DESCRIPTOR_SET:
                {
                    ((tg_spirv_structure*)p_structure)->descriptor_set = p_words[processed_word_count + 3];
                    has_descriptor_set = TG_TRUE;
                } break;
                }
            }
        }
        if (has_binding && has_descriptor_set)
        {
            return TG_TRUE;
        }

        processed_word_count += op_word_count;
    }
    return TG_FALSE;
}

void tg_spirv_internal_reserve(tg_spirv_structure_base* p_structure, u32 member_count)
{
    if (p_structure->member_capacity < member_count)
    {
        if (p_structure->member_capacity == 0)
        {
            p_structure->member_capacity = TG_SPIRV_MIN_CAPACITY;
        }
        while (p_structure->member_capacity < member_count)
        {
            p_structure->member_capacity *= 2;
        }

        const u64 size = p_structure->member_capacity * sizeof(*p_structure->p_members);
        if (p_structure->p_members)
        {
            p_structure->p_members = TG_MEMORY_REALLOC(size, p_structure->p_members);
        }
        else
        {
            p_structure->p_members = TG_MEMORY_ALLOC(size);
        }
    }
}

b32 tg_spirv_internal_fill_structure(u32 word_count, const u32* p_words, u32 id, tg_spirv_structure_base* p_structure)
{
    u32 processed_word_count = 0;
    while (processed_word_count < word_count)
    {
        const u32 opcode = p_words[processed_word_count];
        const tg_spirv_op opcode_enumerant = opcode & 0xffff;
        const u16 op_word_count = opcode >> 16;

        switch (opcode_enumerant)
        {
        case TG_SPIRV_OP_NAME:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                const char* p_name = (const char*)&p_words[processed_word_count + 2];
                const u32 name_length = tg_string_length(p_name);
                if (name_length != 0)
                {
                    TG_ASSERT(name_length + 1 <= TG_SPIRV_MAX_NAME);
                    tg_memory_copy((u64)name_length + 1, p_name, p_structure->p_name);
                }
            }
        } break;
        case TG_SPIRV_OP_MEMBER_NAME:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                const u32 member_index = p_words[processed_word_count + 2];
                tg_spirv_internal_reserve(p_structure, member_index + 1);
                p_structure->member_count++;
                const char* p_member_name = (const char*)&p_words[processed_word_count + 3];
                const u32 member_name_length = tg_string_length(p_member_name);
                TG_ASSERT(member_name_length + 1 <= TG_SPIRV_MAX_NAME);
                tg_memory_copy((u64)member_name_length + 1, p_member_name, p_structure->p_members[member_index].base.p_name);
            }
        } break;
        case TG_SPIRV_OP_DECORATE:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                const tg_spirv_decoration decoration = p_words[processed_word_count + 2];
                switch (decoration)
                {
                case TG_SPIRV_DECORATION_BLOCK:
                {
                    ((tg_spirv_structure*)p_structure)->bind_type = TG_SPIRV_BIND_TYPE_UNIFORM_BLOCK;
                } break;
                case TG_SPIRV_DECORATION_BUFFER_BLOCK:
                {
                    ((tg_spirv_structure*)p_structure)->bind_type = TG_SPIRV_BIND_TYPE_BUFFER_BLOCK;
                } break;
                }
            }
        } break;
        case TG_SPIRV_OP_MEMBER_DECORATE:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                const member_index = p_words[processed_word_count + 2];
                tg_spirv_internal_reserve(p_structure, member_index + 1);
                const tg_spirv_decoration decoration = p_words[processed_word_count + 3];
                if (decoration == TG_SPIRV_DECORATION_OFFSET)
                {
                    ((tg_spirv_substructure*)p_structure->p_members)[member_index].offset = p_words[processed_word_count + 4];
                }
            }
        } break;
        case TG_SPIRV_OP_TYPE_BOOL:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                p_structure->type = opcode_enumerant;
            }
        } break;
        case TG_SPIRV_OP_TYPE_INT:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                p_structure->type = opcode_enumerant;
            }
        } break;
        case TG_SPIRV_OP_TYPE_FLOAT:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                p_structure->type = opcode_enumerant;
            }
        } break;
        case TG_SPIRV_OP_TYPE_VECTOR:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                p_structure->type = opcode_enumerant;
            }
        } break;
        case TG_SPIRV_OP_TYPE_MATRIX:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                p_structure->type = opcode_enumerant;
            }
        } break;
        case TG_SPIRV_OP_TYPE_IMAGE:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                // TODO: do i want to save these?
                //const tg_spirv_dim dim = p_words[processed_word_count + 3];
                //const u8 depth = p_words[processed_word_count + 4];
                //const b32 arrayed = p_words[processed_word_count + 5] == 1;
                //const b32 multisampled = p_words[processed_word_count + 6];
                //const u8 sampled = p_words[processed_word_count + 7];
                //const tg_spirv_image_format format = p_words[processed_word_count + 8];
            }
        } break;
        case TG_SPIRV_OP_TYPE_SAMPLER:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                p_structure->type = opcode_enumerant;
            }
        } break;
        case TG_SPIRV_OP_TYPE_SAMPLED_IMAGE:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                ((tg_spirv_structure*)p_structure)->bind_type = TG_SPIRV_BIND_TYPE_SAMPLED_IMAGE;
                p_structure->type = opcode_enumerant;
                //const u32 image_id = p_words[processed_word_count + 2];
                // TODO: search for image_id?
            }
        } break;
        case TG_SPIRV_OP_TYPE_ARRAY:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                p_structure->type = opcode_enumerant;
            }
        } break;
        case TG_SPIRV_OP_TYPE_RUNTIME_ARRAY:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                p_structure->type = opcode_enumerant;
            }
        } break;
        case TG_SPIRV_OP_TYPE_STRUCT:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                p_structure->type = opcode_enumerant;
                const u32 member_count = op_word_count - 2;
                tg_spirv_internal_reserve(p_structure, member_count);
                for (u32 i = 0; i < member_count; i++)
                {
                    tg_spirv_internal_fill_structure(word_count, p_words, p_words[processed_word_count + 2 + i], (tg_spirv_structure_base*)&p_structure->p_members[i]);
                }
            }
        } break;
        case TG_SPIRV_OP_TYPE_POINTER:
        {
            const u32 target_id = p_words[processed_word_count + 1];
            if (target_id == id)
            {
                const tg_spirv_storage_class storage_class = p_words[processed_word_count + 2];
                if (storage_class == TG_SPIRV_STORAGE_CLASS_UNIFORM || storage_class == TG_SPIRV_STORAGE_CLASS_UNIFORM_CONSTANT)
                {
                    const u32 pointed_to_id = p_words[processed_word_count + 3];
                    tg_spirv_internal_fill_structure(word_count, p_words, pointed_to_id, p_structure);
                }
            }
        } break;
        }

        processed_word_count += op_word_count;
    }
    return TG_TRUE;
}

void tg_spirv_internal_destroy_structure(tg_spirv_structure_base* p_structure)
{
    if (p_structure->p_members)
    {
        for (u32 i = 0; i < p_structure->member_count; i++)
        {
            tg_spirv_internal_destroy_structure((tg_spirv_structure_base*)&p_structure->p_members[i]);
        }
        TG_MEMORY_FREE(p_structure->p_members);
    }
}


void tg_spirv_create_layout(u32 word_count, const u32* p_words, tg_spirv_layout* p_layout)
{
    TG_ASSERT(word_count && p_words && p_layout);

    u32 processed_word_count = 0;

    const u32 magic_number = p_words[processed_word_count++];
    TG_ASSERT(magic_number == TG_SPIRV_MAGIC_NUMBER);
    const u32 version = p_words[processed_word_count++];

    p_layout->version_major_number = (version & 0x00ff0000) >> 16;
    p_layout->version_minor_number = (version & 0x0000ff00) >> 8;
    p_layout->generators_magic_number = p_words[processed_word_count++];
    p_layout->bound = p_words[processed_word_count++];
    p_layout->schema = p_words[processed_word_count++];

    while (processed_word_count < word_count)
    {
        const tg_spirv_op opcode_enumerant = p_words[processed_word_count] & 0xffff;

        if (opcode_enumerant != TG_SPIRV_OP_CAPABILITY &&
            opcode_enumerant != TG_SPIRV_OP_EXTENSION &&
            opcode_enumerant != TG_SPIRV_OP_EXT_INST_IMPORT &&
            opcode_enumerant != TG_SPIRV_OP_MEMORY_MODEL &&
            opcode_enumerant != TG_SPIRV_OP_ENTRY_POINT &&
            opcode_enumerant != TG_SPIRV_OP_EXECUTION_MODE &&
            opcode_enumerant != TG_SPIRV_OP_STRING &&
            opcode_enumerant != TG_SPIRV_OP_SOURCE_EXTENSION &&
            opcode_enumerant != TG_SPIRV_OP_SOURCE &&
            opcode_enumerant != TG_SPIRV_OP_SOURCE_CONTINUED
            )
        {
            p_words = &p_words[processed_word_count];
            word_count -= processed_word_count;
            processed_word_count = 0;
            break;
        }
        else
        {
            const u16 op_word_count = p_words[processed_word_count] >> 16;
            if (opcode_enumerant == TG_SPIRV_OP_ENTRY_POINT)
            {
                p_layout->execution_model = p_words[processed_word_count + 1];
                const char* p_entry_point = (char*)&p_words[processed_word_count + 3];
                const u32 entry_point_length = tg_string_length(p_entry_point);
                TG_ASSERT(entry_point_length + 1 <= TG_SPIRV_MAX_NAME);
                tg_memory_copy((u64)entry_point_length + 1, p_entry_point, p_layout->p_entry_point);
            }
            processed_word_count += op_word_count;
        }
    }

    while (processed_word_count < word_count)
    {
        const tg_spirv_op opcode_enumerant = p_words[processed_word_count] & 0xffff;
        const u16 op_word_count = p_words[processed_word_count] >> 16;

        if (opcode_enumerant == TG_SPIRV_OP_VARIABLE)
        {
            const u32 points_to_id = p_words[processed_word_count + 1];
            const u32 variable_id = p_words[processed_word_count + 2];

            tg_spirv_structure structure = { 0 };
            const b32 result = tg_spirv_internal_set_descriptor_set_and_binding(word_count, p_words, variable_id, (tg_spirv_structure_base*)&structure);
            if (result)
            {
                tg_spirv_internal_fill_structure(word_count, p_words, variable_id, (tg_spirv_structure_base*)&structure);
                tg_spirv_internal_fill_structure(word_count, p_words, points_to_id, (tg_spirv_structure_base*)&structure);
                if (!p_layout->p_structures)
                {
                    p_layout->structure_capacity = TG_SPIRV_MIN_CAPACITY;
                    p_layout->p_structures = TG_MEMORY_ALLOC(p_layout->structure_capacity * sizeof(*p_layout->p_structures));
                }
                else if (p_layout->structure_capacity <= p_layout->structure_count)
                {
                    p_layout->structure_capacity *= 2;
                    p_layout->p_structures = TG_MEMORY_REALLOC(p_layout->structure_capacity * sizeof(*p_layout->p_structures), p_layout->p_structures);
                }
                p_layout->p_structures[p_layout->structure_count++] = structure;
            }
        }

        processed_word_count += op_word_count;
    }
}

void tg_spirv_destroy_layout(tg_spirv_layout* p_layout)
{
    TG_ASSERT(p_layout);

    for (u32 i = 0; i < p_layout->structure_count; i++)
    {
        tg_spirv_internal_destroy_structure((tg_spirv_structure_base*)&p_layout->p_structures[i]);
    }
    TG_MEMORY_FREE(p_layout->p_structures);
}
