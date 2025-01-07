

To run a typical test here it needs to be compiled first:

https://emscripten.org/docs/getting_started/downloads.html
source ~/downloaded_git/emsdk/emsdk_env.sh


emcc hello_world.c -g -o hello_world.wasm


Then run it:

../drekkar_webasm_runtime/drekkar_webasm_runtime hello_world.wasm



To verify the results use node (should give same output):
emcc hello_world.c -g -o hello_world.js
node hello_world.js

