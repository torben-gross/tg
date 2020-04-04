#ifndef TG_INPUT_H
#define TG_INPUT_H

#include "tg/tg_common.h"

typedef enum tg_key
{
	TG_KEY_BACKSPACE        = '\b',
	TG_KEY_TAB              = '\t',
	TG_KEY_RETURN           = '\r',
	TG_KEY_SHIFT            = 0x10,
	TG_KEY_CONTROL          = 0x11,
	TG_KEY_ALT              = 0x12,
	TG_KEY_PAUSE            = 0x13,
	TG_KEY_CAPSLOCK         = 0x14,
	TG_KEY_ESCAPE           = 0x1b,
	TG_KEY_SPACE            = ' ',
	TG_KEY_PAGE_UP          = 0x21,
	TG_KEY_PAGE_DOWN        = 0x22,
	TG_KEY_END              = 0x23,
	TG_KEY_HOME             = 0x24,
	TG_KEY_LEFT             = 0x25,
	TG_KEY_UP               = 0x26,
	TG_KEY_RIGHT            = 0x27,
	TG_KEY_DOWN             = 0x28,
	TG_KEY_DELETE           = 0x2e,
	TG_KEY_0                = '0',
	TG_KEY_1                = '1',
	TG_KEY_2                = '2',
	TG_KEY_3                = '3',
	TG_KEY_4                = '4',
	TG_KEY_5                = '5',
	TG_KEY_6                = '6',
	TG_KEY_7                = '7',
	TG_KEY_8                = '8',
	TG_KEY_9                = '9',
	TG_KEY_A                = 'A',
	TG_KEY_B                = 'B',
	TG_KEY_C                = 'C',
	TG_KEY_D                = 'D',
	TG_KEY_E                = 'E',
	TG_KEY_F                = 'F',
	TG_KEY_G                = 'G',
	TG_KEY_H                = 'H',
	TG_KEY_I                = 'I',
	TG_KEY_J                = 'J',
	TG_KEY_K                = 'K',
	TG_KEY_L                = 'L',
	TG_KEY_M                = 'M',
	TG_KEY_N                = 'N',
	TG_KEY_O                = 'O',
	TG_KEY_P                = 'P',
	TG_KEY_Q                = 'Q',
	TG_KEY_R                = 'R',
	TG_KEY_S                = 'S',
	TG_KEY_T                = 'T',
	TG_KEY_U                = 'U',
	TG_KEY_V                = 'V',
	TG_KEY_W                = 'W',
	TG_KEY_X                = 'X',
	TG_KEY_Y                = 'Y',
	TG_KEY_Z                = 'Z',
	TG_KEY_F1               = 0x70,
	TG_KEY_F2               = 0x71,
	TG_KEY_F3               = 0x72,
	TG_KEY_F4               = 0x73,
	TG_KEY_F5               = 0x74,
	TG_KEY_F6               = 0x75,
	TG_KEY_F7               = 0x76,
	TG_KEY_F8               = 0x77,
	TG_KEY_F9               = 0x78,
	TG_KEY_F10              = 0x79,
	TG_KEY_F11              = 0x7a,
	TG_KEY_F12              = 0x7b,
	TG_KEY_F13              = 0x7c,
	TG_KEY_F14              = 0x7d,
	TG_KEY_F15              = 0x7e,
	TG_KEY_F16              = 0x7f,
	TG_KEY_F17              = 0x80,
	TG_KEY_F18              = 0x81,
	TG_KEY_F19              = 0x82,
	TG_KEY_F20              = 0x83,
	TG_KEY_F21              = 0x84,
	TG_KEY_F22              = 0x85,
	TG_KEY_F23              = 0x86,
	TG_KEY_F24              = 0x87,
	TG_KEY_LEFT_SHIFT       = 0xa0,
	TG_KEY_RIGHT_SHIFT      = 0xa1,
	TG_KEY_LEFT_CONTROL     = 0xa2,
	TG_KEY_RIGHT_CONTROL    = 0xa3
} tg_key;

typedef enum tg_button
{
	TG_BUTTON_LEFT = 0x01,
	TG_BUTTON_RIGHT = 0x02,
	TG_BUTTON_MIDDLE = 0x04,
	TG_BUTTON_X1 = 0x05,
	TG_BUTTON_X2 = 0x06
} tg_button;

/*
---- Internals ----
*/
void tg_input_clear();
void tg_input_on_key_pressed(tg_key key, b32 repeated, ui32 additional_key_repeat_count);
void tg_input_on_key_released(tg_key key);
void tg_input_on_mouse_button_pressed(tg_button button);
void tg_input_on_mouse_button_released(tg_button button);
void tg_input_on_mouse_wheel_rotated(f32 detents);

/*
---- Public Functions ----
*/
ui32 tg_input_get_key_repeat_count(tg_key key);
void tg_input_get_mouse_position(ui32* x, ui32* y);
f32  tg_input_get_mouse_wheel_detents(b32 consume);
b32  tg_input_is_key_down(tg_key key);
b32  tg_input_is_key_pressed(tg_key key, b32 consume);
b32  tg_input_is_mouse_button_down(tg_button button);
b32  tg_input_is_mouse_button_pressed(tg_button button, b32 consume);

#endif
