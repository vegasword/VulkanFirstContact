@echo off
pushd .\\data\\shaders
for %%f in (*.vert) do glslc -o %%~nf.spv %%f -O0
for %%f in (*.frag) do glslc -o %%~nf.spv %%f -O0
popd