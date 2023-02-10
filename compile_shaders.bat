@echo off
pushd .\\data\\shaders
for %%f in (*.vert) do glslc -o %%~nf.spv %%f
for %%f in (*.frag) do glslc -o %%~nf.spv %%f
popd