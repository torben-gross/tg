#include "tg/graphics/tg_graphics_vulkan.h"

#ifdef TG_VULKAN

typedef struct tg_scene
{
	u32                entity_count;
	tg_entity_h*       p_entities_h;
	struct render_data
	{
		u32            rendered_entity_count;
		v3*            p_positions;
		tg_model_h*    p_models_h;
	};
} tg_scene;

#endif
