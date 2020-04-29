#include "tg_camera.h"

#include "platform/tg_platform.h"



void tg_orthographic_camera_init(tg_camera* p_camera, const v2* p_position, f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near)
{
	TG_ASSERT(p_camera && p_position && left != right && bottom != top && far != near);

	tg_orthographic_camera_set_projection(p_camera, left, right, bottom, top, far, near);
	tg_orthographic_camera_set_view(p_camera, p_position);
}

void tg_orthographic_camera_set_projection(tg_camera* p_camera, f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near)
{
	TG_ASSERT(p_camera && left != right && bottom != top && far != near);

	p_camera->projection = tgm_m4_orthographic(left, right, bottom, top, far, near);
}

void tg_orthographic_camera_set_view(tg_camera* p_camera, const v2* p_position)
{
	TG_ASSERT(p_camera && p_position);

	const v3 negated_position = { -p_position->x, -p_position->y, 0.0f };

	p_camera->view = tgm_m4_translate(&negated_position);
}



void tg_perspective_camera_init(tg_camera* p_camera, const v3* p_position, f32 pitch, f32 yaw, f32 roll, f32 fov_y, f32 near, f32 far)
{
	TG_ASSERT(p_camera && near < 0 && far < 0 && near > far);

	tg_perspective_camera_set_projection(p_camera, fov_y, near, far);
	tg_perspective_camera_set_view(p_camera, p_position, pitch, yaw, roll);
}

void tg_perspective_camera_set_projection(tg_camera* p_camera, f32 fov_y, f32 near, f32 far)
{
	TG_ASSERT(p_camera && near < 0 && far < 0 && near > far);

	p_camera->projection = tgm_m4_perspective(fov_y, tg_platform_get_window_aspect_ratio(), near, far);
}

void tg_perspective_camera_set_view(tg_camera* p_camera, const v3* p_position, f32 pitch, f32 yaw, f32 roll)
{
	TG_ASSERT(p_camera);

	const m4 rotation = tgm_m4_euler(pitch, yaw, roll);
	const m4 inverse_rotation = tgm_m4_inverse(&rotation);
	const v3 negated_position = tgm_v3_negated(p_position);
	const m4 inverse_translation = tgm_m4_translate(&negated_position);

	p_camera->view = tgm_m4_multiply_m4(&inverse_rotation, &inverse_translation);
}