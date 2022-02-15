#ifndef TG_GRAPHICS_CORE_H
#define TG_GRAPHICS_CORE_H

#include "tg_common.h"

#include "math/tg_math.h"



#define TG_MAX_SWAPCHAIN_IMAGES           2

#define TG_MAX_FONT_SIZE                  64
#define TG_MAX_SHADER_ATTACHMENTS         8
#define TG_MAX_SHADER_GLOBAL_RESOURCES    16
#define TG_MAX_SHADER_INPUTS              16
#define TG_MAX_SHADER_OUTPUTS             16

#define TG_IMAGE_MAX_MIP_LEVELS(w, h)     ((u32)tgm_f32_log2((f32)tgm_u32_max((u32)w, (u32)h)) + 1)

#define TG_CAMERA_LEFT(camera)            (tgm_m4_mulv4(tgm_m4_inverse(tgm_m4_euler((camera).pitch, (camera).yaw, (camera).roll)), (v4){ -1.0f,  0.0f,  0.0f,  0.0f }).xyz)
#define TG_CAMERA_RIGHT(camera)           (tgm_m4_mulv4(tgm_m4_inverse(tgm_m4_euler((camera).pitch, (camera).yaw, (camera).roll)), (v4){  1.0f,  0.0f,  0.0f,  0.0f }).xyz)
#define TG_CAMERA_DOWN(camera)            (tgm_m4_mulv4(tgm_m4_inverse(tgm_m4_euler((camera).pitch, (camera).yaw, (camera).roll)), (v4){  0.0f, -1.0f,  0.0f,  0.0f }).xyz)
#define TG_CAMERA_UP(camera)              (tgm_m4_mulv4(tgm_m4_inverse(tgm_m4_euler((camera).pitch, (camera).yaw, (camera).roll)), (v4){  0.0f,  1.0f,  0.0f,  0.0f }).xyz)
#define TG_CAMERA_FORWARD(camera)         (tgm_m4_mulv4(tgm_m4_inverse(tgm_m4_euler((camera).pitch, (camera).yaw, (camera).roll)), (v4){  0.0f,  0.0f, -1.0f,  0.0f }).xyz)
#define TG_CAMERA_BACKWARD(camera)        (tgm_m4_mulv4(tgm_m4_inverse(tgm_m4_euler((camera).pitch, (camera).yaw, (camera).roll)), (v4){  0.0f,  0.0f,  1.0f,  0.0f }).xyz)



typedef enum tg_blend_mode
{
	TG_BLEND_MODE_NONE = 0,
	TG_BLEND_MODE_ADD,
	TG_BLEND_MODE_BLEND
} tg_blend_mode;

typedef enum tg_camera_type
{
	TG_CAMERA_TYPE_ORTHOGRAPHIC,
	TG_CAMERA_TYPE_PERSPECTIVE
} tg_camera_type;

typedef enum tg_color_image_format
{
	TG_COLOR_IMAGE_FORMAT_A8B8G8R8_UNORM          = 51,
	TG_COLOR_IMAGE_FORMAT_B8G8R8A8_UNORM          = 44,
	TG_COLOR_IMAGE_FORMAT_R16G16B16A16_SFLOAT     = 97,
    TG_COLOR_IMAGE_FORMAT_R32_UINT                = 98,
	TG_COLOR_IMAGE_FORMAT_R32G32B32A32_SFLOAT     = 109,
	TG_COLOR_IMAGE_FORMAT_R8_UINT                 = 13,
	TG_COLOR_IMAGE_FORMAT_R8_UNORM                = 9,
	TG_COLOR_IMAGE_FORMAT_R8G8_UNORM              = 16,
	TG_COLOR_IMAGE_FORMAT_R8G8B8_UNORM            = 23,
	TG_COLOR_IMAGE_FORMAT_R8G8B8A8_UNORM          = 37
} tg_color_image_format;

typedef enum tg_depth_image_format
{
	TG_DEPTH_IMAGE_FORMAT_D16_UNORM     = 124,
	TG_DEPTH_IMAGE_FORMAT_D32_SFLOAT    = 126
} tg_depth_image_format;

typedef enum tg_image_address_mode
{
	TG_IMAGE_ADDRESS_MODE_CLAMP_TO_BORDER         = 3,
	TG_IMAGE_ADDRESS_MODE_CLAMP_TO_EDGE           = 2,
	TG_IMAGE_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE    = 4,
	TG_IMAGE_ADDRESS_MODE_MIRRORED_REPEAT         = 1,
	TG_IMAGE_ADDRESS_MODE_REPEAT                  = 0
} tg_image_address_mode;

typedef enum tg_image_filter
{
	TG_IMAGE_FILTER_LINEAR     = 1,
	TG_IMAGE_FILTER_NEAREST    = 0
} tg_image_filter;

typedef enum tg_shader_type
{
	TG_SHADER_TYPE_COMPUTE,
	TG_SHADER_TYPE_FRAGMENT,
	TG_SHADER_TYPE_GEOMETRY,
	TG_SHADER_TYPE_VERTEX
} tg_shader_type;

typedef enum tg_vertex_input_attribute_format
{
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_INVALID                = 0,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_SFLOAT             = 100,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_SINT               = 99,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32_UINT               = 98,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_SFLOAT          = 103,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_SINT            = 102,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32_UINT            = 101,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT       = 106,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_SINT         = 105,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32_UINT         = 104,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32A32_SFLOAT    = 109,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32A32_SINT      = 108,
	TG_VERTEX_INPUT_ATTRIBUTE_FORMAT_R32G32B32A32_UINT      = 107
} tg_vertex_input_attribute_format;



typedef struct tg_camera
{
	tg_camera_type    type;
	v3                position;
	f32               pitch;
	f32               yaw;
	f32               roll;
	union
	{
		struct
		{
            f32       l, r, b, t, n, f;
		} ortho;
		struct
		{
			f32       fov_y_in_radians;
			f32       aspect;
			f32       n, f;
		} persp;
	};
} tg_camera;

typedef struct tg_instance
{
	v3u    half_extent;
	m4     translation;
	m4     rotation;

	u32    first_voxel_id;
	u16    packed_log2_whd; // 5 bits each for log2_w, log2_h, log2_d. One bit unused. TODO: we only need to ensure division by 16 for 3 lods, so just not use lower 3 bits? this member would need to become bigger, though.. but for lod down to w,h,d = 2, we need 2^n?
} tg_instance;



void       tg_graphics_init(void);
void       tg_graphics_on_window_resize(u32 width, u32 height);
void       tg_graphics_shutdown(void);
void       tg_graphics_wait_idle(void);



u32        tg_color_image_format_channels(tg_color_image_format format);
tg_size    tg_color_image_format_size(tg_color_image_format format);

u32        tg_vertex_input_attribute_format_get_alignment(tg_vertex_input_attribute_format format);
u32        tg_vertex_input_attribute_format_get_size(tg_vertex_input_attribute_format format);

#endif
