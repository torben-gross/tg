#ifndef TG_MAT4F
#define TG_MAT4F

#include "tg_vec3f.h"

typedef struct tg_mat4f
{
	float data[16];
} tg_mat4f;

tg_mat4f* tg_mat4f_angle_axis(tg_mat4f* result, float angleInRadians, tg_vec3f* axis);
tg_mat4f* tg_mat4f_identity(tg_mat4f* result);
tg_mat4f* tg_mat4f_look_at(tg_mat4f* result, tg_vec3f* from, tg_vec3f* to, tg_vec3f* up);
tg_mat4f* tg_mat4f_orthographic(tg_mat4f* result, float left, float right, float top, float bottom, float near, float far);
tg_mat4f* tg_mat4f_perspective(tg_mat4f* result, float fov, float aspect, float near, float far);
tg_mat4f* tg_mat4f_transposed(tg_mat4f* result, tg_mat4f* mat);

#endif
