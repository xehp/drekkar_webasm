/*
drekkar_wa_env.c

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
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#ifdef __EMSCRIPTEN__
#include <wasi/api.h>
#include <wasi/wasi-helpers.h>
#endif

#include "drekkar_wa_core.h"
#include "drekkar_wa_env.h"

#define MAX_MEM_QUOTA 0x10000000

typedef struct wa_ciovec_type {
	uint32_t buf;
    uint32_t buf_len;
} wa_ciovec_type;


static size_t load_file(dwac_linear_storage_8_type *storage, const char* file_path)
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
		dwac_linear_storage_8_push_uint8_t(storage, ch);
	}

	fclose(fp);
	return storage->size;
}

static int nof_parameters_on_stack(dwac_data *d)
{
	return d->sp - d->fp;
}


/*uint32_fd_write(int32_t  fd, uint32_t iovs_offset, uint32_t iovs_len, uint32_t nwritten_offset);*/
static void wa_fd_write(dwac_data *d)
{
	const int nof_results = 1;
	int nof_parameters_given = nof_parameters_on_stack(d) + nof_results;

	if (nof_parameters_given != 4)
	{
		snprintf(d->exception, sizeof(d->exception), "Wrong number of parameters");
	}

    // POP last parameter first.
	uint32_t nwritten_offset = dwac_pop_value_i64(d);
    uint32_t iovs_len = dwac_pop_value_i64(d);
    uint32_t iovs_offset = dwac_pop_value_i64(d);
    int32_t  fd = dwac_pop_value_i64(d);

	int n = 0;

	// Translate from script internal to host addresses.
	wa_ciovec_type* iovs_offset_ptr = (wa_ciovec_type*) (dwac_translate_to_host_addr_space(d, iovs_offset, 4));
	uint32_t* nwritten_offset_ptr = (uint32_t*) (dwac_translate_to_host_addr_space(d, nwritten_offset, 4));

	//printf("iovs_len %d\n", iovs_len);

	assert((fd>=1) && (fd<=2));
	for(unsigned int i=0; i < iovs_len; ++i)
	{
		wa_ciovec_type *v = &iovs_offset_ptr[i];

		//printf(" <v->buf_len %d> ", v->buf_len);
		const uint8_t* ptr = (uint8_t*)dwac_translate_to_host_addr_space(d, v->buf, v->buf_len);

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
	dwac_push_value_i64(d, WASI_ESUCCESS);
}

// Not tested.
// void memcpy_big(uint32_t dest, uint32_t src, uint32_t num);
static void wa_memcpy_big(dwac_data *d)
{
	uint32_t num = dwac_pop_value_i64(d);
    uint32_t dest = dwac_pop_value_i64(d);
    uint32_t src = dwac_pop_value_i64(d);

	void* dest_ptr = dwac_translate_to_host_addr_space(d, dest, num);
	void* src_ptr = dwac_translate_to_host_addr_space(d, src, num);
	memcpy(dest_ptr, src_ptr, num);
}

// https://github.com/emscripten-core/emscripten/issues/6024
// setTempRet0 is an import in that module, which you must provide. It's a temp
// value that is set for certain 64-bit operations when crossing from JS to wasm
// and back. For example, when wasm returns a 64-bit value to JS, it returns the
// low 32 bits and calls setTempRet0 with the higher 32 bits.
//
// Not tested.
static void setTempRet0(dwac_data *d)
{
	d->temp_value = dwac_pop_value_i64(d);
}

// Not tested.
static void getTempRet0(dwac_data *d)
{
	dwac_push_value_i64(d, d->temp_value);
}

// Import 0x2 'emscripten_resize_heap'  param i32, result i32
static void emscripten_resize_heap(dwac_data *d)
{
	uint64_t a = dwac_pop_value_i64(d);
    printf("emscripten_resize_heap %llu\n", (unsigned long long)a);
    // Will assume we just change current_size_in_pages.
    uint64_t requestedSize = ((a + (DWAC_PAGE_SIZE-1)) / DWAC_PAGE_SIZE) * DWAC_PAGE_SIZE;
    requestedSize = (requestedSize <= DWAC_ARGUMENTS_BASE) ? requestedSize : DWAC_ARGUMENTS_BASE;
    d->memory.current_size_in_pages = requestedSize / DWAC_PAGE_SIZE;
    if (d->memory.current_size_in_pages > d->memory.maximum_size_in_pages)
    {
    	printf("maximum_size_in_pages exceeded 0x%x > 0x%x\n", d->memory.current_size_in_pages, d->memory.maximum_size_in_pages);
    }

	dwac_push_value_i64(d, d->memory.current_size_in_pages * DWAC_PAGE_SIZE);
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
static void test_log_i64(dwac_data *d)
{
	uint64_t n = dwac_pop_value_i64(d);
	printf("log: %lld\n", (long long)n);
}

static void test_log_hex(dwac_data *d)
{
	uint64_t n = dwac_pop_value_i64(d);
	printf("log: %llx\n", (long long)n);
}

static void test_log_ch(dwac_data *d)
{
	uint64_t n = dwac_pop_value_i64(d);
	printf("log: %c\n", (int)n);
}

// Test function, remove later.
static void test_log_str(dwac_data *d)
{
	uint64_t a = dwac_pop_value_i64(d);
	const uint8_t* ptr = (uint8_t*)dwac_translate_to_host_addr_space(d, a, 1);
	printf("log: '%s'\n", ptr);
}

static void log_empty_line(dwac_data *d)
{
	printf("log:\n");
}


// https://refspecs.linuxbase.org/LSB_5.0.0/LSB-Core-generic/LSB-Core-generic/baselib---assert-fail-1.html
// void __assert_fail(const char * assertion, const char * file, unsigned int line, const char * function);
static void assert_fail(dwac_data *d)
{
	// param i32 i32 i32 i32
	uint32_t func = dwac_pop_value_i64(d);
	uint32_t line = dwac_pop_value_i64(d);
	uint32_t file = dwac_pop_value_i64(d);
	uint32_t cond = dwac_pop_value_i64(d);

	const uint8_t* cond_str = (uint8_t*)dwac_translate_to_host_addr_space(d, cond, 256);
	const uint8_t* file_name = (uint8_t*)dwac_translate_to_host_addr_space(d, file, 256);
	const uint8_t* func_name = (uint8_t*)dwac_translate_to_host_addr_space(d, func, 256);

	snprintf(d->exception, sizeof(d->exception), "Assertion failed: %.32s %.32s %u %.32s", cond_str, file_name, line, func_name);
}

static void drekkar_wart_version(dwac_data *d)
{
	uint64_t v = ((uint64_t)DREKKAR_VERSION_MAJOR << 32) | (DREKKAR_VERSION_MINOR << 16) | DREKKAR_VERSION_PATCH;
	dwac_push_value_i64(d, v);
}


// 'env/__syscall_open' param i32 i32 i32, result i32'
//  (import "env" "__syscall_open" (func $fimport$2 (param i32 i32 i32) (result i32)))
// https://man7.org/linux/man-pages/man2/open.2.html
// Remember last argument pops up first.
static void syscall_open(dwac_data *d)
{
	const int nof_results = 1;
	int nof_parameters_given = nof_parameters_on_stack(d) + nof_results;
	if (nof_parameters_given < 3)
	{
		snprintf(d->exception, sizeof(d->exception), "Wrong number of parameters");
	}
	else if (nof_parameters_given>3)
	{
		d->sp -= nof_parameters_given - 3;
	}

	mode_t *mode = dwac_translate_to_host_addr_space(d, dwac_pop_value_i64(d), sizeof(mode_t));
	int* flags = (int*)dwac_translate_to_host_addr_space(d, dwac_pop_value_i64(d), 4);
	const char* pathname = dwac_translate_to_host_addr_space(d, dwac_pop_value_i64(d), 256);


	// TODO
	//snprintf(d->exception, sizeof(d->exception), "Not implemented: env/__syscall_open '%s' %d\n", pathname, *flags);


	int r = open(pathname, *flags, *mode);

	printf("syscall_open '%s' %d  %d\n", pathname, *flags, r);

	dwac_push_value_i64(d, r);
}


// 'env/__syscall_fcntl64' param i32 i32 i32, result i32'
static void syscall_fcntl64(dwac_data *d)
{
	uint32_t p2 = dwac_pop_value_i64(d);
	uint32_t p1 = dwac_pop_value_i64(d);
	uint32_t p0 = dwac_pop_value_i64(d);

	// TODO
	snprintf(d->exception, sizeof(d->exception), "Not implemented: env/__syscall_fcntl64");

	dwac_push_value_i64(d, 0);
}

// int ioctl(int fd, unsigned long request, ...);
// 'env/__syscall_ioctl' param i32 i32 i32, result i32'
static void syscall_ioctl(dwac_data *d)
{
	const int nof_results = 1;
	int nof_parameters_given = nof_parameters_on_stack(d) + nof_results;
	if (nof_parameters_given < 3)
	{
		snprintf(d->exception, sizeof(d->exception), "Wrong number of parameters");
	}
	else if (nof_parameters_given>3)
	{
		printf("syscall_ioctl nof_parameters_given %d\n", nof_parameters_given);
		d->sp -= (nof_parameters_given - 3);
	}


	void* p2 = dwac_translate_to_host_addr_space(d, dwac_pop_value_i64(d), 4);
	unsigned long request = dwac_pop_value_i64(d);
	uint32_t fd = dwac_pop_value_i64(d);

	int r = ioctl(fd, request, p2);
    if (r<0)
    {
    	// Typically errno is set if there was a fail.
    	int *e = (int *)dwac_translate_to_host_addr_space(d, d->errno_location, sizeof(int));
    	*e = errno;
    	printf("syscall_ioctl fail %d %ld  %d %d '%s'\n", fd, request, r, errno, strerror(errno));
    }
    else
    {
    	printf("syscall_ioctl ok %d %ld  %d\n", fd, request, r);
    }

	dwac_push_value_i64(d, r);
}

// 'wasi_snapshot_preview1/fd_read' param i32 i32 i32 i32, result i32'
static void fd_read(dwac_data *d)
{
	uint32_t p3 = dwac_pop_value_i64(d);
	uint32_t p2 = dwac_pop_value_i64(d);
	uint32_t p1 = dwac_pop_value_i64(d);
	uint32_t p0 = dwac_pop_value_i64(d);

	// TODO
	snprintf(d->exception, sizeof(d->exception), "Not implemented: wasi_snapshot_preview1/fd_read");

	dwac_push_value_i64(d, 0);
}

// 'wasi_snapshot_preview1/fd_close' param i32, result i32'
static void fd_close(dwac_data *d)
{
	uint32_t p0 = dwac_pop_value_i64(d);

	// TODO
	snprintf(d->exception, sizeof(d->exception), "Not implemented: wasi_snapshot_preview1/fd_close");

	dwac_push_value_i64(d, 0);
}

// 'env/__syscall_getcwd' param i32 i32, result i32'
static void syscall_getcwd(dwac_data *d)
{
	uint32_t p1 = dwac_pop_value_i64(d);
	uint32_t p0 = dwac_pop_value_i64(d);

	// TODO
	snprintf(d->exception, sizeof(d->exception), "Not implemented: env/__syscall_getcwd");

	dwac_push_value_i64(d, 0);
}

// 'env/__syscall_readlink' param i32 i32 i32, result i32'
static void syscall_readlink(dwac_data *d)
{
	uint32_t p2 = dwac_pop_value_i64(d);
	uint32_t p1 = dwac_pop_value_i64(d);
	uint32_t p0 = dwac_pop_value_i64(d);

	// TODO
	snprintf(d->exception, sizeof(d->exception), "Not implemented: env/__syscall_readlink");

	dwac_push_value_i64(d, 0);
}

// 'env/__syscall_fstat64' param i32 i32, result i32'
static void syscall_fstat64(dwac_data *d)
{
	uint32_t p1 = dwac_pop_value_i64(d);
	uint32_t p0 = dwac_pop_value_i64(d);

	// TODO
	snprintf(d->exception, sizeof(d->exception), "Not implemented: env/__syscall_fstat64");

	dwac_push_value_i64(d, 0);
}

// 'env/__syscall_stat64' param i32 i32, result i32'
// https://github.com/emscripten-core/emscripten/blob/main/system/lib/libc/musl/arch/emscripten/syscall_arch.h
//     int __syscall_stat64(intptr_t path, intptr_t buf);
// https://gist.github.com/mejedi/e0a5ee813c88effaa146ad6bd65fc482
static void syscall_stat64(dwac_data *d)
{
	// Remember last argument pops up first.
	struct stat *statbuf = (struct stat*)dwac_translate_to_host_addr_space(d, dwac_pop_value_i64(d), sizeof(struct stat));
	const char* pathname = (const char*)dwac_translate_to_host_addr_space(d, dwac_pop_value_i64(d), 256);

	// This is not tested!
	int r = stat(pathname, statbuf);

	// Typically errno is set if there was a fail.
    if (r<0)
    {
    	// Typically errno is set if there was a fail.
    	int *e = (int *)dwac_translate_to_host_addr_space(d, d->errno_location, sizeof(int));
    	*e = errno;
    }

	printf("__syscall_stat64 %s %d\n", pathname, r);

	dwac_push_value_i64(d, r);
}

// 'env/__syscall_lstat64' param i32 i32, result i32'
// int __syscall_lstat64(intptr_t path, intptr_t buf);
static void syscall_lstat64(dwac_data *d)
{
	uint32_t p1 = dwac_pop_value_i64(d);
	uint32_t p0 = dwac_pop_value_i64(d);

	// TODO
	snprintf(d->exception, sizeof(d->exception), "Not implemented: env/__syscall_lstat64");

	dwac_push_value_i64(d, 0);
}


// 'env/__syscall_fstatat64' param i32 i32 i32 i32, result i32'
//  (import "env" "__syscall_fstatat64" (func $fimport$12 (param i32 i32 i32 i32) (result i32)))
static void syscall_fstatat64(dwac_data *d)
{
	uint32_t p3 = dwac_pop_value_i64(d);
	uint32_t p2 = dwac_pop_value_i64(d);
	uint32_t p1 = dwac_pop_value_i64(d);
	uint32_t p0 = dwac_pop_value_i64(d);

	// TODO
	snprintf(d->exception, sizeof(d->exception), "Not implemented: env/__syscall_fstatat64");

	dwac_push_value_i64(d, 0);
}

// 'wasi_snapshot_preview1/fd_seek' param i32 i32 i32 i32 i32, result i32'
static void fd_seek(dwac_data *d)
{
	uint32_t p4 = dwac_pop_value_i64(d);
	uint32_t p3 = dwac_pop_value_i64(d);
	uint32_t p2 = dwac_pop_value_i64(d);
	uint32_t p1 = dwac_pop_value_i64(d);
	uint32_t p0 = dwac_pop_value_i64(d);

	// TODO
	snprintf(d->exception, sizeof(d->exception), "Not implemented: wasi_snapshot_preview1/fd_seek");

	dwac_push_value_i64(d, 0);
}

// To tell the runtime which functions we have available for it to call.
static void register_functions(dwac_prog *p)
{
	dwac_register_function(p, "wasi_snapshot_preview1/fd_write", wa_fd_write);
	dwac_register_function(p, "wasi_snapshot_preview1/fd_read", fd_read);
	dwac_register_function(p, "wasi_snapshot_preview1/fd_close", fd_close);
	dwac_register_function(p, "wasi_snapshot_preview1/fd_seek", fd_seek);
	dwac_register_function(p, "env/__assert_fail", assert_fail);
	dwac_register_function(p, "env/emscripten_memcpy_big", wa_memcpy_big);
	dwac_register_function(p, "env/emscripten_resize_heap", emscripten_resize_heap);
	dwac_register_function(p, "env/emscripten_memcpy_js", wa_memcpy_big);
	dwac_register_function(p, "env/setTempRet0", setTempRet0);
	dwac_register_function(p, "env/getTempRet0", getTempRet0);
	dwac_register_function(p, "env/__syscall_open", syscall_open);
	dwac_register_function(p, "env/__syscall_fcntl64", syscall_fcntl64);
	dwac_register_function(p, "env/__syscall_ioctl", syscall_ioctl);
	dwac_register_function(p, "env/__syscall_getcwd", syscall_getcwd);
	dwac_register_function(p, "env/__syscall_readlink", syscall_readlink);
	dwac_register_function(p, "env/__syscall_fstat64", syscall_fstat64);
	dwac_register_function(p, "env/__syscall_stat64", syscall_stat64);
	dwac_register_function(p, "env/__syscall_fstatat64", syscall_fstatat64);
	dwac_register_function(p, "env/__syscall_stat64", syscall_stat64);
	dwac_register_function(p, "env/__syscall_lstat64", syscall_lstat64);

	dwac_register_function(p, "drekkar/wart_version", drekkar_wart_version);
	dwac_register_function(p, "drekkar/log_i64", test_log_i64);
	dwac_register_function(p, "drekkar/log_hex", test_log_hex);
	dwac_register_function(p, "drekkar/log_ch", test_log_ch);
	dwac_register_function(p, "drekkar/log_str", test_log_str);
	dwac_register_function(p, "drekkar/log_empty_line", log_empty_line);
}

static long parse_prog_sections(dwac_prog *p, uint8_t *bytes, size_t file_size, char* exception, size_t size_exception, FILE *log)
{
	const long r = dwac_parse_prog_sections(p, bytes, file_size, exception, size_exception, log);
	if ((r != 0) || (exception[0] != 0))
	{
		printf("exception: %lld '%s'\n", (long long)r, exception);
	}
	return r;
}

static long parse_data_sections(const dwac_prog *p, dwac_data *d)
{
	const long r = dwac_parse_data_sections(p, d);
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


static long set_command_line_arguments(dwac_env_type *e)
{
	if (e->function_name)
	{
		// Push all arguments to stack as numbers.
		for(int i = 0; i < e->argc; i++)
		{
			int64_t n = atoll(e->argv[i]);
			dwac_push_value_i64(e->d, n);
		}
		return 0;
	}
	else
	{
		// Provide arguments to the main function as argc/argv.
		e->argv[0] = e->file_name;
		const long r = dwac_set_command_line_arguments(e->d, e->argc, e->argv);
		assert(e->d->exception[sizeof(e->d->exception)-1]==0);
		if (r)
		{
			printf("exception: %ld %s\n", r, e->d->exception);
			e->d->exception[0] = 0;
		}
		else if (e->d->exception[0] != 0)
		{
			printf("Unhandled exception: %s\n", e->d->exception);
			e->d->exception[0] = 0;
		}
		return r;
	}
}

// TODO Move this to drekkar_core.c
static long long total_memory_usage(dwac_data *d)
{
	return d->memory.lower_mem.capacity +
	(d->memory.upper_mem.end - d->memory.upper_mem.begin) +
	d->memory.arguments.capacity +
	(d->globals.capacity * 8) +
	(d->block_stack.capacity * sizeof(dwac_block_stack_entry)) +
	DWAC_STACK_SIZE * 8 +
	d->pc.nof;
}

static long report_result(const dwac_prog *p, dwac_data *d, const dwac_function *f, FILE* log)
{
	assert(log);
	long ret_val = 0;
	// If the called function had a return value it should be on the stack.
	// Log the values on stack.
	fprintf(log, "Stack:\n");
	while (d->sp != DWAC_SP_INITIAL) {
		const dwac_func_type_type* type = dwac_get_func_type_ptr(p, f->func_type_idx);
		uint32_t nof_results = type->nof_results;
		if (d->sp < nof_results)
		{
			dwac_value_type *v = &d->stack[d->sp];
			uint8_t t = type->results_list[d->sp];
			char tmp[64];
			dwac_value_and_type_to_string(tmp, sizeof(tmp), v, t);
			fprintf(log, "  %s\n", tmp);
		}
		else
		{
			fprintf(log, "  0x%llx\n", (long long)d->stack[d->sp].s64);
		}
		ret_val = d->stack[d->sp].s64;
		d->sp--;
	}
	assert(d->exception[sizeof(d->exception)-1]==0);
	d->exception[0] = 0;
	fprintf(log, "Return value from guest: %ld\n", ret_val);
	return ret_val;
}


static long call_and_run_exported_function(const dwac_prog *p, dwac_data *d, const dwac_function *f, FILE* log)
{
	long long total_gas_usage = 0;
	long r = dwac_call_exported_function(p, d, f->func_idx);
	for(;;)
	{
		total_gas_usage += (DWAC_GAS - d->gas_meter);
		if ((r != DWAC_NEED_MORE_GAS) && (r != DWAC_OK))
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
		else if (r == DWAC_NEED_MORE_GAS)
		{
			if (total_memory_usage(d) > MAX_MEM_QUOTA)
			{
				printf("To much memory used %lld > %d\n", total_memory_usage(d), MAX_MEM_QUOTA);
				return DWAC_MAX_MEM_QUOTA_EXCEEDED;
			}

			// Guest has more work to do. Let it continue some more.
			r = dwac_tick(p, d);
		}
		else if (r == DWAC_OK)
		{
			if (log)
			{
				report_result(p, d, f, log);
			    fprintf(log, "Total gas and memory usage: %lld %lld\n", total_gas_usage, total_memory_usage(d));
			}
			break;
		}
	}
	return r;
}

static long call_errno(dwac_env_type *e)
{
	const dwac_function* f = dwac_find_exported_function(e->p, "__errno_location");
	if (f != NULL)
	{
		long r  = dwac_call_exported_function(e->p, e->d, f->func_idx);
		e->d->errno_location = dwac_pop_value_i64(e->d);
		return r;
	}
	return DWAC_OK;
}

static long call_ctors(const dwac_prog *p, dwac_data *d)
{
	const dwac_function* f = dwac_find_exported_function(p, "__wasm_call_ctors");
	if (f != NULL)
	{
		return dwac_call_exported_function(p, d, f->func_idx);
	}

	return DWAC_OK;
}

static const dwac_function* find_main(const dwac_prog *p)
{
	const dwac_function *f = NULL;

	if (f == NULL) {f = dwac_find_exported_function(p, "__main_argc_argv");}
	if (f == NULL) {f = dwac_find_exported_function(p, "main");}
	if (f == NULL) {f = dwac_find_exported_function(p, "_start");}
	if (f == NULL) {f = dwac_find_exported_function(p, "start");}
	if (f == NULL) {f = dwac_find_exported_function(p, "test");}

	return f;
}

static long find_and_call(dwac_env_type *e)
{
	// Do we need to call "__wasm_call_ctors" also?
	long r = call_errno(e);
	if (r) {return r;}

	r = call_ctors(e->p, e->d);
	if (r) {return r;}

	const dwac_function *f;
	if (e->function_name)
	{
		f = dwac_find_exported_function(e->p, e->function_name);
		if (!f) {
			printf("Did not find function '%s'.\n", e->function_name);
			return DWAC_FUNCTION_NOT_FOUND;
		}
	}
	else
	{
		f = find_main(e->p);
		if (!f) {
			printf("Did not find main or start function.\n");
			return DWAC_FUNCTION_NOT_FOUND;
		}
	}

	return call_and_run_exported_function(e->p, e->d, f, e->log);
}

// Returns zero (WA_OK) if OK.
long dwae_init(dwac_env_type *e)
{
	long r = 0;
	char exception[256] = {0};

	dwac_st_init();
	e->p = DWAC_ST_MALLOC(sizeof(dwac_prog));
	e->d = DWAC_ST_MALLOC(sizeof(dwac_data));

	dwac_linear_storage_8_init(&e->bytes);

	uint32_t file_size = load_file(&e->bytes, e->file_name);

	if (file_size < 8)
	{
		printf("File not found (or too small): '%s', file_size %u.\n", e->file_name, file_size);
		return DWAC_FILE_NOT_FOUND;
	}
	else
	{
		if (e->log) {fprintf(e->log, "File loaded '%s' (%d bytes).\n", e->file_name, (int)e->bytes.size);}
	}

	dwac_prog_init(e->p);

	register_functions(e->p);

	r = parse_prog_sections(e->p, e->bytes.array, file_size, exception, sizeof(exception), e->log);
	assert(exception[sizeof(exception)-1]==0);
	if (r) {dwac_prog_deinit(e->p); return r;}

	dwac_data_init(e->d);

	return r;
}

long dwae_tick(dwac_env_type *e)
{
	long r = 0;

	r = parse_data_sections(e->p, e->d);
	if (r) {return r;}

	r = set_command_line_arguments(e);
	if (r) {return r;}

	r = find_and_call(e);
	if (r) {return r;}

	return r;
}

void dwae_deinit(dwac_env_type *e)
{
	dwac_data_deinit(e->d, e->log);
	dwac_prog_deinit(e->p);
	dwac_linear_storage_8_deinit(&e->bytes);
	DWAC_ST_FREE(e->p);
	DWAC_ST_FREE(e->d);
	dwac_st_deinit();
}

