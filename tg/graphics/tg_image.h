#ifndef TG_IMAGE
#define TG_IMAGE

#include "tg/tg_common.h"

typedef struct tgi_pixel
{
	ui8 r;
	ui8 g;
	ui8 b;
	ui8 a;
} tgi_pixel;

void tgi_load(const char* filename, ui32* width, ui32* height, tgi_pixel** data);
void tgi_load_bmp(const char* filename, ui32* width, ui32* height, tgi_pixel** data);

void tgi_free(tgi_pixel* data);

#endif
