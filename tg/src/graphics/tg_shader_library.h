#ifndef TG_SHADER_LIBRARY_H
#define TG_SHADER_LIBRARY_H

#include "tg_common.h"

TG_DECLARE_HANDLE(tg_compute_shader);
TG_DECLARE_HANDLE(tg_fragment_shader);
TG_DECLARE_HANDLE(tg_vertex_shader);

void                    tg_shader_library_init(void);
void                    tg_shader_library_shutdown(void);

tg_compute_shader_h     tg_shader_library_get_compute_shader(const char* p_filename);
tg_fragment_shader_h    tg_shader_library_get_fragment_shader(const char* p_filename);
tg_vertex_shader_h      tg_shader_library_get_vertex_shader(const char* p_filename);

#endif
