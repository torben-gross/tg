#ifndef TG_CAMERA_H
#define TG_CAMERA_H

#include "math/tg_math.h"
#include "tg_common.h"

typedef struct tg_camera
{
    m4    view;
    m4    projection;
} tg_camera;

void    tg_orthographic_camera_init(tg_camera* p_camera, const v2* p_position, f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near);
void    tg_orthographic_camera_set_projection(tg_camera* p_camera, f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near);
void    tg_orthographic_camera_set_view(tg_camera* p_camera, const v2* p_position);

void    tg_perspective_camera_init(tg_camera* p_camera, const v3* p_position, f32 pitch, f32 yaw, f32 roll, f32 fov_y, f32 near, f32 far);
void    tg_perspective_camera_set_projection(tg_camera* p_camera, f32 fov_y, f32 near, f32 far);
void    tg_perspective_camera_set_view(tg_camera* p_camera, const v3* p_position, f32 pitch, f32 yaw, f32 roll);

#endif
