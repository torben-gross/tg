# tg

This voxel raytracer uses a visibility buffer to store instance ID and voxel ID of respective voxels. Furthermore, sparse voxel octrees are utilized to approximate rotated voxel grids and compute indirect bounce lighting.

![alt text](https://github.com/torben-gross/tg/blob/master/tg/assets/textures/voxel_raytracer.png?raw=true)

Before switching the focus to voxel raytracing after commit [358e48c5](https://github.com/torben-gross/tg/commit/358e48c5), I experimented with infinite flat-shaded terrain using the [Transvoxel](https://transvoxel.org/)-algorithm. This included multithreaded updating of the terrain, [precomputed atmospheric scattering](https://ebruneton.github.io/precomputed_atmospheric_scattering/), PBR, and more.

![alt text](https://github.com/torben-gross/tg/blob/master/tg/assets/textures/transvoxel.png?raw=true)
