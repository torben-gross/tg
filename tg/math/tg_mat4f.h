#ifndef TG_MAT4F
#define TG_MAT4F

typedef struct tg_mat4f
{
	float data[16];
} tg_mat4f;

#define tg_mat4f_identity()     \
(tg_mat4f)                      \
{                               \
	{                           \
		1.0f, 0.0f, 0.0f, 0.0f, \
		0.0f, 1.0f, 0.0f, 0.0f, \
		0.0f, 0.0f, 1.0f, 0.0f, \
		0.0f, 0.0f, 0.0f, 1.0f  \
	}                           \
}

#define tg_mat4f_orthographic(left, right, top, bottom, near, far)           \
(tg_mat4f)                                                                   \
{                                                                            \
	{                                                                        \
		2.0f / (right - left), 0.0f, 0.0f, -(right + left) / (right - left), \
		0.0f, 2.0f / (top - bottom), 0.0f, -(top + bottom) / (top - bottom), \
		0.0f, 0.0f, -2.0f / (far - near), (far + near) / (far - near),       \
		0.0f, 0.0f, 0.0f, 1.0f                                               \
	}                                                                        \
}

tg_mat4f tg_mat4f_transposed(const tg_mat4f* mat);

#endif
