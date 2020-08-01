#ifndef TG_TODO
#define TG_TODO

/*
typedef struct tg_probe
{
	v3    position;
	v3    p_colors[6]; // x-neg, x-pos, y-neg, ...
} tg_probe;

- lay out probes in a 3d-grid at first
- raycast from probe position for each cubic face direction towards random direction
	around the hemisphere of the face (~64 rays per face)
- bias towards normal of face
- trace maybe one or two bounces?
- start tracing on CPU first and 'maybe' convert to GPU
- if first run, take result as light value. otherwise, blend them similar to
	(0.1 * new + 0.9 * present)
*/

#endif
