@echo off
pushd .\\data\\shaders
for %%f in (*.vert) do %VK_SDK_PATH%/Bin/glslc.exe -o %%~nf.spv %%f -O0
for %%f in (*.frag) do %VK_SDK_PATH%/Bin/glslc.exe -o %%~nf.spv %%f -O0
popd