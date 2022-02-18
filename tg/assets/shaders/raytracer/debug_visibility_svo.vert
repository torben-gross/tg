#version 450

#include "shaders/common.inc"



struct tg_svo
{
    v4    min;
    v4    max;
};



TG_IN(0, u32 in_instance_id); #instance
TG_IN(1, v2  in_position);



TG_UBO(0,
    m4    v_mat;
    m4    p_mat;
);

TG_SSBO(3,
    tg_svo svo[];
);



TG_OUT_FLAT(0, u32 v_instance_id);



void main()
{
    v_instance_id    = in_instance_id;
    //tg_svo s         = svo[in_instance_id];
    //
    //v3 extent    = s.max.xyz - s.min.xyz;
    //v3 center    = 0.5 * extent + s.min.xyz;
    //
    //v4 col0     = v4(extent.x, 0.0, 0.0, 0.0);
    //v4 col1     = v4(0.0, extent.y, 0.0, 0.0);
    //v4 col2     = v4(0.0, 0.0, extent.z, 0.0);
    //v4 col3     = v4(center.x, center.y, center.z, 0.0);
    //m4 m_mat    = m4(col0, col1, col2, col3);
    //
    //gl_Position = p_mat * v_mat * m_mat * v4(in_position, 1.0);
    gl_Position = v4(in_position, 0.0, 1.0);
}
