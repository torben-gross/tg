@echo off

pushd assets\shaders
C:\VulkanSDK\1.2.131.2\Bin\glslc.exe shader.vert -o vert.spv
C:\VulkanSDK\1.2.131.2\Bin\glslc.exe shader.frag -o frag.spv
C:\VulkanSDK\1.2.131.2\Bin\glslc.exe present.vert -o present_vert.spv
C:\VulkanSDK\1.2.131.2\Bin\glslc.exe present.frag -o present_frag.spv
C:\VulkanSDK\1.2.131.2\Bin\glslc.exe geometry.vert -o geometry_vert.spv
C:\VulkanSDK\1.2.131.2\Bin\glslc.exe geometry.frag -o geometry_frag.spv
popd
