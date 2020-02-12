#include "tg_mat4f.h"

tg_mat4f tg_mat4f_transposed(const tg_mat4f* mat)
{
	tg_mat4f result = { 0 };
	result.data[0] = mat->data[0];
	result.data[1] = mat->data[4];
	result.data[2] = mat->data[8];
	result.data[3] = mat->data[12];
	result.data[4] = mat->data[1];
	result.data[5] = mat->data[5];
	result.data[6] = mat->data[9];
	result.data[7] = mat->data[13];
	result.data[8] = mat->data[2];
	result.data[9] = mat->data[6];
	result.data[10] = mat->data[10];
	result.data[11] = mat->data[14];
	result.data[12] = mat->data[3];
	result.data[13] = mat->data[7];
	result.data[14] = mat->data[11];
	result.data[15] = mat->data[15];
	return result;
}
