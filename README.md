# drekkar_webasm

Drekkar WebAsm runtime environment<br>

This is a Web Assembly environment intended to be embedded in other applications.
The Web Assembly programs ".wasm" are typically written in "C" and compiled
with emscripten. It can also be used to drive Domain Specific Language (DSL).
Deterministic CPU usage (gas metering) and memory usage can be monitored in runtime.

To make it easy to integrate into other projects the number of files has
been keept at a minimum. Essentially two "C" source files. One to provide
the engine itself and one for the environment. The other files are 
the example main file, header files and test files.

This program was developed on Linux, it's not tested on other OSes.
To see what is tested check the test_code/reg_test.c file. To see what 
is not tested check the source code for comments about "not tested".

Copyright (C) 2023<br>
Henrik Bjorkman http://www.eit.se/hb<br>

GNU General Public License v3<br>
https://www.gnu.org/licenses/gpl-3.0.en.html<br>

IMPORTANT NOTICE! This version of this project is released under GPLv3.<br>
If your project is not open source you can't use this version!!!
You will then need to buy a closed source license from Drekkar AB.

CREDITS<br>
This project owes a lot to the WAC project, ref [3]. It's a lot easier
to have a working code example to look at than to only have the
specifications. You may do any changes to this code but must make sure
to mention that in history. You must also to keep a reference to the
originals, not just this project but also to the WAC project.
Thanks also to W3C [1] and Mozilla [2] for their online documentation.

To compile the test scripts some tools may be needed.<br>
sudo apt-get install binaryen emscripten gcc-multilib g++-multilib libedit-dev:i386<br>

References:<br>
[1] WebAssembly Core Specification Editor’s Draft, 7 November 2023<br>
[2] https://developer.mozilla.org/en-US/docs/WebAssembly/Reference<br>
[3] https://github.com/kanaka/wac/tree/master<br>

History:<br>
Created November 2023 by Henrik Bjorkman.<br><br>

http://www.drekkar.com/<br>
https://github.com/xehp/drekkar_webasm.git<br>



<hr>

This is a runtime environment for generic code, so can it run itself?

That is the final reg test, this is the output:

	henrik@aurora:~/git/drekkar_webasm$ cd drekkar_webasm_runtime/src/
	henrik@aurora:~/git/drekkar_webasm/drekkar_webasm_runtime/src$ emcc drekkar_wa_core.c drekkar_wa_env.c main.c
	main.c:100:32: warning: unknown warning group '-Wformat-truncation', ignored [-Wunknown-warning-option]
	#pragma GCC diagnostic ignored "-Wformat-truncation"
	                               ^
	1 warning generated.
	henrik@aurora:~/git/drekkar_webasm/drekkar_webasm_runtime/src$ ../drekkar_webasm_runtime a.out.wasm --function_name test ~/git/drekkar_webasm/test_code/test.wasm -4 5
	syscall_open 502400 8002 501240
	syscall_ioctl fail 3 21523 0x5413 -1 25 'Inappropriate ioctl for device'
	log:
	log: 'not zero'
	log:
	log: 11
	log: 400
	log: 'hello, world'
	log:
	henrik@aurora:~/git/drekkar_webasm/drekkar_webasm_runtime/src$ 

Some warnings but ignoring those, it looks like YES!
