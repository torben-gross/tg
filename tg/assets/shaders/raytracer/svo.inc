#define TG_SVO_SIDE_LENGTH                1024
#define TG_SVO_BLOCK_SIDE_LENGTH          32
#define TG_SVO_BLOCK_VOXEL_COUNT          (TG_SVO_BLOCK_SIDE_LENGTH * TG_SVO_BLOCK_SIDE_LENGTH * TG_SVO_BLOCK_SIDE_LENGTH)
#define TG_SVO_TRAVERSE_STACK_CAPACITY    5

struct tg_svo
{
    v4    min;
    v4    max;
};

struct tg_svo_node
{
    // Inner node: u16 child_pointer; u8 bit valid_mask; u8 bit leaf_mask;
    // Leaf node:  u32 data_pointer;
    u32    data;
};

struct tg_svo_leaf_node_data
{
    u32    count;
    u32    p_object_idcs[8];   // TG_CLUSTERS_IDX_N_BITS bits per entry
    u16    p_clusters_idcs[8]; // TG_PRIMITIVE_IDX_N_BITS bits per entry
};
