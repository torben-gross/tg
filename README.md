# TG - 3D Voxel Game Engine

![alt text](https://github.com/torben-gross/tg/blob/master/tg/assets/textures/voxel_raytracer.png?raw=true)

**TG is a voxel-based 3D game engine exploiting various ray tracing methods.** The engine runs on the Windows platform and utilizes [Vulkan](https://www.lunarg.com/vulkan-sdk/) as its graphics API. TG supports rendering of tremendous numbers of voxels in real-time and will support dynamic destruction/modification of the world in the near future. Everything in the engine is custom built, without the usage of any external libraries besides the Win32 API and the Vulkan SDK.

## Data model

The data layout is greatly inspired by Nanite, as presented by [Karis et al.](http://advances.realtimerendering.com/s2021/Karis_Nanite_SIGGRAPH_Advances_2021_final.pdf) in 2021.

### Clusters
Voxels are stored in clusters of the size 8³. Each voxel in a cluster can be indexed using 9 bit. For visibility buffer generation, it is sufficient to be able to tell, whether a voxel is solid or not. Therefore, each voxel is comprised of a single bit - 1 means solid; 0 means air. For shading, on the other hand, more detailed material information is required. Hence, a second cluster representation is maintained, which stores an 8-bit index per voxel, that points into a material LUT of up to 256 entries. This LUT can be defined on a per-object basis and may contain properties like albedo and roughness. Clusters are indexed using 31 bit. This way we can theoretically represent more than 10¹² voxels.

### Objects

Each object contains its own transform, so its rotation and translation. Furthermore, it stores the index of its first cluster and the number of clusters it contains in each axis-direction. Therefore, the extent of an object is always a multiple of 8 voxels in each of the three dimensions. Every cluster of an object can be indexed by the cluster's relative x, y, and z offset indices.

As mentioned earlier, each object may have their custom material LUT. To avoid duplication, each object simply contains an index into an array of LUTs, such that LUTs can be reused.

## Rendering

The rendering uses a combination of voxel-based ray tracing and [visibility buffering](https://jcgt.org/published/0002/02/04/) to achieve both incredible performance for first-hit computation and huge voxel counts. Furthermore, approximate SVOs are utilized to approximate real-time GI.

### Visibility Buffer Generation

- The bounding box (rotated, translated cube) of each cluster is rasterized using instancing and a simple vertex/fragment shader setup.
- In the vertex shader, each cluster is transformed using its relative index inside of the respective object and the objects world transform.
- In the fragment shader, the 8³ grid is ray traced using the method presented by [Amanatides et al.](https://www.researchgate.net/publication/2611491_A_Fast_Voxel_Traversal_Algorithm_for_Ray_Tracing) in 1987.
- If a hit is detected, a 64-bit unsigned integer is composed of
  - 24 bit for depth,
  - 31 bit for the cluster index and
  - 9 bit for the voxel index.
- Using 64-bit atomics and the [atomicMin](https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/atomicMin.xhtml) instruction, the content of the [depth and visibility buffer](https://advances.realtimerendering.com/s2021/Karis_Nanite_SIGGRAPH_Advances_2021_final.pdf#page=84) is generated.

### Real-Time GI

On start-up, we compute a _sparse voxel octree_ (SVO) around the player. As objects can be rotated and translated around the world freely, the SVO approximates the actual cluster content in a grid-like manner. To save memory, each voxel is represented using only a single bit. After computing the visibility buffer, we can shoot secondary rays and traverse the SVO to approximate GI.

The SVO may be updated at runtime, when objects move. This way, the GI solution always uses the current world representation.

## Installation And Usage

The repository comes with a `tg.sln` file in the root folder. The only additional dependency is [Vulkan SDK 1.3.204.1](https://vulkan.lunarg.com/sdk/home). If another version of the SDK is installed, or if it is not installed at `C:\VulkanSDK\1.3.204.1`, update the project as follows:

- Open the solution in Visual Studio (preferably version 2019, may be downloaded [here](https://visualstudio.microsoft.com/vs/older-downloads/))
- Select the project `tg`
- Press `ALT+ENTER` to open `Property Pages`
- Adjust `Configuration:` at the top to `All Configurations`
- Open `Configuration Properties` &rarr; `C/C++` &rarr; `General`
  - In `Additional Include Directories`, update the path `C:\VulkanSDK\1.3.204.1\Include` as required
- Open `Configuration Properties` &rarr; `Linker` &rarr; `General`
  - In `Additional Library Directories`, update the path `C:\VulkanSDK\1.3.204.1\Lib` as required

## Previous Work

Before switching the focus to voxel ray tracing after commit [358e48c5](https://github.com/torben-gross/tg/commit/358e48c5), this engine focussed on large-scale terrain rendering in a flat-shaded low-poly style. The terrain could be edited in real-time and was updated using multithreading.

![alt text](https://github.com/torben-gross/tg/blob/master/tg/assets/textures/transvoxel.png?raw=true)

A non-exhaustive list of features:

- Editable large-scale terrain (Using a modified version of _Transvoxel Algorithm_ by [Lengyel 2010](https://transvoxel.org/))
- Precomputed atmospheric scattering (Using a custom Vulkan implementation of the method presented by [Bruneton 2017](https://ebruneton.github.io/precomputed_atmospheric_scattering/))
- Deferred rendering for solid geometry
- Forward rendering for transparency
- PBR (NDF: GGX (Trowbridge-Reitz), Geometric Shadowing: Smith's Schlick-GGX, Fresnel Schlick [[Specular BRDF Reference]](http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html))
- Custom math library
- Custom obj-loader
- Custom bitmap-loader
- Normal computation for triangle meshes on GPU
- SVO and KD-tree generation from triangle meshes partially utilizing the GPU
- Custom font rendering
- Various memory management utilities
