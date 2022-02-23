#include "util/tg_string.h"

#include "math/tg_math.h"
#include "memory/tg_memory.h"

void tg_string_create(char* p_data, TG_INOUT tg_string* p_string)
{
	TG_ASSERT(p_data && p_string);

	p_string->count_no_nul = tg_strlen_no_nul(p_data);
	if (p_string->capacity < (tg_size)p_string->count_no_nul + 1LL)
	{
		p_string->capacity = (tg_size)p_string->count_no_nul + 1LL;
		if (p_string->p_data)
		{
			TG_REALLOC(p_string->capacity, p_string->p_data);
		}
		else
		{
			p_string->p_data = TG_MALLOC(p_string->capacity);
		}
	}
	tg_strcpy(p_string->capacity, p_string->p_data, p_data);
}

void tg_string_destroy(tg_string* p_string)
{
	TG_ASSERT(p_string);

	if (p_string->p_data)
	{
		TG_FREE(p_string->p_data);
	}
	p_string->count_no_nul = 0;
	p_string->capacity = 0;
}

tg_size tg_strcpy(tg_size size, char* p_buffer, const char* p_string)
{
	tg_size i = 0;
	for (; i < size; i++)
	{
		p_buffer[i] = p_string[i];
		if (p_string[i] == '\0')
		{
			i++;
			break;
		}
	}
	TG_ASSERT(p_buffer[i - 1] == '\0');
	return i;
}

tg_size tg_strcpy_line(tg_size size, char* p_buffer, const char* p_string)
{
	tg_size i = 0;
	for (; i < size; i++)
	{
		p_buffer[i] = p_string[i];
		if (p_string[i] == '\0')
		{
			i++;
			break;
		}
		else if (p_string[i] == '\n')
		{
			i++;
			TG_ASSERT(i < size);
			p_buffer[i] = '\0';
			i++;
			break;
		}
	}
	TG_ASSERT(p_buffer[i - 1] == '\0');
	return i;
}

tg_size tg_strcpy_no_nul(tg_size size, char* p_buffer, const char* p_string)
{
	tg_size i = 0;
	for (; i < size; i++)
	{
		if (p_string[i] == '\0')
		{
			break;
		}
		p_buffer[i] = p_string[i];
	}
	TG_ASSERT(p_string[i] == '\0');
	return i;
}

tg_size tg_strncpy(tg_size size, char* p_buffer, tg_size size_to_copy, const char* p_string)
{
	TG_ASSERT(size >= size_to_copy + 1);

	tg_size i = 0;
	for (; i < size_to_copy; i++)
	{
		p_buffer[i] = p_string[i];
		if (p_string[i] == '\0')
		{
			i++;
			break;
		}
	}
	p_buffer[size_to_copy] = '\0';
	return i;
}

tg_size tg_strncpy_no_nul(tg_size size, char* p_buffer, tg_size size_to_copy, const char* p_string)
{
	TG_ASSERT(size >= size_to_copy);

	tg_size i = 0;
	for (; i < size_to_copy; i++)
	{
		if (p_string[i] == '\0')
		{
			break;
		}
		p_buffer[i] = p_string[i];
	}
	return i;
}

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

b32 tg_string_line_ends_with(const char* p_string, const char* p_postfix)
{
	b32 result = TG_FALSE;

	u32 string_length = 0;
	const char* p_it = p_string;
	while (*p_it != '\r' && *p_it != '\n' && *p_it != '\0')
	{
		string_length++;
		p_it++;
	}

	const u32 postfix_length = tg_strlen_no_nul(p_postfix);
	if (postfix_length <= string_length)
	{
		result = TG_TRUE;
		for (u32 i = 0; i < postfix_length; i++)
		{
			if (p_postfix[postfix_length - 1 - i] != p_string[string_length - 1 - i])
			{
				result = TG_FALSE;
				break;
			}
		}
	}

	return result;
}

const char* tg_string_next_line(const char* p_string)
{
	const char* p_result = p_string;
	while (*p_result++ != '\n');
	return p_result;
}

const char* tg_string_skip_whitespace(const char* p_string)
{
	const char* p_result = p_string;
	while (*p_result == '\t' || *p_result == '\n' || *p_result == '\r' || *p_result == ' ')
	{
		p_result++;
	}
	return p_result;
}

b32 tg_string_starts_with(const char* p_string, const char* p_prefix)
{
	b32 result = TG_TRUE;
	while (*p_prefix)
	{
		result &= *p_string++ == *p_prefix++;
	}
	return result;
}

void tg_stringf(u32 size, char* p_buffer, const char* p_format, ...)
{
	TG_ASSERT(size && p_buffer && p_format);

	char* p_variadic_arguments = TG_NULL;
	tg_variadic_start(p_variadic_arguments, p_format);
	tg_stringf_va(size, p_buffer, p_format, p_variadic_arguments);
	tg_variadic_end(p_variadic_arguments);
}

void tg_stringf_va(u32 size, char* p_buffer, const char* p_format, char* p_variadic_arguments)
{
#define WRITE_CHAR(c) TG_ASSERT((tg_size)p_buffer_position - (tg_size)p_buffer < size); *p_buffer_position++ = (c);

	TG_ASSERT(size && p_buffer && p_format);

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
				const char c = tg_variadic_arg(p_variadic_arguments, char);
				WRITE_CHAR(c);
			} break;

			case 'd': // TODO: this does not generate a proper string! Exponent is missing entirely...
			{
				f64 f = tg_variadic_arg(p_variadic_arguments, f64);

				if (f < 0)
				{
					WRITE_CHAR('-');
					f *= -1.0f;
				}

				u32 integral_part = (u32)f;
				const u32 integral_digit_count = tgm_i32_digits(integral_part); // TODO: this part can certainly be extracted
				i32 integral_pow = tgm_i32_pow(10, integral_digit_count - 1);
				for (u32 i = 0; i < integral_digit_count; i++)
				{
					const i32 digit = integral_part / integral_pow;
					WRITE_CHAR((char)tgm_i32_abs(digit) + '0');
					integral_part -= digit * integral_pow;
					integral_pow /= 10;
				}

				WRITE_CHAR('.');

				u32 decimal_part = (u32)((f - (f64)((u32)f)) * tgm_f64_pow(10.0f, 9.0f));
				const u32 decimal_digit_count = tgm_i32_digits(decimal_part);
				i32 decimal_pow = tgm_i32_pow(10, decimal_digit_count - 1);
				for (u32 i = 0; i < decimal_digit_count; i++)
				{
					const i32 digit = decimal_part / decimal_pow;
					WRITE_CHAR((char)tgm_i32_abs(digit) + '0');
					decimal_part -= digit * decimal_pow;
					decimal_pow /= 10;
				}
			} break;

			case 'i':
			{
				i32 integer = tg_variadic_arg(p_variadic_arguments, i32);

				if (integer < 0)
				{
					WRITE_CHAR('-');
				}

				const u32 digit_count = tgm_i32_digits(integer);
				i32 pow = tgm_i32_pow(10, digit_count - 1);
				for (u32 i = 0; i < digit_count; i++)
				{
					const i32 digit = integer / pow;
					WRITE_CHAR((char)tgm_i32_abs(digit) + '0');
					integer -= digit * pow;
					pow /= 10;
				}
			} break;

			case 's':
			{
				const char* s = tg_variadic_arg(p_variadic_arguments, const char*);
				while (*s)
				{
					WRITE_CHAR(*s++);
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
					WRITE_CHAR((char)tgm_i32_abs(digit) + '0');
					integer -= digit * pow;
					pow /= 10;
				}
			} break;

			default: TG_INVALID_CODEPATH(); break;
			}
		}
		else
		{
			WRITE_CHAR(*p_format_position++);
		}
	}
	WRITE_CHAR('\0');

#undef WRITE_CHAR
}

u32 tg_strlen_no_nul(const char* p_string)
{
	TG_ASSERT(p_string);

	const char* p_it = p_string;
	while (*p_it)
	{
		p_it++;
	}
	const u32 result = (u32)(p_it - p_string);
	return result;
}

tg_size tg_strsize(const char* p_string)
{
	TG_ASSERT(p_string);

	const char* p_it = p_string;
	while (*p_it++);
	const tg_size result = (tg_size)(p_it - p_string);
	return result;
}

u32 tg_string_parse_f32(u32 size, char* p_buffer, f32 v)
{
	const u32 result = tg_string_parse_f32_no_nul(size, p_buffer, v) + 1;
	TG_ASSERT(result <= size);
	p_buffer[result - 1] = '\0';
	return result;
}

u32 tg_string_parse_f32_no_nul(u32 size, char* p_buffer, f32 v)
{
	u32 count = 0;

	if (v < 0)
	{
		TG_ASSERT(count + 1 < size);
		p_buffer[count++] = '-';
		v *= -1.0f;
	}

	const u32 integral_part = (u32)v;
	count += tg_string_parse_u32_no_nul(size - count, &p_buffer[count], integral_part);

	p_buffer[count++] = '.';

	TG_ASSERT(count + 8 < size);
	f32 decimal_part = v - (f32)(u32)v;
	for (u32 i = 0; i < 8; i++)
	{
		decimal_part *= 10.0f;
		const u32 digit = (u32)decimal_part;
		p_buffer[count++] = '0' + (char)digit;
		decimal_part -= (f32)digit;
	}

	TG_ASSERT(count < size);
	p_buffer[count++] = 'f';

	return count;
}

u32 tg_string_parse_f64(u32 size, char* p_buffer, f64 v)
{
	const u32 result = tg_string_parse_f64_no_nul(size, p_buffer, v) + 1;
	TG_ASSERT(result <= size);
	p_buffer[result - 1] = '\0';
	return result;
}

u32 tg_string_parse_f64_no_nul(u32 size, char* p_buffer, f64 v)
{
	u32 count = 0;

	if (v < 0)
	{
		TG_ASSERT(count + 1 < size);
		p_buffer[count++] = '-';
		v *= -1.0f;
	}

	const u64 integral_part = (u64)v;
	count += tg_string_parse_u64_no_nul(size - count, &p_buffer[count], integral_part);

	p_buffer[count++] = '.';

	TG_ASSERT(count + 16 < size);
	f64 decimal_part = v - (f64)(u64)v;
	for (u32 i = 0; i < 16; i++)
	{
		decimal_part *= 10.0;
		const u32 digit = (u32)decimal_part;
		p_buffer[count++] = '0' + (char)digit;
		decimal_part -= (f64)digit;
	}

	return count;
}

u32 tg_string_parse_i32(u32 size, char* p_buffer, i32 v)
{
	const u32 result = tg_string_parse_i32_no_nul(size, p_buffer, v) + 1;
	TG_ASSERT(result <= size);
	p_buffer[result - 1] = '\0';
	return result;
}

u32 tg_string_parse_i32_no_nul(u32 size, char* p_buffer, i32 v)
{
	u32 count = 0;

	if (v < 0)
	{
		TG_ASSERT(count < size);
		p_buffer[count++] = '-';
		v *= -1;

	}
	count += tg_string_parse_u32_no_nul(size - count, &p_buffer[count], (u32)v);

	return count;
}

u32 tg_string_parse_u32(u32 size, char* p_buffer, u32 v)
{
	const u32 result = tg_string_parse_u32_no_nul(size, p_buffer, v) + 1;
	TG_ASSERT(result <= size);
	p_buffer[result - 1] = '\0';
	return result;
}

u32 tg_string_parse_u32_no_nul(u32 size, char* p_buffer, u32 v)
{
	const u32 count = tgm_u32_digits(v);

	TG_ASSERT(count <= size);
	u32 pow = tgm_u32_pow(10, count - 1);
	TG_ASSERT(pow != 0);
	for (u32 i = 0; i < count; i++)
	{
		const u32 digit = v / pow;
		p_buffer[i] = '0' + (char)digit;
		v -= digit * pow;
		pow /= 10;
	}

	return count;
}

u32 tg_string_parse_u64(u32 size, char* p_buffer, u64 v)
{
	const u32 result = tg_string_parse_u64_no_nul(size, p_buffer, v) + 1;
	TG_ASSERT(result <= size);
	p_buffer[result - 1] = '\0';
	return result;
}

u32 tg_string_parse_u64_no_nul(u32 size, char* p_buffer, u64 v)
{
	const u32 count = tgm_u64_digits(v);

	TG_ASSERT(count <= size);
	u32 pow = tgm_u32_pow(10, count - 1);
	TG_ASSERT(pow != 0i64);
	for (u32 i = 0; i < count; i++)
	{
		const u64 digit = (u32)(v / (u64)pow);
		p_buffer[i] = '0' + (char)digit;
		v -= digit * pow;
		pow /= 10;
	}

	return count;
}

void tg_string_replace_characters(char* p_string, char replace, char with)
{
	TG_ASSERT(p_string);

	char* p_it = p_string;
	do
	{
		if (*p_it == replace)
		{
			*p_it = with;
		}
	} while (*++p_it != '\0');
}

f32 tg_string_to_f32(const char* p_string) // TODO: this does not work with exponents!
{
	TG_ASSERT(p_string);
	TG_ASSERT(*p_string == '-' || *p_string >= '0' && *p_string <= '9');

	f32 sign = 1.0f;
	f32 decimal = 0.0f;
	f32 integral = 0.0f;

	if (*p_string == '-')
	{
		sign = -1.0f;
		p_string++;
	}
	while (*p_string >= '0' && *p_string <= '9')
	{
		decimal *= 10.0f;
		decimal += (f32)(*p_string - '0');
		p_string++;
	}
	if (*p_string++ == '.')
	{
		f32 f = 10.0f;
		while (*p_string >= '0' && *p_string <= '9')
		{
			integral += (f32)(*p_string - '0') / f;
			f *= 10.0f;
			p_string++;
		}
	}

	const f32 result = sign * (decimal + integral);
	return result;
}

i32 tg_string_to_i32(const char* p_string)
{
	TG_ASSERT(p_string);
	TG_ASSERT(*p_string == '-' || *p_string >= '0' && *p_string <= '9');

	i32 sign = 1;
	i32 decimal = 0;

	if (*p_string == '-')
	{
		sign = -1;
		p_string++;
	}
	while (*p_string >= '0' && *p_string <= '9')
	{
		decimal *= 10;
		decimal += (i32)(*p_string - '0');
		p_string++;
	}

	const i32 result = sign * decimal;
	return result;
}

u32 tg_string_to_u32(const char* p_string)
{
	TG_ASSERT(p_string);
	TG_ASSERT(*p_string >= '0' && *p_string <= '9');

	u32 result = 0;

	while (*p_string >= '0' && *p_string <= '9')
	{
		result *= 10;
		result += (i32)(*p_string - '0');
		p_string++;
	}

	return result;
}
