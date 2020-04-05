#include "tg/util/tg_string.h"

#include "tg/math/tg_math.h"
#include <stdarg.h>

void tg_string_format(ui32 size, char* p_buffer, const char* p_format, ...)
{
	TG_ASSERT(size && p_buffer && p_format);

	va_list list;
	va_start(list, p_format);

	const char* p_format_position = p_format;
	char* p_buffer_position = p_buffer;
	while (*p_format_position != '\0')
	{
		if (*p_format_position == '%')
		{
			p_format_position++;
			const char format_parameter = *p_format_position++;

			switch (format_parameter)
			{

			case 'd': // TODO: this does not generate a proper string! Exponent is missing entirely...
			{
				f64 f = va_arg(list, f64);

				if (f < 0)
				{
					*p_buffer_position++ = '-';
					f *= -1.0f;
				}

				ui32 integral_part = (ui32)f;
				const ui32 integral_digit_count = tgm_i32_digits(integral_part); // TODO: this part can certainly be extracted
				i32 integral_pow = tgm_i32_pow(10, integral_digit_count - 1);
				for (ui32 i = 0; i < integral_digit_count; i++)
				{
					const i32 digit = integral_part / integral_pow;
					*p_buffer_position++ = tgm_i32_abs(digit) + '0';
					integral_part -= digit * integral_pow;
					integral_pow /= 10;
				}

				*p_buffer_position++ = '.';

				ui32 decimal_part = (ui32)((f - (f64)((ui32)f)) * tgm_f64_pow(10.0f, 9.0f));
				const ui32 decimal_digit_count = tgm_i32_digits(decimal_part);
				i32 decimal_pow = tgm_i32_pow(10, decimal_digit_count - 1);
				for (ui32 i = 0; i < decimal_digit_count; i++)
				{
					const i32 digit = decimal_part / decimal_pow;
					*p_buffer_position++ = tgm_i32_abs(digit) + '0';
					decimal_part -= digit * decimal_pow;
					decimal_pow /= 10;
				}
			} break;

			case 'i':
			{
				i32 integer = va_arg(list, i32);

				if (integer < 0)
				{
					*p_buffer_position++ = '-';
				}

				const ui32 digit_count = tgm_i32_digits(integer);
				i32 pow = tgm_i32_pow(10, digit_count - 1);
				for (ui32 i = 0; i < digit_count; i++)
				{
					const i32 digit = integer / pow;
					*p_buffer_position++ = tgm_i32_abs(digit) + '0';
					integer -= digit * pow;
					pow /= 10;
				}
			} break;

			case 's':
			{
				const char* s = va_arg(list, const char*);
				while (*s)
				{
					*p_buffer_position++ = *s++;
				}
			} break;

			case 'u':
			{
				ui32 integer = va_arg(list, ui32);

				const ui32 digit_count = tgm_ui32_digits(integer);
				ui32 pow = tgm_ui32_pow(10, digit_count - 1);
				for (ui32 i = 0; i < digit_count; i++)
				{
					const ui32 digit = integer / pow;
					*p_buffer_position++ = tgm_i32_abs(digit) + '0';
					integer -= digit * pow;
					pow /= 10;
				}
			} break;

			default:
			{
				TG_ASSERT(TG_FALSE);
			} break;
			}
		}
		else
		{
			*p_buffer_position++ = *p_format_position++;
		}
	}
	*p_buffer_position = '\0';

	va_end(list);
}

ui32 tg_string_length(const char* p_string)
{
	ui32 length = 0;
	while (*p_string++)
	{
		length++;
	}
	return length;
}