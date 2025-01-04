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
#include <dirent.h>
#include <fcntl.h>
#include <sys/syscall.h>
#ifdef __EMSCRIPTEN__
#include <wasi/api.h>
#include <wasi/wasi-helpers.h>
#endif

#include "drekkar_wa_core.h"
#include "drekkar_wa_env.h"

// Enable this macro if lots of debug logging is needed.
//#define DBG



#ifdef DBG
#define D(...) {fprintf(stdout, __VA_ARGS__);}
#else
#define D(...)
#endif



#define MAX_MEM_QUOTA 0x10000000

typedef struct wa_ciovec_type {
	uint32_t buf;
    uint32_t buf_len;
} wa_ciovec_type;


/*static void log_hex(const uint8_t *ptr, size_t n)
{
	for(size_t i = 0; i < n;i++)
	{
		printf("%02x", ptr[i]);
	}
}*/

// Tip: To know what the file should look like try something like:
//      od -t x1 hello_world.wasm
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
	return (d->sp + DWAC_SP_OFFSET) - d->fp;
}

static int is_param_ok(dwac_data *d, int expected_nof_params)
{
	const int nof_parameters_given = nof_parameters_on_stack(d);
	if (nof_parameters_given != expected_nof_params)
	{
		snprintf(d->exception, sizeof(d->exception), "Wrong number of parameters %d %d", nof_parameters_given, expected_nof_params);
		return 0;
	}
	return 1;
}

/*uint32_fd_write(int32_t  fd, uint32_t iovs_offset, uint32_t iovs_len, uint32_t nwritten_offset);*/
// https://wasix.org/docs/api-reference/wasi/fd_write
static void wa_fd_write(dwac_data *d)
{
	if (!is_param_ok(d, 4)) {return;}

    // POP last parameter first.
	uint32_t nwritten_offset = dwac_pop_value_i64(d);
    uint32_t iovs_len = dwac_pop_value_i64(d);
    uint32_t iovs_offset = dwac_pop_value_i64(d);
    int32_t  fd = dwac_pop_value_i64(d);

	int n = 0;

	// Translate from script internal to host addresses.
	const wa_ciovec_type* iovs_offset_ptr = (wa_ciovec_type*) (dwac_translate_to_host_addr_space(d, iovs_offset, 4));
	uint32_t* nwritten_offset_ptr = (uint32_t*) (dwac_translate_to_host_addr_space(d, nwritten_offset, 4));

	//printf("iovs_len %d\n", iovs_len);

	assert((fd>=1) && (fd<=2));
	for(unsigned int i=0; i < iovs_len; ++i)
	{
		const wa_ciovec_type *v = &iovs_offset_ptr[i];

		//printf(" <v->buf_len %d> ", v->buf_len);
		const uint8_t* ptr = (uint8_t*)dwac_translate_to_host_addr_space(d, v->buf, v->buf_len);

		write(fd, ptr, v->buf_len);
		n += v->buf_len;
	}

	*nwritten_offset_ptr = n;

	//snprintf(d->exception, sizeof(d->exception), "Not implemented: wa_fd_write");

	// Push return value.
	dwac_push_value_i64(d, WASI_ESUCCESS);
}

// int32_t emscripten_memcpy_big(int32_t dest, int32_t src, int32_t num);
// (import "env" "emscripten_memcpy_big" (func $fimport$1 (param i32 i32 i32) (result i32)))
static void memcpy_big(dwac_data *d)
{
	if (!is_param_ok(d, 3)) {return;}
	//long n = 0;

    // POP last parameter first.
	uint32_t num = dwac_pop_value_i64(d);
    uint32_t src = dwac_pop_value_i64(d);
    uint32_t dest = dwac_pop_value_i64(d);

	void* dest_ptr = dwac_translate_to_host_addr_space(d, dest, num);
	const void* src_ptr = dwac_translate_to_host_addr_space(d, src, num);
	memcpy(dest_ptr, src_ptr, num);

	dwac_push_value_i64(d, WASI_ESUCCESS);
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
	if (!is_param_ok(d, 1)) {return;}
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


// Test functions, remove later.
static void test_log_i64(dwac_data *d)
{
	if (!is_param_ok(d, 1)) {return;}
	uint64_t n = dwac_pop_value_i64(d);
	printf("log: %lld\n", (long long)n);
}
static void test_log_hex(dwac_data *d)
{
	if (!is_param_ok(d, 1)) {printf("test_log_hex\n"); return;}
	uint64_t n = dwac_pop_value_i64(d);
	printf("log: %llx\n", (long long)n);
}
static void test_log_ch(dwac_data *d)
{
	if (!is_param_ok(d, 1)) {return;}
	uint64_t n = dwac_pop_value_i64(d);
	printf("log: %c\n", (int)n);
}
static void test_log_str(dwac_data *d)
{
	if (!is_param_ok(d, 1)) {return;}
	uint64_t a = dwac_pop_value_i64(d);
	const uint8_t* ptr = (uint8_t*)dwac_translate_to_host_addr_space(d, a, 1);
	printf("log: '%s'\n", ptr);
}
static void log_empty_line(dwac_data *d)
{
	if (!is_param_ok(d, 0)) {printf("log_empty_line\n"); return;}
	printf("log:\n");
}


// https://refspecs.linuxbase.org/LSB_5.0.0/LSB-Core-generic/LSB-Core-generic/baselib---assert-fail-1.html
// void __assert_fail(const char * assertion, const char * file, unsigned int line, const char * function);
static void assert_fail(dwac_data *d)
{
	if (!is_param_ok(d, 4)) {return;}

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
// int open(const char *pathname, int flags, mode_t mode);
// Remember last argument pops up first.
static void syscall_open(dwac_data *d)
{
	if (!is_param_ok(d, 3)) {return;}

	int mode_i = dwac_pop_value_i64(d);
	int flags = dwac_pop_value_i64(d);
	int pathname_i = dwac_pop_value_i64(d);

	mode_t *mode = dwac_translate_to_host_addr_space(d, mode_i, sizeof(mode_t));
	const char* pathname = dwac_translate_to_host_addr_space(d, pathname_i, 1);

	int r = open(pathname, flags, *mode);

	printf("syscall_open '%s' 0x%x  %d\n", pathname, flags, r);

	dwac_push_value_i64(d, r);
}


// Not tested.
// 'env/__syscall_fcntl64' param i32 i32 i32, result i32'
static void syscall_fcntl64(dwac_data *d)
{
	if (!is_param_ok(d, 3)) {return;}

	/*uint32_t p2 =*/ dwac_pop_value_i64(d);
	/*uint32_t p1 =*/ dwac_pop_value_i64(d);
	/*uint32_t p0 =*/ dwac_pop_value_i64(d);

	// TODO
	snprintf(d->exception, sizeof(d->exception), "Not implemented: env/__syscall_fcntl64");

	dwac_push_value_i64(d, 0);
}

// Not tested.
// int ioctl(int fd, unsigned long request, ...);
//  (import "env" "__syscall_ioctl" (func $fimport$4 (param i32 i32 i32) (result i32)))
// 'env/__syscall_ioctl' param i32 i32 i32, result i32'
// https://github.com/WebAssembly/wasi-libc/blob/main/libc-top-half/musl/arch/mipsn32/bits/ioctl.h
//
// https://drewdevault.com/2022/05/14/generating-ioctls.html
// 14 May 2022 — There are thousands of ioctls provided by Linux,
// and each of them is assigned a constant like TIOCGWINSZ (0x5413).
// Some constants, including ...
//
//
static void syscall_ioctl(dwac_data *d)
{
	int nof_parameters_given = nof_parameters_on_stack(d);
	if (nof_parameters_given < 3)
	{
		snprintf(d->exception, sizeof(d->exception), "Insufficient number of parameters");
	}
	else if (nof_parameters_given>3)
	{
		printf("syscall_ioctl nof_parameters_given %d\n", nof_parameters_given);
		d->sp -= (nof_parameters_given - 3);
	}

	void* ptr = dwac_translate_to_host_addr_space(d, dwac_pop_value_i64(d), 1);
	unsigned long request = dwac_pop_value_i64(d);
	uint32_t fd = dwac_pop_value_i64(d);

	int r = ioctl(fd, request, ptr);
    if (r<0)
    {
    	// Typically errno is set if there was a fail.
    	int *e = (int *)dwac_translate_to_host_addr_space(d, d->errno_location, sizeof(int));
    	*e = errno;
    	printf("syscall_ioctl fail %d %ld 0x%lx %d %d '%s'\n", fd, request, request, r, errno, strerror(errno));
    }
    else
    {
    	printf("syscall_ioctl ok %d %ld  %d\n", fd, request, r);
    }

	dwac_push_value_i64(d, r);
}

// 'wasi_snapshot_preview1/fd_read' param i32 i32 i32 i32, result i32'
//
// https://wasix.org/docs/api-reference/wasi/fd_read
// In POSIX systems, reading data from a file typically involves
// updating the file cursor, which determines the next position
// from which data will be read. The fd_read() function allows
// reading data from a file without modifying the file cursor's
// position. This can be useful in scenarios where applications
// need to read data from a specific location in a file without
// altering the cursor's state.
//
// OMG! Why change everything all the time? And what do we
// need to do about it? Where then is file cursor updated?
//
// The implementation below is not as described above but is
// tested using emscripten.
static void fd_read(dwac_data *d)
{
	if (!is_param_ok(d, 4)) {return;}
	long n = 0;

    // POP last parameter first.
	uint32_t nread_offset = dwac_pop_value_i64(d);
    uint32_t iovs_len = dwac_pop_value_i64(d);
    uint32_t iovs_offset = dwac_pop_value_i64(d);
    int32_t  fd = dwac_pop_value_i64(d);

    D("fd_read %d %d %d %d\n", fd, iovs_offset, iovs_len, nread_offset);


	// Translate from script internal to host addresses.
	wa_ciovec_type* iovs_offset_ptr = (wa_ciovec_type*) (dwac_translate_to_host_addr_space(d, iovs_offset, 4));
	uint32_t* nread_offset_ptr = (uint32_t*) (dwac_translate_to_host_addr_space(d, nread_offset, 4));

	//printf("iovs_len %d\n", iovs_len);

	assert((fd>=0) || (fd<=3)); // TODO Don't hard code 3.
	for(unsigned int i=0; i < iovs_len; ++i)
	{
		wa_ciovec_type *v = &iovs_offset_ptr[i];

		//printf(" <v->buf_len %d> ", v->buf_len);
		uint8_t* ptr = (uint8_t*)dwac_translate_to_host_addr_space(d, v->buf, v->buf_len);

		ssize_t r = read(fd, ptr, v->buf_len);
		if (r>=0)
		{
			/*printf("fd_read ");
			log_hex(ptr, r);
			printf("\n");*/
			n += r;
		}
		else
		{
			printf("fd_read fail %zd\n", r);
			dwac_push_value_i64(d, r);
			return;
		}
	}

	*nread_offset_ptr = n;

    D("fd_read n = %ld\n", n);

	//snprintf(d->exception, sizeof(d->exception), "Not implemented:fd_read");

	// Push return value.
	dwac_push_value_i64(d, WASI_ESUCCESS);
}

// 'wasi_snapshot_preview1/fd_close' param i32, result i32'
//  (import "wasi_snapshot_preview1" "fd_close" (func $fimport$7 (param i32) (result i32)))
static void fd_close(dwac_data *d)
{
	if (!is_param_ok(d, 1)) {return;}

	uint32_t fd = dwac_pop_value_i64(d);

	assert(fd == 3); // TODO
	D("fd_close %d\n", fd);
	close(fd);

	dwac_push_value_i64(d, WASI_ESUCCESS);
}

// Not tested.
// 'env/__syscall_getcwd' param i32 i32, result i32'
static void syscall_getcwd(dwac_data *d)
{
	if (!is_param_ok(d, 2)) {return;}

	/*uint32_t p1 =*/ dwac_pop_value_i64(d);
	/*uint32_t p0 =*/ dwac_pop_value_i64(d);

	// TODO
	snprintf(d->exception, sizeof(d->exception), "Not implemented: env/__syscall_getcwd");

	dwac_push_value_i64(d, 0);
}

// Not tested.
// 'env/__syscall_readlink' param i32 i32 i32, result i32'
// ssize_t readlink(const char *restrict pathname, char *restrict buf, size_t bufsiz);
static void syscall_readlink(dwac_data *d)
{
	if (!is_param_ok(d, 3)) {return;}

	uint32_t bufsiz = dwac_pop_value_i64(d);
	uint32_t buf = dwac_pop_value_i64(d);
	uint32_t pathname = dwac_pop_value_i64(d);

	const char* pathname_ptr = dwac_translate_to_host_addr_space(d, pathname, 1);
	char* buf_ptr = dwac_translate_to_host_addr_space(d, buf, bufsiz);

	ssize_t r = readlink(pathname_ptr, buf_ptr, bufsiz);

	D("env/__syscall_readlink '%s' %zd %u '%s'\n", pathname_ptr, r, bufsiz, buf_ptr);

	dwac_push_value_i64(d, r);
}

// Not tested.
// 'env/__syscall_fstat64' param i32 i32, result i32'
static void syscall_fstat64(dwac_data *d)
{
	if (!is_param_ok(d, 2)) {return;}

	/*uint32_t p1 =*/ dwac_pop_value_i64(d);
	/*uint32_t p0 =*/ dwac_pop_value_i64(d);

	// TODO
	snprintf(d->exception, sizeof(d->exception), "Not implemented: env/__syscall_fstat64");

	dwac_push_value_i64(d, 0);
}

#if 0
// https://linux.die.net/man/2/getdents64
struct linux_dirent {
    unsigned long  d_ino;     /* Inode number */
    unsigned long  d_off;     /* Offset to next linux_dirent */
    unsigned short d_reclen;  /* Length of this linux_dirent */
    char           d_name[];  /* Filename (null-terminated) */
                        /* length is actually (d_reclen - 2 -
                           offsetof(struct linux_dirent, d_name) */
    /*
    char           pad;       // Zero padding byte
    char           d_type;    // File type (only since Linux 2.6.4;
                              // offset is (d_reclen - 1))
    */

};
#endif


struct guest_stat {
    uint32_t st_dev;
    uint32_t padding;
    uint32_t st_ino;
    uint32_t st_mode;
};

// Not tested.
// 'env/__syscall_stat64' param i32 i32, result i32'
// https://github.com/emscripten-core/emscripten/blob/main/system/lib/libc/musl/arch/emscripten/syscall_arch.h
//     int __syscall_stat64(intptr_t path, intptr_t buf);
// https://gist.github.com/mejedi/e0a5ee813c88effaa146ad6bd65fc482
// When I was experimenting here it turned out someone else was also at same time:
// https://github.com/emscripten-core/emscripten/issues/20840
static void syscall_stat64(dwac_data *d)
{
	if (!is_param_ok(d, 2)) {return;}

    #ifdef __EMSCRIPTEN__

	// Unable to use system calls
	// https://github.com/emscripten-core/emscripten/issues/6708
	const char* pathname = (const char*)dwac_translate_to_host_addr_space(d, dwac_pop_value_i64(d), 1);
	snprintf(d->exception, sizeof(d->exception), "Not implemented: env/__syscall_stat64 '%s'", pathname);
	int r = -1;

	#elif 1

	struct guest_stat *statbuf = (struct guest_stat *)dwac_translate_to_host_addr_space(d, dwac_pop_value_i64(d), sizeof(struct guest_stat));
	const char* pathname = (const char*)dwac_translate_to_host_addr_space(d, dwac_pop_value_i64(d), 256);

	struct stat sb;
	int r = stat(pathname, &sb);
	statbuf->st_dev = sb.st_dev;
	statbuf->st_ino = sb.st_ino;
	statbuf->st_mode = sb.st_mode;
	printf("stat '%s' %d %x %llx  %p %p  %p %p\n", pathname, r, sb.st_mode, (long long)statbuf->st_mode, &sb, &sb.st_mode, statbuf, &statbuf->st_mode);

    #else

	struct linux_dirent* statbuf = dwac_translate_to_host_addr_space(d, dwac_pop_value_i64(d), sizeof(struct linux_dirent));
	const char* pathname = (const char*)dwac_translate_to_host_addr_space(d, dwac_pop_value_i64(d), 1);

	//uint32_t r = syscall(SYS_getdents64, pathname, statbuf);
	uint32_t r = getdents64(pathname, statbuf);

    #endif

	// Typically errno is set if there was a fail.
	if (r<0)
	{
		// Typically errno is set if there was a fail.
		int *e = (int *)dwac_translate_to_host_addr_space(d, d->errno_location, sizeof(int));
		*e = errno;
	}

	dwac_push_value_i64(d, r);
}

// Not implemented.
// 'env/__syscall_lstat64' param i32 i32, result i32'
// int __syscall_lstat64(intptr_t path, intptr_t buf);
static void syscall_lstat64(dwac_data *d)
{
	if (!is_param_ok(d, 2)) {return;}

	/*uint32_t p1 =*/ dwac_pop_value_i64(d);
	/*uint32_t p0 =*/ dwac_pop_value_i64(d);

	// TODO
	snprintf(d->exception, sizeof(d->exception), "Not implemented: env/__syscall_lstat64");

	dwac_push_value_i64(d, 0);
}


// Not implemented.
// 'env/__syscall_fstatat64' param i32 i32 i32 i32, result i32'
//  (import "env" "__syscall_fstatat64" (func $fimport$12 (param i32 i32 i32 i32) (result i32)))
static void syscall_fstatat64(dwac_data *d)
{
	if (!is_param_ok(d, 4)) {return;}

	/*uint32_t p3 =*/ dwac_pop_value_i64(d);
	/*uint32_t p2 =*/ dwac_pop_value_i64(d);
	/*uint32_t p1 =*/ dwac_pop_value_i64(d);
	/*uint32_t p0 =*/ dwac_pop_value_i64(d);

	// TODO
	snprintf(d->exception, sizeof(d->exception), "Not implemented: env/__syscall_fstatat64");

	dwac_push_value_i64(d, 0);
}

// Not implemented.
// 'wasi_snapshot_preview1/fd_seek' param i32 i32 i32 i32 i32, result i32'
static void fd_seek(dwac_data *ctx)
{
	if (!is_param_ok(ctx, 5)) {return;}

	/*uint32_t p4 =*/ dwac_pop_value_i64(ctx);
	/*uint32_t p3 =*/ dwac_pop_value_i64(ctx);
	/*uint32_t p2 =*/ dwac_pop_value_i64(ctx);
	/*uint32_t p1 =*/ dwac_pop_value_i64(ctx);
	/*uint32_t p0 =*/ dwac_pop_value_i64(ctx);

	// TODO
	snprintf(ctx->exception, sizeof(ctx->exception), "Not implemented: wasi_snapshot_preview1/fd_seek");

	dwac_push_value_i64(ctx, 0);
}

static size_t wa_get_command_line_arguments_string_size(uint32_t argc, const char **argv)
{
	size_t tot_arg_size = 0;
	for (int i = 0; i < argc; ++i)
	{
		tot_arg_size += (strlen(argv[i]) + 1);
	}
	return tot_arg_size;
}

// little endian
static void put32(uint8_t *ptr, uint32_t v)
{
	ptr[0] = v;
	ptr[1] = v >> 8;
	ptr[2] = v >> 16;
	ptr[3] = v >> 24;
}

// https://wasix.org/docs/api-reference/wasi/args_sizes_get
static void args_sizes_get(dwac_data *ctx)
{
	if (!is_param_ok(ctx, 2)) {return;}

	uint32_t argv_buf_size = dwac_pop_value_i64(ctx);
	uint32_t argc = dwac_pop_value_i64(ctx);

	uint32_t* argc_ptr = (uint32_t*)dwac_translate_to_host_addr_space(ctx, argc, 4);
	uint32_t* argv_buf_size_ptr = (uint32_t*)dwac_translate_to_host_addr_space(ctx, argv_buf_size, 4);

	*argc_ptr = ctx->dwac_emscripten_argc;
	*argv_buf_size_ptr = wa_get_command_line_arguments_string_size(ctx->dwac_emscripten_argc, ctx->dwac_emscripten_argv);

	printf("args_sizes_get %u %u %d %zu\n", argc, argv_buf_size, ctx->dwac_emscripten_argc, ctx->memory.arguments.size);

	dwac_push_value_i64(ctx, 0);
}

// https://wasix.org/docs/api-reference/wasi/args_get
static void args_get(dwac_data *ctx)
{
	if (!is_param_ok(ctx, 2)) {return;}

	uint32_t argv_buf = dwac_pop_value_i64(ctx);
	uint32_t argv = dwac_pop_value_i64(ctx);

	printf("args_get %u %u\n", argv, argv_buf);

	uint8_t* argv_ptr = (uint8_t*)dwac_translate_to_host_addr_space(ctx, argv, (DWAC_PTR_SIZE * ctx->dwac_emscripten_argc));

	// Copy the strings over to guest memory.
	for (int i = 0; i < ctx->dwac_emscripten_argc; ++i)
	{
		put32(argv_ptr + (DWAC_PTR_SIZE * i), argv_buf);
		const uint32_t n = strlen(ctx->dwac_emscripten_argv[i]);
		uint8_t *ptr = dwac_translate_to_host_addr_space(ctx, argv_buf, n);
		memcpy(ptr, ctx->dwac_emscripten_argv[i], n);
		argv_buf += n + 1;
	}


	//const uint32_t memory_reserved_by_compiler_location = DWAC_ARGUMENTS_BASE;
	//uint8_t* memory_reserved_by_compiler_ptr = (uint8_t*)dwac_translate_to_host_addr_space(ctx, argv, (DWAC_PTR_SIZE * ctx->dwac_emscripten_argc) + ctx->dwac_emscripten_arg_string_size);

	// Copy the strings over to guest memory.
	// See also dwac_set_command_line_arguments.
	// The below is a quick and dirty since pointers are not actually pointing to the data copied but
	// still at the data copied from.
	/*for(int i = 0; i < ctx->dwac_emscripten_argc; i++)
	{
		uint32_t n = *(memory_reserved_by_compiler_ptr + (i * DWAC_PTR_SIZE));
		printf("n %u\n", (unsigned int)n);
		*argv_ptr = n;
	}*/
	//memcpy(argv_ptr, memory_reserved_by_compiler_ptr, DWAC_PTR_SIZE * ctx->dwac_emscripten_argc);
	//memcpy(argv_buf_ptr, memory_reserved_by_compiler_ptr + (DWAC_PTR_SIZE * ctx->dwac_emscripten_argc), ctx->arg_string_size_in_bytes);

	dwac_push_value_i64(ctx, 0);
}

// https://wasix.org/docs/api-reference/wasi/proc_exit
static void proc_exit(dwac_data *ctx)
{
	if (!is_param_ok(ctx, 1)) {return;}

	int64_t exit_code = dwac_pop_value_i64(ctx);

	snprintf(ctx->exception, sizeof(ctx->exception), "exit %lld", (long long int)exit_code);

	// TODO Program shall exit with this code.
	// exit(exit_code);
	// But this is not great if more than one guest program is running.
	// Will just put the result back on stack for now.

	dwac_push_value_i64(ctx, exit_code);
}

#ifndef __EMSCRIPTEN__
// 'env/__syscall_getdents64' param i32 i32 i32, result i32'
// fd, buf, BUF_SIZE
// https://linux.die.net/man/2/getdents64
// int getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count);
static void syscall_getdents64(dwac_data *d)
{
	if (!is_param_ok(d, 3)) {return;}

	uint32_t buf_size = dwac_pop_value_i64(d);
	uint32_t buf = dwac_pop_value_i64(d);
	uint32_t fd = dwac_pop_value_i64(d);


	const char* buf_ptr = (const char*)dwac_translate_to_host_addr_space(d, buf, buf_size);

	uint32_t nread = syscall(SYS_getdents64, fd, buf_ptr, buf_size);

	D("env/__syscall_getdents64 %x %x\n", fd, buf_size);

	dwac_push_value_i64(d, nread);
}
#endif

// To tell the runtime which functions we have available for it to call.
// NOTE! If the guest is to be fully sand boxed some of the functions below
// need to be disabled (comment out registration of those).
static void register_functions(dwac_prog *p)
{
	D("register_functions\n");

	dwac_register_function(p, "wasi_snapshot_preview1/fd_write", wa_fd_write);
	dwac_register_function(p, "wasi_snapshot_preview1/fd_read", fd_read);
	dwac_register_function(p, "wasi_snapshot_preview1/fd_close", fd_close);
	dwac_register_function(p, "wasi_snapshot_preview1/fd_seek", fd_seek);
	dwac_register_function(p, "wasi_snapshot_preview1/args_sizes_get", args_sizes_get);
	dwac_register_function(p, "wasi_snapshot_preview1/args_get", args_get);
	dwac_register_function(p, "wasi_snapshot_preview1/proc_exit", proc_exit);

	dwac_register_function(p, "env/__assert_fail", assert_fail);
	dwac_register_function(p, "env/emscripten_memcpy_big", memcpy_big);
	dwac_register_function(p, "env/emscripten_resize_heap", emscripten_resize_heap);
	dwac_register_function(p, "env/emscripten_memcpy_js", memcpy_big);
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
	dwac_register_function(p, "env/__syscall_lstat64", syscall_lstat64);
	#ifndef __EMSCRIPTEN__
	dwac_register_function(p, "env/__syscall_getdents64", syscall_getdents64);
	#endif

	dwac_register_function(p, "drekkar/wart_version", drekkar_wart_version);
	dwac_register_function(p, "drekkar/log_i64", test_log_i64);
	dwac_register_function(p, "drekkar/log_hex", test_log_hex);
	dwac_register_function(p, "drekkar/log_ch", test_log_ch);
	dwac_register_function(p, "drekkar/log_str", test_log_str);
	dwac_register_function(p, "drekkar/log_empty_line", log_empty_line);
}

static long check_exception(const dwac_prog *p, dwac_data *d, long r)
{
	if ((r != DWAC_NEED_MORE_GAS) && (r != DWAC_OK))
	{
		printf("exception %ld '%s'\n", r, d->exception);
		assert(d->exception[sizeof(d->exception)-1]==0);
		dwac_log_block_stack(p, d);
		d->exception[0] = 0;
	}
	else if (d->exception[0] != 0)
	{
		printf("Unhandled exception '%s'\n", d->exception);
		assert(d->exception[sizeof(d->exception)-1]==0);
		dwac_log_block_stack(p, d);
		d->exception[0] = 0;
		return DWAC_EXCEPTION;
	}
	else if (dwac_total_memory_usage(d) > MAX_MEM_QUOTA)
	{
		printf("To much memory used %lld > %d\n", dwac_total_memory_usage(d), MAX_MEM_QUOTA);
		assert(d->exception[sizeof(d->exception)-1]==0);
		dwac_log_block_stack(p, d);
		return DWAC_MAX_MEM_QUOTA_EXCEEDED;
	}
	return r;
}

static long set_command_line_arguments(dwac_env_type *e)
{
	D("set_command_line_arguments %d\n", e->argc);

	if (e->function_name)
	{
		// Not the C main function so push all arguments to stack as numbers.
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
		long r = dwac_set_command_line_arguments(e->d, e->argc, e->argv);
		r = check_exception(e->p, e->d, r);
		return r;
	}
}

static long parse_prog_sections(dwac_prog *p, dwac_data *d, uint8_t *bytes, size_t file_size, FILE *log)
{
	D("parse_prog_sections\n");
	const long r = dwac_parse_prog_sections(p, d, bytes, file_size, log);
	return check_exception(p, d, r);
}

static long parse_data_sections(const dwac_prog *p, dwac_data *d)
{
	D("parse_data_sections\n");
	const long r = dwac_parse_data_sections(p, d);
	return check_exception(p, d, r);;
}

static long call_and_run_exported_function(const dwac_prog *p, dwac_data *d, const dwac_function *f, FILE* log)
{
	D("call_and_run_exported_function\n");
	long long total_gas_usage = 0;
	long r = dwac_call_exported_function(p, d, f->func_idx);
	for(;;)
	{
		total_gas_usage += (DWAC_GAS - d->gas_meter);
		r = check_exception(p, d, r);
		if (r == DWAC_NEED_MORE_GAS)
		{
			// Guest has more work to do. Let it continue some more.
			r = dwac_tick(p, d);
		}
		else if (r == DWAC_OK)
		{
			// Guest is done.
			if (log)
			{
				dwac_report_result(p, d, f, log);
			    fprintf(log, "Total gas and memory usage: %lld %lld\n", total_gas_usage, dwac_total_memory_usage(d));
			}
			break;
		}
		else
		{
			return r;
		}
	}
	return r;
}

static long call_errno(dwac_env_type *e)
{
	D("call_errno\n");
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
	D("call_ctors\n");
	const dwac_function* f = dwac_find_exported_function(p, "__wasm_call_ctors");
	if (f != NULL)
	{
		return dwac_call_exported_function(p, d, f->func_idx);
	}
	return DWAC_OK;
}

static const dwac_function* find_main(const dwac_prog *p)
{
	D("find_main\n");
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
	D("find_and_call\n");
	long r = call_errno(e);
	r = check_exception(e->p, e->d, r);
	if (r) {return r;}

	r = call_ctors(e->p, e->d);
	r = check_exception(e->p, e->d, r);
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
	D("dwae_init\n");
	long r = 0;

	dwac_st_init();
	e->p = DWAC_ST_MALLOC(sizeof(dwac_prog));
	e->d = DWAC_ST_MALLOC(sizeof(dwac_data));

	dwac_linear_storage_8_init(&e->bytes);

	uint32_t file_size = load_file(&e->bytes, e->file_name);

	/*printf("hex:\n");
	log_hex(e->bytes.array, e->bytes.size);
	printf("\n");*/

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
	dwac_data_init(e->d);

	register_functions(e->p);

	r = parse_prog_sections(e->p, e->d, e->bytes.array, file_size, e->log);
	if (r)
	{
		dwae_deinit(e);
		return r;
	}

	return r;
}

long dwae_tick(dwac_env_type *e)
{
	D("dwae_tick\n");

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
	D("dwae_deinit\n");
	dwac_data_deinit(e->d, e->log);
	dwac_prog_deinit(e->p);
	dwac_linear_storage_8_deinit(&e->bytes);
	DWAC_ST_FREE(e->p);
	DWAC_ST_FREE(e->d);
	dwac_st_deinit();
}

