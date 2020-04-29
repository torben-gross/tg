#include "graphics/vulkan/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

#include "memory/tg_memory.h"
#include "platform/tg_platform.h"



tg_camera_h tg_orthographic_camera_create(const v2* p_position, f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near)
{
	TG_ASSERT(p_position && left != right && bottom != top && far != near);

	tg_camera_h camera_h = TG_MEMORY_ALLOC(sizeof(*camera_h));

	tg_orthographic_camera_set_projection(camera_h, left, right, bottom, top, far, near);
	tg_orthographic_camera_set_view(camera_h, p_position);

	return camera_h;
}

void tg_orthographic_camera_set_projection(tg_camera_h camera_h, f32 left, f32 right, f32 bottom, f32 top, f32 far, f32 near)
{
	TG_ASSERT(camera_h && left != right && bottom != top && far != near);

	camera_h->projection = tgm_m4_orthographic(left, right, bottom, top, far, near);
}

void tg_orthographic_camera_set_view(tg_camera_h camera_h, const v2* p_position)
{
	TG_ASSERT(camera_h && p_position);

	const v3 negated_position = { -p_position->x, -p_position->y, 0.0f };

	camera_h->view = tgm_m4_translate(&negated_position);
}

void tg_orthographic_camera_destroy(tg_camera_h camera_h)
{
	TG_ASSERT(camera_h);

	TG_MEMORY_FREE(camera_h);
}



tg_camera_h tg_perspective_camera_create(const v3* p_position, f32 pitch, f32 yaw, f32 roll, f32 fov_y, f32 near, f32 far)
{
	TG_ASSERT(near < 0 && far < 0 && near > far);

	tg_camera_h camera_h = TG_MEMORY_ALLOC(sizeof(*camera_h));

	tg_perspective_camera_set_projection(camera_h, fov_y, near, far);
	tg_perspective_camera_set_view(camera_h, p_position, pitch, yaw, roll);

	return camera_h;
}

void tg_perspective_camera_set_projection(tg_camera_h camera_h, f32 fov_y, f32 near, f32 far)
{
	TG_ASSERT(camera_h && near < 0 && far < 0 && near > far);

	camera_h->projection = tgm_m4_perspective(fov_y, tg_platform_get_window_aspect_ratio(), near, far);
}

void tg_perspective_camera_set_view(tg_camera_h camera_h, const v3* p_position, f32 pitch, f32 yaw, f32 roll)
{
	TG_ASSERT(camera_h);

	const m4 rotation = tgm_m4_euler(pitch, yaw, roll);
	const m4 inverse_rotation = tgm_m4_inverse(&rotation);
	const v3 negated_position = tgm_v3_negated(p_position);
	const m4 inverse_translation = tgm_m4_translate(&negated_position);

	camera_h->view = tgm_m4_multiply_m4(&inverse_rotation, &inverse_translation);
}

void tg_perspective_camera_destroy(tg_camera_h camera_h)
{
	TG_ASSERT(camera_h);

	TG_MEMORY_FREE(camera_h);
}

#endif
