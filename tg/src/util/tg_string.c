#include "util/tg_string.h"

#include "math/tg_math.h"

b32 tg_string_equal(const char* p_s0, const char* p_s1)
{
	b32 result = TG_TRUE;
	while (*p_s0 && *p_s1)
	{
		result &= *p_s0++ == *p_s1++;
	}
	result &= !*p_s0 && !*p_s1;
	return result;
}

void tg_string_format(u32 size, char* p_buffer, const char* p_format, ...)
{
	TG_ASSERT(size && p_buffer && p_format);

	char* p_variadic_arguments = TG_NULL;
	tg_variadic_start(p_variadic_arguments, p_format);
	tg_string_format_va(size, p_buffer, p_format, p_variadic_arguments);
	tg_variadic_end(p_variadic_arguments);
}

void tg_string_format_va(u32 size, char* p_buffer, const char* p_format, char* p_variadic_arguments)
{
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
			case 'c':
			{
				const c = tg_variadic_arg(p_variadic_arguments, char);
				*p_buffer_position++ = c;
			} break;

			case 'd': // TODO: this does not generate a proper string! Exponent is missing entirely...
			{
				f64 f = tg_variadic_arg(p_variadic_arguments, f64);

				if (f < 0)
				{
					*p_buffer_position++ = '-';
					f *= -1.0f;
				}

				u32 integral_part = (u32)f;
				const u32 integral_digit_count = tgm_i32_digits(integral_part); // TODO: this part can certainly be extracted
				i32 integral_pow = tgm_i32_pow(10, integral_digit_count - 1);
				for (u32 i = 0; i < integral_digit_count; i++)
				{
					const i32 digit = integral_part / integral_pow;
					*p_buffer_position++ = tgm_i32_abs(digit) + '0';
					integral_part -= digit * integral_pow;
					integral_pow /= 10;
				}

				*p_buffer_position++ = '.';

				u32 decimal_part = (u32)((f - (f64)((u32)f)) * tgm_f64_pow(10.0f, 9.0f));
				const u32 decimal_digit_count = tgm_i32_digits(decimal_part);
				i32 decimal_pow = tgm_i32_pow(10, decimal_digit_count - 1);
				for (u32 i = 0; i < decimal_digit_count; i++)
				{
					const i32 digit = decimal_part / decimal_pow;
					*p_buffer_position++ = tgm_i32_abs(digit) + '0';
					decimal_part -= digit * decimal_pow;
					decimal_pow /= 10;
				}
			} break;

			case 'i':
			{
				i32 integer = tg_variadic_arg(p_variadic_arguments, i32);

				if (integer < 0)
				{
					*p_buffer_position++ = '-';
				}

				const u32 digit_count = tgm_i32_digits(integer);
				i32 pow = tgm_i32_pow(10, digit_count - 1);
				for (u32 i = 0; i < digit_count; i++)
				{
					const i32 digit = integer / pow;
					*p_buffer_position++ = tgm_i32_abs(digit) + '0';
					integer -= digit * pow;
					pow /= 10;
				}
			} break;

			case 's':
			{
				const char* s = tg_variadic_arg(p_variadic_arguments, const char*);
				while (*s)
				{
					*p_buffer_position++ = *s++;
				}
			} break;

			case 'u':
			{
				u32 integer = tg_variadic_arg(p_variadic_arguments, u32);

				const u32 digit_count = tgm_u32_digits(integer);
				u32 pow = tgm_u32_pow(10, digit_count - 1);
				for (u32 i = 0; i < digit_count; i++)
				{
					const u32 digit = integer / pow;
					*p_buffer_position++ = tgm_i32_abs(digit) + '0';
					integer -= digit * pow;
					pow /= 10;
				}
			} break;

			default: TG_INVALID_CODEPATH(); break;
			}
		}
		else
		{
			*p_buffer_position++ = *p_format_position++;
		}
	}
	*p_buffer_position = '\0';
}

u32 tg_string_length(const char* p_string)
{
	const char* p_it = p_string;
	while (*p_it)
	{
		p_it++;
	}
	const u32 result = (u32)(p_it - p_string);
	return result;
}

void tg_string_replace_characters(char* p_string, char replace, char with)
{
	char* p_it = p_string;
	do
	{
		if (*p_it == replace)
		{
			*p_it = with;
		}
	} while (*++p_it != '\0');
}
