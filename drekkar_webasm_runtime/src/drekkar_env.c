/*
wa_env.c

Drekkar WebAsm runtime environment
https://www.drekkar.com/
https://github.com/xehp/drekkar_webasm.git

The ambition was to support a wasi environment for an embedded WebAsm program.
Currently only "fd.write" is implemented so there is a little more work to do.


This is enough to run "hello, world". To run our "hello_arg.c" program
compile the test file with emscripten.

Some tools may be needed:
sudo apt-get install binaryen emscripten gcc-multilib g++-multilib libedit-dev:i386

Then compile with:
emcc hello_arg.c

Copyright (C) 2023 Henrik Bjorkman http://www.eit.se/hb/.


Created October 2023 by Henrik
*/


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>

#include "drekkar_core.h"
#include "drekkar_env.h"

//#define RUN_TEST_WASM "test_const"

#define MAX_MEM_QUOTA 0x10000000

typedef struct wa_ciovec_type {
	uint32_t buf;
    uint32_t buf_len;
} wa_ciovec_type;


static size_t load_file(drekkar_linear_storage_8_type *storage, const char* file_path)
{
	FILE *fp = fopen(file_path, "r+");
	if (fp == NULL) {
		printf("File not found '%s'\n", file_path);
		return 0;
	}

	for(;;)
	{
		const int ch = fgetc(fp);
		if (ch < 0)
		{
			break;
		}
		drekkar_linear_storage_8_push_uint8_t(storage, ch);
	}

	fclose(fp);
	printf("Bytes loaded from file '%s' %d\n", file_path, (int)storage->size);
	return storage->size;
}


/*uint32_fd_write(int32_t  fd, uint32_t iovs_offset, uint32_t iovs_len, uint32_t nwritten_offset);*/
static void wa_fd_write(drekkar_wa_data *d)
{
    // POP last parameter first.
	uint32_t nwritten_offset = drekkar_wa_pop_value_i64(d);
    uint32_t iovs_len = drekkar_wa_pop_value_i64(d);
    uint32_t iovs_offset = drekkar_wa_pop_value_i64(d);
    int32_t  fd = drekkar_wa_pop_value_i64(d);

	int n = 0;

	// Translate from script internal to host addresses.
	wa_ciovec_type* iovs_offset_ptr = (wa_ciovec_type*) (drekkar_wa_translate_to_host_addr_space(d, iovs_offset, 4));
	uint32_t* nwritten_offset_ptr = (uint32_t*) (drekkar_wa_translate_to_host_addr_space(d, nwritten_offset, 4));

	//printf("iovs_len %d\n", iovs_len);

	assert((fd>=1) && (fd<=2));
	for(unsigned int i=0; i < iovs_len; ++i)
	{
		wa_ciovec_type *v = &iovs_offset_ptr[i];

		//printf(" <v->buf_len %d> ", v->buf_len);
		const uint8_t* ptr = (uint8_t*)drekkar_wa_translate_to_host_addr_space(d, v->buf, v->buf_len);

		/*if (v->buf_len > 0x10000)
		{
			snprintf(d->exception, sizeof(d->exception), "fd_write: To long 0x%x.", v->buf_len);
			uint32_t buf_len = 16;
			char buf[0x4000];
			wa_hex(buf, sizeof(buf), ptr, buf_len);
			printf("%s\n", buf);
			n += buf_len;
		}
		else*/
		{
			write(fd, ptr, v->buf_len);
			n += v->buf_len;
		}
	}

	*nwritten_offset_ptr = n;

	// Push return value.
	drekkar_wa_push_value_i64(d, WASI_ESUCCESS);
}

// Not tested.
// void memcpy_big(uint32_t dest, uint32_t src, uint32_t num);
static void wa_memcpy_big(drekkar_wa_data *d)
{
	uint32_t num = drekkar_wa_pop_value_i64(d);
    uint32_t dest = drekkar_wa_pop_value_i64(d);
    uint32_t src = drekkar_wa_pop_value_i64(d);

	void* dest_ptr = drekkar_wa_translate_to_host_addr_space(d, dest, num);
	void* src_ptr = drekkar_wa_translate_to_host_addr_space(d, src, num);
	memcpy(dest_ptr, src_ptr, num);
}

// https://github.com/emscripten-core/emscripten/issues/6024
// setTempRet0 is an import in that module, which you must provide. It's a temp
// value that is set for certain 64-bit operations when crossing from JS to wasm
// and back. For example, when wasm returns a 64-bit value to JS, it returns the
// low 32 bits and calls setTempRet0 with the higher 32 bits.
//
// Not tested.
static void setTempRet0(drekkar_wa_data *d)
{
	d->temp_value = drekkar_wa_pop_value_i64(d);
}

// Not tested.
static void getTempRet0(drekkar_wa_data *d)
{
	drekkar_wa_push_value_i64(d, d->temp_value);
}

// Import 0x2 'emscripten_resize_heap'  param i32, result i32
static void emscripten_resize_heap(drekkar_wa_data *d)
{
	uint64_t a = drekkar_wa_pop_value_i64(d);
    printf("emscripten_resize_heap %llu\n", (unsigned long long)a);
    // Will assume we just change current_size_in_pages.
    uint64_t requestedSize = ((a + (DREKKAR_WA_PAGE_SIZE-1)) / DREKKAR_WA_PAGE_SIZE) * DREKKAR_WA_PAGE_SIZE;
    requestedSize = (requestedSize <= DREKKAR_ARGUMENTS_BASE) ? requestedSize : DREKKAR_ARGUMENTS_BASE;
    d->memory.current_size_in_pages = requestedSize / DREKKAR_WA_PAGE_SIZE;
    if (d->memory.current_size_in_pages > d->memory.maximum_size_in_pages)
    {
    	printf("maximum_size_in_pages exceeded 0x%x > 0x%x\n", d->memory.current_size_in_pages, d->memory.maximum_size_in_pages);
    }

	drekkar_wa_push_value_i64(d, d->memory.current_size_in_pages * DREKKAR_WA_PAGE_SIZE);
}

#if 0
// https://stackoverflow.com/questions/67498446/how-to-pass-command-line-arguments-to-c-code-with-webassembly-and-js
// In that case you would want to call _start rather than main
void wa_args_get(Module *m)
{

}

void wa_args_sizes_get(Module *m)
{

}
#endif


// Test function, remove later.
static void test_log(drekkar_wa_data *d)
{
	uint64_t n = drekkar_wa_pop_value_i64(d);
	printf("log: %lld\n", (long long)n);
}

// Test function, remove later.
static void test_hello(drekkar_wa_data *d)
{
	(void)d;
	printf("Hello!\n");
}

// https://refspecs.linuxbase.org/LSB_5.0.0/LSB-Core-generic/LSB-Core-generic/baselib---assert-fail-1.html
// void __assert_fail(const char * assertion, const char * file, unsigned int line, const char * function);
static void assert_fail(drekkar_wa_data *d)
{
	// param i32 i32 i32 i32
	uint32_t func = drekkar_wa_pop_value_i64(d);
	uint32_t line = drekkar_wa_pop_value_i64(d);
	uint32_t file = drekkar_wa_pop_value_i64(d);
	uint32_t cond = drekkar_wa_pop_value_i64(d);

	const uint8_t* cond_str = (uint8_t*)drekkar_wa_translate_to_host_addr_space(d, cond, 256);
	const uint8_t* file_name = (uint8_t*)drekkar_wa_translate_to_host_addr_space(d, file, 256);
	const uint8_t* func_name = (uint8_t*)drekkar_wa_translate_to_host_addr_space(d, func, 256);

	snprintf(d->exception, sizeof(d->exception), "Assertion failed: %.32s %.32s %u %.32s", cond_str, file_name, line, func_name);
}

static void drekkar_wrt_version(drekkar_wa_data *d)
{
	uint64_t v = ((uint64_t)DREKKAR_VERSION_MAJOR << 32) | (DREKKAR_VERSION_MINOR << 16) | DREKKAR_VERSION_PATCH;
	drekkar_wa_push_value_i64(d, v);
}

// To tell the runtime which functions we have available for it to call.
static void register_functions(drekkar_wa_prog *p)
{
	drekkar_wa_register_function(p, "fd_write", wa_fd_write);
	drekkar_wa_register_function(p, "__assert_fail", assert_fail);
	drekkar_wa_register_function(p, "emscripten_memcpy_big", wa_memcpy_big);
	drekkar_wa_register_function(p, "emscripten_resize_heap", emscripten_resize_heap);
	drekkar_wa_register_function(p, "emscripten_memcpy_js", wa_memcpy_big);
	drekkar_wa_register_function(p, "setTempRet0", setTempRet0);
	drekkar_wa_register_function(p, "getTempRet0", getTempRet0);
	drekkar_wa_register_function(p, "drekkar_wrt_version", drekkar_wrt_version);
	drekkar_wa_register_function(p, "test_log", test_log);
	drekkar_wa_register_function(p, "test_hello", test_hello);
}

static long parse_prog_sections(drekkar_wa_prog *p, uint8_t *bytes, size_t file_size, char* exception, size_t size_exception, FILE *log)
{
	const long r = drekkar_wa_parse_prog_sections(p, bytes, file_size, exception, size_exception, log);
	if ((r != 0) || (exception[0] != 0))
	{
		printf("exception: %lld '%s'\n", (long long)r, exception);
	}
	return r;
}

static long parse_data_sections(const drekkar_wa_prog *p, drekkar_wa_data *d)
{
	const long r = drekkar_wa_parse_data_sections(p, d);
	assert(d->exception[sizeof(d->exception)-1]==0);
	if (r)
	{
		printf("exception: %ld %s\n", r, d->exception);
		d->exception[0] = 0;
	}
	else if (d->exception[0] != 0)
	{
		printf("Unhandled exception: %s\n", d->exception);
		d->exception[0] = 0;
	}
	return r;
}


static long set_command_line_arguments(drekkar_wa_data *d, uint32_t argc, const char **argv)
{
	#ifdef RUN_TEST_WASM
	// The test web assembly will expect two values on stack.
	wa_push_value_I32(d, 11);
	wa_push_value_I32(d, 13);
	return 0;
	#else

	const long r = drekkar_wa_set_command_line_arguments(d, argc, argv);
	assert(d->exception[sizeof(d->exception)-1]==0);
	if (r)
	{
		printf("exception: %ld %s\n", r, d->exception);
		d->exception[0] = 0;
	}
	else if (d->exception[0] != 0)
	{
		printf("Unhandled exception: %s\n", d->exception);
		d->exception[0] = 0;
	}
	return r;
	#endif
}

// TODO Move this to drekkar_core.c
static long long total_memory_usage(drekkar_wa_data *d)
{
	return d->memory.lower_mem.capacity +
	(d->memory.upper_mem.end - d->memory.upper_mem.begin) +
	d->memory.arguments.size +
	(d->globals.capacity * 8) +
	(d->block_stack.capacity * sizeof(drekkar_block_stack_entry)) +
	DREKKAR_STACK_SIZE +
	d->pc.nof;
}

static void report_result(const drekkar_wa_prog *p, drekkar_wa_data *d, const drekkar_wa_function *f)
{
	// If the called function had a return value it should be on the stack.
	// Log the values on stack.
	printf("Stack:\n");
	while (d->sp != DREKKAR_SP_INITIAL) {
		const drekkar_wa_func_type_type* type = drekkar_get_func_type_ptr(p, f->func_type_idx);
		uint32_t nof_results = type->nof_results;
		if (d->sp < nof_results)
		{
			drekkar_wa_value_type *v = &d->stack[d->sp];
			uint8_t t = type->results_list[d->sp];
			char tmp[64];
			drekkar_wa_value_and_type_to_string(tmp, sizeof(tmp), v, t);
			printf("  %s\n", tmp);
		}
		else
		{
			printf("  0x%llx\n", (long long)d->stack[d->sp].s64);
		}
		d->sp--;
	}
	assert(d->exception[sizeof(d->exception)-1]==0);
	d->exception[0] = 0;
}


static long call_and_run_exported_function(const drekkar_wa_prog *p, drekkar_wa_data *d, const drekkar_wa_function *f)
{
	long long total_gas_usage = 0;
	long r = drekkar_wa_call_exported_function(p, d, f->func_idx);
	for(;;)
	{
		total_gas_usage += (DREKKAR_GAS - d->gas_meter);
		if ((r != DREKKAR_WA_NEED_MORE_GAS) && (r != DREKKAR_WA_OK))
		{
			printf("exception %ld '%s'\n", r, d->exception);
			assert(d->exception[sizeof(d->exception)-1]==0);
			d->exception[0] = 0;
			break;
		}
		else if (d->exception[0] != 0)
		{
			printf("Unhandled exception '%s'\n", d->exception);
			d->exception[0] = 0;
			break;
		}
		else if (r == DREKKAR_WA_NEED_MORE_GAS)
		{
			if (total_memory_usage(d) > MAX_MEM_QUOTA)
			{
				printf("To much memory used %lld > %d\n", total_memory_usage(d), MAX_MEM_QUOTA);
				return DREKKAR_WA_MAX_MEM_QUOTA_EXCEEDED;
			}

			// Guest has more work to do. Let it continue some more.
			r = drekkar_wa_tick(p, d);
		}
		else if (r == DREKKAR_WA_OK)
		{
			report_result(p,d, f);
			printf("Total gas and memory usage: %lld %lld\n", total_gas_usage, total_memory_usage(d));
			break;
		}
	}
	return r;
}

static long call_ctors(const drekkar_wa_prog *p, drekkar_wa_data *d)
{
	const drekkar_wa_function* f = drekkar_wa_find_exported_function(p, "__wasm_call_ctors");
	if (f != NULL)
	{
		return drekkar_wa_call_exported_function(p, d, f->func_idx);
	}
	return DREKKAR_WA_OK;
}

static const drekkar_wa_function* find_main(const drekkar_wa_prog *p)
{
	const drekkar_wa_function *f = NULL;

	#ifdef RUN_TEST_WASM
	if (f == NULL) {f = drekkar_wa_find_exported_function(p, RUN_TEST_WASM);}
	#endif

	if (f == NULL) {f = drekkar_wa_find_exported_function(p, "__main_argc_argv");}

	if (f == NULL) {f = drekkar_wa_find_exported_function(p, "main");}

	if (f == NULL) {f = drekkar_wa_find_exported_function(p, "_start");}

	if (f == NULL) {f = drekkar_wa_find_exported_function(p, "start");}

	if (f == NULL) {f = drekkar_wa_find_exported_function(p, "test");}

	return f;
}

static long find_and_call(const drekkar_wa_prog *p, drekkar_wa_data *d)
{
	// Do we need to call "__wasm_call_ctors" also?
	long r = call_ctors(p, d);
	if (r) {return r;}


	const drekkar_wa_function *f = find_main(p);
	if (!f) {
		printf("Did not find main or start function.\n");
		return DREKKAR_WA_FUNCTION_NOT_FOUND;
	}

	r = call_and_run_exported_function(p, d, f);

	return r;
}

// Returns zero (WA_OK) if OK.
long drekkar_wa_env_init(drekkar_wa_env_type *e)
{
	long r = 0;
	char exception[256] = {0};

	e->p = DREKKAR_ST_MALLOC(sizeof(drekkar_wa_prog));
	e->d = DREKKAR_ST_MALLOC(sizeof(drekkar_wa_data));

	drekkar_linear_storage_8_init(&e->bytes);

	uint32_t file_size = load_file(&e->bytes, e->file_name);

	if (file_size < 8)
	{
		printf("test_was: file_size: %u\n", file_size);
		return DREKKAR_WA_FILE_NOT_FOUND;
	}

	drekkar_wa_prog_init(e->p);

	register_functions(e->p);

	r = parse_prog_sections(e->p, e->bytes.array, file_size, exception, sizeof(exception), e->log);
	assert(exception[sizeof(exception)-1]==0);
	if (r) {drekkar_wa_prog_deinit(e->p); return r;}

	drekkar_wa_data_init(e->d);

	return r;
}

long drekkar_wa_env_tick(drekkar_wa_env_type *e)
{
	long r = 0;

	e->argv[0] = e->file_name;

	r = parse_data_sections(e->p, e->d);
	if (r) {return r;}

	r = set_command_line_arguments(e->d, e->argc, e->argv);
	if (r) {return r;}

	r = find_and_call(e->p, e->d);
	if (r) {return r;}

	return r;
}

void drekkar_wa_env_deinit(drekkar_wa_env_type *e)
{
	drekkar_wa_data_deinit(e->d, e->log);
	drekkar_wa_prog_deinit(e->p);
	drekkar_linear_storage_8_deinit(&e->bytes);
	DREKKAR_ST_FREE(e->p);
	DREKKAR_ST_FREE(e->d);
}

