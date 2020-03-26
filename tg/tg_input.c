#include "tg_input.h"

#include <string.h>

typedef struct tg_input
{
	bool    down_buttons[8];
	bool    down_keys[256];

	bool    pressed_buttons[8];
	bool    pressed_keys[256];
} tg_input;

tg_input input;

void tg_input_on_mouse_button_pressed(tg_button button)
{
	input.down_buttons[button] = true;
	input.pressed_buttons[button] = true;
}

void tg_input_on_mouse_button_released(tg_button button)
{
	input.down_buttons[button] = false;
}

void tg_input_on_key_pressed(tg_key key)
{
	input.down_keys[key] = true;
	input.pressed_keys[key] = true;
}

void tg_input_on_key_released(tg_key key)
{
	input.down_keys[key] = false;
}

void tg_input_clear()
{
	memset(input.pressed_buttons, 0, sizeof(input.pressed_buttons));
	memset(input.pressed_keys, 0, sizeof(input.pressed_keys));
}

bool tg_input_is_mouse_button_pressed(tg_button button, bool consume)
{
	const bool result = input.pressed_buttons[button];
	if (consume)
	{
		input.pressed_buttons[button] = false;
	}
	return result;
}

bool tg_input_is_mouse_button_down(tg_button button)
{
	return input.down_buttons[button];
}

bool tg_input_is_key_pressed(tg_key key, bool consume)
{
	const bool result = input.pressed_keys[key];
	if (consume)
	{
		input.pressed_keys[key] = false;
	}
	return result;
}

bool tg_input_is_key_down(tg_key key)
{
	return input.down_keys[key];
}

