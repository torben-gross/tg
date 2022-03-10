#include "tg_input.h"

#include "memory/tg_memory.h"
#include "platform/tg_platform.h"


#define TG_INPUT_PRESSED_KEY_STACK_CAPACITY 32


typedef struct tg_input
{
	b32    down_buttons[8];
	b32    pressed_buttons[8];

	b32    down_keys[256];
	b32    pressed_keys[256];
	u32    key_repeat_counts[256];

	u32    pressed_key_stack_count;
	b32    p_pressed_key_stack[TG_INPUT_PRESSED_KEY_STACK_CAPACITY];

	f32    mouse_wheel_detents;
} tg_input;



tg_input input = { 0 };



void tg_input_clear(void)
{
	tg_memory_nullify(sizeof(input.pressed_buttons), input.pressed_buttons);
	tg_memory_nullify(sizeof(input.pressed_keys), input.pressed_keys);
	input.pressed_key_stack_count = 0;
	input.mouse_wheel_detents = 0.0f;
}

void tg_input_on_mouse_button_pressed(tg_button button)
{
	input.down_buttons[button] = TG_TRUE;
	input.pressed_buttons[button] = TG_TRUE;
}

void tg_input_on_mouse_button_released(tg_button button)
{
	input.down_buttons[button] = TG_FALSE;
}

void tg_input_on_mouse_wheel_rotated(f32 detents)
{
	input.mouse_wheel_detents += detents;
}

void tg_input_on_key_pressed(tg_key key, b32 repeated, u32 additional_key_repeat_count)
{
	TG_ASSERT(additional_key_repeat_count == 1);
	input.down_keys[key] = TG_TRUE;
	if (!repeated)
	{
		input.pressed_keys[key] = TG_TRUE;
	}
	else
	{
		input.key_repeat_counts[key] += additional_key_repeat_count;
	}
	if (input.pressed_key_stack_count < TG_INPUT_PRESSED_KEY_STACK_CAPACITY)
	{
		input.p_pressed_key_stack[input.pressed_key_stack_count++] = key;
		if (repeated)
		{
			for (u32 i = 0; i < additional_key_repeat_count - 1; i++)
			{
				if (input.pressed_key_stack_count < TG_INPUT_PRESSED_KEY_STACK_CAPACITY)
				{
					input.p_pressed_key_stack[input.pressed_key_stack_count++] = key;
				}
				else
				{
					TG_DEBUG_LOG("Key input has been missed! The stack is too small.");
				}
			}
		}
	}
	else
	{
		TG_DEBUG_LOG("Key input has been missed! The stack is too small.");
	}
}

void tg_input_on_key_released(tg_key key)
{
	input.down_keys[key] = TG_FALSE;
	input.key_repeat_counts[key] = 0;
}



u32 tg_input_get_key_repeat_count(tg_key key)
{
	return input.key_repeat_counts[key];
}

void tg_input_get_mouse_position(u32* x, u32* y)
{
	tgp_get_mouse_position(x, y);
}

f32  tg_input_get_mouse_wheel_detents(b32 consume)
{
	const f32 result = input.mouse_wheel_detents;
	if (consume)
	{
		input.mouse_wheel_detents = TG_FALSE;
	}
	return result;
}

b32 tg_input_is_key_down(tg_key key)
{
	return input.down_keys[key];
}

b32 tg_input_is_key_pressed(tg_key key, b32 consume)
{
	const b32 result = input.pressed_keys[key];
	if (consume)
	{
		input.pressed_keys[key] = TG_FALSE;
	}
	return result;
}

b32 tg_input_is_mouse_button_down(tg_button button)
{
	return input.down_buttons[button];
}

b32 tg_input_is_mouse_button_pressed(tg_button button, b32 consume)
{
	const b32 result = input.pressed_buttons[button];
	if (consume)
	{
		input.pressed_buttons[button] = TG_FALSE;
	}
	return result;
}

u32 tg_input_get_pressed_key_stack_size(void)
{
	return input.pressed_key_stack_count;
}

tg_key tg_input_get_pressed_key(u32 stack_idx)
{
	TG_ASSERT(stack_idx < input.pressed_key_stack_count);

	return input.p_pressed_key_stack[stack_idx];
}

char tg_input_to_char(tg_key key)
{
	char result = '\0';

	if (key == TG_KEY_SPACE)
	{
		result = ' ';
	}
	else if (key >= TG_KEY_0 && key <= TG_KEY_9)
	{
		result = (char)key;
	}
	else if (key >= TG_KEY_A && key <= TG_KEY_Z)
	{
		result = (char)key + 'a' - 'A';
	}
	else if (key >= TG_KEY_PLUS && key <= TG_KEY_PERIOD)
	{
		result = (char)key - (TG_KEY_PLUS - '+');
	}

	return result;
}

char tg_input_to_upper_case(char c)
{
	TG_ASSERT(c >= 'a' && c <= 'z');

	const char result = c - ('a' - 'A');
	return result;
}

b32 tg_input_is_letter(tg_key key)
{
	const b32 result = key >= TG_KEY_A && key <= TG_KEY_Z;
	return result;
}
