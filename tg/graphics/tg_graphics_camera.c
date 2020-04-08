#include "tg/graphics/tg_graphics.h"

#include "tg/memory/tg_memory_allocator.h"
#include "tg/platform/tg_platform.h"



typedef struct tg_camera
{

	struct
	{
		f32    fov_y;
		f32    near;
		f32    far;
		m4     projection;
	} projection;

	struct
	{
		v3     position;
		f32    pitch;
		f32    yaw;
		f32    roll;
		m4     view;
	} view;

} tg_camera;



void tg_graphics_camera_create(const v3* p_position, f32 pitch, f32 yaw, f32 roll, f32 fov_y, f32 near, f32 far, tg_camera_h* p_camera_h)
{
	TG_ASSERT(near < 0 && far < 0 && near > far && p_camera_h);

	*p_camera_h = TG_MEMORY_ALLOCATOR_ALLOCATE(sizeof(**p_camera_h));

	tg_graphics_camera_set_projection(fov_y, near, far, *p_camera_h);
	tg_graphics_camera_set_view(p_position, pitch, yaw, roll, *p_camera_h);
}

m4 tg_graphics_camera_get_projection(tg_camera_h camera_h)
{
	return camera_h->projection.projection;
}

m4 tg_graphics_camera_get_view(tg_camera_h camera_h)
{
	return camera_h->view.view;
}

void tg_graphics_camera_set_projection(f32 fov_y, f32 near, f32 far, tg_camera_h camera_h)
{
	TG_ASSERT(near < 0 && far < 0 && near > far && camera_h);

	camera_h->projection.fov_y = fov_y;
	camera_h->projection.near = near;
	camera_h->projection.far = far;

	camera_h->projection.projection = tgm_m4_perspective(fov_y, tg_platform_get_window_aspect_ratio(), near, far);
}

void tg_graphics_camera_set_view(const v3* p_position, f32 pitch, f32 yaw, f32 roll, tg_camera_h camera_h)
{
	TG_ASSERT(camera_h);

	camera_h->view.position = *p_position;
	camera_h->view.pitch = pitch;
	camera_h->view.yaw = yaw;
	camera_h->view.roll = roll;

	const m4 rotation = tgm_m4_euler(pitch, yaw, roll);
	const m4 inverse_rotation = tgm_m4_inverse(&rotation);
	const v3 negated_position = tgm_v3_negated(p_position);
	const m4 inverse_translation = tgm_m4_translate(&negated_position);

	camera_h->view.view = tgm_m4_multiply_m4(&inverse_rotation, &inverse_translation);
}

void tg_graphics_camera_destroy(tg_camera_h camera_h)
{
	TG_ASSERT(camera_h);

	TG_MEMORY_ALLOCATOR_FREE(camera_h);
}
