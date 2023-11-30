/* drekkar_core.h
 *
 * Drekkar WebAsm runtime environment
 * http://www.drekkar.com
 * https://github.com/xehp/drekkar_webasm.git
 *
 * References:
 * [1] WebAssembly Core Specification Editor’s Draft, 7 November 2023
 *     https://webassembly.github.io/spec/core/bikeshed/
 *     https://webassembly.github.io/spec/core/_download/WebAssembly.pdf
 * [2] https://developer.mozilla.org/en-US/docs/WebAssembly/Reference
 * [3] https://github.com/kanaka/wac/tree/master
 *
 * Copyright (C) 2023 Henrik Bjorkman http://www.eit.se/hb
 *
 * History:
 * Created November 2023 by Henrik Bjorkman.
 */
#ifndef DREKKAR_CORE_H
#define DREKKAR_CORE_H

#include <stdint.h>



// Begin of file version.h


// Don't edit VER_STR & VER_XSTR, these are only used internally in version.h.
#define DREKKAR_VER_XSTR(s) DREKKAR_VER_STR(s)
#define DREKKAR_VER_STR(s) #s




// Enter name and version numbers here:

#define DREKKAR_VERSION_NAME "DrekkarWebAsm"

// Semantic Versioning (https://semver.org/) shall be used.
// Given a version number MAJOR.MINOR.PATCH, increment the:
//    MAJOR version when you make incompatible API changes
//    MINOR version when you add functionality in a backward compatible manner
//    PATCH version when you make backward compatible bug fixes
// Warning! We do not follow this at early stages when major is 0.
#define DREKKAR_VERSION_MAJOR 0
#define DREKKAR_VERSION_MINOR 5
#define DREKKAR_VERSION_PATCH 0


#define DREKKAR_VERSION_STRING DREKKAR_VERSION_NAME " " DREKKAR_VER_XSTR(DREKKAR_VERSION_MAJOR) "." DREKKAR_VER_XSTR(DREKKAR_VERSION_MINOR) "." DREKKAR_VER_XSTR(DREKKAR_VERSION_PATCH)



// End of file version.h





// Begin of file hash_list.h


// Size must be power of 2.
#define DREKKAR_HASH_LIST_INIT_SIZE 32
#define DREKKAR_HASH_LIST_MAX_KEY_SIZE (64)


typedef struct drekkar_hash_entry drekkar_hash_entry;
typedef struct drekkar_hash_list drekkar_hash_list;

// TODO Perhaps we can trust that key is stored by someone else and just keep the pointer here?
struct drekkar_hash_entry
{
	void* ptr;
	char key[DREKKAR_HASH_LIST_MAX_KEY_SIZE+1]; // +1 for a terminating zero.
};


struct drekkar_hash_list
{
	long size;
	long capacity;
	drekkar_hash_entry* array;
};

void drekkar_hash_list_init(drekkar_hash_list *list);
void drekkar_hash_list_deinit(drekkar_hash_list *list);
long drekkar_hash_list_put(drekkar_hash_list *list, const char* key_ptr, void* ptr);
void* drekkar_hash_list_find(const drekkar_hash_list *list, const char* key_ptr);


// End of file hash_list.h




// Begin of file linear_storage_64.h



typedef struct drekkar_linear_storage_64_type drekkar_linear_storage_64_type;


struct drekkar_linear_storage_64_type
{
	size_t size;
	size_t capacity;
	uint64_t* array;
};

void drekkar_linear_storage_64_init(drekkar_linear_storage_64_type *list);
void drekkar_linear_storage_64_deinit(drekkar_linear_storage_64_type *list);

void drekkar_linear_storage_64_grow_if_needed(drekkar_linear_storage_64_type *s, size_t needed_size);
void drekkar_linear_storage_64_set(drekkar_linear_storage_64_type *s, size_t idx, const uint64_t u);
uint64_t drekkar_linear_storage_64_get(drekkar_linear_storage_64_type *s, size_t idx);
void drekkar_linear_storage_64_push(drekkar_linear_storage_64_type *s, uint64_t value);
uint64_t drekkar_linear_storage_64_pop(drekkar_linear_storage_64_type *s);

// End of file linear_storage_64.h



// Begin of file linear_storage_8.h



typedef struct drekkar_linear_storage_8_type drekkar_linear_storage_8_type;


struct drekkar_linear_storage_8_type
{
	size_t size;
	size_t capacity;
	uint8_t* array;
};

void drekkar_linear_storage_8_init(drekkar_linear_storage_8_type *list);
void drekkar_linear_storage_8_deinit(drekkar_linear_storage_8_type *list);

void drekkar_linear_storage_8_grow_if_needed(drekkar_linear_storage_8_type *s, size_t needed_size_in_bytes);
void drekkar_linear_storage_8_set_mem(drekkar_linear_storage_8_type *s, size_t offset, const uint8_t *src, size_t nof_bytes);
void drekkar_linear_storage_8_set_uint64_t(drekkar_linear_storage_8_type *s, size_t offset, const uint64_t u);
void drekkar_linear_storage_8_set_uint32_t(drekkar_linear_storage_8_type *s, size_t offset, const uint32_t u);
void drekkar_linear_storage_8_set_uint16_t(drekkar_linear_storage_8_type *s, size_t offset, const uint16_t u);
void drekkar_linear_storage_8_set_uint8_t(drekkar_linear_storage_8_type *s, size_t offset, const uint8_t u);
void* drekkar_linear_storage_8_get_ptr(drekkar_linear_storage_8_type *s, size_t offset, size_t nof_bytes);
uint64_t drekkar_linear_storage_8_get_uint64_t(drekkar_linear_storage_8_type *s, size_t offset);
uint32_t drekkar_linear_storage_8_get_uint32_t(drekkar_linear_storage_8_type *s, size_t offset);
uint16_t drekkar_linear_storage_8_get_uint16_t(drekkar_linear_storage_8_type *s, size_t offset);
uint8_t drekkar_linear_storage_8_get_uint8_t(drekkar_linear_storage_8_type *s, size_t offset);
void drekkar_linear_storage_8_push_uint64_t(drekkar_linear_storage_8_type *s, const uint64_t u);
void drekkar_linear_storage_8_push_uint32_t(drekkar_linear_storage_8_type *s, const uint32_t u);
void drekkar_linear_storage_8_push_uint16_t(drekkar_linear_storage_8_type *s, const uint16_t u);
void drekkar_linear_storage_8_push_uint8_t(drekkar_linear_storage_8_type *s, const uint8_t u);

// End of file linear_storage_8.h





// Begin of file linear_storage_size.h


typedef struct drekkar_linear_storage_size_type drekkar_linear_storage_size_type;


struct drekkar_linear_storage_size_type
{
	long size; // In number of elements.
	size_t capacity;  // -"-
	uint8_t* array;
	size_t element_size; // In bytes.
};

void drekkar_linear_storage_size_init(drekkar_linear_storage_size_type *list, size_t element_size);
void drekkar_linear_storage_size_deinit(drekkar_linear_storage_size_type *list);

void drekkar_linear_storage_size_grow_if_needed(drekkar_linear_storage_size_type *s, size_t needed_size);

void drekkar_linear_storage_size_set(drekkar_linear_storage_size_type *s, size_t idx, const uint8_t* ptr);
void* drekkar_linear_storage_size_get(drekkar_linear_storage_size_type *s, size_t idx);
void* drekkar_linear_storage_size_push(drekkar_linear_storage_size_type *s);
void* drekkar_linear_storage_size_pop(drekkar_linear_storage_size_type *s);
void* drekkar_linear_storage_size_top(drekkar_linear_storage_size_type *s);


// End of file linear_storage_size.h



// Begin of file sys_time.h




// If using C++ there is a another way:
// http://www.cplusplus.com/faq/sequences/arrays/sizeof-array/#cpp
#ifndef SIZEOF_ARRAY
//#define SIZEOF_ARRAY( a ) (sizeof( a ) / sizeof( a[ 0 ] ))
#define SIZEOF_ARRAY(a) ((sizeof(a)/sizeof(0[a])) / ((size_t)(!(sizeof(a) % sizeof(0[a])))))
#endif


int64_t drekkar_st_get_time_us();
void drekkar_st_init();
void drekkar_st_deinit();



// Here are some functions and macros to help debugging memory leaks.
// Performance will be affected.
// Define macro DREKKAR_ST_DEBUG to get more debugging info.
// Or define macro NDEBUG to get less.
// For medium level do not define any of the two.
// WARNING DREKKAR_ST_DEBUG is not thread safe. So do not use DREKKAR_ST_DEBUG if multi threading is used.
//#define DREKKAR_ST_DEBUG
//#define NDEBUG


#ifndef DREKKAR_ST_DEBUG

void* drekkar_st_malloc(size_t size);
void* drekkar_st_calloc(size_t num, size_t size);
void drekkar_st_free(void* ptr);
int drekkar_st_is_valid_size(const void* ptr, size_t size);
void* drekkar_st_resize(void* ptr, size_t old_size, size_t new_size);
void* drekkar_st_realloc(void* ptr, size_t new_size);
void* drekkar_st_recalloc(void *ptr, size_t old_num, size_t new_num, size_t size);
int drekkar_st_is_valid_min(const void* ptr, size_t size);
size_t drekkar_st_size(const void* ptr);

#ifndef NDEBUG
#define DREKKAR_ST_MALLOC(size) drekkar_st_malloc(size)
#define DREKKAR_ST_CALLOC(num, size) drekkar_st_calloc(num, size)
#define DREKKAR_ST_FREE(ptr) {drekkar_st_free(ptr); ptr = NULL;}
#define DREKKAR_ST_ASSERT_SIZE(ptr, size) assert(st_is_valid(ptr, size))
#define DREKKAR_ST_RESIZE(ptr, old_size, new_size) drekkar_st_resize(ptr, old_size, new_size);
#define DREKKAR_ST_ASSERT_MIN(ptr, size) assert(drekkar_st_is_valid_min(ptr, size))
#define DREKKAR_ST_FREE_SIZE(ptr, size) {assert(drekkar_st_is_valid_size(ptr, size)); drekkar_st_free(ptr); ptr = NULL;}
#define DREKKAR_ST_REALLOC(ptr, new_size) drekkar_st_realloc(ptr, new_size);
#define ST_RECALLOC(ptr, old_num, new_num, size) drekkar_st_recalloc(ptr, old_num, new_num, size);
#else
#define DREKKAR_ST_MALLOC(size) malloc(size);
#define DREKKAR_ST_CALLOC(num, size) calloc(num, size)
#define DREKKAR_ST_FREE(ptr) free(ptr)
#define DREKKAR_ST_ASSERT_SIZE(ptr, size)
#define DREKKAR_ST_ASSERT_MIN(ptr, size)
#define DREKKAR_ST_FREE_SIZE(ptr, size) free(ptr))
#endif

#else

void* drekkar_st_malloc(size_t size, const char *file, unsigned int line);
void* drekkar_st_calloc(size_t num, size_t size, const char *file, unsigned int line);
void drekkar_st_free(const void* ptr, const char *file, unsigned int line);
int drekkar_st_is_valid_size(const void* ptr, size_t size);
void* drekkar_st_resize(void* ptr, size_t old_size, size_t new_size, const char *file, unsigned int line);
void* drekkar_st_realloc(void* ptr, size_t new_size, const char *file, unsigned int line);
int drekkar_st_is_valid_min(const void* ptr, size_t size);
size_t drekkar_st_size(const void* ptr);
void drekkar_st_log_linked_list();

#define DREKKAR_ST_MALLOC(size) drekkar_st_malloc(size, __FILE__, __LINE__)
#define DREKKAR_ST_CALLOC(num, size) drekkar_st_calloc(num, size, __FILE__, __LINE__)
#define DREKKAR_ST_FREE(ptr) {drekkar_st_free(ptr, __FILE__, __LINE__); ptr = NULL;}
#define DREKKAR_ST_ASSERT_SIZE(ptr, size) assert(drekkar_st_is_valid_size(ptr, size))
#define DREKKAR_ST_RESIZE(ptr, old_size, new_size) drekkar_st_resize(ptr, old_size, new_size, __FILE__, __LINE__);
#define DREKKAR_ST_ASSERT_MIN(ptr, size) assert(drekkar_st_is_valid_min(ptr, size))
#define DREKKAR_ST_FREE_SIZE(ptr, size) {assert(drekkar_st_is_valid_size(ptr, size)); drekkar_st_free(ptr, __FILE__, __LINE__); ptr = NULL;}
#define DREKKAR_ST_REALLOC(ptr, new_size) drekkar_st_realloc(ptr, new_size, __FILE__, __LINE__);

#endif

// End of file sys_time.h


// Begin of file virtual_storage.h


typedef struct drekkar_virtual_storage_type drekkar_virtual_storage_type;


struct drekkar_virtual_storage_type
{
	size_t begin;
	size_t end;
	size_t inc;
	uint8_t* array;
};

void drekkar_virtual_storage_init(drekkar_virtual_storage_type *list);
void drekkar_virtual_storage_deinit(drekkar_virtual_storage_type *list);
void drekkar_virtual_storage_grow_buffer_if_needed(drekkar_virtual_storage_type *s, size_t begin, size_t nof_bytes, size_t min, size_t max);

void drekkar_virtual_storage_set_mem(drekkar_virtual_storage_type *s, size_t offset, const uint8_t *src, size_t nof_bytes);
void drekkar_virtual_storage_set_uint64_t(drekkar_virtual_storage_type *s, size_t offset, const uint64_t i);
void drekkar_virtual_storage_set_uint32_t(drekkar_virtual_storage_type *s, size_t offset, const uint32_t i);
void drekkar_virtual_storage_set_uint16_t(drekkar_virtual_storage_type *s, size_t offset, const uint16_t i);
void drekkar_virtual_storage_set_uint8_t(drekkar_virtual_storage_type *s, size_t offset, const uint8_t i);
void* drekkar_virtual_storage_get_ptr(drekkar_virtual_storage_type *s, size_t offset, size_t nof_bytes);
uint64_t drekkar_virtual_storage_get_uint64_t(drekkar_virtual_storage_type *s, size_t offset);
uint32_t drekkar_virtual_storage_get_uint32_t(drekkar_virtual_storage_type *s, size_t offset);
uint16_t drekkar_virtual_storage_get_uint16_t(drekkar_virtual_storage_type *s, size_t offset);
uint8_t drekkar_virtual_storage_get_uint8_t(drekkar_virtual_storage_type *s, size_t offset);

// End of file virtual_storage.h



// Begin of file wa_leb.h


typedef struct drekkar_leb128_reader_type drekkar_leb128_reader_type;

struct drekkar_leb128_reader_type
{
	size_t pos;
	size_t nof;
	const uint8_t* array;
	long errors;
};


// End of file wa_leb.h








// Define this macro if gas metering feature is needed.
// This defines the number of operations to do per tick.
// Comment the line below out if gas metering is not need.
#define DREKKAR_GAS 0x10000

// Shall code be translated during load of program or when a function is called?
// Define macro below if it shall be during load.
//#define TRANSLATE_ALL_AT_LOAD

#define DREKKAR_WA_MAGIC 0x6d736100
#define DREKKAR_WA_VERSION 0x01

// NOTE Stack size must be a power of 2 since we use a mask to prevent
// a stack overflow to write outside buffer. The stack can still overflow
// but that way a stack over or under flow can't write outside the stack.
#define DREKKAR_STACK_SIZE  0x10000

// Fun story: When testing WA_STACK_SIZE larger than than 0x40000 it didn't work.
// Turned out my test program placed the WaData on stack (host computers)
// And it got to big for its stack.

// An alternative for the stack and memory is to use memmap and allocate 4GiByte for each.

// Starting stack at -1 instead of 0 was an optimization found by examining ref [3].
// So we need an initial value for the stack pointer that is not zero.
#define DREKKAR_SP_OFFSET 1
#if DREKKAR_STACK_SIZE == 0x10000
#define drekkar_stack_pointer_type uint16_t
#define DREKKAR_SP_INITIAL ((uint16_t)(-1))
#else
#define drekkar_stack_pointer_type uint32_t
#define DREKKAR_SP_INITIAL ((uint32_t)(-1))
#endif

//#define SKIP_FLOAT

// Ref [1] 4.2.8. Memory Instances -> One page is 64Ki bytes.
// It seems ref [3] had page size as 0x10000*sizeof(uint32_t)
// that seems wrong, will keep it at 0x10000 in bytes.
#define DREKKAR_WA_PAGE_SIZE 0x10000

#define DREKKAR_ARGUMENTS_BASE 0xFF000000
#define DREKKAR_MAX_NOF_PAGES (DREKKAR_ARGUMENTS_BASE / 0x10000)


enum {
	DREKKAR_WA_OK = 0,
	DREKKAR_WA_NEED_MORE_GAS,
	DREKKAR_WA_STACK_OVERFLOW,
	DREKKAR_WA_BLOCKSTACK_OVERFLOW,
	DREKKAR_WA_BLOCKSTACK_UNDERFLOW,
	DREKKAR_WA_IMPORT_FIELD_NOT_FOUND,
	DREKKAR_WA_FEATURE_NOT_SUPPORTED_YET,
	DREKKAR_WA_UNKNOWN_GLOBAL_TYPE,
	DREKKAR_WA_UNKNOWN_KIND,
	DREKKAR_WA_ONLY_ONE_TABLE_IS_SUPPORTED,
	DREKKAR_WA_ONLY_ONE_MEMORY_IS_SUPPORTED,
	DREKKAR_WA_TABLE_OVERFLOW,
	DREKKAR_WA_UNKNOWN_SECTION,
	DREKKAR_WA_MEMORY_OUT_OF_RANGE,
	DREKKAR_WA_EXCEPTION,
	DREKKAR_WA_ELSE_WITHOUT_IF,
	DREKKAR_WA_MISSING_CODE_AT_END,
	DREKKAR_WA_FUNCTION_MISSING_RETURN,
	DREKKAR_WA_UNEXPECTED_RETURN,
	DREKKAR_WA_OP_CODE_ZERO,
	DREKKAR_WA_BLOCK_STACK_UNDER_FLOW,
	DREKKAR_WA_CALL_STACK_OVER_FLOW,
	DREKKAR_WA_TABLE_SIZE_EXCEEDED,
	DREKKAR_WA_CALL_FAILED,
	DREKKAR_WA_TO_MANY_TABLES,
	DREKKAR_WA_OUT_OF_RANGE_IN_TABLE,
	DREKKAR_WA_MISMATCH_CALL_TYPE,
	DREKKAR_WA_INDIRECT_CALL_FAILED,
	DREKKAR_WA_INDIRECT_CALL_INSUFFICIENT_NOF_PARAM,
	DREKKAR_WA_INDIRECT_CALL_MISMATCH_PARAM_TYPES,
	DREKKAR_WA_INTERNAL_ERROR,
	DREKKAR_WA_DIVIDE_BY_ZERO,
	DREKKAR_WA_INTEGER_OVERFLOW,
	DREKKAR_WA_INVALID_INTEGER_CONVERSION,
	DREKKAR_WA_UNKNOWN_OPCODE,
	DREKKAR_WA_UNREACHABLE_CODE_REACHED,
	DREKKAR_WA_NOT_WEBASM_OR_SUPPORTED_VERSION,
	DREKKAR_WA_FILE_NOT_FOUND,
	DREKKAR_WA_FUNCTION_NOT_FOUND,
	DREKKAR_WA_MISSING_OPCODE_END,
	DREKKAR_WA_ELEMENT_TYPE_NOT_SUPPORTED,
	DREKKAR_WA_EXTERNAL_CALL_FAILED,
	DREKKAR_WA_BLOCK_STACK_UNDER_RUN,
	DREKKAR_WA_LEB_DECODE_FAILED,
	DREKKAR_WA_TABLE_MAX_TO_BIG,
	DREKKAR_WA_INDIRECT_CALL_OF_UNKNOWN_TYPE,
	DREKKAR_WA_FUNCTION_INDEX_OUT_OF_RANGE,
	DREKKAR_WA_IMPORTED_FUNC_AS_START,
	DREKKAR_WA_TO_MUCH_MEMORY_REQUESTED,
	DREKKAR_WA_TO_MANY_RESULT_VALUES,
	DREKKAR_WA_TO_MANY_PARAMETERS,
	DREKKAR_WA_MISALLIGNED_SECTION,
	DREKKAR_WA_UNKNOWN_TYPE_OF_IMPORT,
	DREKKAR_WA_EXPORT_TYPE_NOT_IMPL_YET,
	DREKKAR_WA_EXTERNAL_STACK_MISMATCH,
	DREKKAR_WA_VALUE_TYPE_NOT_SUPPORED_YET,
	DREKKAR_WA_NO_RESULT_ON_STACK,
	DREKKAR_WA_WRONG_FUNCTION_TYPE,
	DREKKAR_WA_OUT_OF_RANGE_IN_CODE_SECTION,
	DREKKAR_WA_GLOBAL_IDX_OUT_OF_RANGE,
	DREKKAR_WA_VECTORS_NOT_SUPPORTED,
	DREKKAR_WA_EXPORT_NAME_TO_LONG,
	DREKKAR_WA_NO_END_OR_ELSE,
	DREKKAR_WA_NO_END,
	DREKKAR_WA_NO_TYPE_INFO,
	DREKKAR_WA_MISSING_RETURN_VALUES,
	DREKKAR_WA_TO_MANY_FUNCTION_TYPES,
	DREKKAR_WA_TO_MANY_IMPORTS,
	DREKKAR_WA_TO_MANY_FUNCTIONS,
	DREKKAR_WA_TO_MANY_TABLE_ELEMENTS,
	DREKKAR_WA_TO_MANY_EXPORTS,
	DREKKAR_WA_TO_MANY_ELEMENTS,
	DREKKAR_WA_TO_MANY_ENTRIES,
	DREKKAR_WA_TOO_MANY_LOCAL_VARIABLES,
	DREKKAR_WA_TO_MANY_DATA_SEGMENTS,
	DREKKAR_WA_TO_MANY_GLOBALS,
	DREKKAR_WA_TO_MUCH_ARGUMENTS,
	DREKKAR_WA_TO_BIG_BRANCH_TABLE,
	DREKKAR_WA_LABEL_OUT_OF_RANGE,
	DREKKAR_WA_PARAMETRIC_INSTRUCTIONS_NOT_SUPPORTED_YET,
	DREKKAR_WA_FLOAT_IS_NOT_SUPPORTED_IN_THIS_VERSION,
	DREKKAR_WA_NOT_SUPPORTED_TABLE_TYPE,
	DREKKAR_WA_ONLY_ONE_SECTION_ALLOWED,
	DREKKAR_WA_CAN_NOT_CALL_IMPORTED_HERE,
	DREKKAR_WA_FUNC_IDX_OUT_OF_RANGE,
	DREKKAR_WA_INSUFFICIENT_PARRAMETERS_FOR_CALL,
	DREKKAR_WA_NOT_AN_IDX_OF_IMPORTED_FUNCTION,
	DREKKAR_WA_EXCEPTION_FROM_IMPORTED_FUNCTION,
	DREKKAR_WA_MAX_MEM_QUOTA_EXCEEDED,
};

typedef struct drekkar_wa_data drekkar_wa_data;

// [1] 6.4.4. Value Types
// Value type is made up for numtype, vectype and reftype.
//     [1] 6.4.1. Number Types
//     I32, I64, F32, F64 are number types, these can be stored on stack.
//     [1] 6.4.2. Vector Types
//     Ignored this for now.
//     [1] 6.4.3. Reference Types
//     ANYFUNC, FUNC, BLOCK can not be put on stack but on call/block stack.
//
// [1] 5.3.1. Number Types
//     Number types are encoded by a single byte.
// These single bytes (0x7F, 0x7E, 0x7D, 0x7C, 0x40) are selected so that
// if encoded as signed LEB128 they are negative but still single bytes.
//
// [1] 5.3.4. Value Types
// Value types can occur in contexts where type indices are also allowed,
// such as in the case of block types. Thus, the binary format for types
// corresponds to the signed LEB128 encoding of small negative sN values,
// so that they can coexist with (positive) type indices in the future.
//
// [1] 5.3.6. Function Types
// Function types are encoded by the byte 0x60.
//
// If something is added here, add it to type_name also.
enum drekkar_value_type_enum
{
	// TODO prefix these also?
    //WA_INVALID_VALUE = 0,
	WA_EMPTY_TYPE = 0x40,
	WA_FUNC = 0x60,
	WA_EXTERNREF = 0x6f,
	WA_ANYFUNC = 0x70,
	WA_VECTYPE = 0x7b,
	WA_F64 = 0x7c,
	WA_F32 = 0x7d,
	WA_I64 = 0x7e,
	WA_I32 = 0x7f,
};

// [1] 5.5.5. Import Section & 5.5.10. Export Section
enum drekkar_kind_type_enum
{
	// TODO prefix these also?
	WA_FUNCTYPE = 0x00,
	WA_TABLETYPE = 0x01,
	WA_MEMTYPE = 0x02,
	WA_GLOBALTYPE = 0x03,
};

enum drekkar_block_type_enum
{
	// TODO prefix these also?
	wa_block_type_invalid = 0,
	wa_block_type_init_exp = 1,
	wa_block_type_block = 2,
	wa_block_type_loop = 3,
	wa_block_type_if = 4,
	wa_block_type_internal_func = 5,
	wa_block_type_imported_func = 6,
};

typedef struct drekkar_wa_value_type
{
	union {
		int32_t s32;
		uint32_t u32;
		int64_t s64;
		uint64_t u64;
		#ifndef SKIP_FLOAT
		float f32;
		double f64;
		#endif
	};
} drekkar_wa_value_type;

typedef struct drekkar_wa_func_type_type
{
	uint32_t nof_parameters;
	uint8_t parameters_list[32]; // Max 32 parameters.
	uint32_t nof_results; // Number of return values.
	uint8_t results_list[8]; // Max 8 return values.
} drekkar_wa_func_type_type;

// The type for function pointers to imported functions.
typedef void (*drekkar_wa_func_ptr)(drekkar_wa_data *d);


typedef struct drekkar_func_info_type
{
	uint32_t func_idx;
	uint32_t return_addr;
} drekkar_func_info_type;

// branch address for branching out of a block or loop back in a loop.
// If it is a loop then br_addr will point to the first instruction after the loop opcode.
typedef struct drekkar_block_info_type
{
	uint32_t br_addr;
} drekkar_block_info_type;

// Then else and end addresses in an if block.
typedef struct drekkar_if_else_info
{
	uint32_t end_addr;
	uint32_t else_addr;
} drekkar_if_else_info;

// This is the type of data that is pushed on the call/block stack.
typedef struct drekkar_block_stack_entry
{
	// The type of the block and function currently being executed.
	int32_t func_type_idx; // Index into types from section 1, or -64 for empty type (see value_type_enum).
	uint8_t block_type_code; // See block_type_enum for possible values.
	union {
		drekkar_func_info_type func_info; // For function idx >= nof_imported.
		drekkar_block_info_type block_and_loop_info; // For non functions, blocks loops etc.
		drekkar_if_else_info if_else_info; // For non functions, blocks loops etc.
	};

	// The saved stack and frame pointers (as used by previous function).
	drekkar_stack_pointer_type stack_pointer;
	drekkar_stack_pointer_type frame_pointer;
} drekkar_block_stack_entry;


typedef struct drekkar_internal_function_type
{
		uint32_t nof_local;
		uint32_t start_addr; // Location of the first opcode of a function or block.
		uint32_t end_addr; // Location of the "end" opcode.
} drekkar_internal_function_type;

typedef struct drekkar_imported_func_type
{
	drekkar_wa_func_ptr func_ptr;
} drekkar_imported_func_type;

typedef struct drekkar_wa_function
{
	int32_t func_type_idx; // Index into types from section 1, or -64 for empty type (see value_type_enum).
	uint32_t func_idx;
	uint8_t block_type_code;
	union {
			drekkar_internal_function_type internal_function;
			drekkar_imported_func_type external_function;
	};
} drekkar_wa_function;

// Imported and internal functions. Imported first then the internal ones.
typedef struct drekkar_wa_functions_vector_type
{
	uint32_t nof_imported;
	uint32_t total_nof;
	drekkar_wa_function *functions_array;
} drekkar_wa_functions_vector_type;


typedef struct drekkar_wa_prog
{
	drekkar_leb128_reader_type bytecodes;
	drekkar_linear_storage_size_type function_types_vector;
	drekkar_wa_functions_vector_type funcs_vector;
	drekkar_hash_list exported_functions_list;
	uint32_t start_function_idx;
	drekkar_hash_list available_functions_list;

	// Ref [2] WebAssembly.Table()
	//   A WebAssembly.Table object is a resizable typed array of opaque values,
	//   like function references, that are accessed by an Instance.
	// TODO Seems 32 bits is enough. ref [3] use 32 bits table.
	drekkar_linear_storage_64_type func_table;
} drekkar_wa_prog;


// [2] WebAssembly.Memory()
// A WebAssembly.Memory object is a resizable ArrayBuffer that holds the raw bytes of memory accessed by an Instance.
typedef struct drekkar_wa_memory
{
	uint32_t maximum_size_in_pages;
	uint32_t current_size_in_pages; // current size (64K pages)
	drekkar_linear_storage_8_type lower_mem;
	drekkar_virtual_storage_type upper_mem;
	drekkar_linear_storage_8_type  arguments; // Area where command line arguments are stored.
} drekkar_wa_memory;

// Stores all data for a WebAssembly.Instance.
struct drekkar_wa_data
{
	drekkar_leb128_reader_type pc;

	// Main stack and stack pointer.
	drekkar_stack_pointer_type sp;
	drekkar_stack_pointer_type fp;
	drekkar_wa_value_type stack[DREKKAR_STACK_SIZE];

	// The call and block stack.
	drekkar_linear_storage_size_type block_stack; // Storage for entries of type drekkar_block_stack_entry.

	// Globals and memory
	drekkar_linear_storage_64_type globals;
	drekkar_wa_memory memory;
	uint64_t temp_value;
	long gas_meter;

	char exception[256]; // If an error happens, additional info might be written here.
};

long drekkar_wa_value_and_type_to_string(char* buf, size_t size, const drekkar_wa_value_type *v, uint8_t t);
long drekkar_wa_setup_function_call(const drekkar_wa_prog *p, drekkar_wa_data *d, uint32_t fidx);
long drekkar_wa_tick(const drekkar_wa_prog *p, drekkar_wa_data *d);
const drekkar_wa_function *drekkar_wa_find_exported_function(const drekkar_wa_prog *p, const char *name);
long drekkar_wa_parse_prog_sections(drekkar_wa_prog *p, const uint8_t *bytes, uint32_t byte_count, char* exception, size_t exception_size, FILE* log);
long drekkar_wa_parse_data_sections(const drekkar_wa_prog *p, drekkar_wa_data *d);
void drekkar_wa_prog_init(drekkar_wa_prog *p);
void drekkar_wa_prog_deinit(drekkar_wa_prog *p);
void drekkar_wa_data_init(drekkar_wa_data *d);
void drekkar_wa_data_deinit(drekkar_wa_data *d, FILE* log);
long drekkar_wa_set_command_line_arguments(drekkar_wa_data *d, uint32_t argc, const char **argv);
void* drekkar_wa_translate_to_host_addr_space(drekkar_wa_data *d, uint32_t offset, size_t size);
void drekkar_wa_register_function(drekkar_wa_prog *p, const char* name, drekkar_wa_func_ptr ptr);

void drekkar_wa_push_value_i64(drekkar_wa_data *d, int64_t v);
int32_t drekkar_wa_pop_value_i64(drekkar_wa_data *d);
const drekkar_wa_func_type_type* drekkar_get_func_type_ptr(const drekkar_wa_prog *p, int32_t type_idx);
long drekkar_wa_call_exported_function(const drekkar_wa_prog *p, drekkar_wa_data *d, uint32_t func_idx);

#endif
