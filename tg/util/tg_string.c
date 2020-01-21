#include "tg_string.h"
#include <stdlib.h>
#include <string.h>

tg_string* tg_string_create(char* string)
{
	uint64 len = (uint64)strlen(string);
	tg_string* str = (tg_string*)malloc(sizeof(*str) + (len + 1) * sizeof(char));
	if (str)
	{
		str->length = len;
		memcpy(((uint64*)str) + 1, string, (len + 1) * sizeof(*str->data));
		return str;
	}
	return NULL;
}

void tg_string_destroy(tg_string* string)
{
	free(string);
}
