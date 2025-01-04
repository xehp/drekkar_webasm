

To run a typical test here it needs to be compiled first:

emcc hello_world.c -g -o hello_world.wasm


Then run it:

../drekkar_webasm_runtime/drekkar_webasm_runtime hello_world.wasm

