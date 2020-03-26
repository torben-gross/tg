#include "tg_input.h"

#include "tg/platform/tg_platform.h"
#include <string.h>

typedef struct tg_input
{
	bool    down_buttons[8];
	bool    pressed_buttons[8];

	bool    down_keys[256];
	bool    pressed_keys[256];
	ui32    key_repeat_counts[256];
} tg_input;

tg_input input = { 0 };

void tg_input_get_mouse_position(ui32* x, ui32* y)
{
	tg_platform_get_mouse_position(x, y);
}

void tg_input_on_mouse_button_pressed(tg_button button)
{
	input.down_buttons[button] = true;
	input.pressed_buttons[button] = true;
}

void tg_input_on_mouse_button_released(tg_button button)
{
	input.down_buttons[button] = false;
}

void tg_input_on_key_pressed(tg_key key, bool repeated, ui32 additional_key_repeat_counts)
{
	input.down_keys[key] = true;
	if (!repeated)
	{
		input.pressed_keys[key] = true;
	}
	else
	{
		input.key_repeat_counts[key] += additional_key_repeat_counts;
	}
}

void tg_input_on_key_released(tg_key key)
{
	input.down_keys[key] = false;
	input.key_repeat_counts[key] = 0;
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

ui32 tg_input_get_key_repeat_count(tg_key key)
{
	return input.key_repeat_counts[key];
}

