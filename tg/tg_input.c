#include "tg_input.h"

typedef struct tg_input
{
	bool    buttons[8];
	bool    keys[256];
} tg_input;

tg_input input;

void tg_input_on_mouse_button_pressed(tg_button button)
{
	input.buttons[button] = true;
}

void tg_input_on_mouse_button_released(tg_button button)
{
	input.buttons[button] = false;
}

void tg_input_on_key_pressed(tg_key key)
{
	input.keys[key] = true;
}

void tg_input_on_key_released(tg_key key)
{
	input.keys[key] = false;
}

bool tg_input_is_mouse_button_pressed(tg_button button)
{
	TG_ASSERT(false);
	return false;
}

bool tg_input_is_mouse_button_down(tg_button button)
{
	return input.buttons[button];
}

bool tg_input_is_key_pressed(tg_key key)
{
	TG_ASSERT(false);
	return false;
}

bool tg_input_is_key_down(tg_key key)
{
	return input.keys[key];
}

