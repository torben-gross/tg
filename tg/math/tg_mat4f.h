#ifndef TG_MAT4F
#define TG_MAT4F

#include "tg_vec3f.h"

typedef struct tg_mat4f
{
	float data[16];
} tg_mat4f;

tg_mat4f* tg_mat4f_angle_axis(tg_mat4f* result, float angle_in_radians, tg_vec3f* axis);
tg_mat4f* tg_mat4f_identity(tg_mat4f* result);
tg_mat4f* tg_mat4f_look_at(tg_mat4f* result, tg_vec3f* from, tg_vec3f* to, tg_vec3f* up);
tg_mat4f* tg_mat4f_orthographic(tg_mat4f* result, float left, float right, float bottom, float top, float near, float far);
tg_mat4f* tg_mat4f_perspective(tg_mat4f* result, float fov_in_radians, float aspect, float near, float far);
tg_mat4f* tg_mat4f_transpose(tg_mat4f* result, tg_mat4f* mat);

#endif
