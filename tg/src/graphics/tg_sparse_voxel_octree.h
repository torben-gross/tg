#ifndef TG_SPARSE_VOXEL_OCTREE
#define TG_SPARSE_VOXEL_OCTREE

#include "graphics/tg_graphics.h"
#include "math/tg_math.h"
#include "tg_common.h"

#define TG_SVO_DIMS           64
#define TG_SVO_DIMS3          262144 //64^3
#define TG_SVO_ATTACHMENTS    8      // albedo := 4, metallic := 1, roughness := 1, ao := 1, count := 1

TG_DECLARE_HANDLE(tg_render_command);
TG_DECLARE_HANDLE(tg_voxelizer);

typedef struct tg_voxel
{
    v4     albedo;
    f32    metallic, roughness, ao;
} tg_voxel;

typedef struct tg_voxelizer
{
    VkFence                            fence;
    VkRenderPass                       render_pass;
    tg_vulkan_framebuffer              framebuffer;
    tg_vulkan_pipeline                 pipeline;
    u32                                descriptor_set_count;
    tg_vulkan_descriptor_set           p_descriptor_sets[TG_MAX_RENDER_COMMANDS];
    tg_vulkan_buffer                   view_projection_ubo;
    tg_vulkan_image_3d                 p_image_3ds[TG_SVO_ATTACHMENTS];
    tg_vulkan_buffer                   p_voxel_buffers[TG_SVO_ATTACHMENTS];
    VkCommandBuffer                    command_buffer;
} tg_voxelizer;

typedef struct tg_svo_node
{
    b32             leaf;
    union
    {
        tg_voxel    voxel;
        u32         child_offset;
    };
} tg_svo_node;

typedef struct tg_svo
{
    v3i            min_coordinates;
    tg_svo_node    p_nodes[TG_SVO_DIMS3];
} tg_svo;

void    tg_voxelizer_create(tg_voxelizer* p_voxelizer);
void    tg_voxelizer_begin(tg_voxelizer* p_voxelizer);
void    tg_voxelizer_exec(tg_voxelizer* p_voxelizer, tg_render_command_h h_render_command);
void    tg_voxelizer_end(tg_voxelizer* p_voxelizer, v3i min_corner_index_3d, tg_voxel* p_voxels);
void    tg_voxelizer_destroy(tg_voxelizer* p_voxelizer);

void    tg_svo_init(const tg_voxel* p_voxels);

#endif
