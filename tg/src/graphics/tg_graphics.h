#ifndef TG_GRAPHICS_H
#define TG_GRAPHICS_H

#include "graphics/tg_graphics_core.h"



#ifdef TG_VULKAN

#include "graphics/vulkan/tgvk_core.h"

#include "graphics/vulkan/tgvk_atmosphere.h"
#include "graphics/vulkan/tgvk_font.h"
#include "graphics/vulkan/tgvk_mesh.h"
#include "graphics/vulkan/tgvk_raytracer.h"
#include "graphics/vulkan/tgvk_render_target.h"

#elif TG_DIRECT_X_12

#error Not implemented
#include "graphics/dx12/tgdx12_core.h"

#include "graphics/dx12/tgdx12_atmosphere.h"
#include "graphics/dx12/tgdx12_font.h"
#include "graphics/dx12/tgdx12_mesh.h"
#include "graphics/dx12/tgdx12_raytracer.h"
#include "graphics/dx12/tgdx12_render_target.h"

#else

#error Not implemented

#endif



#endif
