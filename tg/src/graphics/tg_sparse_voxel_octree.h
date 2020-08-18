#ifndef TG_SPARSE_VOXEL_OCTREE
#define TG_SPARSE_VOXEL_OCTREE

#include "graphics/tg_graphics.h"
#include "math/tg_math.h"
#include "tg_common.h"

#define TG_SVO_DIMS     64
#define TG_SVO_DIMS3    262144 //64^3

TG_DECLARE_HANDLE(tg_render_command);
TG_DECLARE_HANDLE(tg_voxelizer);

typedef struct tg_voxel
{
    f32    normal_x;              // normal can be reconstructed from x and y
    f32    normal_y;// TODO: this normal could only use only 16 bits for x and y respectively, which will shrink this struct down to 12 bytes
    u32    albedo;                // rgba
    u32    metallic_roughness_ao; // metallic: 12, roughness: 12, ao: 8
} tg_voxel;

typedef struct tg_voxelizer
{
    VkFence                            fence;
    VkRenderPass                       render_pass;
    tg_vulkan_framebuffer              framebuffer;
    tg_vulkan_descriptor_set_layout    descriptor_set_layout;
    u32                                descriptor_set_count;
    tg_vulkan_descriptor_set           p_descriptor_sets[TG_MAX_RENDER_COMMANDS];
    VkPipelineLayout                   graphics_pipeline_layout;
    VkPipeline                         graphics_pipeline;
    tg_vulkan_buffer                   view_projection_ubo;
    tg_vulkan_image_3d                 image_3d;
    tg_vulkan_buffer                   voxel_buffer;
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
void    tg_voxelizer_begin(tg_voxelizer* p_voxelizer, v3i index_3d);
void    tg_voxelizer_exec(tg_voxelizer* p_voxelizer, tg_render_command_h h_render_command);
void    tg_voxelizer_end(tg_voxelizer* p_voxelizer, tg_voxel* p_voxels);
void    tg_voxelizer_destroy(tg_voxelizer* p_voxelizer);

void    tg_svo_init(tg_voxel* p_voxels);

#endif
