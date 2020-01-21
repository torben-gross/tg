#ifndef TG_STRING_H
#define TG_STRING_H

#include "tg/tg_common.h"

typedef struct
{
	uint64 length;
	char data[0];
} tg_string;

tg_string* tg_string_create(char* data);
void tg_string_destroy(tg_string* string);

#endif
