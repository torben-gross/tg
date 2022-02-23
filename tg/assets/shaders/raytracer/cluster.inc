#define TG_PRIMITIVE_IDX_N_BITS                  9
#define TG_N_PRIMITIVES_PER_CLUSTER              (1 << TG_PRIMITIVE_IDX_N_BITS) // 512
#define TG_N_PRIMITIVES_PER_CLUSTER_CUBE_ROOT    8
#define TG_CLUSTERS_IDX_N_BITS                   31
#define TG_N_CLUSTERS                            (1 << TG_CLUSTERS_IDX_N_BITS) // 2,147,483,648
#define TG_CLUSTER_SIZE(n_bits_per_element)      (TG_N_PRIMITIVES_PER_CLUSTER / 8 * (n_bits_per_element))
