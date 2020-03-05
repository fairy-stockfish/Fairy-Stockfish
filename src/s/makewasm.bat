
rem build wasm executable

copy Makefile_wasm Makefile

make ARCH=wasm build -j

