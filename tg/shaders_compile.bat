@echo off

pushd assets\shaders
C:\VulkanSDK\1.2.131.2\Bin\glslc.exe shader.vert -o vert.spv
C:\VulkanSDK\1.2.131.2\Bin\glslc.exe shader.frag -o frag.spv
popd
