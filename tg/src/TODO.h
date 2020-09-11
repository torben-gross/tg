#ifndef TG_TODO
#define TG_TODO

/*
create LUT for normals for terrain (6 faces * 64 x * 64 y * 3 floats). instead of
normals, pass index to LUT and look up normal in vertex shader. this will reduce memory
consumption. the normal could potentially even be generated through that index w/o LUT,
which could reduce memory but reduce performance...

typedef struct tg_probe
{
	v3    position;
	v3    p_colors[6]; // x-neg, x-pos, y-neg, ...
} tg_probe;

- lay out probes in a 3d-grid at first
- raycast from probe position for each cubic face direction towards random direction
	around the hemisphere of the face (~64 rays per face)
- bias towards normal of face
- start tracing on CPU first and 'maybe' convert to GPU
- if first run, take result as light value. otherwise, blend them similar to
	(0.1 * new + 0.9 * present)
- create sparse voxel-octree for static and regular grid for dynamic entities. raytrace
	both, if performance allows it
*/

#endif
