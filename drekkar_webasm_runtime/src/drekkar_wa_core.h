/* drekkar_wa_core.h
 *
 * Drekkar WebAsm Core (DWAC)
 *
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
#ifndef DREKKAR_WA_CORE_H
#define DREKKAR_WA_CORE_H

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
#define DREKKAR_VERSION_MINOR 9
#define DREKKAR_VERSION_PATCH 0


#define DREKKAR_VERSION_STRING DREKKAR_VERSION_NAME " " DREKKAR_VER_XSTR(DREKKAR_VERSION_MAJOR) "." DREKKAR_VER_XSTR(DREKKAR_VERSION_MINOR) "." DREKKAR_VER_XSTR(DREKKAR_VERSION_PATCH)



// End of file version.h


// Begin of file sys_time.h




// If using C++ there is a another way:
// http://www.cplusplus.com/faq/sequences/arrays/sizeof-array/#cpp
#ifndef SIZEOF_ARRAY
//#define SIZEOF_ARRAY( a ) (sizeof( a ) / sizeof( a[ 0 ] ))
#define SIZEOF_ARRAY(a) ((sizeof(a)/sizeof(0[a])) / ((size_t)(!(sizeof(a) % sizeof(0[a])))))
#endif


int64_t dwac_st_get_time_us();
void dwac_st_init();
void dwac_st_deinit();



// Here are some functions and macros to help debugging memory leaks.
// Performance will be affected.
// Define macro DWAC_ST_DEBUG to get more debugging info.
// Or define macro NDEBUG to get less.
// For medium level do not define any of the two.
// WARNING DWAC_ST_DEBUG is not thread safe. So do not use DWAC_ST_DEBUG if multi threading is used.
//#define DWAC_ST_DEBUG
//#define NDEBUG


#ifndef DWAC_ST_DEBUG

void* dwac_st_malloc(size_t size);
void* dwac_st_calloc(size_t num, size_t size);
void dwac_st_free(void* ptr);
int dwac_st_is_valid_size(const void* ptr, size_t size);
void* dwac_st_resize(void* ptr, size_t old_size, size_t new_size);
void* dwac_st_realloc(void* ptr, size_t new_size);
void* dwac_st_recalloc(void *ptr, size_t old_num, size_t new_num, size_t size);
int dwac_st_is_valid_min(const void* ptr, size_t size);
size_t dwac_st_size(const void* ptr);

#ifndef NDEBUG
#define DWAC_ST_MALLOC(size) dwac_st_malloc(size)
#define DWAC_ST_CALLOC(num, size) dwac_st_calloc(num, size)
#define DWAC_ST_FREE(ptr) {dwac_st_free(ptr); ptr = NULL;}
#define DWAC_ST_ASSERT_SIZE(ptr, size) assert(st_is_valid(ptr, size))
#define DWAC_ST_RESIZE(ptr, old_size, new_size) dwac_st_resize(ptr, old_size, new_size);
#define DWAC_ST_ASSERT_MIN(ptr, size) assert(dwac_st_is_valid_min(ptr, size))
#define DWAC_ST_FREE_SIZE(ptr, size) {assert(dwac_st_is_valid_size(ptr, size)); dwac_st_free(ptr); ptr = NULL;}
#define DWAC_ST_REALLOC(ptr, new_size) dwac_st_realloc(ptr, new_size);
#define ST_RECALLOC(ptr, old_num, new_num, size) dwac_st_recalloc(ptr, old_num, new_num, size);
#else
#define DWAC_ST_MALLOC(size) malloc(size);
#define DWAC_ST_CALLOC(num, size) calloc(num, size)
#define DWAC_ST_FREE(ptr) free(ptr)
#define DWAC_ST_ASSERT_SIZE(ptr, size)
#define DWAC_ST_ASSERT_MIN(ptr, size)
#define DWAC_ST_FREE_SIZE(ptr, size) free(ptr))
#endif

#else

void* dwac_st_malloc(size_t size, const char *file, unsigned int line);
void* dwac_st_calloc(size_t num, size_t size, const char *file, unsigned int line);
void dwac_st_free(const void* ptr, const char *file, unsigned int line);
int dwac_st_is_valid_size(const void* ptr, size_t size);
void* dwac_st_resize(void* ptr, size_t old_size, size_t new_size, const char *file, unsigned int line);
void* dwac_st_realloc(void* ptr, size_t new_size, const char *file, unsigned int line);
int dwac_st_is_valid_min(const void* ptr, size_t size);
size_t dwac_st_size(const void* ptr);
void dwac_st_log_linked_list();

#define DWAC_ST_MALLOC(size) dwac_st_malloc(size, __FILE__, __LINE__)
#define DWAC_ST_CALLOC(num, size) dwac_st_calloc(num, size, __FILE__, __LINE__)
#define DWAC_ST_FREE(ptr) {dwac_st_free(ptr, __FILE__, __LINE__); ptr = NULL;}
#define DWAC_ST_ASSERT_SIZE(ptr, size) assert(dwac_st_is_valid_size(ptr, size))
#define DWAC_ST_RESIZE(ptr, old_size, new_size) dwac_st_resize(ptr, old_size, new_size, __FILE__, __LINE__);
#define DWAC_ST_ASSERT_MIN(ptr, size) assert(dwac_st_is_valid_min(ptr, size))
#define DWAC_ST_FREE_SIZE(ptr, size) {assert(dwac_st_is_valid_size(ptr, size)); dwac_st_free(ptr, __FILE__, __LINE__); ptr = NULL;}
#define DWAC_ST_REALLOC(ptr, new_size) dwac_st_realloc(ptr, new_size, __FILE__, __LINE__);

#endif

// End of file sys_time.h




// Begin of file hash_list.h


// Size must be power of 2.
#define DWAC_HASH_LIST_INIT_SIZE 256 // TODO Set it to just 16 again.
#define DWAC_HASH_LIST_MAX_KEY_SIZE (64)


typedef struct dwac_hash_entry dwac_hash_entry;
typedef struct dwac_hash_list dwac_hash_list;

// TODO Perhaps we can trust that key is stored by someone else and just keep the pointer here?
struct dwac_hash_entry
{
	void* ptr;
	char key[DWAC_HASH_LIST_MAX_KEY_SIZE+1]; // +1 for a terminating zero.
};


struct dwac_hash_list
{
	long size;
	long capacity;
	dwac_hash_entry* array;
};

void dwac_hash_list_init(dwac_hash_list *list);
void dwac_hash_list_deinit(dwac_hash_list *list);
long dwac_hash_list_put(dwac_hash_list *list, const char* key_ptr, void* ptr);
void* dwac_hash_list_find(const dwac_hash_list *list, const char* key_ptr);


// End of file hash_list.h




// Begin of file linear_storage_64.h



typedef struct dwac_linear_storage_64_type dwac_linear_storage_64_type;


struct dwac_linear_storage_64_type
{
	size_t size;
	size_t capacity;
	uint64_t* array;
};

void dwac_linear_storage_64_init(dwac_linear_storage_64_type *list);
void dwac_linear_storage_64_deinit(dwac_linear_storage_64_type *list);

void dwac_linear_storage_64_grow_if_needed(dwac_linear_storage_64_type *s, size_t needed_size);
void dwac_linear_storage_64_set(dwac_linear_storage_64_type *s, size_t idx, const uint64_t u);
uint64_t dwac_linear_storage_64_get(dwac_linear_storage_64_type *s, size_t idx);
void dwac_linear_storage_64_push(dwac_linear_storage_64_type *s, uint64_t value);
uint64_t dwac_linear_storage_64_pop(dwac_linear_storage_64_type *s);

// End of file linear_storage_64.h



// Begin of file linear_storage_8.h



typedef struct dwac_linear_storage_8_type dwac_linear_storage_8_type;


struct dwac_linear_storage_8_type
{
	size_t size;
	size_t capacity;
	uint8_t* array;
};

void dwac_linear_storage_8_init(dwac_linear_storage_8_type *list);
void dwac_linear_storage_8_deinit(dwac_linear_storage_8_type *list);

void dwac_linear_storage_8_grow_if_needed(dwac_linear_storage_8_type *s, size_t needed_size_in_bytes);
void dwac_linear_storage_8_set_mem(dwac_linear_storage_8_type *s, size_t offset, const uint8_t *src, size_t nof_bytes);
void dwac_linear_storage_8_set_uint64_t(dwac_linear_storage_8_type *s, size_t offset, const uint64_t u);
void dwac_linear_storage_8_set_uint32_t(dwac_linear_storage_8_type *s, size_t offset, const uint32_t u);
void dwac_linear_storage_8_set_uint16_t(dwac_linear_storage_8_type *s, size_t offset, const uint16_t u);
void dwac_linear_storage_8_set_uint8_t(dwac_linear_storage_8_type *s, size_t offset, const uint8_t u);
void* dwac_linear_storage_8_get_ptr(dwac_linear_storage_8_type *s, size_t offset, size_t nof_bytes);
uint64_t dwac_linear_storage_8_get_uint64_t(dwac_linear_storage_8_type *s, size_t offset);
uint32_t dwac_linear_storage_8_get_uint32_t(dwac_linear_storage_8_type *s, size_t offset);
uint16_t dwac_linear_storage_8_get_uint16_t(dwac_linear_storage_8_type *s, size_t offset);
uint8_t dwac_linear_storage_8_get_uint8_t(dwac_linear_storage_8_type *s, size_t offset);
void dwac_linear_storage_8_push_uint64_t(dwac_linear_storage_8_type *s, const uint64_t u);
void dwac_linear_storage_8_push_uint32_t(dwac_linear_storage_8_type *s, const uint32_t u);
void dwac_linear_storage_8_push_uint16_t(dwac_linear_storage_8_type *s, const uint16_t u);
void dwac_linear_storage_8_push_uint8_t(dwac_linear_storage_8_type *s, const uint8_t u);

// End of file linear_storage_8.h





// Begin of file linear_storage_size.h


typedef struct dwac_linear_storage_size_type dwac_linear_storage_size_type;


struct dwac_linear_storage_size_type
{
	long size; // In number of elements.
	size_t capacity;  // -"-
	uint8_t* array;
	size_t element_size; // In bytes.
};

void dwac_linear_storage_size_init(dwac_linear_storage_size_type *list, size_t element_size);
void dwac_linear_storage_size_deinit(dwac_linear_storage_size_type *list);

void dwac_linear_storage_size_grow_if_needed(dwac_linear_storage_size_type *s, size_t needed_size);

void dwac_linear_storage_size_set(dwac_linear_storage_size_type *s, size_t idx, const uint8_t* ptr);
void* dwac_linear_storage_size_get(dwac_linear_storage_size_type *s, size_t idx);
void* dwac_linear_storage_size_push(dwac_linear_storage_size_type *s);
void* dwac_linear_storage_size_pop(dwac_linear_storage_size_type *s);
void* dwac_linear_storage_size_top(dwac_linear_storage_size_type *s);


// End of file linear_storage_size.h





// Begin of file virtual_storage.h


typedef struct dwac_virtual_storage_type dwac_virtual_storage_type;


struct dwac_virtual_storage_type
{
	size_t begin;
	size_t end;
	size_t inc;
	uint8_t* array;
};

void dwac_virtual_storage_init(dwac_virtual_storage_type *list);
void dwac_virtual_storage_deinit(dwac_virtual_storage_type *list);
void dwac_virtual_storage_grow_buffer_if_needed(dwac_virtual_storage_type *s, size_t begin, size_t nof_bytes, size_t min, size_t max);

void dwac_virtual_storage_set_mem(dwac_virtual_storage_type *s, size_t offset, const uint8_t *src, size_t nof_bytes);
void dwac_virtual_storage_set_uint64_t(dwac_virtual_storage_type *s, size_t offset, const uint64_t i);
void dwac_virtual_storage_set_uint32_t(dwac_virtual_storage_type *s, size_t offset, const uint32_t i);
void dwac_virtual_storage_set_uint16_t(dwac_virtual_storage_type *s, size_t offset, const uint16_t i);
void dwac_virtual_storage_set_uint8_t(dwac_virtual_storage_type *s, size_t offset, const uint8_t i);
void* dwac_virtual_storage_get_ptr(dwac_virtual_storage_type *s, size_t offset, size_t nof_bytes);
uint64_t dwac_virtual_storage_get_uint64_t(dwac_virtual_storage_type *s, size_t offset);
uint32_t dwac_virtual_storage_get_uint32_t(dwac_virtual_storage_type *s, size_t offset);
uint16_t dwac_virtual_storage_get_uint16_t(dwac_virtual_storage_type *s, size_t offset);
uint8_t dwac_virtual_storage_get_uint8_t(dwac_virtual_storage_type *s, size_t offset);

// End of file virtual_storage.h



// Begin of file wa_leb.h


typedef struct dwac_leb128_reader_type dwac_leb128_reader_type;

struct dwac_leb128_reader_type
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
#define DWAC_GAS 0x10000

// Shall code be translated during load of program or when a function is called?
// Define macro below if it shall be during load.
//#define TRANSLATE_ALL_AT_LOAD

#define DWAC_MAGIC 0x6d736100
#define DWAC_VERSION 0x01

// NOTE Stack size must be a power of 2 since we use a mask to prevent
// a stack overflow to write outside buffer. The stack can still overflow
// but that way a stack over or under flow can't write outside the stack.
#define DWAC_STACK_SIZE  0x10000

// Fun story: When testing WA_STACK_SIZE larger than than 0x40000 it didn't work.
// Turned out my test program placed the WaData on stack (host computers)
// And it got to big for its stack.

// An alternative for the stack and memory is to use memmap and allocate 4GiByte for each.

// Starting stack at -1 instead of 0 was an optimization found by examining ref [3].
// So we need an initial value for the stack pointer that is not zero.
#define DWAC_SP_OFFSET 1
#if DWAC_STACK_SIZE == 0x10000
#define dwac_stack_pointer_type uint16_t
#define DWAC_SP_INITIAL ((uint16_t)(-1))
#else
#define dwac_stack_pointer_type uint32_t
#define DWAC_SP_INITIAL ((uint32_t)(-1))
#endif

//#define SKIP_FLOAT

// Ref [1] 4.2.8. Memory Instances -> One page is 64Ki bytes.
// It seems ref [3] had page size as 0x10000*sizeof(uint32_t)
// that seems wrong, will keep it at 0x10000 in bytes.
#define DWAC_PAGE_SIZE 0x10000

#define DWAC_ARGUMENTS_BASE 0xFF000000
#define DWAC_MAX_NOF_PAGES (DWAC_ARGUMENTS_BASE / 0x10000)


enum {
	DWAC_OK = 0,
	DWAC_NEED_MORE_GAS,
	DWAC_STACK_OVERFLOW,
	DWAC_BLOCKSTACK_OVERFLOW,
	DWAC_BLOCKSTACK_UNDERFLOW,
	DWAC_IMPORT_FIELD_NOT_FOUND,
	DWAC_FEATURE_NOT_SUPPORTED_YET,
	DWAC_UNKNOWN_GLOBAL_TYPE,
	DWAC_UNKNOWN_KIND,
	DWAC_ONLY_ONE_TABLE_IS_SUPPORTED,
	DWAC_ONLY_ONE_MEMORY_IS_SUPPORTED,
	DWAC_TABLE_OVERFLOW,
	DWAC_UNKNOWN_SECTION,
	DWAC_MEMORY_OUT_OF_RANGE,
	DWAC_EXCEPTION,
	DWAC_ELSE_WITHOUT_IF,
	DWAC_MISSING_CODE_AT_END,
	DWAC_FUNCTION_MISSING_RETURN,
	DWAC_UNEXPECTED_RETURN,
	DWAC_OP_CODE_ZERO,
	DWAC_BLOCK_STACK_UNDER_FLOW,
	DWAC_CALL_STACK_OVER_FLOW,
	DWAC_TABLE_SIZE_EXCEEDED,
	DWAC_CALL_FAILED,
	DWAC_TO_MANY_TABLES,
	DWAC_OUT_OF_RANGE_IN_TABLE,
	DWAC_MISMATCH_CALL_TYPE,
	DWAC_INDIRECT_CALL_FAILED,
	DWAC_INDIRECT_CALL_INSUFFICIENT_NOF_PARAM,
	DWAC_INDIRECT_CALL_MISMATCH_PARAM_TYPES,
	DWAC_INTERNAL_ERROR,
	DWAC_DIVIDE_BY_ZERO,
	DWAC_INTEGER_OVERFLOW,
	DWAC_INVALID_INTEGER_CONVERSION,
	DWAC_UNKNOWN_OPCODE,
	DWAC_UNREACHABLE_CODE_REACHED,
	DWAC_NOT_WEBASM_OR_SUPPORTED_VERSION,
	DWAC_FILE_NOT_FOUND,
	DWAC_FUNCTION_NOT_FOUND,
	DWAC_MISSING_OPCODE_END,
	DWAC_ELEMENT_TYPE_NOT_SUPPORTED,
	DWAC_EXTERNAL_CALL_FAILED,
	DWAC_BLOCK_STACK_UNDER_RUN,
	DWAC_LEB_DECODE_FAILED,
	DWAC_TABLE_MAX_TO_BIG,
	DWAC_INDIRECT_CALL_OF_UNKNOWN_TYPE,
	DWAC_FUNCTION_INDEX_OUT_OF_RANGE,
	DWAC_IMPORTED_FUNC_AS_START,
	DWAC_TO_MUCH_MEMORY_REQUESTED,
	DWAC_TO_MANY_RESULT_VALUES,
	DWAC_TO_MANY_PARAMETERS,
	DWAC_MISALLIGNED_SECTION,
	DWAC_UNKNOWN_TYPE_OF_IMPORT,
	DWAC_EXPORT_TYPE_NOT_IMPL_YET,
	DWAC_EXTERNAL_STACK_MISMATCH,
	DWAC_VALUE_TYPE_NOT_SUPPORED_YET,
	DWAC_NO_RESULT_ON_STACK,
	DWAC_WRONG_FUNCTION_TYPE,
	DWAC_OUT_OF_RANGE_IN_CODE_SECTION,
	DWAC_GLOBAL_IDX_OUT_OF_RANGE,
	DWAC_VECTORS_NOT_SUPPORTED,
	DWAC_EXPORT_NAME_TO_LONG,
	DWAC_NO_END_OR_ELSE,
	DWAC_NO_END,
	DWAC_NO_TYPE_INFO,
	DWAC_MISSING_RETURN_VALUES,
	DWAC_TO_MANY_FUNCTION_TYPES,
	DWAC_TO_MANY_IMPORTS,
	DWAC_TO_MANY_FUNCTIONS,
	DWAC_TO_MANY_TABLE_ELEMENTS,
	DWAC_TO_MANY_EXPORTS,
	DWAC_TO_MANY_ELEMENTS,
	DWAC_TO_MANY_ENTRIES,
	DWAC_TOO_MANY_LOCAL_VARIABLES,
	DWAC_TO_MANY_DATA_SEGMENTS,
	DWAC_TO_MANY_GLOBALS,
	DWAC_TO_MUCH_ARGUMENTS,
	DWAC_TO_BIG_BRANCH_TABLE,
	DWAC_LABEL_OUT_OF_RANGE,
	DWAC_PARAMETRIC_INSTRUCTIONS_NOT_SUPPORTED_YET,
	DWAC_FLOAT_IS_NOT_SUPPORTED_IN_THIS_VERSION,
	DWAC_NOT_SUPPORTED_TABLE_TYPE,
	DWAC_ONLY_ONE_SECTION_ALLOWED,
	DWAC_CAN_NOT_CALL_IMPORTED_HERE,
	DWAC_FUNC_IDX_OUT_OF_RANGE,
	DWAC_INSUFFICIENT_PARRAMETERS_FOR_CALL,
	DWAC_NOT_AN_IDX_OF_IMPORTED_FUNCTION,
	DWAC_EXCEPTION_FROM_IMPORTED_FUNCTION,
	DWAC_MAX_MEM_QUOTA_EXCEEDED,
	DWAC_BRANCH_ADDR_OUT_OF_RANGE,
	DWAC_PC_ADDR_OUT_OF_RANGE,
	DWAC_ADDR_OUT_OF_RANGE,
};

typedef struct dwac_data dwac_data;

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
enum dwac_value_type_enum
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
enum dwac_kind_type_enum
{
	DWAC_FUNCTYPE = 0x00,
	DWAC_TABLETYPE = 0x01,
	DWAC_MEMTYPE = 0x02,
	DWAC_GLOBALTYPE = 0x03,
};

enum dwac_block_type_enum
{
	dwac_block_type_invalid = 0,
	dwac_block_type_init_exp = 1,
	dwac_block_type_block = 2,
	dwac_block_type_loop = 3,
	dwac_block_type_if = 4,
	dwac_block_type_internal_func = 5,
	dwac_block_type_imported_func = 6,
};

typedef struct dwac_value_type
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
} dwac_value_type;

typedef struct dwac_func_type_type
{
	uint32_t nof_parameters;
	uint8_t parameters_list[32]; // Max 32 parameters.
	uint32_t nof_results; // Number of return values.
	uint8_t results_list[8]; // Max 8 return values.
} dwac_func_type_type;

// The type for function pointers to imported functions.
typedef void (*dwac_func_ptr)(dwac_data *d);


// For function idx >= nof_imported &  dwac_block_type_init_exp.
typedef struct dwac_func_info_type
{
	uint32_t func_idx;
	uint32_t return_addr;
	dwac_stack_pointer_type frame_pointer; // The saved frame pointer (as used by previous function).
} dwac_func_info_type;

// branch address for branching out of a block or loop back in a loop.
// If it is a loop then br_addr will point to the first instruction after the loop opcode.
typedef struct dwac_block_info_type
{
	uint32_t br_addr;
} dwac_block_info_type;

// Then else and end addresses in an if block.
typedef struct dwac_if_else_info
{
	uint32_t end_addr;
	uint32_t else_addr;
} dwac_if_else_info;

// This is the type of data that is pushed on the call/block stack.
typedef struct dwac_block_stack_entry
{
	// The type of the block and function currently being executed.
	int32_t func_type_idx; // Index into types from section 1, or -64 for empty type (see value_type_enum).
	uint8_t block_type_code; // See block_type_enum for possible values.
	union {
		dwac_func_info_type func_info; // For function idx >= nof_imported &  dwac_block_type_init_exp.
		dwac_block_info_type block_and_loop_info; // For non functions, blocks loops etc.
		dwac_if_else_info if_else_info; // For non functions, blocks loops etc.
	};

	// The saved stack pointer (as used by previous function or block).
	dwac_stack_pointer_type stack_pointer;
} dwac_block_stack_entry;


typedef struct dwac_internal_function_type
{
		uint32_t nof_local;
		uint32_t start_addr; // Location of the first opcode of a function or block.
		uint32_t end_addr; // Location of the "end" opcode.
} dwac_internal_function_type;

typedef struct dwac_imported_func_type
{
	dwac_func_ptr func_ptr;
} dwac_imported_func_type;

typedef struct dwac_function
{
	int32_t func_type_idx; // Index into types from section 1, or -64 for empty type (see value_type_enum).
	uint32_t func_idx;
	uint8_t block_type_code;
	union {
			dwac_internal_function_type internal_function;
			dwac_imported_func_type external_function;
	};
} dwac_function;

// Imported and internal functions. Imported first then the internal ones.
typedef struct dwac_functions_vector_type
{
	uint32_t nof_imported;
	uint32_t total_nof;
	dwac_function *functions_array;
} dwac_functions_vector_type;


typedef struct dwac_prog
{
	dwac_leb128_reader_type bytecodes;
	dwac_linear_storage_size_type function_types_vector;
	dwac_functions_vector_type funcs_vector;
	dwac_hash_list exported_functions_list;
	uint32_t start_function_idx;
	dwac_hash_list available_functions_list;

	// Ref [2] WebAssembly.Table()
	//   A WebAssembly.Table object is a resizable typed array of opaque values,
	//   like function references, that are accessed by an Instance.
	// TODO Seems 32 bits is enough. ref [3] use 32 bits table.
	dwac_linear_storage_64_type func_table;
} dwac_prog;


// [2] WebAssembly.Memory()
// A WebAssembly.Memory object is a resizable ArrayBuffer that holds the raw bytes of memory accessed by an Instance.
typedef struct dwac_memory
{
	uint32_t maximum_size_in_pages;
	uint32_t current_size_in_pages; // current size (64K pages)
	dwac_linear_storage_8_type lower_mem;
	dwac_virtual_storage_type upper_mem;
	dwac_linear_storage_8_type  arguments; // Area where command line arguments are stored.
} dwac_memory;

// Stores all data for a WebAssembly.Instance.
struct dwac_data
{
	dwac_leb128_reader_type pc;

	// Main stack and stack pointer.
	dwac_stack_pointer_type sp;
	dwac_stack_pointer_type fp;
	dwac_value_type stack[DWAC_STACK_SIZE];

	// The call and block stack.
	dwac_linear_storage_size_type block_stack; // Storage for entries of type dwac_block_stack_entry.

	// Globals and memory
	dwac_linear_storage_64_type globals;
	dwac_memory memory;
	uint64_t temp_value;
	long gas_meter;

	// Typically errno is set if there was a fail from syscalls.
	// This can be removed if no syscalls will be used.
	uint32_t errno_location;

	char exception[256]; // If an error happens, additional info might be written here.
};

long dwac_value_and_type_to_string(char* buf, size_t size, const dwac_value_type *v, uint8_t t);
long dwac_setup_function_call(const dwac_prog *p, dwac_data *d, uint32_t fidx);
long dwac_tick(const dwac_prog *p, dwac_data *d);
const dwac_function *dwac_find_exported_function(const dwac_prog *p, const char *name);
long dwac_parse_prog_sections(dwac_prog *p, const uint8_t *bytes, uint32_t byte_count, char* exception, size_t exception_size, FILE* log);
long dwac_parse_data_sections(const dwac_prog *p, dwac_data *d);
void dwac_prog_init(dwac_prog *p);
void dwac_prog_deinit(dwac_prog *p);
void dwac_data_init(dwac_data *d);
void dwac_data_deinit(dwac_data *d, FILE* log);
long dwac_set_command_line_arguments(dwac_data *d, uint32_t argc, const char **argv);
void* dwac_translate_to_host_addr_space(dwac_data *d, uint32_t offset, size_t size);
void dwac_register_function(dwac_prog *p, const char* name, dwac_func_ptr ptr);

void dwac_push_value_i64(dwac_data *d, int64_t v);
int32_t dwac_pop_value_i64(dwac_data *d);
const dwac_func_type_type* dwac_get_func_type_ptr(const dwac_prog *p, int32_t type_idx);
long dwac_call_exported_function(const dwac_prog *p, dwac_data *d, uint32_t func_idx);

#endif
