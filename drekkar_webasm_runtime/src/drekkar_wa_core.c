/*
 * drekkar_wa_core.c
 *
 * Drekkar WebAsm runtime environment
 * http://www.drekkar.com/
 * https://github.com/xehp/drekkar_webasm.git
 *
 * Copyright (C) 2023
 * Henrik Bjorkman http://www.eit.se/hb
 *
 * GNU General Public License v3
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * IMPORTANT NOTICE! This version of this project is released under GPLv3.
 * If your project is not open source you can't use this version!!!
 * You will then need to buy a closed source license from Drekkar AB.
 *
 * CREDITS
 * This project owes a lot to the WAC project, ref [3]. It's a lot easier
 * to have a working code example to look at than to only have the
 * specifications. You may do any changes to this code but must make sure
 * to mention that in history. You must also to keep a reference to the
 * originals, not just this project but also to the WAC project.
 * Thanks also to W3C and Mozilla for their online documentation.
 *
 * To compile the test scripts some tools may be needed.
 * sudo apt-get install binaryen emscripten gcc-multilib g++-multilib libedit-dev:i386
 *
 * References:
 * [1] WebAssembly Core Specification Editor’s Draft, 7 November 2023
 *     https://webassembly.github.io/spec/core/bikeshed/
 *     https://webassembly.github.io/spec/core/_download/WebAssembly.pdf
 * [2] https://developer.mozilla.org/en-US/docs/WebAssembly/Reference
 * [3] https://github.com/kanaka/wac/tree/master
 *
 * History:
 * Created November 2023 by Henrik Bjorkman.
 */


#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include "drekkar_wa_core.h"


// Enable this macro if lots of debug logging is needed.
//#define DBG





#ifdef DBG
#define D(...) {fprintf(stdout, __VA_ARGS__);}
#else
#define D(...)
#endif





// Begin of file sys_time.h




static long alloc_counter = 0;
static long alloc_size = 0;
static int logged_alloc_counter = 0x10000;



void dwac_st_init()
{
	alloc_counter = 0;
	alloc_size = 0;
}

void dwac_st_deinit()
{
	if ((alloc_counter) || (alloc_size))
	{
		printf("Memory leak detected: alloc_counter %ld, %ld bytes.\n", alloc_counter, alloc_size);
		#ifdef ST_DEBUG
		printf("Memory first allocated here:\n");
		st_log_linked_list();
		printf("\n");
		#else
		assert(alloc_counter == 0);
		#endif
	}

	alloc_counter = 0;
	alloc_size = 0;
}

// Uncomment if all memory shall be filled with pattern when allocated.
#define ST_DEBUG_FILL_PATTERN 0x00

#define ST_MAGIC_NUMBER 0x67

#if defined(__x86_64)
#define ST_MAX_SIZE 0x100000000000
#else
#define ST_MAX_SIZE 0xF0000000
#endif


typedef struct header header;

struct header
{
	size_t size; // including header and footer
	#ifdef DWAC_ST_DEBUG
	header *prev;
	header *next;
	const char* file;
	size_t line;
	#endif
};

#define ST_HEADER_SIZE (sizeof(header))
#define ST_FOOTER_SIZE 1

#ifdef DWAC_ST_DEBUG

// TODO These are not thread safe. So do not use DWAC_ST_DEBUG if multithreading is used.
static header *head = NULL;
static header *tail = NULL;

static void add_linked_list(header *h, const char *file, unsigned int line)
{
	if (tail == NULL)
	{
		assert(head==NULL);
		head = h;
		tail = h;
		h->next = NULL;
		h->prev = NULL;
	}
	else
	{
		// New objects are added at tail.
		assert(head!=NULL);
		assert(tail->next==NULL);
		h->prev = tail;
		tail->next = h;
		tail = h;
		h->next = NULL;
	}
	h->file = file;
	h->line = line;
}

static void remove_from_linked_list(header *h, const char *file, unsigned int line)
{
	if (h->prev != NULL)
	{
		h->prev->next = h->next;
	}
	else
	{
		head = h->next;
	}
	if (h->next != NULL)
	{
		h->next->prev = h->prev;
	}
	else
	{
		tail = h->prev;
	}
}

void dwac_st_log_linked_list()
{
	header* ptr = head;
	while(ptr)
	{
		printf("%s:%zu (%zu bytes)\n", ptr->file, ptr->line, ptr->size-(ST_HEADER_SIZE+ST_FOOTER_SIZE));
		ptr = ptr->next;
	}
}
#endif

// To help debugging use these instead of malloc/free directly.
// This will add a size field to every allocated data area and
// when free is done it can check that data is valid.
// This will cause some overhead but it can easily be removed
// later when the program works perfectly.

// Allocate a block of memory, when no longer needed sys_free
// must be called.
#ifndef DWAC_ST_DEBUG
void* dwac_st_malloc(size_t size)
#else
void* dwac_st_malloc(size_t size, const char *file, unsigned int line)
#endif
{
	assert(size<=ST_MAX_SIZE);

	const size_t size_inc_header_footer = size + (ST_HEADER_SIZE+ST_FOOTER_SIZE);
	uint8_t* p = malloc(size_inc_header_footer);
	assert(p != 0);
	#ifdef ST_DEBUG_FILL_PATTERN
	memset(p, ST_DEBUG_FILL_PATTERN, size_inc_header_footer);
	#endif
	header* h = (header*)p;

	h->size = size_inc_header_footer;
	//*(size_t*)(ptr+size_inc_header_footer-ST_FOOTER_SIZE) = ST_MAGIC_NUMBER;
	p[size_inc_header_footer-1] = ST_MAGIC_NUMBER;
	assert(p[size_inc_header_footer-1] == ST_MAGIC_NUMBER);
	++alloc_counter;
	alloc_size += size;
	#ifdef DWAC_ST_DEBUG
	add_linked_list(h, file, line);
	#endif
	// Some logging (this can be removed later).
	if (alloc_counter >= (2*logged_alloc_counter))
	{
		D("sys_alloc %lu %ld\n", alloc_size, alloc_counter);
		logged_alloc_counter = alloc_counter;
	}

	return p + ST_HEADER_SIZE;
}

#ifndef DWAC_ST_DEBUG
void* dwac_st_calloc(size_t num, size_t size)
#else
void* dwac_st_calloc(size_t num, size_t size, const char *file, unsigned int line)
#endif
{
	assert(size<ST_MAX_SIZE);
	const size_t size_inc_header_footer = (num * size) + (ST_HEADER_SIZE+ST_FOOTER_SIZE);
	uint8_t* p = malloc(size_inc_header_footer);
	assert(p != 0);
	memset(p, 0, size_inc_header_footer);
	header* h = (header*)p;
	h->size = size_inc_header_footer;
	//*(size_t*)(ptr+size_inc_header_footer-ST_FOOTER_SIZE) = ST_MAGIC_NUMBER;
	p[size_inc_header_footer-1] = ST_MAGIC_NUMBER;
	assert(p[size_inc_header_footer-1] == ST_MAGIC_NUMBER);
	++alloc_counter;
	alloc_size += (num * size);
	#ifdef DWAC_ST_DEBUG
	add_linked_list(h, file, line);
	#endif
	// Some logging (this can be removed later).
	if (alloc_counter >= (2*logged_alloc_counter))
	{
		D("sys_alloc %lu %ld\n", alloc_size, alloc_counter);
		logged_alloc_counter = alloc_counter;
	}

	return p + ST_HEADER_SIZE;
}

// This must be called for all memory blocks allocated using
// sys_alloc when the memory block is no longer needed.
// TODO Shall we allow free on a NULL pointer? Probably not but for now we do.
#ifndef DWAC_ST_DEBUG
void dwac_st_free(void* ptr)
#else
void dwac_st_free(const void* ptr, const char *file, unsigned int line)
#endif
{
	//printf("sys_free %ld %d\n", (long)size, alloc_counter);
	if (ptr != NULL)
	{
		uint8_t *p = (uint8_t*)ptr - ST_HEADER_SIZE;
		header* h = (header*)p;
		const size_t size_inc_header_footer = h->size;
		assert((size_inc_header_footer>ST_HEADER_SIZE) && (size_inc_header_footer < (ST_MAX_SIZE + (ST_HEADER_SIZE + ST_FOOTER_SIZE))));
		assert(p[size_inc_header_footer-1] == ST_MAGIC_NUMBER);
		h->size = 0;
		#ifdef DWAC_ST_DEBUG
		remove_from_linked_list(h, file, line);
		#endif
		#ifdef ST_DEBUG_FILL_PATTERN
		memset(p, 0, size_inc_header_footer);
		#else
		p[size_inc_header_footer-1] = 0;
		#endif
		free(p);
		--alloc_counter;
		alloc_size -= (size_inc_header_footer - (ST_HEADER_SIZE+ST_FOOTER_SIZE));
		assert(alloc_counter>=0);
	}
	else
	{
		//printf("sys_free NULL %zu\n", size);
	}
}

// This can be used to check that a memory block allocated by sys_alloc
// is still valid (at least points to an object of expected size).
int dwac_st_is_valid_size(const void* ptr, size_t size)
{
	const uint8_t *p = (uint8_t*)ptr - ST_HEADER_SIZE;
	const header* h = (header*)p;
	const size_t size_inc_header_footer = h->size;
	return ((ptr != NULL) && (size_inc_header_footer == size + (ST_HEADER_SIZE+ST_FOOTER_SIZE)) && (p[size_inc_header_footer-1] == ST_MAGIC_NUMBER));
}

int dwac_st_is_valid_min(const void* ptr, size_t size)
{
	const uint8_t *p = (uint8_t*)ptr - ST_HEADER_SIZE;
	const header* h = (header*)p;
	const size_t size_inc_header_footer = h->size;
	return (ptr != NULL) && (size_inc_header_footer >= size + (ST_HEADER_SIZE+ST_FOOTER_SIZE)) && (p[size_inc_header_footer-1] == ST_MAGIC_NUMBER);
}

#ifndef DWAC_ST_DEBUG
void* dwac_st_resize(void* ptr, size_t old_size, size_t new_size)
#else
void* dwac_st_resize(void* ptr, size_t old_size, size_t new_size, const char *file, unsigned int line)
#endif
{
	assert((dwac_st_is_valid_size(ptr, old_size)) && (new_size>old_size));
	uint8_t* old_ptr = ptr - ST_HEADER_SIZE;
	const size_t new_size_inc_header_footer = new_size + (ST_HEADER_SIZE+ST_FOOTER_SIZE);
	#ifdef DWAC_ST_DEBUG
	remove_from_linked_list((header*)old_ptr, file, line);
	#endif
	#if 0
	uint8_t *new_ptr = realloc(old_ptr, new_size_inc_header_footer);
	#else
	uint8_t *new_ptr = malloc(new_size_inc_header_footer);
	memcpy(new_ptr + ST_HEADER_SIZE, old_ptr + ST_HEADER_SIZE, old_size);
	free(old_ptr);
	old_ptr = NULL;
	#endif
	assert(new_ptr != 0);
	#ifdef ST_DEBUG_FILL_PATTERN
	if (new_size > old_size)
	{
		memset(new_ptr + ST_HEADER_SIZE + old_size, ST_DEBUG_FILL_PATTERN, new_size - old_size);
	}
	#endif
	#ifdef DWAC_ST_DEBUG
	add_linked_list((header*)new_ptr, file, line);
	#endif

	*(size_t*)new_ptr = new_size_inc_header_footer;
	new_ptr[new_size_inc_header_footer-1] = ST_MAGIC_NUMBER;
	assert(new_ptr[new_size_inc_header_footer-1] == ST_MAGIC_NUMBER);
	alloc_size += (new_size - old_size);
	return new_ptr + ST_HEADER_SIZE;
}

// Shall be same as standard realloc but with our extra debugging checks.
#ifndef DWAC_ST_DEBUG
void* dwac_st_realloc(void* ptr, size_t new_size)
#else
void* dwac_st_realloc(void* ptr, size_t new_size, const char *file, unsigned int line)
#endif
{
	if (ptr)
	{
		assert(*(size_t*)(ptr - ST_HEADER_SIZE) >= (ST_HEADER_SIZE+ST_FOOTER_SIZE));
		const uint8_t *old_ptr = (uint8_t*)ptr - ST_HEADER_SIZE;
		const size_t old_size_inc_header_footer = *(size_t*)old_ptr;
		const size_t old_size = old_size_inc_header_footer - (ST_HEADER_SIZE+ST_FOOTER_SIZE);
		#ifndef DWAC_ST_DEBUG
		return dwac_st_resize(ptr, old_size, new_size);
		#else
		return dwac_st_resize(ptr, old_size, new_size, file, line);
		#endif
	}
	else
	{
		#ifdef DWAC_ST_DEBUG
		return dwac_st_malloc(new_size, __FILE__, __LINE__);
		#else
		return dwac_st_malloc(new_size);
		#endif
	}
}


size_t dwac_st_size(const void *ptr)
{
	if (ptr)
	{
		assert(*(size_t*)(ptr - ST_HEADER_SIZE) >= (ST_HEADER_SIZE+ST_FOOTER_SIZE));
		uint8_t *p = (uint8_t*)ptr - ST_HEADER_SIZE;
		size_t old_size_inc_header_footer = *(size_t*)p;
		return old_size_inc_header_footer - (ST_HEADER_SIZE+ST_FOOTER_SIZE);
	}
	else
	{
		return 0;
	}
}

// End of file sys_time.h





// Begin of file hash_list.c



static long calculate_hash(const char* ptr)
{
	long h = 0;
	while (*ptr)
	{
		h = (*ptr++)*7793 + (7727 * h) + 6679;
	}
	return h;
}


void dwac_hash_list_init(dwac_hash_list *list)
{
	memset((void*)list, 0, sizeof(*list));
	list->size = 0;

	// No list yet, create a first small list.
	assert(list->array == NULL);
	list->capacity = DWAC_HASH_LIST_INIT_SIZE;
	list->array = DWAC_ST_MALLOC(list->capacity*sizeof(dwac_hash_entry));
	memset(list->array, 0, list->capacity*sizeof(dwac_hash_entry));
	//D("hash_list initial capacity %ld\n", list->capacity);

}

void dwac_hash_list_deinit(dwac_hash_list *list)
{
	// Free all entries
	for(long i = 0; i < list->capacity; ++i)
	{
		dwac_hash_entry* o = (list->array)+i;
		if (o->ptr != NULL)
		{
			o->ptr = NULL;
		}
	}
	DWAC_ST_FREE_SIZE(list->array, list->capacity*sizeof(dwac_hash_entry));
	list->size = 0;
	list->capacity = 0;
	list->array = NULL;
}


static dwac_hash_entry* hash_list_find_empty(dwac_hash_entry* a, long capacity,  const char* key_ptr)
{
	long hash = calculate_hash(key_ptr);

	long idx = hash & (capacity-1);

	for(;;)
	{
		if ((a+idx)->ptr == NULL)
		{
			return a+idx;
		}
		idx = (idx+1) & (capacity-1);
	}
	return NULL;
}

static dwac_hash_entry* hash_list_find_entry(dwac_hash_entry* a, long capacity, const char* key_ptr)
{
	long hash = calculate_hash(key_ptr);

	long idx = hash & (capacity-1);

	for(;;)
	{
		dwac_hash_entry* e = (a+idx);
		if (e->ptr == NULL)
		{
			// Found empty slot.
			return e;
		}
		else if (strncmp(key_ptr, e->key, DWAC_HASH_LIST_MAX_KEY_SIZE) == 0)
		{
			// Found identical key.
			return e;
		}
		idx = (idx+1) & (capacity-1);
	}
	return NULL;
}

// Return:
//    0 : OK
//   -1 : key already exist
//   -2 : key was too long.
long dwac_hash_list_put(dwac_hash_list *list, const char* key_ptr, void* ptr)
{
	// Expand hash list if its half full.
	if ((list->size * 2) >= list->capacity)
	{
		// List is almost full.
		//  Make a bigger list.
		assert(list->capacity != 0);
		const long old_capacity = list->capacity;
		dwac_hash_entry* old_storage = list->array;

		const long new_capacity = old_capacity * 2;
		dwac_hash_entry* new_storage = DWAC_ST_MALLOC(new_capacity*sizeof(dwac_hash_entry));
		memset(new_storage, 0, new_capacity*sizeof(dwac_hash_entry));
		D("hash_list expanded to %ld\n", new_capacity);

		// Need to reenter all data over to new list.
		for(long i = 0; i < old_capacity; ++i)
		{
			dwac_hash_entry* o = (old_storage)+i;
			if (o->ptr != NULL)
			{
				// Find a position in new list where this entry can be placed.
				dwac_hash_entry* e = hash_list_find_empty(new_storage, new_capacity, o->key);
				assert(e);
				assert(e->ptr==NULL);
				e->ptr = o->ptr;
				strncpy(e->key, o->key, DWAC_HASH_LIST_MAX_KEY_SIZE);
				e->key[sizeof(e->key)-1] = 0;
			}
		}

		DWAC_ST_FREE_SIZE(list->array, list->capacity*sizeof(dwac_hash_entry));

		list->array = new_storage;
		list->capacity = new_capacity;
	}

	dwac_hash_entry* e = hash_list_find_entry(list->array, list->capacity, key_ptr);
	assert(e);
	if (e->ptr == NULL)
	{
		// It was an empty slot.
		strncpy(e->key, key_ptr, DWAC_HASH_LIST_MAX_KEY_SIZE);
		e->key[sizeof(e->key)-1] = 0;
		list->size++;
		e->ptr = ptr;
	}
	else
	{
		// Already have that key.
		if (e->ptr != ptr) {return -1;}
	}

	// If Key was longer than max allowed then we did enter but it got truncated so its a fail.
	// TODO For long keys add a hash at the end.
	return (strlen(key_ptr) <= DWAC_HASH_LIST_MAX_KEY_SIZE) ? 0 : -2;
}

void* dwac_hash_list_find(const dwac_hash_list *list, const char* key_ptr)
{
	dwac_hash_entry* e = hash_list_find_entry(list->array, list->capacity, key_ptr);
	return e->ptr;
}

// End of file hash_list.c







// Begin of file linear_storage_64.h



void dwac_linear_storage_64_init(dwac_linear_storage_64_type* s)
{
	s->size = 0;
	s->capacity = 0;
	s->array = NULL;
}

void dwac_linear_storage_64_deinit(dwac_linear_storage_64_type* s)
{
	if (s->array != NULL)
	{
		DWAC_ST_FREE_SIZE(s->array, s->capacity * sizeof(uint64_t));
		s->array = NULL;
	}
	memset(s, 0, sizeof(*s));
}

static void linear_storage_64_grow_buffer_if_needed(dwac_linear_storage_64_type *s, size_t needed_capacity)
{
	if (needed_capacity > s->capacity)
	{
		// There is not enough space.
		if (s->capacity == 0)
		{
			// No list yet, create the first list.
			s->capacity = needed_capacity;
			s->array = DWAC_ST_MALLOC(s->capacity * sizeof(uint64_t));
			memset(s->array, 0, s->capacity * sizeof(uint64_t));
		}
		else
		{
			// Need to make a bigger list. Make it at least twice bigger
			// so that we dont need to resize too often.
			long new_capacity = s->capacity * 2;
			if (new_capacity < needed_capacity) {new_capacity = needed_capacity;}
			s->array = DWAC_ST_RESIZE(s->array, s->capacity  * sizeof(uint64_t), new_capacity * sizeof(uint64_t));
			for(size_t i = s->capacity; i <new_capacity; ++i) {s->array[i]=0;}
			s->capacity = new_capacity;
		}
	}
}

void dwac_linear_storage_64_grow_if_needed(dwac_linear_storage_64_type *s, size_t needed_size)
{
	if (needed_size > s->size)
	{
		linear_storage_64_grow_buffer_if_needed(s, needed_size);
	}
}

void dwac_linear_storage_64_set(dwac_linear_storage_64_type *s, size_t idx, const uint64_t i)
{
	if (idx >= s->size)
	{
		linear_storage_64_grow_buffer_if_needed(s, idx+1);
		s->size = idx+1;
	}
	s->array[idx] = i;
}

uint64_t dwac_linear_storage_64_get(dwac_linear_storage_64_type *s, size_t idx)
{
	if (idx >= s->size)
	{
		linear_storage_64_grow_buffer_if_needed(s, idx+1);
		s->size = idx+1;
	}
	return s->array[idx];
}

void dwac_linear_storage_64_push(dwac_linear_storage_64_type *s, uint64_t value)
{
	assert(s->size >= 0);
	if (s->size >= s->capacity)
	{
		linear_storage_64_grow_buffer_if_needed(s, s->size + 1);
	}
	s->array[s->size++] = value;
}

// Not tested!
uint64_t dwac_linear_storage_64_pop(dwac_linear_storage_64_type *s)
{
	if (s->size > 0)
	{
		return s->array[--s->size];
	}
	return 0;
}

// End of file linear_storage_64.h



// Begin of file linear_storage_8.h



void dwac_linear_storage_8_init(dwac_linear_storage_8_type* s)
{
	s->size = 0;
	s->capacity = 0;
	s->array = NULL;
}

void dwac_linear_storage_8_deinit(dwac_linear_storage_8_type* s)
{
	if (s->array != NULL)
	{
		DWAC_ST_FREE_SIZE(s->array, s->capacity);
		s->array = NULL;
	}
	memset(s, 0, sizeof(*s));
}

static void linear_storage_8_grow_buffer_if_needed(dwac_linear_storage_8_type *s, size_t offset, size_t nof_bytes)
{
	size_t need = offset + nof_bytes;

	if (need > s->size)
	{
		if (need > s->capacity)
		{
			// There is not enough space.
			if (s->capacity == 0)
			{
				// No list yet, create a first small list.
				s->capacity = 256;
				while(s->capacity < need) {s->capacity *=2;}
				s->array = DWAC_ST_MALLOC(s->capacity);
				memset(s->array, 0, s->capacity);
			}
			else
			{
				// Make a bigger list.
				long new_capacity = s->capacity * 2;
				if (new_capacity <= need) {new_capacity = need;}
				s->array = DWAC_ST_RESIZE(s->array, s->capacity, new_capacity);
				memset(s->array + s->capacity, 0, new_capacity - s->capacity);
				s->capacity = new_capacity;
			}
		}
		s->size = need;
	}
}

void dwac_linear_storage_8_grow_if_needed(dwac_linear_storage_8_type *s, size_t needed_size_in_bytes)
{
	linear_storage_8_grow_buffer_if_needed(s, 0, needed_size_in_bytes);
}

// Set memory at offset.
void dwac_linear_storage_8_set_mem(dwac_linear_storage_8_type *s, size_t offset, const uint8_t *src, size_t nof_bytes)
{
	linear_storage_8_grow_buffer_if_needed(s, offset, nof_bytes);
	memcpy(s->array + offset, src, nof_bytes);
}

void dwac_linear_storage_8_set_uint64_t(dwac_linear_storage_8_type *s, size_t offset, const uint64_t i)
{
	linear_storage_8_grow_buffer_if_needed(s, offset, 8);
	*((uint64_t*)(s->array+offset)) = i;
}

void dwac_linear_storage_8_set_uint32_t(dwac_linear_storage_8_type *s, size_t offset, const uint32_t i)
{
	linear_storage_8_grow_buffer_if_needed(s, offset, 4);
	*((uint32_t*)(s->array+offset)) = i;
}

void dwac_linear_storage_8_set_uint16_t(dwac_linear_storage_8_type *s, size_t offset, const uint16_t i)
{
	linear_storage_8_grow_buffer_if_needed(s, offset, 2);
	*((uint16_t*)(s->array+offset)) = i;
}

void dwac_linear_storage_8_set_uint8_t(dwac_linear_storage_8_type *s, size_t offset, const uint8_t i)
{
	linear_storage_8_grow_buffer_if_needed(s, offset, 1);
	*((uint8_t*)(s->array+offset)) = i;
}

void* dwac_linear_storage_8_get_ptr(dwac_linear_storage_8_type *s, size_t offset, size_t nof_bytes)
{
	linear_storage_8_grow_buffer_if_needed(s, offset, nof_bytes);
	return s->array + offset;
}

uint64_t dwac_linear_storage_8_get_uint64_t(dwac_linear_storage_8_type *s, size_t offset)
{
	linear_storage_8_grow_buffer_if_needed(s, offset, 8);
	return *(uint64_t*)(s->array+offset);
}

uint32_t dwac_linear_storage_8_get_uint32_t(dwac_linear_storage_8_type *s, size_t offset)
{
	linear_storage_8_grow_buffer_if_needed(s, offset, 4);
	return *(uint32_t*)(s->array+offset);
}

uint16_t dwac_linear_storage_8_get_uint16_t(dwac_linear_storage_8_type *s, size_t offset)
{
	linear_storage_8_grow_buffer_if_needed(s, offset, 2);
	return *(uint16_t*)(s->array+offset);
}

uint8_t dwac_linear_storage_8_get_uint8_t(dwac_linear_storage_8_type *s, size_t offset)
{
	linear_storage_8_grow_buffer_if_needed(s, offset, 1);
	return *(uint8_t*)(s->array+offset);
}


void dwac_linear_storage_8_push_uint8_t(dwac_linear_storage_8_type *s, const uint8_t u)
{
	size_t offset = s->size;
	linear_storage_8_grow_buffer_if_needed(s, offset, sizeof(uint8_t));
	uint8_t* p = (uint8_t*)s->array;
	p[offset] = u;
/*
	size_t pos = s->size;
	grow_buffer_if_needed(s, pos, sizeof(uint8_t));
	uint8_t* p = (uint8_t*)(s->array + pos);
	*p = u;*/
}

// End of file linear_storage_8.h





// Begin of file linear_storage_size.h


void dwac_linear_storage_size_init(dwac_linear_storage_size_type* s, size_t element_size)
{
	s->size = 0;
	s->capacity = 0;
	s->array = NULL;
	s->element_size = element_size;
}

void dwac_linear_storage_size_deinit(dwac_linear_storage_size_type* s)
{
	if (s->array != NULL)
	{
		DWAC_ST_FREE_SIZE(s->array, s->capacity * s->element_size);
		s->array = NULL;
	}
	memset(s, 0, sizeof(*s));
}

static void linear_storage_size_grow_buffer_if_needed(dwac_linear_storage_size_type *s, size_t needed_size)
{
	if (needed_size > s->capacity)
	{
		// There is not enough space.
		if (s->capacity == 0)
		{
			// No list yet, create the first list.
			s->capacity = needed_size;
			s->array = DWAC_ST_MALLOC(s->capacity * s->element_size);
			memset(s->array, 0, s->capacity * s->element_size);
		}
		else
		{
			// Need to make a bigger list. Make it at least twice bigger
			// so that we dont need to resize too often.
			long new_capacity = s->capacity * 2;
			while(new_capacity < needed_size) {new_capacity *=2;}
			s->array = DWAC_ST_RESIZE(s->array, s->capacity  * s->element_size, new_capacity * s->element_size);
			memset(s->array + (s->capacity * s->element_size), 0, (new_capacity - s->capacity) * s->element_size);
			s->capacity = new_capacity;
		}
	}
}

void dwac_linear_storage_size_grow_if_needed(dwac_linear_storage_size_type *s, size_t needed_size)
{
	linear_storage_size_grow_buffer_if_needed(s, needed_size);
}

void dwac_linear_storage_size_set(dwac_linear_storage_size_type *s, size_t idx, const uint8_t* ptr)
{
	assert(s->size >= 0);
	if (idx >= s->size)
	{
		linear_storage_size_grow_buffer_if_needed(s, idx + 1);
		s->size = idx + 1;
	}
	memcpy(s->array + (idx * s->element_size), ptr, s->element_size);
}

void* dwac_linear_storage_size_get(dwac_linear_storage_size_type *s, size_t idx)
{
	assert(s->size >= 0);
	if (idx >= s->size)
	{
		linear_storage_size_grow_buffer_if_needed(s, idx + 1);
		s->size = idx + 1;
	}
	return s->array + (idx * s->element_size);
}


// Returns a pointer to where the data to be pushed shall be written.
void* dwac_linear_storage_size_push(dwac_linear_storage_size_type *s)
{
	assert(s->size >= 0);
	if (s->size >= s->capacity)
	{
		linear_storage_size_grow_buffer_if_needed(s, s->size + 1);
	}
	return s->array + ((s->size++) * s->element_size);
}

// Not tested.
void* dwac_linear_storage_size_pop(dwac_linear_storage_size_type *s)
{
	if (s->size > 0)
	{
		--s->size;
		return s->array + (s->size * s->element_size);
	}
	return NULL;
}

// Gives a pointer to the top most element on the stack.
void* dwac_linear_storage_size_top(dwac_linear_storage_size_type *s)
{
	assert(s->size > 0);
	if (s->size >= s->capacity)
	{
		linear_storage_size_grow_buffer_if_needed(s, s->size + 1);
	}
	return s->array + ((s->size-1) * s->element_size);
}

void* dwac_linear_storage_size_get_const(const dwac_linear_storage_size_type *s, size_t idx)
{
	assert(s->size >= 0);
	if (idx >= s->size)
	{
		return NULL;
	}
	return s->array + (idx * s->element_size);
}


// End of file linear_storage_size.h





// Begin of file virtual_storage.h


#define DIV_U_ROUND_UP(a,b) (((a)+((b)-1))/(b))


void dwac_virtual_storage_init(dwac_virtual_storage_type* s)
{
	s->begin = 0;
	s->end = 0;
	s->inc = 0;
	s->array = NULL;
}

void dwac_virtual_storage_deinit(dwac_virtual_storage_type* s)
{
	if (s->array != NULL)
	{
		DWAC_ST_FREE_SIZE(s->array, (s->end - s->begin));
		s->array = NULL;
	}
	memset(s, 0, sizeof(*s));
}

void dwac_virtual_storage_grow_buffer_if_needed(dwac_virtual_storage_type *s, size_t begin, size_t nof_bytes, size_t min, size_t max)
{
	size_t end = begin + nof_bytes;

	if (begin < min) {begin = min;}
	if (end > max) {end = max;}
	assert(begin < end);

	if ((begin < s->begin) || (end >= s->end))
	{
		// There is not enough space.

		// When we allocate we want it at some even boundary,
		// just so we don't resize for every lousy little byte.
		if (s->inc == 0) {s->inc = 0x1000;}

		begin = (begin / s->inc) * s->inc;
		end = DIV_U_ROUND_UP(end, s->inc) * s->inc;

		if ((begin < min) || (end > max))
		{
			if (begin < min)
			{
				begin = min;
			}
			if (end > max)
			{
				end = max;
			}
		}
		else
		{
			// Make a bigger increment next time.
			if ((s->inc) < 0x20000000)
			{
				s->inc *= 2;
			}
		}

		if (s->array == NULL)
		{
			// No list yet, create a first small list.
			size_t new_capacity = end - begin;
			s->array = DWAC_ST_MALLOC(new_capacity);
			memset(s->array, 0, new_capacity);
		}
		else
		{
			// Never make it smaller.
			if (begin > s->begin)
			{
				begin = s->begin;
			}
			if (end < s->end)
			{
				end = s->end;
			}

			assert(begin<=s->begin);
			assert(end>=s->end);

			// Make a bigger list.
			size_t new_capacity = end - begin;
			size_t move = s->begin - begin;
			uint8_t* new_array = DWAC_ST_MALLOC(new_capacity);
			memset(new_array, 0, new_capacity);
			memcpy(new_array + move, s->array, (s->end - s->begin));
			DWAC_ST_FREE_SIZE(s->array, (s->end - s->begin));
			s->array = new_array;
		}
		s->begin = begin;
		s->end = end;
	}
}

// Set memory at offset.
void virtual_storage_set_mem(dwac_virtual_storage_type *s, size_t offset, const uint8_t *src, size_t nof_bytes)
{
	dwac_virtual_storage_grow_buffer_if_needed(s, offset, nof_bytes, 0, -1);
	memcpy(s->array + offset, src, nof_bytes);
}

void dwac_virtual_storage_set_uint64_t(dwac_virtual_storage_type *s, size_t offset, const uint64_t i)
{
	dwac_virtual_storage_grow_buffer_if_needed(s, offset, 8, 0, -1);
	*((uint64_t*)(s->array+offset-s->begin)) = i;
}

void dwac_virtual_storage_set_uint32_t(dwac_virtual_storage_type *s, size_t offset, const uint32_t i)
{
	dwac_virtual_storage_grow_buffer_if_needed(s, offset, 4, 0, -1);
	*((uint32_t*)(s->array+offset-s->begin)) = i;
}

void dwac_virtual_storage_set_uint16_t(dwac_virtual_storage_type *s, size_t offset, const uint16_t i)
{
	dwac_virtual_storage_grow_buffer_if_needed(s, offset, 2, 0, -1);
	*((uint16_t*)(s->array+offset-s->begin)) = i;
}

void dwac_virtual_storage_set_uint8_t(dwac_virtual_storage_type *s, size_t offset, const uint8_t i)
{
	dwac_virtual_storage_grow_buffer_if_needed(s, offset, 1, 0, -1);
	*((uint8_t*)(s->array+offset-s->begin)) = i;
}

void* dwac_virtual_storage_get_ptr(dwac_virtual_storage_type *s, size_t offset, size_t nof_bytes)
{
	dwac_virtual_storage_grow_buffer_if_needed(s, offset, nof_bytes, 0, -1);
	return s->array + offset-s->begin;
}

uint64_t dwac_virtual_storage_get_uint64_t(dwac_virtual_storage_type *s, size_t offset)
{
	dwac_virtual_storage_grow_buffer_if_needed(s, offset, 8, 0, -1);
	return *(uint64_t*)(s->array+offset-s->begin);
}

uint32_t dwac_virtual_storage_get_uint32_t(dwac_virtual_storage_type *s, size_t offset)
{
	dwac_virtual_storage_grow_buffer_if_needed(s, offset, 4, 0, -1);
	return *(uint32_t*)(s->array+offset-s->begin);
}

uint16_t dwac_virtual_storage_get_uint16_t(dwac_virtual_storage_type *s, size_t offset)
{
	dwac_virtual_storage_grow_buffer_if_needed(s, offset, 2, 0, -1);
	return *(uint16_t*)(s->array+offset-s->begin);
}

uint8_t dwac_virtual_storage_get_uint8_t(dwac_virtual_storage_type *s, size_t offset)
{
	dwac_virtual_storage_grow_buffer_if_needed(s, offset, 1, 0, -1);
	return *(uint8_t*)(s->array+offset-s->begin);
}

// End of file virtual_storage.h






// Begin of file wa_leb.c




// [1] 5.2.2. Integers
// All integers are encoded using the LEB128 variable-length integer encoding,
// in either unsigned or signed variant.


// Unpack a 64 bit value from a little endian byte buffer.
static uint64_t decode64_le(const uint8_t *buf)
{
	return (uint64_t)buf[7] << 56LL | (uint64_t)buf[6] << 48LL | (uint64_t)buf[5] << 40LL | (uint64_t)buf[4] << 32LL | (uint64_t)buf[3] << 24LL | (uint64_t)buf[2] << 16LL | (uint64_t)buf[1] << 8LL | (uint64_t)buf[0];
}

// Unpack a 32 bit value from a little endian byte buffer.
static uint32_t decode32_le(const uint8_t *buf)
{
	return (uint32_t)buf[3] << 24 | (uint32_t)buf[2] << 16 | (uint32_t)buf[1] << 8 | (uint32_t)buf[0];
}



static void leb128_reader_init(dwac_leb128_reader_type *r, const uint8_t* bytes, size_t size)
{
	r->pos = 0;
	r->nof = size;
	r->array = bytes;
	r->errors = 0;
}

/*static void leb128_reader_deinit(dwac_leb128_reader_type *r)
{
	r->pos = 0;
	r->nof = 0;
	r->array = NULL;
	r->errors = 0;
}*/

static uint64_t leb_read(dwac_leb128_reader_type *r, uint32_t max_bits)
{
	uint64_t result = 0;
	uint64_t shift = 0;
	for(;;) {
	  const uint8_t byte = r->array[r->pos++];
	  result |= (byte & 0x7fLL) << shift;
	  if ((byte & 0x80U) == 0)
	    break;
	  shift += 7;
	}
	return result;
}


static int64_t leb_read_signed(dwac_leb128_reader_type *r, uint32_t max_bits)
{
// Something is fishy with this code. Don't know what.
// Update, needed to write 0x7f as 0x7fLL.

	int64_t result = 0;
	uint64_t shift = 0;
	for(;;)
	{
	    const uint8_t byte = r->array[r->pos++];
	    result |= (byte & 0x7fLL) << shift;
	    shift += 7;
	    if ((0x80 & byte) == 0)
	    {
	        if ((shift < 64) && (byte & 0x40) != 0)
	        {
	            result |= (~0LL << shift);
	            return result;
	        }
	        return result;
	    }
	}
	return result;
}

static uint32_t leb_read_uint32(dwac_leb128_reader_type *r)
{
	#ifdef __LITTLE_ENDIAN__
    const uint32_t v = *((const uint32_t*) (r->array + r->pos));
	#else
	const uint32_t v = decode32_le(r->array + r->pos);
	#endif
    r->pos += 4;
    return v;
}

#ifndef SKIP_FLOAT
static uint64_t leb_read_uint64(dwac_leb128_reader_type *r)
{
	#ifdef __LITTLE_ENDIAN__
    const uint64_t v = *((const uint64_t*) (r->array + r->pos));
	#else
	const uint64_t v = decode64_le(r->array + r->pos);
	#endif
    r->pos += 8;
    return v;
}
#endif

static uint8_t leb_read_uint8(dwac_leb128_reader_type *r)
{
    return r->array[r->pos++];
}

// NOTE! String is not null terminated.
static const char *leb_read_string(dwac_leb128_reader_type *r, size_t *len)
{
    uint32_t str_len = leb_read(r, 32);
    if (len) { *len = str_len;}
    if(str_len > (r->nof - r->pos)) {return NULL;}
    const char * str = (const char *)(r->array + r->pos);
    r->pos += str_len;
    return str;
}

static size_t leb_len(const uint8_t *ptr)
{
	const uint8_t *tmp = ptr;
	for(;;) {
	  const uint8_t byte = *tmp++;
	  if ((byte & 0x80U) == 0)
	    break;
	}
	return tmp - ptr;
}



// End of file wa_leb.c



// Unsure if elements section should be parsed as part of program or data.
// Code is a little bit more complicated if its part of prog but it seems
// tables can not change while running the program so it should be prog.
// Comment / Uncomment to change.
//#define PARSE_ELEMENTS_IN_DATA

// Some macros for convenient stack handling.

#if (DWAC_STACK_SIZE == 0x10000) || (DWAC_STACK_SIZE == 0x100000000)
#define SP_MASK(v) (v)
#else
#define SP_MASK(sp) ((sp) & (DWAC_STACK_SIZE - 1))
#endif

#if 1
#define SP_INC(d) (SP_MASK(++d->sp))
#define SP_DEC(d) (SP_MASK(d->sp--))
#else
static uint32_t sp_inc(dwac_data * d)
{
	return (SP_MASK(++d->sp));
}
static uint32_t sp_dec(dwac_data * d)
{
	return (SP_MASK(d->sp--));
}
#define SP_INC(d) sp_inc(d)
#define SP_DEC(d) sp_dec(d)
#endif

#define PUSH(d) (d->stack[SP_INC(d)])
#define POP(d) (d->stack[SP_DEC(d)])
#define TOP(d) (d->stack[d->sp])

// When setting the data we want to set all 64 bits, so not using s32, u32 here.
#define PUSH_I32(d, v) {PUSH(d).s64 = v;}
#define PUSH_U32(d, v) {PUSH(d).u64 = v;}
#define PUSH_I64(d, v) {PUSH(d).s64 = v;}
#define PUSH_U64(d, v) {PUSH(d).u64 = v;}
#ifndef SKIP_FLOAT
#define PUSH_F32(d, v) {PUSH(d).f32 = v;}
#define PUSH_F64(d, v) {PUSH(d).f64 = v;}
#define PUSH_F32I(d, v) {PUSH(d).u64 = v;}
#define PUSH_F64I(d, v) {PUSH(d).u64 = v;}
#endif

#define POP_I32(d) (POP(d).s32)
#define POP_U32(d) (POP(d).u32)
#define POP_I64(d) (POP(d).s64)
#define POP_U64(d) (POP(d).u64)
#ifndef SKIP_FLOAT
#define POP_F32(d) (POP(d).f32)
#define POP_F64(d) (POP(d).f64)
#define POP_F32I(d) (POP(d).u32)
#define POP_F64I(d) (POP(d).u64)
#endif

#define TOP_I32(d) (TOP(d).s32)
#define TOP_U32(d) (TOP(d).u32)
#define TOP_I64(d) (TOP(d).s64)
#define TOP_U64(d) (TOP(d).u64)
#ifndef SKIP_FLOAT
#define TOP_F32(d) (TOP(d).f32)
#define TOP_F64(d) (TOP(d).f64)
#define TOP_F32I(d) (TOP(d).u32)
#define TOP_F64I(d) (TOP(d).u64)
#endif

// When setting the data we want to set all 64 bits, so not using s32, u32 here.
#define SET_I32(d, v) TOP(d).s64 = v;
#define SET_U32(d, v) TOP(d).u64 = v;
#define SET_I64(d, v) TOP(d).s64 = v;
#define SET_U64(d, v) TOP(d).u64 = v;
#ifndef SKIP_FLOAT
#define SET_F32(d, v) TOP(d).f32 = v;
#define SET_F64(d, v) TOP(d).f64 = v;
#define SET_F32I(d, v) TOP(d).u64 = v;
#define SET_F64I(d, v) TOP(d).u64 = v;
#endif

#define STACK_SIZE(d) (((d)->sp) + DWAC_SP_OFFSET)


#define INVALID_FUNCTION_INDEX 0xFFFFFFFF
#define WA_MAGIC_STACK_VALUE 0x7876898575

#ifndef SKIP_FLOAT

#define ABS(a) (((a) >= 0) ? (a) : -(a))

// [1] 2.2.3. Floating-Point
// Floating-point data represents 32 or 64 bit values that correspond to the respective
// binary formats of the [IEEE-754-2019] standard (Section 3.3).
// [1] 5.2.3. Floating-Point
// Floating-point values are encoded directly by their [IEEE-754-2019] (Section 3.4)
// bit pattern in little endian byte order:

// Compare floats and doubles is a bit uncertain.
// https://stackoverflow.com/questions/4915462/how-should-i-do-floating-point-comparison
// https://frama-c.com/2013/05/09/Definition-of-FLT_EPSILON.html
// https://learn.microsoft.com/en-us/dotnet/api/system.double.epsilon?view=net-7.0
static const double FLOAT_EPSILON = (0x0.000002p0);
static int nearly_equal_float(double a, double b)
{

	// If exactly same, like NaN etc.
	if (memcmp((void*) (&a), (void*) (&b), 8) == 0)
		return true;

	// If almost same.
	if (ABS(a - b) <= (FLOAT_EPSILON * 2))
	{
		return true;
	}

	// There is much more to this but it will depend a lot on application so we can't solve all here.
	return false;
}
static const double DOUBLE_EPSILON = (4.94065645841247E-324);
static int nearly_equal_double(double a, double b)
{
	// If exactly same, like NaN etc.
	if (memcmp((void*) (&a), (void*) (&b), 8) == 0)
		return true;

	// If almost same
	if (ABS(a - b) <= (DOUBLE_EPSILON * 2))
	{
		return true;
	}

	// There is much more to this but it will depend a lot on application so we can't solve all here.
	return false;
}
#endif



static uint32_t rotl32(uint32_t value, unsigned int shift)
{
	shift &= (32 - 1);
	return (value << shift) | ((value) >> (32 - (shift)));
}

static uint32_t rotr32(uint32_t value, unsigned int shift)
{
	shift &= (32 - 1);
	return (value >> shift) | (value << (32 - (shift)));
}

static uint64_t rotl64(uint64_t value, unsigned int shift)
{
	shift &= (64 - 1);
	return (value << shift) | (value >> (64 - (shift)));
}

static uint64_t rotr64(uint64_t value, unsigned int shift)
{
	shift &= (64 - 1);
	return (value >> shift) | (value << (64 - (shift)));
}


/*
static void dump_stack(const wa_data *d)
{
	for(int i=0; i < (d->sp + 1); ++i)
	{
		printf("0x%02x 0x%llx\n", i, (unsigned long long)d->stack[i].u64);
	}
}
*/

// get memory size in bytes.
static uint32_t wa_get_mem_size(const dwac_data *d)
{
	return d->memory.current_size_in_pages * DWAC_PAGE_SIZE;
}

// Code here is from WAC ref [3]
// Copyright (C) Joel Martin <github@martintribe.org>
// Mozilla Public License 2.0.
// There may be more code from WAC in other places.
//
// These are some built in block value types. Used for blocks, loops and ifs.
// It will tell how many return values and parameters a block expect.
// A block can also be of other types that if so are given in section 1.
static const dwac_func_type_type block_types[5] =
{
	{ .nof_parameters = 0, .nof_results = 0, .results_list = { 0 }, },
	{ .nof_parameters = 0, .nof_results = 1, .results_list = { DWAC_I32, 0 }, },
	{ .nof_parameters = 0, .nof_results = 1, .results_list = { DWAC_I64, 0 }, },
	{ .nof_parameters = 0, .nof_results = 1, .results_list = { DWAC_F32, 0 }, },
	{ .nof_parameters = 0, .nof_results = 1, .results_list = { DWAC_F64, 0 }, }
};



// Ref [1] 5.4.1. Control Instructions
//     Block types are encoded in special compressed form, by either
//     the byte 0x40 indicating the empty type, as a single value type,
//     or as a type index encoded as a positive signed integer.
//
// 0x40 -> WA_EMPTY_TYPE, The others are from ref [3].
// Positive values are mapped to types parsed in section 1.
const dwac_func_type_type* dwac_get_func_type_ptr(const dwac_prog *p, int32_t type_idx)
{
	if ((type_idx >= 0) && (type_idx < p->function_types_vector.size))
	{
		return (dwac_func_type_type*) &p->function_types_vector.array[type_idx*sizeof(dwac_func_type_type)];
	}
	else if (type_idx < 0)
	{
		switch (-type_idx)
		{
			case DWAC_EMPTY_TYPE: return &block_types[0];
			case DWAC_I32: return &block_types[1];
			case DWAC_I64: return &block_types[2];
			case DWAC_F32: return &block_types[3];
			case DWAC_F64: return &block_types[4];
			default: break;
		}
	}
	printf("Error in get_func_type_ptr %d\n", type_idx);
	return NULL;
}

// String representation of value_type_enum.
static const char* type_name(uint8_t t)
{
	switch (t)
	{
		//case WA_INVALID_VALUE: return "invalid";
		case DWAC_EMPTY_TYPE: return "void";
		case DWAC_FUNC: return "func";
		case DWAC_EXTERNREF: return "externref";
		case DWAC_ANYFUNC: return "anyfunc";
		case DWAC_VECTYPE: return "vectype";
		case DWAC_F64: return "f64";
		case DWAC_F32: return "f32";
		case DWAC_I64: return "i64";
		case DWAC_I32: return "i32";
		default: return "unknown";
	}
	return "?";
}

// Convert stack value to string representation with type.
long dwac_value_and_type_to_string(char *buf, size_t size, const dwac_value_type *v, uint8_t t)
{
	switch (t)
	{
		case DWAC_I32:
			return snprintf(buf, size, "0x%x:i32", v->u32);
			break;
		case DWAC_I64:
			return snprintf(buf, size, "0x%llx:i64", (long long unsigned) v->u64);
			break;
		#ifndef SKIP_FLOAT
		case DWAC_F32:
			return snprintf(buf, size, "%.7g:f32", v->f32);
			break;
		case DWAC_F64:
			return snprintf(buf, size, "%.7g:f64", v->f64);
			break;
		#endif
		case DWAC_ANYFUNC:
			snprintf(buf, size, "%llx:ANYFUNC", (long long unsigned) v->u64);
			break;
		case DWAC_FUNC:
			snprintf(buf, size, "0x%llx:FUNC", (long long unsigned) v->u64);
			break;
		case DWAC_EMPTY_TYPE:
			snprintf(buf, size, "0x%llx:EMPTY_TYPE", (long long unsigned) v->u64);
			break;
		/*case WA_INVALID_VALUE:
			snprintf(buf, size, "0x%llx:INVALID", (long long unsigned) v->u64);
			break;*/
		default:
			break;
	}
	return snprintf(buf, size, "0x%llx:unknown%u", (long long unsigned) v->u64, t);
}

size_t dwac_func_type_to_string(char *buf, size_t size, const dwac_func_type_type *type)
{
	size_t n = 0;
	n += snprintf(buf + n, size - n, "(param");
	if (type->nof_parameters == 0)
	{
		n += snprintf(buf + n, size - n, " void");
	}
	else
		for (int i = 0; i < type->nof_parameters; ++i)
		{
			n += snprintf(buf + n, size - n, " %s", type_name(type->parameters_list[i]));
		}
	n += snprintf(buf + n, size - n, ") (result");
	if (type->nof_results == 0)
	{
		n += snprintf(buf + n, size - n, " void");
	}
	else
	{
		for (int i = 0; i < type->nof_results; ++i)
		{
			n += snprintf(buf + n, size - n, " %s", type_name(type->results_list[i]));
		}
	}
	n += snprintf(buf + n, size - n, ")");
	return n;
}


// Merge the two by copying all data in upper into the lower.
static void merge_memories(dwac_data *d)
{
	D("merge upper memory with lower\n");
	assert(d->memory.lower_mem.capacity <= d->memory.upper_mem.begin);
	dwac_linear_storage_8_grow_if_needed(&(d->memory.lower_mem), d->memory.upper_mem.end);
	memcpy(d->memory.lower_mem.array + d->memory.upper_mem.begin, d->memory.upper_mem.array, d->memory.upper_mem.end - d->memory.upper_mem.begin);
	dwac_virtual_storage_deinit(&d->memory.upper_mem);
}

// Translate address to host address space.
// Compilers often using memory from lower addresses and high addresses.
// So here is lower and upper memory hoping to catch those.
static uint8_t* translate_addr_grow_if_needed(dwac_data *d, size_t addr, size_t size)
{
	const size_t end = addr + size;
	assert(end >= addr);

	// Is it in lower memory?
	if (end <= d->memory.lower_mem.capacity)
	{
		return d->memory.lower_mem.array + addr;
	}

	// Is it in upper memory?
	if ((addr >= d->memory.upper_mem.begin) && (end <= d->memory.upper_mem.end))
	{
		return d->memory.upper_mem.array + addr - d->memory.upper_mem.begin;
	}

	// Perhaps its arguments memory?
	if ((addr >= DWAC_ARGUMENTS_BASE) && ((end) <= (DWAC_ARGUMENTS_BASE + d->memory.arguments.size)))
	{
		return d->memory.arguments.array + (addr - DWAC_ARGUMENTS_BASE);
	}

	// Wanted range is not in existing memory.
	// Need to expand memory, will hopefully not happen too often.

	if (d->memory.upper_mem.end != 0)
	{
		if ((d->memory.lower_mem.capacity >= d->memory.upper_mem.begin) || (addr > 4 * d->memory.upper_mem.end))
		{
			merge_memories(d);
		}
	}

	//const size_t upper_begin = (d->memory.upper_mem.begin != 0) ? d->memory.upper_mem.begin : wa_get_mem_size(d);
	//const size_t cut = (d->memory.lower_mem.capacity + upper_begin) / 2;
	const size_t cut = 0xF000;

	if ((end <= cut) || (addr <= (2 * d->memory.lower_mem.capacity)))
	{
		// Grow lower memory.
		//printf("grow lower memory %zx %zx %zx %x %x %zx %zx %zx\n", d->memory.lower_mem.capacity, d->memory.upper_mem.begin, d->memory.upper_mem.end, wa_get_mem_size(d), WA_ARGUMENTS_BASE, cut, addr, size);
		if ((d->memory.upper_mem.end != 0) &&
				((end >= d->memory.upper_mem.begin) || ((d->memory.lower_mem.capacity * 2) >= d->memory.upper_mem.begin)))
		{
			// Lower memory has grown up to upper memory.
			assert(end <= d->memory.upper_mem.end);
			merge_memories(d);
		}
		void *ptr = dwac_linear_storage_8_get_ptr(&(d->memory.lower_mem), addr, size);
		//printf("lower memory now %zx\n", d->memory.lower_mem.capacity);
		return ptr;
	}
	else if (end <= wa_get_mem_size(d))
	{
		// Grow upper memory.
		//printf("grow upper memory %zx %zx %zx %x %x %zx %zx %zx\n", d->memory.lower_mem.capacity, d->memory.upper_mem.begin, d->memory.upper_mem.end, wa_get_mem_size(d), WA_ARGUMENTS_BASE, cut, addr, size);
		dwac_virtual_storage_grow_buffer_if_needed(&d->memory.upper_mem, addr, size, d->memory.lower_mem.capacity, wa_get_mem_size(d));
		//printf("upper memory now %zx %zx\n", d->memory.upper_mem.begin, d->memory.upper_mem.end);
		return d->memory.upper_mem.array + addr - d->memory.upper_mem.begin;
	}
	else
	{
		// This is outside and above upper memory, so it's an error.
		printf("outside memory %zx %zx %zx %x %x %zx %zx %zx\n", d->memory.lower_mem.capacity, d->memory.upper_mem.begin, d->memory.upper_mem.end, wa_get_mem_size(d), DWAC_ARGUMENTS_BASE, cut, addr, size);
		snprintf(d->exception, sizeof(d->exception), "Mem out of range 0x%zx 0x%zx 0x%x 0x%x", addr, size, wa_get_mem_size(d), DWAC_ARGUMENTS_BASE);

		if (end <= DWAC_ARGUMENTS_BASE)
		{
			dwac_virtual_storage_grow_buffer_if_needed(&d->memory.upper_mem, addr, size, d->memory.lower_mem.capacity, DWAC_ARGUMENTS_BASE);
			return d->memory.upper_mem.array + addr - d->memory.upper_mem.begin;
		}
		else if (addr >= DWAC_ARGUMENTS_BASE)
		{
			return dwac_linear_storage_8_get_ptr(&d->memory.arguments, addr - DWAC_ARGUMENTS_BASE, size);
		}
		else
		{
			// We might come here if someone reads/writes over the boundary of upper and arguments memory.
			return NULL;
		}
	}
}

void* dwac_translate_to_host_addr_space(dwac_data *d, uint32_t offset, size_t size)
{
	return translate_addr_grow_if_needed(d, offset, size);
}

// NOTE! This translate get/set code is only tested on a little endian host.

static int32_t translate_get_int32(dwac_data *d, uint32_t addr)
{
	const int32_t *host_addr = (const int32_t*) translate_addr_grow_if_needed(d, addr, 4);
	return *host_addr;
}

static int64_t translate_get_int64(dwac_data *d, uint32_t addr)
{
	const int64_t *host_addr = (const int64_t*) translate_addr_grow_if_needed(d, addr, 8);
	return *host_addr;
}

static int8_t translate_get_int8(dwac_data *d, uint32_t addr)
{
	const int8_t *host_addr = (const int8_t*) translate_addr_grow_if_needed(d, addr, 1);
	return *host_addr;
}

static int16_t translate_get_int16(dwac_data *d, uint32_t addr)
{
	const int16_t *host_addr = (const int16_t*) translate_addr_grow_if_needed(d, addr, 2);
	return *host_addr;
}

static void translate_set_int32(dwac_data *d, uint32_t addr, int32_t value)
{
	int32_t *host_addr = (int32_t*) translate_addr_grow_if_needed(d, addr, 4);
	*host_addr = value;
}

static void translate_set_int64(dwac_data *d, uint32_t addr, int64_t value)
{
	int64_t *host_addr = (int64_t*) translate_addr_grow_if_needed(d, addr, 8);
	*host_addr = value;
}

static void translate_set_int8(dwac_data *d, uint32_t addr, int8_t value)
{
	int8_t *host_addr = (int8_t*) translate_addr_grow_if_needed(d, addr, 1);
	*host_addr = value;
}

static void translate_set_int16(dwac_data *d, uint32_t addr, int16_t value)
{
	int16_t *host_addr = (int16_t*) translate_addr_grow_if_needed(d, addr, 2);
	*host_addr = value;
}


static long get_oplen(const uint8_t *ptr)
{
	switch (*ptr)
	{
		case 0x0c ... 0x0d:
		case 0x10:
		case 0x20 ... 0x24:
		case 0x3f ... 0x40:
		case 0x41:
		case 0x42:
		case 0x02 ... 0x04:
			return 1 + leb_len(ptr + 1);
		case 0x0e:
		{
			dwac_leb128_reader_type r;
			leb128_reader_init(&r, ptr + 1, 4);
			uint32_t target_count = leb_read(&r, 32);
			for (uint32_t i = 0; i < target_count; i++)
			{
				leb_read(&r, 32);
			}
			leb_read(&r, 32);
			//assert((1 + r.pos) < 0x10000);
			return 1 + r.pos;
		}
		case 0x11:
		case 0x28 ... 0x3e:
		{
			const uint8_t *tmp = ptr + 1;
			tmp += leb_len(tmp);
			tmp += leb_len(tmp);
			return tmp - ptr;
		}
		case 0x43:
			return 5;
		case 0x44:
			return 9;
		default:
			return 1;
	}
}


static uint32_t find_br_addr(const dwac_prog *p, const dwac_data *d, uint32_t pos)
{
	uint32_t level = 1;
	while (level > 0)
	{
		switch (d->pc.array[pos])
		{
			case 0x02: // block
			case 0x03: // loop
			case 0x04: // if
			{
				level++;
				break;
			}
			case 0x05: // else
				break;
			case 0x0b: // end
			{
				level--;
				if (level==0)
				{
					return pos;
				}
				break;
			}
			default:
				// do nothing
				break;
		}
		pos += get_oplen(d->pc.array + pos);
	}
	return pos;
}

static uint32_t find_else_or_end(const dwac_prog *p, const dwac_data *d, uint32_t pos)
{
	uint32_t level = 1;
	while (level > 0)
	{
		switch (d->pc.array[pos])
		{
			case 0x02: // block
			case 0x03: // loop
			case 0x04: // if
			{
				level++;
				break;
			}
			case 0x05: // else
				if (level==1)
				{
					return pos;
				}
				break;
			case 0x0b: // end
			{
				level--;
				if (level==0)
				{
					return pos;
				}
				break;
			}
			default:
				// do nothing
				break;
		}
		pos += get_oplen(d->pc.array + pos);
	}
	return pos;
}


// Parameters and local variables are pushed to stack.
// Return address is pushed on block stack.
// PC is set to the begin of the function.
// Parameters: fidx is index to the function to be called.
//
// If "gas metering" is not needed it would have been possible
// to recursively call wa_tick instead of this block stack stuff.
long dwac_setup_function_call(const dwac_prog *p, dwac_data *d, uint32_t function_idx)
{
	D("dwac_setup_function_call %d\n", function_idx);
	if (function_idx < p->funcs_vector.nof_imported) {return DWAC_CAN_NOT_CALL_IMPORTED_HERE;}
	if (function_idx >= p->funcs_vector.total_nof) {return DWAC_FUNC_IDX_OUT_OF_RANGE;}
	dwac_function *func = &p->funcs_vector.functions_array[function_idx];
	assert(func && (func->func_type_idx >= 0));
	const dwac_func_type_type *type = dwac_get_func_type_ptr(p, func->func_type_idx);
	assert(type);
	const dwac_stack_pointer_signed_type stack_size = STACK_SIZE(d);
	if (stack_size < type->nof_parameters)
	{
		snprintf(d->exception, sizeof(d->exception), "Insufficent nof parameters calling %u.", function_idx);
		return DWAC_INSUFFICIENT_PARRAMETERS_FOR_CALL;
	}
	const dwac_stack_pointer_type expected_sp_after_call = d->sp - type->nof_parameters; // not counting results here

	// Some data to save until returning.
	dwac_block_stack_entry *block = (dwac_block_stack_entry*) dwac_linear_storage_size_push(&d->block_stack);
	block->block_type_code = dwac_block_type_internal_func;
	block->func_type_idx = func->func_type_idx;
	block->func_info.func_idx = function_idx;
	block->stack_pointer = expected_sp_after_call;
	block->func_info.frame_pointer = d->fp;
	block->func_info.return_addr = d->pc.pos;

	// Remember current stack pointer, as it was before call.
	// The called function need this to know where its parameters and local variables on stack begin.
	// Add one since SP started at -1 as part of small CPU optimize (see WA_SP_INITIAL).
	d->fp = expected_sp_after_call + DWAC_SP_OFFSET;

	// Reserve space on operand stack for local variables of the function to be called.
	d->sp += func->internal_function.nof_local;

	// Set program counter to start of function.
	d->pc.pos = func->internal_function.start_addr;

	//printf("wa_setup_function_call 0x%x 0x%x 0x%x  0x%x 0x%x 0x%x\n", d->fp, d->sp, d->pc.pos, type->nof_results, type->nof_parameters, func->internal_function.nof_local);
	return DWAC_OK;
}

static long call_imported_function(const dwac_prog *p, dwac_data *d, uint32_t function_idx)
{
	D("call_imported_function %d\n", function_idx);
	if (function_idx >= p->funcs_vector.nof_imported) {return DWAC_NOT_AN_IDX_OF_IMPORTED_FUNCTION;}
	dwac_function *func = &p->funcs_vector.functions_array[function_idx];
	assert(func && (func->func_type_idx >= 0));
	const dwac_func_type_type *type = dwac_get_func_type_ptr(p, func->func_type_idx);
	assert(type);
	const dwac_stack_pointer_signed_type stack_size = STACK_SIZE(d);
	if (stack_size < type->nof_parameters)
	{
		snprintf(d->exception, sizeof(d->exception), "Insufficent nof parameters calling %u.", function_idx);
		return DWAC_INSUFFICIENT_PARRAMETERS_FOR_CALL;
	}
	const dwac_stack_pointer_type expected_sp_after_call = d->sp - type->nof_parameters; // not counting results here
	dwac_stack_pointer_type saved_frame_pointer = d->fp;
	d->fp = expected_sp_after_call + DWAC_SP_OFFSET;

	//printf("Calling '%s'\n", func->import_info.name);

	dwac_func_ptr f = func->external_function.func_ptr;
	(f)(d);

	if (d->exception[0] != 0)
	{
		return DWAC_EXCEPTION_FROM_IMPORTED_FUNCTION;
	}
	else if (d->sp != expected_sp_after_call + type->nof_results)
	{
		char tmp[256];
		dwac_func_type_to_string(tmp, sizeof(tmp), type);
		snprintf(d->exception, sizeof(d->exception), "Unexpected nof parameters and/or arguments, %d != %d + %d, %s.", (int) d->sp, (int) expected_sp_after_call, type->nof_results, tmp);
		return DWAC_EXTERNAL_STACK_MISMATCH;
	}

	d->fp = saved_frame_pointer;

	return DWAC_OK;
}

#ifdef LOG_FUNC_NAMES
const char* dwac_get_func_name(const dwac_prog *p, long function_idx)
{
	#ifdef LOG_FUNC_NAMES
	const char* func_name = dwac_linear_storage_size_get_const(&p->func_names, function_idx);
	return func_name ? func_name : "";
	#else
	return "";
	#endif
}
#endif

// This is then main state event machine that runs the program.
// Returns zero if OK
long dwac_tick(const dwac_prog *p, dwac_data *d)
{
	D("dwac_tick\n");

	assert(d->block_stack.size != 0);
	if (d->stack[DWAC_STACK_SIZE - 1].s64 != WA_MAGIC_STACK_VALUE) {return DWAC_STACK_OVERFLOW;}
	if (d->pc.pos >= d->pc.nof) {return DWAC_PC_ADDR_OUT_OF_RANGE;}
	if (d->exception[0] !=  0) {return DWAC_EXCEPTION;}

	// Regarding gas metering. As a CPU optimization: Instead of counting every
	// opcode we only count the control opcodes (0x00 ... 0x11).
	d->gas_meter = DWAC_GAS;

	for(;;)
	{
		assert((d->pc.pos < d->pc.nof));
		const uint8_t opcode = leb_read_uint8(&d->pc);
		D("<%02x> ", opcode);
		switch (opcode)
		{
			case 0x00: // unreachable
				D("unreachable\n");
				// The unreachable instruction causes an unconditional trap.
				sprintf(d->exception, "%s", "unreachable");
				return DWAC_OP_CODE_ZERO;
			case 0x01: // nop
				// The nop instruction does nothing.
				D("nop\n");
				if (--d->gas_meter <= 0) {return DWAC_NEED_MORE_GAS;}
				break;
			case 0x02: // block
			{
				// [1] 5.4.1. Control Instructions
				//     Block types are encoded in special compressed form, by either
				//     the byte 0x40 indicating the empty type, as a single value type,
				//     or as a type index encoded as a positive signed integer.
				//     ...
				//     To avoid any loss in the range of allowed indices, it is treated as a 33 bit signed integer.
				//
				// When reading the byte 0x40 as signed LEB the result is -64 (or -0x40).
				// So func_type_idx need to be a signed variable, longer than 32 bits.
				// Ref [3] assumed in its lookup parser that func_type_idx is always one byte
				// but in the "main" loop it was 32 bit LEB.
				const int64_t blocktype = leb_read_signed(&d->pc, 33);

				dwac_block_stack_entry *block = (dwac_block_stack_entry*) dwac_linear_storage_size_push(&d->block_stack);
				block->block_type_code = dwac_block_type_block;
				block->func_type_idx = blocktype;
				block->block_and_loop_info.br_addr = find_br_addr(p, d, d->pc.pos);
				block->stack_pointer = d->sp; // Or just set it to zero?

				D("block\n");

				const dwac_func_type_type *ptr = dwac_get_func_type_ptr(p, block->func_type_idx);
				if (ptr == NULL)
				{
					printf("value_type %02llx\n", (long long) block->func_type_idx);
					return DWAC_VALUE_TYPE_NOT_SUPPORED_YET;
				}

				if (block->block_and_loop_info.br_addr > d->pc.nof) {return DWAC_BRANCH_ADDR_OUT_OF_RANGE;}
				if (d->pc.pos >= d->pc.nof) {return DWAC_PC_ADDR_OUT_OF_RANGE;}
				if (--d->gas_meter <= 0) {return DWAC_NEED_MORE_GAS;}
				break;
			}
			case 0x03: // loop
			{
				// The loop statement creates a label that can later be branched back to with a
				// br or br_if.
				const int64_t blocktype = leb_read_signed(&d->pc, 33);

				dwac_block_stack_entry *block = (dwac_block_stack_entry*) dwac_linear_storage_size_push(&d->block_stack);
				block->block_type_code = dwac_block_type_loop;
				block->func_type_idx = blocktype;
				block->block_and_loop_info.br_addr = d->pc.pos;
				block->stack_pointer = d->sp;

				D("loop\n");

				const dwac_func_type_type *ptr = dwac_get_func_type_ptr(p, block->func_type_idx);
				if (ptr == NULL)
				{
					printf("value_type %02llx\n", (long long) block->func_type_idx);
					return DWAC_VALUE_TYPE_NOT_SUPPORED_YET;
				}

				if (d->pc.pos >= d->pc.nof) {return DWAC_PC_ADDR_OUT_OF_RANGE;}
				if (--d->gas_meter <= 0) {return DWAC_NEED_MORE_GAS;}
				break;
			}
			case 0x04: // if
			{
				const int64_t blocktype = leb_read_signed(&d->pc, 33);

				dwac_block_stack_entry *block = (dwac_block_stack_entry*) dwac_linear_storage_size_push(&d->block_stack);
				block->block_type_code = dwac_block_type_if;
				block->func_type_idx = blocktype;
				block->stack_pointer = d->sp;

				// Here we search the addresses of both else and end regardless of condition.
				// We could check condition and search for only one of those. But emscripten
				// seems to not use if/else so that optimization will be of little use for now.

				uint32_t addr = find_else_or_end(p, d, d->pc.pos);
				if (addr >= d->pc.nof) {return DWAC_ADDR_OUT_OF_RANGE;}

				if (d->pc.array[addr] == 0x0b) // 0x0b = end
				{
					// An end was found.
					block->if_else_info.else_addr = 0;
					block->if_else_info.end_addr = addr;
				}
				else if (d->pc.array[addr] == 0x05) // 0x05 = else
				{
					// An else was found so continue to find end also.
					block->if_else_info.else_addr = addr;
					uint32_t end_addr = find_else_or_end(p, d, block->if_else_info.else_addr + 1);
					if ((d->pc.array[end_addr] != 0x0b) || (end_addr >= d->pc.nof))
					{
						sprintf(d->exception, "%s", "No end in sight!");
						return DWAC_NO_END;
					}
					block->if_else_info.end_addr = end_addr;
				}
				else
				{
					sprintf(d->exception, "%s", "No end or else found.");
					return DWAC_NO_END_OR_ELSE;
				}

				const uint32_t cond = POP_I32(d);
				if (cond == 0)
				{
					// Condition was not true, check if there is an else.
					if (block->if_else_info.else_addr == 0)
					{
						// No else block.
						d->block_stack.size--;
						d->pc.pos = block->if_else_info.end_addr + 1;
					}
					else
					{
						// Condition was not true so continue with the else
						// until end is found.
						d->pc.pos = block->if_else_info.else_addr + 1;
					}
				}
				else
				{
					// Condition was true, do nothing here, continue until else opcode is found.
				}

				D("if %u\n", cond);

				const dwac_func_type_type *ptr = dwac_get_func_type_ptr(p, block->func_type_idx);
				if (ptr == NULL)
				{
					printf("value_type %02llx\n", (long long) block->func_type_idx);
					return DWAC_VALUE_TYPE_NOT_SUPPORED_YET;
				}

				if (d->pc.pos >= d->pc.nof) {return DWAC_PC_ADDR_OUT_OF_RANGE;}
				if (--d->gas_meter <= 0) {return DWAC_NEED_MORE_GAS;}
				break;
			}
			case 0x05: // else
			{
				// Program has reached an else. So now it shall skip to the end of it?
				const dwac_block_stack_entry *f = (dwac_block_stack_entry*) dwac_linear_storage_size_top(&d->block_stack);
				d->pc.pos = f->if_else_info.end_addr;

				D("else\n");

				if (d->pc.pos >= d->pc.nof) {return DWAC_PC_ADDR_OUT_OF_RANGE;}
				if (--d->gas_meter <= 0) {return DWAC_NEED_MORE_GAS;}
				break;
			}
			case 0x0b: // end
			{
				// Reached the end of a block or function take new PC from the call/block stack.
				D("end\n");

				// Pop from block stack.
				const dwac_block_stack_entry *block = (dwac_block_stack_entry*) dwac_linear_storage_size_pop(&d->block_stack);

				if (block == NULL)
				{
					snprintf(d->exception, sizeof(d->exception), "callstack underflow");
					return DWAC_BLOCK_STACK_UNDER_FLOW;
				}

				const dwac_func_type_type *t = dwac_get_func_type_ptr(p, block->func_type_idx);

				if (t==NULL)
				{
					snprintf(d->exception, sizeof(d->exception), "No type info %ld", (long)block->func_type_idx);
					return DWAC_NO_TYPE_INFO;
				}

				// Restore stack pointer to what is was before block.
				// The result if any is to remain on stack while local variables shall be dropped.
				// So keep a number of entries on top of stack, drop everything in between.
				#if DWAC_STACK_SIZE == 0x10000
				int16_t entries_available_on_stack = (int16_t)(d->sp) - (int16_t)block->stack_pointer;
				if (entries_available_on_stack >= t->nof_results)
				{
					// Move t->nof_results entries from top of stack to block->stack_pointer.
					for (uint32_t n = 0; n < t->nof_results; ++n)
					{
						const uint16_t to = block->stack_pointer + t->nof_results - n;
						const uint16_t from = d->sp - n;
						d->stack[to] = d->stack[from];
					}
					d->sp = block->stack_pointer + t->nof_results;
				}
				else
				{
					snprintf(d->exception, sizeof(d->exception), "missing return values");
					return DWAC_MISSING_RETURN_VALUES;
				}
				#else
				int32_t entries_available_on_stack = (int32_t)(d->sp) - (int32_t)block->stack_pointer;
				if (entries_available_on_stack >= t->nof_results)
				{
					for (uint32_t n = 0; n < t->nof_results; ++n)
					{
						d->stack[SP_MASK(block->stack_pointer + t->nof_results - n)] = d->stack[SP_MASK(d->sp - n)];
					}
					d->sp = block->stack_pointer + t->nof_results;
				}
				else
				{
					snprintf(d->exception, sizeof(d->exception), "missing return values");
					return DWAC_MISSING_RETURN_VALUES;
				}
				#endif

				switch(block->block_type_code)
				{
					case dwac_block_type_internal_func:
					{
						// Restore frame pointer to what previous block had.
						d->fp = block->func_info.frame_pointer;

						// If its a function that ends then it has a special return address.
						// If so set program counter to the saved return address.
						if (block->block_type_code == dwac_block_type_internal_func)
						{
							d->pc.pos = block->func_info.return_addr;
						}

						//printf("fp 0x%x 0x%x 0x%x\n", d->fp, d->sp, d->pc.pos);

						if (d->block_stack.size == 0)
						{
							return DWAC_OK;
						}
						break;
					}
					case dwac_block_type_init_exp:
					{
						return DWAC_OK;
					}
					default: break;
				}

				if (d->stack[DWAC_STACK_SIZE - 1].s64 != WA_MAGIC_STACK_VALUE) {return DWAC_STACK_OVERFLOW;}
				if (d->pc.pos >= d->pc.nof) {return DWAC_PC_ADDR_OUT_OF_RANGE;}
				if (--d->gas_meter <= 0) {return DWAC_NEED_MORE_GAS;}
				break;
			}
			case 0x0c: // br
			{
				// The br statement branches out of a block or back in a loop.
				const uint32_t labelidx = leb_read(&d->pc, 32);
				if (labelidx >= d->block_stack.size)
				{
					sprintf(d->exception, "%s", "Branch stack under run");
					return DWAC_BLOCK_STACK_UNDER_RUN;
				}
				d->block_stack.size -= labelidx;
				const dwac_block_stack_entry *f = (dwac_block_stack_entry*) dwac_linear_storage_size_top(&d->block_stack);
				d->pc.pos = f->block_and_loop_info.br_addr;

				D("br\n");

				if (d->stack[DWAC_STACK_SIZE - 1].s64 != WA_MAGIC_STACK_VALUE) {return DWAC_STACK_OVERFLOW;}
				if (d->pc.pos >= d->pc.nof) {return DWAC_PC_ADDR_OUT_OF_RANGE;}
				if (--d->gas_meter <= 0) {return DWAC_NEED_MORE_GAS;}
				break;
			}
			case 0x0d: // br_if
			{
				D("br_if\n");
				// This is the end of a loop, check condition to see if loop shall continue?
				// First get how many levels of blocks program shall get out of.
				const uint32_t labelidx = leb_read(&d->pc, 32);
				// Take condition value from stack.
				const uint32_t cond = POP_I32(d);
				if (labelidx >= d->block_stack.size)
				{
					sprintf(d->exception, "%s", "Branch stack under run");
					return DWAC_BLOCK_STACK_UNDER_RUN;
				}
				if (cond)
				{
					// Pop a number of blocks (may be zero) from block stack and set program counter.
					d->block_stack.size -= labelidx;
					const dwac_block_stack_entry *f = (dwac_block_stack_entry*) dwac_linear_storage_size_top(&d->block_stack);
					d->pc.pos = f->block_and_loop_info.br_addr;
				}
				else
				{
					/* do nothing, will just continue with next opcode. */
				}

				if (d->stack[DWAC_STACK_SIZE - 1].s64 != WA_MAGIC_STACK_VALUE) {return DWAC_STACK_OVERFLOW;}
				if (d->pc.pos >= d->pc.nof) {return DWAC_PC_ADDR_OUT_OF_RANGE;}
				if (--d->gas_meter <= 0) {return DWAC_NEED_MORE_GAS;}
				break;
			}
			case 0x0e: // br_table
			{
				// Branch using br_table to get labelidx.

				// [1] 2.4.8. Control Instructions
				// br_table performs an indirect branch through an operand indexing into the
				// label vector that is an immediate to the instruction, or to a default target
				// if the operand is out of bounds.

				const size_t max_nof = 16 + d->pc.nof/16;
				const uint32_t table_size = leb_read(&d->pc, 32);
				if (table_size > max_nof) {return DWAC_TO_BIG_BRANCH_TABLE;}
				uint64_t *a = DWAC_ST_MALLOC(table_size * sizeof(uint64_t));

				// Read and setup the entire table.
				for (uint32_t i = 0; i < table_size; i++)
				{
					a[i] = leb_read(&d->pc, 64);
				}

				const uint32_t default_labelidx = leb_read(&d->pc, 32);

				int32_t idx = POP_I32(d);

				const uint32_t labelidx = ((idx >= 0) && (idx < (int32_t) table_size)) ? a[idx] : default_labelidx;

				if (labelidx >= d->block_stack.size)
				{
					sprintf(d->exception, "%s", "Block stack under run");
					return DWAC_BLOCKSTACK_UNDERFLOW;
				}

				if (labelidx > d->block_stack.size) {return DWAC_LABEL_OUT_OF_RANGE;}

				d->block_stack.size -= labelidx;
				const dwac_block_stack_entry *f = (dwac_block_stack_entry*) dwac_linear_storage_size_top(&d->block_stack);
				d->pc.pos = f->block_and_loop_info.br_addr;

				DWAC_ST_FREE(a);

				D("br_table\n");

				if (d->pc.pos >= d->pc.nof) {return DWAC_PC_ADDR_OUT_OF_RANGE;}
				if (--d->gas_meter <= 0) {return DWAC_NEED_MORE_GAS;}
				break;
			}
			case 0x0f: // return
			{
				// Drop any ongoing if, loops etc.
				dwac_block_stack_entry *storage_array = (dwac_block_stack_entry*) d->block_stack.array;
				while ((d->block_stack.size != 0) && (storage_array[d->block_stack.size - 1].block_type_code != dwac_block_type_internal_func))
				{
					d->block_stack.size--;
				}

				if (d->block_stack.size != 0)
				{
					// Set the program count to the end of the function
					const dwac_block_stack_entry *f = (dwac_block_stack_entry*) dwac_linear_storage_size_top(&d->block_stack);

					if (f->block_type_code != dwac_block_type_internal_func)
					{
						return DWAC_UNEXPECTED_RETURN;
					}
					dwac_function *func = &p->funcs_vector.functions_array[f->func_info.func_idx];
					d->pc.pos = func->internal_function.end_addr;
				}
				else
				{
					return DWAC_BLOCKSTACK_UNDERFLOW;
				}

				D("return\n");

				if (d->stack[DWAC_STACK_SIZE - 1].s64 != WA_MAGIC_STACK_VALUE) {return DWAC_STACK_OVERFLOW;}
				if (d->pc.pos >= d->pc.nof) {return DWAC_PC_ADDR_OUT_OF_RANGE;}
				if (--d->gas_meter <= 0) {return DWAC_NEED_MORE_GAS;}
				break;
			}
			case 0x10: // call
			{
				// 0x10 x:funcidx
				const uint32_t function_idx = leb_read(&d->pc, 32);

				D("call %u %s\n", function_idx, dwac_get_func_name(p, function_idx));

				if (function_idx < p->funcs_vector.nof_imported)
				{
					const long r = call_imported_function(p, d, function_idx);
					if (r)
					{
						return r;
					}
				}
				else
				{
					long r = dwac_setup_function_call(p, d, function_idx);
					if (r)
					{
						return r;
					}
				}

				if (d->stack[DWAC_STACK_SIZE - 1].s64 != WA_MAGIC_STACK_VALUE) {return DWAC_STACK_OVERFLOW;}
				if (d->pc.pos >= d->pc.nof) {return DWAC_PC_ADDR_OUT_OF_RANGE;}
				if (--d->gas_meter <= 0) {return DWAC_NEED_MORE_GAS;}
				break;
			}
			case 0x11: // call_indirect
			{
				// [2] 4.4.8.11. call_indirect x y
				// [2] call_indirect calls a function in a table.
				// Indirect call, the function to call is taken via table.
				// call_indirect tableidx typeidx
				//
				// [1] 5.4.1 Control Instructions
				// 0x11 y:typeidx x:tableidx

				const uint32_t typeidx = leb_read(&d->pc, 32);

				const uint32_t tableidx = leb_read(&d->pc, 32);
				if (tableidx != 0) {return DWAC_ONLY_ONE_TABLE_IS_SUPPORTED;}

				// Get index into table from stack.
				uint32_t idx_into_table = POP_I32(d);

				// Ref [3] had some code "if (m->options.mangle_table_index)..." here.
				// No idea what that was about.

				//  Check that its in range.
				if (idx_into_table >= p->func_table.size)
				{
					// br_if had also a default. Not so here then.
					sprintf(d->exception, "%d", idx_into_table);
					return DWAC_OUT_OF_RANGE_IN_TABLE;
				}

				// Get function index via table.
				const int64_t function_idx = p->func_table.array[idx_into_table];

				//  Check that its in range.
				if (function_idx >= p->funcs_vector.total_nof)
				{
					sprintf(d->exception, "%lu %u", (long) function_idx, p->funcs_vector.total_nof);
					return DWAC_FUNCTION_INDEX_OUT_OF_RANGE;
				}

				dwac_function *func = &p->funcs_vector.functions_array[function_idx];

				// The type of the function must match the typeidx​.
				int64_t func_typeidx = func->func_type_idx;
				if (typeidx != func_typeidx)
				{
					sprintf(d->exception, "%lld != %lld", (long long)func_typeidx, (long long)typeidx);
					return DWAC_WRONG_FUNCTION_TYPE;
				}

				// Do we at have enough parameters on stack?
				const dwac_func_type_type *ft_ptr = dwac_get_func_type_ptr(p, func_typeidx);
				const int64_t available = STACK_SIZE(d) - d->fp;
				if (ft_ptr->nof_parameters > available)
				{
					sprintf(d->exception, "%d > %lld.", ft_ptr->nof_parameters, (long long)available);
					return DWAC_INDIRECT_CALL_INSUFFICIENT_NOF_PARAM;
				}

				// Is it an imported or internal function to call?
				if (function_idx < p->funcs_vector.nof_imported)
				{
					const long r = call_imported_function(p, d, function_idx);
					if (r) {return r;}
				}
				else
				{
					const int r = dwac_setup_function_call(p, d, function_idx);
					if (r) {return DWAC_INDIRECT_CALL_FAILED;}
				}

				D("call_indirect %u %u %u %u %s\n", typeidx, tableidx, idx_into_table, (unsigned int)function_idx, dwac_get_func_name(p, function_idx));

				if (d->stack[DWAC_STACK_SIZE - 1].s64 != WA_MAGIC_STACK_VALUE) {return DWAC_STACK_OVERFLOW;}
				if (d->pc.pos >= d->pc.nof) {return DWAC_PC_ADDR_OUT_OF_RANGE;}
				if (--d->gas_meter <= 0) {return DWAC_NEED_MORE_GAS;}
				break;
			}

			case 0x1a: // drop
				d->sp--;
				D("drop\n");
				break;
			case 0x1b: // select
			{
				// Select one of the two topmost values, put it back to stack.
				// Small CPU optimize here (from ref [3]).
				const uint32_t cond = POP_I32(d);
				d->sp--;
				if (!cond)
				{
					TOP(d) = d->stack[SP_MASK(d->sp + 1)];
				}
				D("select %u\n", cond);
				break;
			}
			case 0x1c:
				// 5.4.3. Parametric Instructions
				return DWAC_PARAMETRIC_INSTRUCTIONS_NOT_SUPPORTED_YET;
				break;

			case 0x20: // local.get
			{
				// Get a local variable value, push it to stack.
				const uint32_t localidx = leb_read(&d->pc, 32);
				PUSH(d) = d->stack[SP_MASK(d->fp + localidx)];
				D("local.get %u 0x%llx\n", localidx, (unsigned long long)TOP_U64(d));
				break;
			}
			case 0x21: // local.set
			{
				// Pop value from stack, set a local variable.
				const uint32_t localidx = leb_read(&d->pc, 32);
				const dwac_value_type a = POP(d);
				d->stack[SP_MASK(d->fp + localidx)] = a;
				D("local.set 0x%x 0x%llx\n", localidx, (unsigned long long)a.u64);
				break;
			}
			case 0x22: // local.tee
			{
				// Same as local.set but value also stay on stack instead of popped.
				const uint32_t localidx = leb_read(&d->pc, 32);
				d->stack[SP_MASK(d->fp + localidx)] = TOP(d);
				D("local.tee 0x%x 0x%llx  0x%x 0x%x\n", localidx, (unsigned long long)TOP_U64(d), d->fp, d->sp);
				break;
			}
			case 0x23: // global.get
			{
				// Get a global variable, push it to stack.
				const uint32_t globalidx = leb_read(&d->pc, 32);
				if (globalidx >= d->globals.size) {return DWAC_GLOBAL_IDX_OUT_OF_RANGE;}
				PUSH_U64(d, d->globals.array[globalidx]);
				D("global.get 0x%x 0x%llx\n", globalidx, (long long unsigned)TOP_U64(d));
				break;
			}
			case 0x24: // global.set
			{
				const uint32_t globalidx = leb_read(&d->pc, 32);
				if (globalidx >= d->globals.size) {return DWAC_GLOBAL_IDX_OUT_OF_RANGE;}
				d->globals.array[globalidx] = POP_U64(d);
				D("global.set 0x%x 0x%llx\n", globalidx, (long long unsigned)d->globals.array[globalidx]);
				break;
			}

			case 0x25: // table.get
			case 0x26: // table.set
				// Remember that if func_table can be changed (table.set is implemented) then
				// func_table shall be part of dwac_data and not part of dwac_prog structs.
				// So will not implement this for now.
				// See also 0xFC codes 12 .. 17.
				//const uint32_t tableidx = leb_read(&d->pc, 32);
				//sprintf(d->exception, "0x%x 0x%x", opcode, tableidx);
				return DWAC_TABLE_INSTRUCTIONS_NOT_SUPPORTED;

			case 0x28: // i32.load
			{
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const uint32_t addr = POP_U32(d);
				PUSH_I32(d, translate_get_int32(d, offset + addr));
				D("i32.load 0x%x 0x%x 0x%x\n", offset, addr, TOP_U32(d));
				break;
			}
			case 0x29: // i64.load
			{
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const uint32_t addr = POP_U32(d);
				PUSH_I64(d, translate_get_int64(d, offset + addr));
				D("i64.load 0x%x 0x%x 0x%llx\n", offset, addr, (unsigned long long)TOP_U64(d));
				break;
			}
			#ifndef SKIP_FLOAT
			case 0x2a: // f32.load
			{
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const uint32_t addr = POP_U32(d);
				const uint32_t v = translate_get_int32(d, offset + addr);
				PUSH_F32I(d, v);
				D("f32.load");
				break;
		    }
		    case 0x2b: // f64.load
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const uint32_t addr = POP_U32(d);
				const uint64_t v = translate_get_int64(d, offset + addr);
				PUSH_F64I(d, v);
				D("f64.load");
				break;
		    }
			#endif
		    case 0x2c: // i32.load8_s
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const uint32_t addr = POP_U32(d);
				const int8_t value = translate_get_int8(d, offset + addr);
				PUSH_I32(d, value);
				D("i32.load8_s 0x%x\n", TOP_U32(d));
				break;
		    }
		    case 0x2d: // i32.load8_u
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const uint32_t addr = POP_U32(d);
				const uint8_t value = translate_get_int8(d, offset + addr);
				PUSH_U32(d, value);
				D("i32.load8_u 0x%x\n", TOP_U32(d));
				break;
		    }
		    case 0x2e: // i32.load16_s
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const uint32_t addr = POP_U32(d);
				const int16_t value = translate_get_int16(d, offset + addr);
				PUSH_I32(d, value);
				D("i32.load16_s 0x%x\n", TOP_U32(d));
				break;
		    }
		    case 0x2f: // i32.load16_u
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const uint32_t addr = POP_U32(d);
				const uint16_t value = translate_get_int16(d, offset + addr);
				PUSH_U32(d, value);
				D("i32.load16_u 0x%x\n", TOP_U32(d));
				break;
		    }
		    case 0x30: // i64.load8_s
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const uint32_t addr = POP_U32(d);
				const int8_t value = translate_get_int8(d, offset + addr);
				PUSH_I64(d, value);
				D("i64.load8_s 0x%llx\n", (long unsigned long)TOP_U64(d));
				break;
		    }
		    case 0x31: // i64.load8_u
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const uint32_t addr = POP_U32(d);
				const uint8_t value = translate_get_int8(d, offset + addr);
				PUSH_U64(d, value);
				D("i64.load8_u 0x%llx\n", (long unsigned long)TOP_U64(d));
				break;
		    }
		    case 0x32: // i64.load16_s
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const uint32_t addr = POP_U32(d);
				const int16_t value = translate_get_int16(d, offset + addr);
				PUSH_I64(d, value);
				D("i64.load16_s 0x%llx\n", (long unsigned long)TOP_U64(d));
				break;
		    }
		    case 0x33: // i64.load16_u
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const uint32_t addr = POP_U32(d);
				const uint16_t value = translate_get_int16(d, offset + addr);
				PUSH_U64(d, value);
				D("i64.load16_u 0x%llx\n", (long unsigned long)TOP_U64(d));
				break;
		    }
		    case 0x34: // i64.load32_s
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const uint32_t addr = POP_U32(d);
				const int32_t value = translate_get_int32(d, offset + addr);
				PUSH_I64(d, value);
				D("i64.load32_s 0x%llx\n", (long unsigned long)TOP_U64(d));
				break;
		    }
		    case 0x35: // i64.load32_u
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const uint32_t addr = POP_U32(d);
				const uint32_t value = translate_get_int32(d, offset + addr);
				PUSH_U64(d, value);
				D("i64.load32_u 0x%llx\n", (long unsigned long)TOP_U64(d));
				break;
		    }

		    case 0x36: // i32.store
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const int32_t value = POP_I32(d);
				const uint32_t addr = POP_U32(d);
				translate_set_int32(d, offset + addr, value);
				D("i32.store 0x%llx\n", (long unsigned long)value);
				break;
		    }
		    case 0x37: // i64.store
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const int64_t value = POP_I64(d);
				const uint32_t addr = POP_U32(d);
				translate_set_int64(d, offset + addr, value);
				D("i64.store 0x%llx\n", (long unsigned long)value);
				break;
		    }
		    case 0x38: // f32.store
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const uint32_t value = POP_U32(d);
				const uint32_t addr = POP_U32(d);
				translate_set_int32(d, offset + addr, value);
				D("f32.store");
				break;
		    }
		    case 0x39: // f64.store
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				// Take value and address from stack.
				const uint64_t value = POP_U64(d);
				const uint32_t addr = POP_U32(d);
				translate_set_int64(d, offset + addr, value);
				D("f64.store");
				break;
		    }
		    case 0x3a: // i32.store8
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const int32_t value = POP_I32(d);
				const uint32_t addr = POP_U32(d);
				translate_set_int8(d, offset + addr, value);
				D("i32.store8 0x%x 0x%x 0x%x\n", offset, value, addr);
				break;
		    }
		    case 0x3b: // i32.store16
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const int32_t value = POP_I32(d);
				const uint32_t addr = POP_U32(d);
				translate_set_int16(d, offset + addr, value);
				D("i32.store16 0x%x 0x%x 0x%x\n", offset, value, addr);
				break;
		    }
		    case 0x3c: // i64.store8
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const int8_t value = POP_I32(d);
				const uint32_t addr = POP_U32(d);
				translate_set_int8(d, offset + addr, value);
				D("i64.store8 0x%x 0x%llx 0x%x\n", offset, (long long unsigned)value, addr);
				break;
		    }
		    case 0x3d: // i32.store16
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const int16_t value = POP_I32(d);
				const uint32_t addr = POP_U32(d);
				translate_set_int16(d, offset + addr, value);
				D("i32.store16 0x%x 0x%llx 0x%x\n", offset, (long long unsigned)value, addr);
				break;
		    }
		    case 0x3e: // i64.store32
		    {
				/*const uint32_t flags =*/ leb_read(&d->pc, 32);
				const uint32_t offset = leb_read(&d->pc, 32);
				const int32_t value = POP_I64(d);
				const int32_t addr = POP_I32(d);
				translate_set_int32(d, offset + addr, value);
				D("i64.store32 0x%x 0x%llx 0x%x\n", offset, (long long unsigned)value, addr);
				break;
		    }

			case 0x3f: // current_memory
			{
				uint32_t memidx = leb_read(&d->pc, 32);
				if (memidx != 0) {return DWAC_ONLY_ONE_MEMORY_IS_SUPPORTED;}
				PUSH_I32(d, d->memory.current_size_in_pages);
				D("current_memory 0x%x\n", d->memory.current_size_in_pages);
				break;
			}
			case 0x40: // grow_memory
			{
				// [2] Return value: The previous size of the memory, in units of WebAssembly pages.
				// Not tested, seems emscripten use emscripten_resize_heap instead.
				uint32_t memory_index = leb_read(&d->pc, 32);
				if (memory_index != 0) {return DWAC_ONLY_ONE_MEMORY_IS_SUPPORTED;}
				const uint32_t current_size_in_pages = d->memory.current_size_in_pages;
				const uint32_t requested_increase = TOP_U32(d);
				if ((requested_increase + current_size_in_pages) > d->memory.maximum_size_in_pages)
				{
					// TODO Deny growing of memory?
				}
				SET_U32(d, current_size_in_pages);
				D("grow_memory %u %u\n", current_size_in_pages, requested_increase);
				if (--d->gas_meter <= 0) {return DWAC_NEED_MORE_GAS;}
				break;
			}

			case 0x41: // i32.const
				// Push i32 immediate operand to stack.
				PUSH_I32(d, leb_read_signed(&d->pc, 32));
				D("i32.const 0x%x\n", TOP_U32(d));
				break;
			case 0x42: // i64.const
				PUSH_I64(d, leb_read_signed(&d->pc, 64));
				D("i64.const 0x%llx\n", (long long unsigned)TOP_U64(d));
				break;

			#ifndef SKIP_FLOAT
			case 0x43: // f32.const
			{
				// Push f32 immediate operand to stack.
				// [1] 5.2.3. Floating-Point
				// Floating-point values are encoded directly by their [IEEE-754-2019]
				// (Section 3.4) bit pattern in little endian byte order:
				// So not LEB128 encoded. An alternative would have been to have two LEB values,
				// one for mantissa and one for exponent.
				// TODO This might fail on a big endian host (not tested).
				const uint32_t a = leb_read_uint32(&d->pc);
				PUSH_F32I(d, a);
				D("f32.const 0x%x %g\n", a, TOP_F32(d));
				break;
			}
			case 0x44: // f64.const
			{
				// TODO This might fail on a big endian host (not tested).
				const uint64_t a = leb_read_uint64(&d->pc);
				PUSH_F64I(d, a);
				D("f64.const 0x%llx %g\n", (unsigned long long)a, TOP_F64(d));
				break;
			}
			#endif

			case 0x45: // i32.eqz
				SET_I32(d, (TOP_I32(d) == 0));
				D("i32.eqz 0x%x\n", TOP_I32(d));
				break;
			case 0x46: // i32.eq
			{
				const int32_t b = POP_I32(d);
				SET_I32(d, (TOP_I32(d) == b));
				D("i32.eq 0x%x\n", TOP_I32(d));
				break;
			}
			case 0x47: // i32.ne
			{
				const int32_t b = POP_I32(d);
				SET_I32(d, (TOP_I32(d) != b));
				D("i32.ne 0x%x\n", TOP_I32(d));
				break;
			}
			case 0x48: // i32.lt_s
			{
				const int32_t b = POP_I32(d);
				SET_I32(d, (TOP_I32(d) < b));
				D("i32.lt_s 0x%x\n", TOP_I32(d));
				break;
			}
			case 0x49: // i32.lt_u
			{
				const uint32_t b = POP_U32(d);
				SET_I32(d, (TOP_U32(d) < b));
				D("i32.lt_u 0x%x\n", TOP_I32(d));
				break;
			}
			case 0x4a: // i32.gt_s
			{
				const int32_t b = POP_I32(d);
				SET_I32(d, (TOP_I32(d) > b));
				D("i32.gt_s 0x%x\n", TOP_I32(d));
				break;
			}
			case 0x4b: // i32.gt_u
			{
				const uint32_t b = POP_U32(d);
				SET_I32(d, (TOP_U32(d) > b));
				D("i32.gt_u 0x%x\n", TOP_I32(d));
				break;
			}
			case 0x4c: // i32.le_s
			{
				const int32_t b = POP_I32(d);
				SET_I32(d, (TOP_I32(d) <= b));
				D("i32.le_s 0x%x\n", TOP_I32(d));
				break;
			}
			case 0x4d: // i32.le_u
			{
				const uint32_t b = POP_U32(d);
				SET_I32(d, (TOP_U32(d) <= b));
				D("i32.le_u 0x%x\n", TOP_I32(d));
				break;
			}
			case 0x4e: // i32.ge_s
			{
				const int32_t b = POP_I32(d);
				SET_I32(d, (TOP_I32(d) >= b));
				D("i32.ge_s 0x%x\n", TOP_I32(d));
				break;
			}
			case 0x4f: // i32.ge_u
			{
				const uint32_t b = POP_U32(d);
				SET_I32(d, (TOP_U32(d) >= b));
				D("i32.ge_u 0x%x\n", TOP_I32(d));
				break;
			}
			case 0x50: // i64.eqz
				SET_I32(d, (TOP_I64(d) == 0));
				D("i32.eqz 0x%x\n", TOP_I32(d));
				break;
			case 0x51: // i64.eq
			{
				const int64_t b = POP_I64(d);
				SET_I32(d, (TOP_I64(d) == b));
				D("i32.eq 0x%x\n", TOP_I32(d));
				break;
			}
			case 0x52: // i64.ne
			{
				const int64_t b = POP_I64(d);
				SET_I32(d, (TOP_I64(d) != b));
				D("i32.ne 0x%x\n", TOP_I32(d));
				break;
			}
			case 0x53: // i64.lt_s
			{
				const int64_t b = POP_I64(d);
				SET_I32(d, (TOP_I64(d) < b));
				D("i32.lt_s 0x%x\n", TOP_I32(d));
				break;
			}
			case 0x54: // i64.lt_u
			{
				const uint64_t b = POP_U64(d);
				SET_I32(d, (TOP_U64(d) < b));
				D("i32.lt_u 0x%x\n", TOP_I32(d));
				break;
			}
			case 0x55: // i64.gt_s
			{
				const int64_t b = POP_I64(d);
				SET_I32(d, (TOP_I64(d) > b));
				D("i32.gt_s 0x%x\n", TOP_I32(d));
				break;
			}
			case 0x56: // i64.gt_u
			{
				const uint64_t b = POP_U64(d);
				SET_I32(d, (TOP_U64(d) > b));
				D("i32.gt_u 0x%x\n", TOP_I32(d));
				break;
			}
			case 0x57: // i64.le_s
			{
				const int64_t b = POP_I64(d);
				SET_I32(d, (TOP_I64(d) <= b));
				D("i32.le_s 0x%x\n", TOP_I32(d));
				break;
			}
			case 0x58: // i64.le_u
			{
				const uint64_t b = POP_U64(d);
				SET_I32(d, (TOP_U64(d) <= b));
				D("i32.le_u 0x%x\n", TOP_I32(d));
				break;
			}
			case 0x59: // i64.ge_s
			{
				const int64_t b = POP_I64(d);
				SET_I32(d, (TOP_I64(d) >= b));
				D("i32.ge_s 0x%x\n", TOP_I32(d));
				break;
			}
			case 0x5a: // i64.ge_u
			{
				const uint64_t b = POP_U64(d);
				SET_I32(d, (TOP_U64(d) >= b));
				D("i32.ge_u 0x%x\n", TOP_I32(d));
				break;
			}

			#ifndef SKIP_FLOAT
			case 0x5b: // f32.eq
			{ // not tested
				const float b = POP_F32(d);
				const float a = TOP_F32(d);
				const int c = nearly_equal_float(a, b);
				SET_I32(d, c);
				D("f32.eq %g %g %d\n", a, b, c);
				break;
			}
			case 0x5c: // f32.ne
			{ // not tested
				const float b = POP_F32(d);
				const float a = TOP_F32(d);
				const int c = !nearly_equal_float(a, b);
				SET_I32(d, c);
				D("f32.ne %g %g %d\n", a, b, c);
				break;
			}
			case 0x5d: // f32.lt
			{
				const float b = POP_F32(d);
				const float a = TOP_F32(d);
				const int c = (a < b);
				SET_I32(d, c);
				D("f32.lt %g %g %d\n", a, b, c);
				break;
			}
			case 0x5e: // f32.gt
			{
				const float b = POP_F32(d);
				const float a = TOP_F32(d);
				const int c = a > b;
				SET_I32(d, c);
				D("f32.gt %g %g %d\n", a, b, c);
				break;
			}
			case 0x5f: // f32.le
			{
				const float b = POP_F32(d);
				const float a = TOP_F32(d);
				const int c = (a <= b);
				SET_I32(d, c);
				D("f32.le %g %g %d\n", a, b, c);
				break;
			}
			case 0x60: // f32.ge
			{
				const float b = POP_F32(d);
				const float a = TOP_F32(d);
				const int c = (a >= b);
				SET_I32(d, c);
				D("f32.ge %g %g %d\n", a, b, c);
				break;
			}
			case 0x61: // f64.eq
			{
				const double b = POP_F64(d);
				const double a = TOP_F64(d);
				SET_I32(d, nearly_equal_double(a, b));
				D("f64.eq %g %g %d\n", a, b, TOP_I32(d));
				break;
			}
			case 0x62: // f64.ne
			{
				const double b = POP_F64(d);
				const double a = TOP_F64(d);
				SET_I32(d, !nearly_equal_float(a, b));
				D("f64.ne %g %g %d\n", a, b, TOP_I32(d));
				break;
			}
			case 0x63: // f64.lt
			{
				const double b = POP_F64(d);
				const double a = TOP_F64(d);
				SET_I32(d, (a < b));
				D("f64.lt %g %g %d\n", a, b, TOP_I32(d));
				break;
			}
			case 0x64: // f64.gt
			{
				const double b = POP_F64(d);
				const double a = TOP_F64(d);
				SET_I32(d, (a > b));
				D("f64.gt %g %g %d\n", a, b, TOP_I32(d));
				break;
			}
			case 0x65: // f64.le
			{
				const double b = POP_F64(d);
				const double a = TOP_F64(d);
				SET_I32(d, (a <= b));
				D("f64.le %g %g %d\n", a, b, TOP_I32(d));
				break;
			}
			case 0x66: // f64.ge
			{
				const double b = POP_F64(d);
				const double a = TOP_F64(d);
				SET_I32(d, (a >= b));
				D("f64.ge %g %g %d\n", a, b, TOP_I32(d));
				break;
			}
			#endif

			case 0x67: // i32.clz
			{   // Not tested
				// Count leading zeros in a binary number.
				const int32_t a = TOP_I32(d);
				const int32_t c = __builtin_clz(a);
				SET_I32(d, c);
				printf("i32.clz 0x%x 0x%x\n", a, c);
				break;
			}
			case 0x68: // i32.ctz
			{   // Not tested
				// Count trailing zeros in a binary number.
				const int32_t a = TOP_I32(d);
				const int32_t c = __builtin_ctz(a);
				SET_I32(d, c);
				printf("i32.ctz 0x%x 0x%x\n", a, c);
				break;
			}
			case 0x69: // i32.popcnt
			{   // Not tested
				// Count number of 1s in a binary number.
				const int32_t a = TOP_I32(d);
				const int32_t c = __builtin_popcount(a);
				SET_I32(d, c);
				printf("i32.popcnt 0x%x 0x%x\n", a, c);
				break;
			}
			case 0x6a: // i32.add
			{
				const int32_t b = POP_I32(d);
				D("i32.add 0x%x 0x%x 0x%x\n", TOP_I32(d), b, TOP_I32(d) + b);
				SET_I32(d, TOP_I32(d) + b);
				break;
			}
			case 0x6b: // i32.sub
			{
				const int32_t b = POP_I32(d);
				D("i32.sub 0x%x 0x%x 0x%x\n", TOP_I32(d), b, TOP_I32(d) + b);
				SET_I32(d, TOP_I32(d) - b);
				break;
			}
			case 0x6c: // i32.mul
			{
				const int32_t b = POP_I32(d);
				D("i32.mul 0x%x 0x%x 0x%x\n", TOP_I32(d), b, TOP_I32(d) + b);
				SET_I32(d, TOP_I32(d) * b);
				break;
			}
			case 0x6d: // i32.div_s
			{
				const int32_t b = POP_I32(d);
				const int32_t a = TOP_I32(d);
				if (b == 0)
				{
					sprintf(d->exception, "Divide %d by zero", a);
					return DWAC_DIVIDE_BY_ZERO;
				}
				// When examining ref [3], they had some addition check here.
				// if (a == 0x80000000 && b == -1) {return WA_INTEGER_OVERFLOW;}
				// https://stackoverflow.com/questions/46378104/why-does-integer-division-by-1-negative-one-result-in-fpe
				// Don't want undefined behavior so will just replicate that code from ref [3].
				// Need to do some testing some day.
				if ((a == 0x80000000) && (b == -1))
				{
					sprintf(d->exception, "Integer overflow (a == 0x80000000) && (b == -1).");
					return DWAC_INTEGER_OVERFLOW;
				}
				SET_I32(d, a / b);
				D("i32.div_s 0x%x 0x%x 0x%x\n", a, b, TOP_I32(d));
				break;
			}
			case 0x6e: // i32.div_u
			{
				const uint32_t b = POP_U32(d);
				const uint32_t a = TOP_U32(d);
				if (b == 0)
				{
					sprintf(d->exception, "Divide %u by zero.", a);
					return DWAC_DIVIDE_BY_ZERO;
				}
				SET_U32(d, a / b);
				D("i32.div_u 0x%x 0x%x 0x%x\n", a, b, TOP_I32(d));
				break;
			}
			case 0x6f: // i32.rem_s
			{
				const int32_t b = POP_I32(d);
				const int32_t a = TOP_I32(d);
				if (b == 0)
				{
					sprintf(d->exception, "Divide %d by zero", a);
					return DWAC_DIVIDE_BY_ZERO;
				}
				SET_I32(d, ((a == 0x80000000) && (b == -1)) ? 0 : a % b);
				D("i32.rem_s 0x%x 0x%x 0x%x\n", a, b, TOP_I32(d));
				break;
			}
			case 0x70: // i32.rem_u
			{
				const uint32_t b = POP_U32(d);
				const uint32_t a = TOP_U32(d);
				if (b == 0)
				{
					sprintf(d->exception, "Divide %u by zero.", a);
					return DWAC_DIVIDE_BY_ZERO;
				}
				SET_U32(d, a % b);
				D("i32.rem_u 0x%x 0x%x 0x%x\n", a, b, TOP_I32(d));
				break;
			}
			case 0x71: // i32.and
			{
				const uint32_t b = POP_U32(d);
				D("i32.and 0x%x 0x%x 0x%x\n", TOP_I32(d), b, TOP_I32(d) + b);
				SET_U32(d, TOP_U32(d) & b);
				break;
			}
			case 0x72: // i32.or
			{
				const uint32_t b = POP_U32(d);
				D("i32.or 0x%x 0x%x 0x%x\n", TOP_I32(d), b, TOP_I32(d) + b);
				SET_U32(d, TOP_U32(d) | b);
				break;
			}
			case 0x73: // i32.xor
			{
				const int32_t b = POP_I32(d);
				D("i32.xor 0x%x 0x%x 0x%x\n", TOP_I32(d), b, TOP_I32(d) + b);
				SET_I32(d, TOP_I32(d) ^ b);
				break;
			}
			case 0x74: // i32.shl
			{
				const int32_t b = POP_I32(d);
				D("i32.shl 0x%x 0x%x 0x%x\n", TOP_I32(d), b, TOP_I32(d) << b);
				SET_I32(d, TOP_I32(d) << b);
				break;
			}
			case 0x75: // i32.shr_S
			{
				const int32_t b = POP_I32(d);
				const int32_t a = TOP_I32(d);
				SET_I32(d, a >> b);
				D("i32.shl 0x%x 0x%x 0x%x\n", a, b, TOP_I32(d));
				break;
			}
			case 0x76: // i32.shr_u
			{
				const uint32_t b = POP_U32(d);
				const uint32_t a = TOP_U32(d);
				SET_U32(d, a >> b);
				D("i32.shr_u 0x%x 0x%x 0x%x\n", a, b, TOP_I32(d));
				break;
			}
			case 0x77: // i32.rotl
			{ // not tested.
				// Rotate left.
				const uint32_t b = POP_U32(d);
				const uint32_t a = TOP_U32(d);
				const uint32_t c = rotl32(a, b);
				SET_U32(d, c);
				D("i32.rotl 0x%x 0x%x 0x%x\n", a, b, c);
				break;
			}
			case 0x78: // i32.rotr
			{ // not tested.
				const uint32_t b = POP_U32(d);
				const uint32_t a = TOP_U32(d);
				const uint32_t c = rotr32(a, b);
				SET_U32(d, c);
				D("i32.rotr 0x%x 0x%x 0x%x\n", a, b, c);
				break;
			}
			case 0x79: // i64.clz
			{   // Not tested
				// Count leading zeros  in a binary number.
				const int64_t a = TOP_I64(d);
				const int32_t c = __builtin_clzll(a);
				SET_I32(d, c);
				D("i64.clz 0x%llx %d\n", (long long)a, c);
				break;
			}
			case 0x7a: // i64.ctz
			{   // Not tested
				// Count trailing zeros in a binary number.
				const int64_t a = TOP_I64(d);
				const int32_t c = __builtin_ctzll(a);
				SET_I32(d, c);
				D("i64.ctz 0x%llx %d\n", (long long)a, c);
				break;
			}
			case 0x7b: // i64.popcnt
			{   // Not tested
				// Count number of 1s in a binary number.
				const int64_t a = TOP_I64(d);
				const int32_t c = __builtin_popcountll(a);
				SET_I32(d, c);
				D("i64.popcnt 0x%llx %d\n", (long long)a, c);
				break;
			}
			case 0x7c: // i64.add
			{
				const int64_t b = POP_I64(d);
				D("i64.add 0x%llx 0x%llx 0x%llx\n", (long long)TOP_I64(d), (long long)b, (long long)TOP_I64(d)+b);
				SET_I64(d, TOP_I64(d) + b);
				break;
			}
			case 0x7d: // i64.sub
			{
				const int64_t b = POP_I64(d);
				D("i64.sub 0x%llx 0x%llx 0x%llx\n", (long long)TOP_I64(d), (long long)b, (long long)TOP_I64(d)-b);
				SET_I64(d, TOP_I64(d) - b);
				break;
			}
			case 0x7e: // i64.mul
			{
				const int64_t b = POP_I64(d);
				D("i64.mul 0x%llx 0x%llx 0x%llx\n", (long long)TOP_I64(d), (long long)b, (long long)TOP_I64(d)*b);
				SET_I64(d, TOP_I64(d) * b);
				break;
			}
			case 0x7f: // i64.div_s
			{
				const int64_t b = POP_I64(d);
				const int64_t a = TOP_I64(d);
				if (b == 0)
				{
					sprintf(d->exception, "Divide %lld by zero", (long long)a);
					return DWAC_DIVIDE_BY_ZERO;
				}
				if ((a == 0x8000000000000000LL) && (b == -1))
				{
					sprintf(d->exception, "Integer overflow (a == 0x80000000) && (b == -1).");
					return DWAC_INTEGER_OVERFLOW;
				}
				SET_I64(d, a / b);
				D("i64.div_s 0x%llx 0x%llx 0x%llx\n", (long long unsigned)a, (long long unsigned)b, (long long unsigned)TOP_U64(d));
				break;
			}
			case 0x80: // i64.div_u
			{
				const uint64_t b = POP_U64(d);
				const uint64_t a = TOP_U64(d);
				if (b == 0)
				{
					sprintf(d->exception, "Divide %llu by zero.", (long long unsigned) a);
					return DWAC_DIVIDE_BY_ZERO;
				}
				SET_U64(d, a / b);
				D("i64.div_u 0x%llx 0x%llx 0x%llx\n", (long long unsigned)a, (long long unsigned)b, (long long unsigned)TOP_U64(d));
				break;
			}
			case 0x81: // i64.rem_s
			{
				const int64_t b = POP_I64(d);
				const int64_t a = TOP_I64(d);
				if (b == 0)
				{
					sprintf(d->exception, "Divide %lld by zero", (long long) a);
					return DWAC_DIVIDE_BY_ZERO;
				}
				SET_I64(d, ((a == 0x8000000000000000LL) && (b == -1)) ? 0 : a % b);
				D("i64.rem_s 0x%llx 0x%llx 0x%llx\n", (long long unsigned)a, (long long unsigned)b, (long long unsigned)TOP_U64(d));
				break;
			}
			case 0x82: // i64.rem_u
			{
				const uint64_t b = POP_U64(d);
				const uint64_t a = TOP_U64(d);
				if (b == 0)
				{
					sprintf(d->exception, "Divide %llu by zero.", (long long unsigned) a);
					return DWAC_DIVIDE_BY_ZERO;
				}
				SET_U64(d, a % b);
				D("i64.rem_u 0x%llx 0x%llx 0x%llx\n", (long long unsigned)a, (long long unsigned)b, (long long unsigned)TOP_U64(d));
				break;
			}
			case 0x83: // i64.and
			{
				const int64_t b = POP_I64(d);
				D("i64.and 0x%llx 0x%llx 0x%llx\n", (unsigned long long)TOP_U64(d), (unsigned long long)b, (unsigned long long)(TOP_U64(d) & b));
				SET_I64(d, TOP_I64(d) & b);
				break;
			}
			case 0x84: // i64.or
			{
				const int64_t b = POP_I64(d);
				D("i64.or 0x%llx 0x%llx 0x%llx\n", (unsigned long long)TOP_U64(d), (unsigned long long)b, (unsigned long long)(TOP_U64(d) | b));
				SET_I64(d, TOP_I64(d) | b);
				break;
			}
			case 0x85: // i64.xor
			{
				const int64_t b = POP_I64(d);
				D("i64.xor 0x%llx 0x%llx 0x%llx\n", (unsigned long long)TOP_U64(d), (unsigned long long)b, (unsigned long long)(TOP_U64(d) ^ b));
				SET_I64(d, TOP_I64(d) ^ b);
				break;
			}
			case 0x86: // i64.shl
			{
				const int64_t b = POP_I64(d);
				const int64_t a = TOP_I64(d);
				SET_I64(d, a << b);
				D("i64.shl 0x%llx 0x%llx 0x%llx\n", (unsigned long long)a, (unsigned long long)b, (unsigned long long)(TOP_U64(d)));
				break;
			}
			case 0x87: // i64.shr_S
			{
				const int64_t b = POP_I64(d);
				const int64_t a = TOP_I64(d);
				SET_I64(d, a >> b);
				D("i64.shr_S 0x%llx 0x%llx 0x%llx\n", (unsigned long long)a, (unsigned long long)b, (unsigned long long)(TOP_U64(d)));
				break;
			}
			case 0x88: // i64.shr_u
			{
				const uint64_t b = POP_U64(d);
				const uint64_t a = TOP_I64(d);
				SET_U64(d, a >> b);
				D("i64.shr_u 0x%llx 0x%llx 0x%llx\n", (long long)a, (long long)b, (long long)TOP_I64(d));
				break;
			}
			case 0x89: // i64.rotl
			{ // not tested.
				// Rotate left.
				const uint64_t b = POP_U64(d);
				const uint64_t a = TOP_U64(d);
				const uint64_t c = rotl64(a, b);
				SET_U64(d, c);
				printf("i64.rotl 0x%llx 0x%llx 0x%llx\n", (unsigned long long)a, (unsigned long long)b, (unsigned long long)c);
				break;
			}
			case 0x8a: // i64.rotr
			{ // not tested.
				// Rotate right.
				const uint64_t b = POP_U64(d);
				const uint64_t a = TOP_U64(d);
				const uint64_t c = rotr64(a, b);
				SET_U64(d, c);
				printf("i64.rotr 0x%llx 0x%llx 0x%llx\n", (unsigned long long)a, (unsigned long long)b, (unsigned long long)c);
				break;
			}

			#ifndef SKIP_FLOAT
			case 0x8b: // f32.abs
			{
				const float a = TOP_F32(d);
				const float c = fabs(a);
				D("f32.abs %g %g\n", a, c);
				SET_F32(d, c);
				break;
			}
			case 0x8c: // f32.neg
			{ // not tested
				const float a = TOP_F32(d);
				const float c = -a;
				D("f32.neg %g %g\n", a, c);
				SET_F32(d, c);
				break;
			}
			case 0x8d: // f32.ceil
			{ // not tested
				float a = TOP_F32(d);
				SET_F32(d, ceil(a));
				D("f32.ceil %g %g\n", a, ceil(a));
				break;
			}
			case 0x8e: // f32.floor
			{
				float a = TOP_F32(d);
				SET_F32(d, floor(a));
				D("f32.floor %g %g\n", a, floor(a));
				break;
			}
			case 0x8f: // f32.trunc
			{ // not tested
				float a = TOP_F32(d);
				SET_F32(d, trunc(a));
				D("f32.trunc %g %g\n", a, trunc(a));
				break;
			}
			case 0x90: // f32.nearest
			{ // not tested
				float a = TOP_F32(d);
				SET_F32(d, rint(a));
				D("f32.nearest %g %g\n", a, rint(a));
				break;
			}
			case 0x91: // f32.sqrt
			{ // not tested
				float a = TOP_F32(d);
				SET_F32(d, sqrt(a));
				D("f32.sqrt %g %g\n", a, sqrt(a));
				break;
			}
			case 0x92: // f32.add
			{
				const float b = POP_F32(d);
				const float a = TOP_F32(d);
				const float c = a + b;
				D("f32.add %g %g %g\n", a,b,c);
				SET_F32(d, c);
				break;
			}
			case 0x93: // f32.sub
			{
				const float b = POP_F32(d);
				const float a = TOP_F32(d);
				const float c = a - b;
				D("f32.sub %g %g %g\n", a,b,c);
				SET_F32(d, c);
				break;
			}
			case 0x94: // f32.mul
			{
				const float b = POP_F32(d);
				const float a = TOP_F32(d);
				const float c = a * b;
				D("f32.mul %g %g %g\n", a,b,c);
				SET_F32(d, c);
				break;
			}
			case 0x95: // f32.div
			{ // not tested
				const float b = POP_F32(d);
				const float a = TOP_F32(d);
				const float c = a / b;
				D("f32.div %g %g %g\n", a,b,c);
				SET_F32(d, c);
				break;
			}
			case 0x96: // f32.min
			{ // not tested
				const float b = POP_F32(d);
				const float a = TOP_F32(d);
				const float c = fmin(a, b);
				SET_F32(d, c);
				D("f32.min %g %g %g\n", a,b,c);
				break;
			}
			case 0x97: // f32.max
			{ // not tested
				const float b = POP_F32(d);
				const float a = TOP_F32(d);
				const float c = fmax(a, b);
				SET_F32(d, c);
				D("f32.max %g %g %g\n", a,b,c);
				break;
			}
			case 0x98: // f32.copysign
			{ // not tested
				const float b = POP_F32(d);
				const float a = TOP_F32(d);
				const float c = signbit(b) ? -fabs(a) : fabs(a);
				SET_F32(d, c);
				D("f32.copysign %g %g %g\n", a,b,c);
				break;
			}

			case 0x99: // f64.abs
			{
				const double a = TOP_F64(d);
				SET_F64(d, fabs(a));
				D("f64.abs %g %g\n", a, TOP_F64(d));
				break;
			}
			case 0x9a: // f64.neg
				SET_F64(d, -TOP_F64(d));
				D("f64.neg %g\n", TOP_F64(d));
				break;
			case 0x9b: // f64.ceil
				SET_F64(d, ceil(TOP_F64(d)));
				D("f64.ceil %g\n", TOP_F64(d));
				break;
			case 0x9c: // f64.floor
				SET_F64(d, floor(TOP_F64(d)));
				D("f64.floor %g\n", TOP_F64(d));
				break;
			case 0x9d: // f64.trunc
				SET_F64(d, trunc(TOP_F64(d)));
				D("f64.trunc %g\n", TOP_F64(d));
				break;
			case 0x9e: // f64.nearest
				SET_F64(d, rint(TOP_F64(d)));
				D("f64.nearest %g\n", TOP_F64(d));
				break;
			case 0x9f: // f64.sqrt
				SET_F64(d, sqrt(TOP_F64(d)));
				D("f64.sqrt %g\n", TOP_F64(d));
				break;

			case 0xa0: // f64.add
			{
				const double b = POP_F64(d);
				const double a = TOP_F64(d);
				SET_F64(d, a + b);
				D("f64.add %g %g %g\n", a,b,TOP_F64(d));
				break;
			}
			case 0xa1: // f64.sub
			{
				const double b = POP_F64(d);
				const double a = TOP_F64(d);
				SET_F64(d, a - b);
				D("f64.sub %g %g %g\n", a,b,TOP_F64(d));
				break;
			}
			case 0xa2: // f64.mul
			{
				const double b = POP_F64(d);
				const double a = TOP_F64(d);
				SET_F64(d, a * b);
				D("f64.mul %g %g %g\n", a,b,TOP_F64(d));
				break;
			}
			case 0xa3: // f64.div
			{
				const double b = POP_F64(d);
				const double a = TOP_F64(d);
				SET_F64(d, a / b);
				D("f64.div %g %g %g\n", a,b,TOP_F64(d));
				break;
			}
			case 0xa4: // f64.min
			{
				double b = POP_F64(d);
				double a = TOP_F64(d);
				SET_F64(d, fmin(a, b));
				D("f64.min %g %g %g\n", a, b, TOP_F64(d));
				break;
			}
			case 0xa5: // f64.max
			{
				double b = POP_F64(d);
				double a = TOP_F64(d);
				SET_F64(d, fmax(a, b));
				D("f64.max %g %g %g\n", a, b, TOP_F64(d));
				break;
			}
			case 0xa6: // f64.copysign
			{ // not tested
				double b = POP_F64(d);
				double a = TOP_F64(d);
				double c = signbit(b) ? -fabs(a) : fabs(a);
				SET_F64(d, c);
				D("f64.copysign %g %g %g\n", a, b, c);
				break;
			}
			#endif
			case 0xa7: // i32.wrap_i64
			{
				// [2] The wrap instruction, is used to convert numbers of type i64
				// to type i32. If the number is larger than what an i32 can hold
				// this operation will wrap, resulting in a different number.
				SET_U64(d, TOP_U64(d) & 0x00000000ffffffff);
				D("i32.wrap_i64 0x%llx\n", (unsigned long long)TOP_U64(d));
				break;
			}

			#ifndef SKIP_FLOAT
			case 0xa8: // i32.trunc_f32_s
			{ // not tested
				const float a = TOP_F32(d);
				if (isnan(a))
				{
					sprintf(d->exception, "Not a number.");
					return DWAC_INVALID_INTEGER_CONVERSION;
				}
				else if ((a > INT32_MAX) || (a < INT32_MIN))
				{
					sprintf(d->exception, "Can't convert %g to int32.", a);
					return DWAC_INTEGER_OVERFLOW;
				}
				const int32_t c = a;
				SET_I32(d, c);
				D("i32.trunc_f32_s %g %d\n", a, c);
				break;
			}
			case 0xa9: // i32.trunc_f32_u
			{
				const float a = TOP_F32(d);
				if (isnan(a))
				{
					sprintf(d->exception, "Not a number.");
					return DWAC_INVALID_INTEGER_CONVERSION;
				}
				else if ((a > UINT32_MAX) || (a < (float)0.0))
				{
					sprintf(d->exception, "Can't convert %g to uint32.", a);
					return DWAC_INTEGER_OVERFLOW;
				}
				SET_U32(d, a);
				D("i32.trunc_f32_u %g %u\n", a, TOP_U32(d));
				break;
			}
			case 0xaa: // i32.trunc_f64_s
			{ // not tested
				const double a = TOP_F64(d);
				if (isnan(a))
				{
					sprintf(d->exception, "Not a number.");
					return DWAC_INVALID_INTEGER_CONVERSION;
				}
				else if ((a > INT32_MAX) || (a < INT32_MIN))
				{
					sprintf(d->exception, "Can't convert %g to int32.", a);
					return DWAC_INTEGER_OVERFLOW;
				}
				const int32_t c = a;
				SET_I32(d, c);
				D("i32.trunc_f64_s %g %d\n", a, c);
				break;
			}
			case 0xab: // i32.trunc_f64_u
			{
				const double a = TOP_F64(d);
				if (isnan(a))
				{
					sprintf(d->exception, "Not a number.");
					return DWAC_INVALID_INTEGER_CONVERSION;
				}
				else if ((a > UINT32_MAX) || (a < 0.0))
				{
					sprintf(d->exception, "Can't convert %g to uint32.", a);
					return DWAC_INTEGER_OVERFLOW;
				}
				SET_U32(d, a);
				D("i32.trunc_f64_u %g %u\n", a, TOP_U32(d));
				break;
			}
			#endif
			case 0xac: // i64.extend_i32_s
			{
				// [2] The extend instructions, are used to convert (extend) numbers of type
				// i32 to type i64. There are signed and unsigned versions of this instruction.
				const int32_t a = TOP_I32(d);
				SET_I64(d, (int64_t)a);
				D("i64.extend_i32_s %d %lld\n", a, (long long)TOP_I64(d));
				break;
			}
			case 0xad: // i64.extend_i32_u
			{
				uint32_t a = TOP_U32(d);
				SET_U64(d, (uint64_t)a);
				D("i64.extend_i32_u 0x%x 0x%llx\n", a, (unsigned long long)TOP_U64(d));
				break;
			}

			#ifndef SKIP_FLOAT
			case 0xae: // i64.trunc_f32_s
			{ // not tested
				const float a = TOP_F32(d);
				if (isnan(a))
				{
					sprintf(d->exception, "Not a number.");
					return DWAC_INVALID_INTEGER_CONVERSION;
				}
				else if ((a > INT64_MAX) || (a < INT64_MIN))
				{
					sprintf(d->exception, "Can't convert %g to int64.", a);
					return DWAC_INTEGER_OVERFLOW;
				}
				const int64_t c = a;
				SET_I64(d, c);
				D("i64.trunc_f32_s %g %lld\n", a, (long long)c);
				break;
			}
			case 0xaf: // i64.trunc_f32_u
			{ // not tested
				const double a = TOP_F32(d);
				if (isnan(a))
				{
					sprintf(d->exception, "Not a number.");
					return DWAC_INVALID_INTEGER_CONVERSION;
				}
				else if ((a > UINT64_MAX) || (a < 0.0))
				{
					sprintf(d->exception, "Can't convert %g to uint64.", a);
					return DWAC_INTEGER_OVERFLOW;
				}
				const uint64_t c = a;
				SET_U64(d, c);
				D("i64.trunc_f32_u %g %llu\n", a, (unsigned long long)c);
				break;
			}
			case 0xb0: // i64.trunc_f64_s
			{ // not tested
				const double a = TOP_F64(d);
				if (isnan(a))
				{
					sprintf(d->exception, "Not a number.");
					return DWAC_INVALID_INTEGER_CONVERSION;
				}
				else if (a > INT64_MAX || a < INT64_MIN)
				{
					sprintf(d->exception, "Can't convert %g to int64.", a);
					return DWAC_INTEGER_OVERFLOW;
				}
				const int64_t c = a;
				SET_I64(d, c);
				D("i64.trunc_f64_s %g %llu\n", a, (long long)c);
				break;
			}
			case 0xb1: // i64.trunc_f64_u
			{
				const double a = TOP_F64(d);
				if (isnan(a))
				{
					sprintf(d->exception, "Not a number.");
					return DWAC_INVALID_INTEGER_CONVERSION;
				}
				else if ((a > UINT64_MAX) || (a <= -0.5))
				{
					sprintf(d->exception, "Can't convert %g to uint64.", TOP_F64(d));
					return DWAC_INTEGER_OVERFLOW;
				}
				const uint64_t c = a;
				SET_U64(d, c);
				D("i64.trunc_f64_u %g %llu\n", a, (unsigned long long)c);
				break;
			}
			case 0xb2: // f32.convert_i32_s
			{
				int32_t a = TOP_I32(d);
				SET_F32I(d, a);
				D("f32.convert_i32_s 0x%x %g\n", a, TOP_F32(d));
				break;
			}
			case 0xb3: // f32.convert_i32_u
			{// not tested
				SET_F32(d, TOP_U64(d));
				D("f32.convert_i32_u %g\n", TOP_F32(d));
				break;
			}
			case 0xb4: // f32.convert_i64_s
				SET_F32(d, TOP_I64(d));
				D("f32.convert_i64_s %g\n", TOP_F32(d));
				break;
			case 0xb5: // f32.convert_i64_u
				SET_F32(d, TOP_U64(d));
				D("f32.convert_i64_u %g\n", TOP_F32(d));
				break;
			case 0xb6: // f32.demote_f64
			{
				const double a = TOP_F64(d);
				const float b = a;
				SET_F32(d, b);
				D("f32.demote_f64 %g\n", TOP_F32(d));
				break;
			}
			case 0xb7: // f64.convert_i32_s
				SET_F64(d, TOP_I32(d));
				D("f64.convert_i32_s %g\n", TOP_F64(d));
				break;
			case 0xb8: // f64.convert_i32_u
				SET_F64(d, TOP_U32(d));
				D("f64.convert_i32_u %g\n", TOP_F64(d));
				break;
			case 0xb9: // f64.convert_i64_s
				SET_F64(d, TOP_I64(d));
				D("f64.convert_i64_s %g\n", TOP_F64(d));
				break;
			case 0xba: // f64.convert_i64_u
				SET_F64(d, TOP_U64(d));
				D("f64.convert_i64_u %g\n", TOP_F64(d));
				break;
			case 0xbb: // f64.promote_f32
			{
				const float a = TOP_F32(d);
				const double b = a;
				SET_F64(d, b);
				D("f64.promote_f32 %g\n", TOP_F64(d));
				break;
			}
			case 0xbc: // i32.reinterpret_f32
			{// not tested
				// [2] The reinterpret instructions, are used to reinterpret the bits of a number as a different type.
				// do nothing.
				D("i32.reinterpret_f32 0x%x %g\n", TOP_I32(d), TOP_F32(d));
				break;
			}
			case 0xbd: // i64.reinterpret_f64
			{
				// do nothing.
				D("i64.reinterpret_f64 0x%llx %g\n", (long long unsigned)TOP_I64(d), TOP_F64(d));
				break;
			}
			case 0xbe: // f32.reinterpret_i32
			{   // not tested
				// do nothing.
				D("f32.reinterpret_i32 0x%x %g\n", TOP_I32(d), TOP_F32(d));
				break;
			}
			case 0xbf: // f64.reinterpret_i64
			{   // not tested
				// do nothing.
				D("f64.reinterpret_i64 0x%llx %g\n", (long long unsigned)TOP_I64(d), TOP_F64(d));
				break;
			}
			#endif
			#if 1
			// https://github.com/WebAssembly/sign-extension-ops/blob/master/proposals/sign-extension-ops/Overview.md
			case 0xc0: // i32.extend8_s
			{ // not tested
				int8_t a = TOP_I32(d);
				SET_I32(d, a);
				break;
			}
			case 0xc1: // i32.extend16_s
			{ // not tested
				int16_t a = TOP_I32(d);
				SET_I32(d, a);
				break;
			}
			case 0xc2: // i64.extend8_s
			{ // not tested
				int8_t a = TOP_I64(d);
				SET_I64(d, a);
				break;
			}
			case 0xc3: // i64.extend16_s
			{ // not tested
				int16_t a = TOP_I64(d);
				SET_I64(d, a);
				break;
			}
			case 0xc4: // i64.extend32_s
			{ // not tested
				int32_t a = TOP_I64(d);
				SET_I64(d, a);
				break;
			}
			#endif
			case 0xfc: // memory.init. data.drop, memory.copy, memory.fill
			{
				// 5.4.7. Numeric Instructions
				// The saturating truncation instructions all have a one byte prefix,
				// whereas the actual opcode is encoded by a variable-length unsigned integer.
				const uint32_t actual_opcode = leb_read(&d->pc, 32);
				sprintf(d->exception, "0x%x", actual_opcode);
				// TODO
				return DWAC_SATURATING_NOT_SUPPORTED_YET;
			}
			case 0xfd:
			{
				// [1] 5.4.8. Vector Instructions
				//     All variants of vector instructions are represented by separate byte codes.
				//     They all have a one byte prefix, whereas the actual opcode is encoded by a
				//     variable-length unsigned integer.
				// So the one byte is probably the opcode (0xfd). Then follows a LEB.
				uint32_t memarg = leb_read(&d->pc, 32);
				// TODO This is like an entire additional instruction set.
				sprintf(d->exception, "No vectors implemented 0x%x 0x%x", opcode, memarg);
				return DWAC_VECTORS_NOT_SUPPORTED;
			}
			default:
				sprintf(d->exception, "unrecognized opcode 0x%x", opcode);
				return DWAC_UNKNOWN_OPCODE;
		}
	}
	return DWAC_NEED_MORE_GAS;
}

// This is used to run some code to get a value.
// A clever (hopefully) trick from ref [3].
//
// [1] 5.4.9. Expressions
//     Expressions are encoded by their instruction sequence terminated with an
//     explicit 0x0B opcode for end.
//
// So we need to run some instructions. The result is expected to be placed on the stack.
static long run_init_expr(const dwac_prog *p, dwac_data *d, uint8_t type, uint32_t maxlen)
{
	dwac_block_stack_entry *block = (dwac_block_stack_entry*) dwac_linear_storage_size_push(&d->block_stack);
	block->block_type_code = dwac_block_type_init_exp;
	block->func_type_idx = -type; // Positive numbers are for function types (section 1) so make it negative here.
	block->stack_pointer = DWAC_SP_INITIAL;
	block->func_info.frame_pointer = 0;
	block->func_info.return_addr = 0;

	assert(d->sp == DWAC_SP_INITIAL);
	d->fp = STACK_SIZE(d);

	D("run_init_expr 0x%x 0x%x 0x%llx\n", d->fp, d->sp, (long long)d->pc.pos);

	long r = dwac_tick(p, d);

	if ((r == DWAC_OK) && (d->sp == DWAC_SP_INITIAL))
	{
		return DWAC_NO_RESULT_ON_STACK;
	}
	return r;
}

// Returns NULL if not found.
const dwac_function* dwac_find_exported_function(const dwac_prog *p, const char *name)
{
	return dwac_hash_list_find(&p->exported_functions_list, name);
}

// Find the address from its module name.
static void* find_imported_function(const dwac_prog *p, const char *name)
{
	// TODO: Check that arguments match, one way would be to add signature
	// to name here and in wa_register_function.
	return dwac_hash_list_find(&p->available_functions_list, name);
}

// TODO Perhaps take a pointer to dwac_data instead of "char *exception, size_t exception_size".
// We need some dwac_data in "Element Section" anyway.
long dwac_parse_prog_sections(dwac_prog *p, dwac_data *d, const uint8_t *bytes, uint32_t byte_count, FILE* log)
{
	D("dwac_parse_prog_sections %d\n", byte_count);

	const size_t max_nof = 16 + byte_count/16;

	p->bytecodes.array = bytes;
	p->bytecodes.nof = byte_count;
	p->start_function_idx = INVALID_FUNCTION_INDEX;

	// Check the magic numbers (first 8 bytes).
	{
		const uint32_t magic_word = leb_read_uint32(&p->bytecodes);
		const uint32_t magic_version = leb_read_uint32(&p->bytecodes);
		D("Magic %08x %x\n", magic_word, magic_version);
		if ((magic_word != DWAC_MAGIC) || (magic_version != DWAC_VERSION))
		{
			snprintf(d->exception, sizeof(d->exception), "Not WebAsm or not supported version 0x%08x 0x%08x", magic_word, magic_version);
			return DWAC_NOT_WEBASM_OR_SUPPORTED_VERSION;
		}
	}

	// Read the sections
	while (p->bytecodes.pos < p->bytecodes.nof)
	{
		D("next byte %02x\n", p->bytecodes.array[p->bytecodes.pos]);
		uint32_t section_id = leb_read(&p->bytecodes, 7);
		uint32_t tmp = ~section_id;
		D("section_id %02x\n", section_id);
		uint32_t section_len = leb_read(&p->bytecodes, 32);
		const uint32_t section_begin = p->bytecodes.pos;
		D("Parsing prog section %d, pos 0x%llx, len %d\n", section_id, (long long)p->bytecodes.pos, section_len);
		D("section_id %02x\n", section_id);
		assert(tmp == ~section_id);
		switch (section_id)
		{
			case 0: // Custom Section
			{
				// [1] 5.5.3. Custom Section
				//     Intended to be used for debugging information or third-party extensions, and are
				//     ignored by the WebAssembly semantics.
				#ifdef LOG_FUNC_NAMES
				// Emscripten gives function names in custom section name if "-g" option is given.
				// This is how to read that section.
				size_t strlen = 0;
				const char* csname = leb_read_string(&p->bytecodes, &strlen);
				D("Custom Section %d %.*s\n", section_len, (int)strlen, csname);
				if (memcmp(csname, "name", 4)==0)
				{
					// https://webassembly.github.io/spec/core/appendix/custom.html
					while(p->bytecodes.pos < section_begin + section_len)
					{
						uint8_t subsection_id = leb_read_uint8(&p->bytecodes);
						uint32_t subsection_len = leb_read(&p->bytecodes, 32);
						D("  name subsection %d %d\n", subsection_id, subsection_len);
						switch(subsection_id)
						{
							case 1:
								uint32_t n = leb_read(&p->bytecodes, 32);
								for(int i = 0; i < n; i++)
								{
									size_t func_name_len = 0;
									uint32_t func_idx = leb_read(&p->bytecodes, 32);
									const char* func_name = leb_read_string(&p->bytecodes, &func_name_len);
									D("    %d %.*s\n", func_idx, (int)func_name_len, func_name);
									if (func_name_len>DWAC_HASH_LIST_MAX_KEY_SIZE)
									{
										func_name_len = DWAC_HASH_LIST_MAX_KEY_SIZE;
									}
									uint8_t tmp[DWAC_HASH_LIST_MAX_KEY_SIZE+1]= {0};
									memcpy(tmp, func_name, func_name_len);
									tmp[func_name_len] = 0;
									dwac_linear_storage_size_set(&p->func_names, func_idx, tmp);
								}
								break;
							default:
								p->bytecodes.pos += subsection_len;
								break;
						}
					}
					printf("\n");
				}
				p->bytecodes.pos = section_begin + section_len;
				#else
				D("Custom Section ignored %d %s\n", section_len, p->bytecodes.array + p->bytecodes.pos);
				p->bytecodes.pos += section_len;
				#endif
				break;
			}
			case 1:
				// [1] 5.5.4. Type Section
				if (p->function_types_vector.size > 0) {return DWAC_ONLY_ONE_SECTION_ALLOWED;}
				uint32_t nof_function_types = leb_read(&p->bytecodes, 32);
				if (nof_function_types > max_nof) {return DWAC_TO_MANY_FUNCTION_TYPES;}
				dwac_linear_storage_size_grow_if_needed(&p->function_types_vector, nof_function_types);

				for (uint32_t i = 0; i < nof_function_types; i++)
				{
					dwac_func_type_type *type = dwac_linear_storage_size_get(&p->function_types_vector, i);

					// [1] 5.3.6. Function Types
					//     Function types are encoded by the byte 0x60 followed by the respective vectors of
					//     parameter and result types.
					// Does this mean there can be other types such as limits here?
					int magic = leb_read_uint8(&p->bytecodes);
					if (magic != DWAC_FUNC)
					{
						printf("Not the function type code 0x%x\n", magic);
					}

					type->nof_parameters = leb_read(&p->bytecodes, 32); // How many parameters a function will have.
					if (type->nof_parameters > sizeof(type->parameters_list))
					{
						snprintf(d->exception, sizeof(d->exception), "To many parameters %d\n", type->nof_parameters);
						return DWAC_TO_MANY_PARAMETERS;
					}
					for (uint32_t n = 0; n < type->nof_parameters; n++)
					{
						type->parameters_list[n] = leb_read(&p->bytecodes, 32);
					}
					type->nof_results = leb_read(&p->bytecodes, 32); // How many return values a function will have.
					if (type->nof_results > sizeof(type->results_list))
					{
						snprintf(d->exception, sizeof(d->exception), "To many result %d\n", type->nof_results);
						return DWAC_TO_MANY_RESULT_VALUES;
					}
					for (uint32_t r = 0; r < type->nof_results; r++)
					{
						type->results_list[r] = leb_read(&p->bytecodes, 32);
					}

					char tmp[256];
					dwac_func_type_to_string(tmp, sizeof(tmp), type);
					D("Type %u %s\n", i, tmp);
				}
				break;
			case 2:
			{
				// [1] 2.5.11. Imports
				// [1] 5.5.5. Import Section
				//     The import section has the id 2. It decodes into a vector of
				//     imports that represent the imports component of a module.

				if ((p->funcs_vector.functions_array != NULL) || (p->funcs_vector.nof_imported>0)) {return DWAC_ONLY_ONE_SECTION_ALLOWED;}

				uint32_t nof_imported = leb_read(&p->bytecodes, 32);
				if (nof_imported > max_nof) {return DWAC_TO_MANY_IMPORTS;}

				p->funcs_vector.functions_array = DWAC_ST_MALLOC(nof_imported * sizeof(dwac_function));

				for (uint32_t i = 0; i < nof_imported; i++)
				{
					size_t import_module_size;
					const char *import_module_ptr = leb_read_string(&p->bytecodes, &import_module_size); // Name of so/dll file to look for function in (will probably ignore this).

					size_t import_field_size;
					const char *import_field_ptr = leb_read_string(&p->bytecodes, &import_field_size); // Name of the wanted function (or global etc).

					const uint32_t t = leb_read_uint8(&p->bytecodes);

					switch (t)
					{
						case DWAC_FUNCTYPE:
						{
							dwac_function *f = &p->funcs_vector.functions_array[p->funcs_vector.nof_imported];

							if ((import_module_size + 1 + import_field_size) > DWAC_HASH_LIST_MAX_KEY_SIZE)
							{
								snprintf(d->exception, sizeof(d->exception), "Name to long '%.256s'\n", import_field_ptr);
								return DWAC_EXPORT_NAME_TO_LONG;
							}

							char m[DWAC_HASH_LIST_MAX_KEY_SIZE+1];
							snprintf(m, sizeof(m), "%.*s/%.*s", (int) import_module_size, import_module_ptr, (int) import_field_size, import_field_ptr);

							// Ref[1] 5.4.1. Control Instructions
							//     Unlike any other occurrence, the type index in a block type is encoded as a positive
							//     signed integer, so that its signed LEB128 bit pattern cannot collide with the encoding
							//     of value types or the special code 0x40, which correspond to the LEB128 encoding of
							//     negative integers. To avoid any loss in the range of allowed indices, it is treated
							//     as a 33 bit signed integer.
							//
							// So does the above apply to this also? Will assume this is only func_type_idx so do
							// not need to be signed 33 bits. Same in Section 3 "Function Section" below.
							f->block_type_code = dwac_block_type_imported_func;
							f->func_type_idx = leb_read(&p->bytecodes, 32);

							// Some logging.

							char tmp[256];
							const dwac_func_type_type *type = dwac_get_func_type_ptr(p, f->func_type_idx);
							dwac_func_type_to_string(tmp, sizeof(tmp), type);
							if (log) {fprintf(log, "Import 0x%x '%s' %s\n", p->funcs_vector.nof_imported, m, tmp);}

							void *ptr = find_imported_function(p, m);
							if (ptr == NULL)
							{
								snprintf(d->exception, sizeof(d->exception), "Did not find '%s' %s", m, tmp);
								return DWAC_IMPORT_FIELD_NOT_FOUND;
							}

							f->external_function.func_ptr = ptr;

							p->funcs_vector.nof_imported++;
							break;
						}
						case DWAC_TABLETYPE:
						case DWAC_MEMTYPE:
						case DWAC_GLOBALTYPE:
						default:
						{
							snprintf(d->exception, sizeof(d->exception), "Importing %d, not yet supported '%.*s' '%.*s'\n", t, (int) import_module_size,
									import_module_ptr, (int) import_field_size, import_field_ptr);
							return DWAC_UNKNOWN_TYPE_OF_IMPORT;
							break;
						}
					}
				}
				p->funcs_vector.total_nof = p->funcs_vector.nof_imported;
				break;
			}
			case 3:
				// [1] 5.5.6. Function Section
				//     The function section has the id 3. It decodes into a vector of type indices
				//     that represent the type fields of the functions in the funcs component of a
				//     module. The locals and body fields of the respective functions are encoded
				//     separately in the code section.
				if (p->funcs_vector.total_nof != p->funcs_vector.nof_imported) {return DWAC_ONLY_ONE_SECTION_ALLOWED;}

				p->funcs_vector.total_nof += leb_read(&p->bytecodes, 32); // How many web assembly functions there are.
				if (p->funcs_vector.total_nof > max_nof) {return DWAC_TO_MANY_FUNCTIONS;}
				p->funcs_vector.functions_array = DWAC_ST_REALLOC(p->funcs_vector.functions_array, p->funcs_vector.total_nof * sizeof(dwac_function));
				for (uint32_t i = p->funcs_vector.nof_imported; i < p->funcs_vector.total_nof; i++)
				{
					dwac_function *f = &p->funcs_vector.functions_array[i];
					f->block_type_code = dwac_block_type_internal_func;
					p->funcs_vector.functions_array[i].func_type_idx = leb_read(&p->bytecodes, 32);
					p->funcs_vector.functions_array[i].func_idx = i;
				}
				break;
			case 4:
			{
				// [1] 5.5.7. Table Section
				//     The table section has the id 4. It decodes into a vector of tables that
				//     represent the tables component of a module.

				// This will only set the number of tables and their size. The content of the tables
				// are set in "Element Section".

				uint32_t nof_tables = leb_read(&p->bytecodes, 32);

				// Only one table is supported.
				// Check also that we did not already get a table from Import(2) section.
				if ((p->func_table.size != 0) || (nof_tables != 1))
				{
					snprintf(d->exception, sizeof(d->exception), "Only one table is supported.\n");
					return DWAC_ONLY_ONE_TABLE_IS_SUPPORTED;
				}

				uint32_t table_type = leb_read(&p->bytecodes, 33);
				if (table_type != DWAC_ANYFUNC)
				{
					return DWAC_NOT_SUPPORTED_TABLE_TYPE;
				}

				uint32_t flags = leb_read(&p->bytecodes, 32);
				uint32_t nof_table_elements = leb_read(&p->bytecodes, 32);
				if (nof_table_elements > max_nof) {return DWAC_TO_MANY_TABLE_ELEMENTS;}
				dwac_linear_storage_64_grow_if_needed(&p->func_table, nof_table_elements);
				if (flags & 0x1)
				{
					/*uint32_t maximum =*/leb_read(&p->bytecodes, 32);
				}

				break;
			}
			case 5:    // Memory Section
				// This is parsed in data.
				p->bytecodes.pos += section_len;
				break;
			case 6: // Global Section
				p->bytecodes.pos += section_len;
				break;
			case 7:
			{
				// [1] 5.5.10. Export Section
				//     The export section has the id 7. It decodes into a vector of exports that represent
				//     the exports component of a module.
				const uint32_t nof_exports = leb_read(&p->bytecodes, 32);
				if (nof_exports > max_nof) {return DWAC_TO_MANY_EXPORTS;}
				for (uint32_t i = 0; i < nof_exports; i++)
				{
					size_t name_len = 0;
					const char *name = leb_read_string(&p->bytecodes, &name_len);
					uint32_t type = leb_read_uint8(&p->bytecodes);
					uint32_t index = leb_read(&p->bytecodes, 32);

					if ((name==NULL) || (name_len > 64))
					{
						snprintf(d->exception, sizeof(d->exception), "Name to long '%.*s'\n", (int)name_len, name);
						return DWAC_EXPORT_NAME_TO_LONG;
					}

					switch (type)
					{
						case DWAC_FUNCTYPE:
						{
							// TODO wa_exported_functions_type only contains one pointer so set the pointer directly in
							// hash list exported_functions_list. That is calloc is not needed.
							char export_name[64+1];
							strncpy(export_name, name, name_len);
							export_name[name_len] = 0;;

							dwac_function *e = &p->funcs_vector.functions_array[index];

							// Some logging
							const dwac_func_type_type *t = dwac_get_func_type_ptr(p, e->func_type_idx);
							char tmp[256];
							dwac_func_type_to_string(tmp, sizeof(tmp), t);
							if (log) {fprintf(log, "Exported 0x%x '%s'  %s\n", index, export_name, tmp);}

							dwac_hash_list_put(&p->exported_functions_list, export_name, e);
							break;
						}
						case DWAC_TABLETYPE:
							if (log) {fprintf(log, "Ignored export of table '%.*s' 0x%x\n", (int) name_len, name, index);}
							break;
						case DWAC_MEMTYPE:
							if (log) {fprintf(log, "Ignored export of memory '%.*s' 0x%x\n", (int) name_len, name, index);}
							break;
						case DWAC_GLOBALTYPE:
							if (log) {fprintf(log, "Ignored export of global '%.*s' 0x%x\n", (int) name_len, name, index);}
							break;
						default:
							snprintf(d->exception, sizeof(d->exception), "Unknown type %d for '%.*s'.", type, (int) name_len, name);
							return DWAC_EXPORT_TYPE_NOT_IMPL_YET;
					}
				}
				break;
			}
			case 8: // 5.5.11. Start Section
				p->start_function_idx = leb_read(&p->bytecodes, 32);
				break;
			case 9: // [1] 5.5.12. Element Section
			{
				#ifndef PARSE_ELEMENTS_IN_DATA

				// Need to run some script code in this section so need a runtime here.
				leb128_reader_init(&d->pc, p->bytecodes.array, p->bytecodes.nof);
				d->pc.pos = p->bytecodes.pos;

				// The initial contents of a table is uninitialized. Element segments can be
				// used to initialize a subrange of a table from a static vector of elements.
				uint32_t nof_elements = leb_read(&d->pc, 32);
				if (nof_elements > max_nof) {return DWAC_TO_MANY_ELEMENTS;}
				for (uint32_t i = 0; i < nof_elements; i++)
				{
					{
						const uint32_t index = leb_read(&d->pc, 32);
						if (index != 0) {return DWAC_ONLY_ONE_TABLE_IS_SUPPORTED;}
					}

					// Run the init_expr to get offset into table on the stack.
					run_init_expr(p, d, DWAC_I32, section_len);

					size_t offset = POP_I32(d);

					uint32_t nof_entries = leb_read(&d->pc, 32);
					if (nof_entries > max_nof) {return DWAC_TO_MANY_ENTRIES;}

					dwac_linear_storage_64_grow_if_needed(&p->func_table, offset + nof_entries);

					for (uint32_t j = 0; j < nof_entries; ++j)
					{
						const uint64_t v = leb_read(&d->pc, 64);
						dwac_linear_storage_64_set(&p->func_table, offset + j, v);
					}
				}
				p->bytecodes.pos += section_len;
				assert(p->bytecodes.pos == d->pc.pos);
				#else
				p->bytecodes.pos += section_len;
				#endif
				break;
			}
			case 10: // [1] 5.5.13. Code Section
			{
				uint32_t nof_code_entries = leb_read(&p->bytecodes, 32);
				if ((nof_code_entries + p->funcs_vector.nof_imported) > p->funcs_vector.total_nof)
				{
					snprintf(d->exception, sizeof(d->exception), "To many code entries. %d %d %d.", nof_code_entries, p->funcs_vector.nof_imported, p->funcs_vector.total_nof);
					return DWAC_OUT_OF_RANGE_IN_CODE_SECTION;
				}

				for (uint32_t i = 0; i < nof_code_entries; i++)
				{
					// the actual function code, which in turn consists of
					dwac_function *f = &p->funcs_vector.functions_array[p->funcs_vector.nof_imported + i];

					// size of the function code in bytes
					const uint32_t code_size = leb_read(&p->bytecodes, 32);
					const uint32_t code_start = p->bytecodes.pos;

					// the declaration of locals

					const uint32_t nof_local_variables = leb_read(&p->bytecodes, 32);
					if (nof_local_variables > max_nof) {return DWAC_TOO_MANY_LOCAL_VARIABLES;}

					f->internal_function.nof_local = 0;

					for (uint32_t j = 0; j < nof_local_variables; j++)
					{
						// Local declarations are compressed into a vector
						uint32_t count = leb_read(&p->bytecodes, 32); // Vector length.
						f->internal_function.nof_local += count;
						uint32_t valtype = leb_read(&p->bytecodes, 7);
						switch (valtype)
						{
							case DWAC_I32:
							case DWAC_F32:
							case DWAC_FUNC:
							case DWAC_ANYFUNC:
							case DWAC_EXTERNREF:
							case DWAC_I64:
							case DWAC_F64:
								break;
							case DWAC_VECTYPE:
							default:
								return DWAC_VECTORS_NOT_SUPPORTED;
						}
					}

					f->internal_function.nof_local += 10;

					// the function body as an expression.
					f->internal_function.start_addr = p->bytecodes.pos;
					f->internal_function.end_addr = code_start + code_size - 1;
					f->block_type_code = dwac_block_type_internal_func;

					// Ref [3] did this extra check here, why not, doing so also.
					if (bytes[f->internal_function.end_addr] != 0x0b)
					{
						snprintf(d->exception, sizeof(d->exception), "Missing end opcode at 0x%x.", f->internal_function.end_addr);
						return DWAC_MISSING_OPCODE_END;
					}

					p->bytecodes.pos = f->internal_function.end_addr + 1;
				}
				break;
			}
			case 11: // [1] 5.5.14. Data Section
			case 12: // [1] 5.5.15. Data Count Section
				// Not implemented yet.
				p->bytecodes.pos += section_len;
				break;
			default:
				snprintf(d->exception, sizeof(d->exception), "Section %d unimplemented\n", section_id);
				return DWAC_UNKNOWN_SECTION;
				p->bytecodes.pos += section_len;
		}
		if (p->bytecodes.pos != (section_begin + section_len))
		{
			snprintf(d->exception, sizeof(d->exception), "Section %d did not add up, %u + %u != %llu\n", section_id, section_begin, section_len, (long long unsigned)p->bytecodes.pos);
			return DWAC_MISALLIGNED_SECTION;
		}
	}

	#ifdef TRANSLATE_ALL_AT_LOAD
	for (uint32_t func_idx = p->funcs_vector.nof_imported; func_idx < p->funcs_vector.total_nof; func_idx++)
	{
		const long r = find_blocks_for_internal_function(p, func_idx);
		if (r != DWAC_OK) {return r;}
	}
	#endif

	return DWAC_OK;
}


long dwac_parse_data_sections(const dwac_prog *p, dwac_data *d)
{
	D("dwac_parse_data_sections\n");

	const size_t max_nof = 16 + p->bytecodes.nof/16;

	leb128_reader_init(&d->pc, p->bytecodes.array, p->bytecodes.nof);

	// Skip the magic numbers, already checked in "wa_parse_prog_sections".
	d->pc.pos += 8;

	// Read the sections
	while (d->pc.pos < d->pc.nof)
	{
		uint32_t id = leb_read(&d->pc, 7);
		uint32_t section_len = leb_read(&d->pc, 32);
		const uint32_t section_begin = d->pc.pos;
		D("Parsing data section %d, pos 0x%llx, len %d\n", id, (long long)p->bytecodes.pos, section_len);

		switch (id)
		{
			case 0: // Custom Section
			case 1: // Type Section
			case 2: // Import Section
			case 3: // Function Section
			case 4: // [1] 5.5.7. Table Section
				// Taken care of by prog so skip these now.
				d->pc.pos += section_len;
				break;
			case 5: // Memory Section
			{
				// [1] 5.3.8. Memory Types
				uint32_t lim = leb_read(&d->pc, 32);
				if ((lim != 1) || (d->memory.lower_mem.array != NULL)) {return DWAC_ONLY_ONE_MEMORY_IS_SUPPORTED;}

				uint32_t flags = leb_read(&d->pc, 32);

				// Initial size in pages, as requested by compiler, one page is 0x10000 (PAGE_SIZE).
				d->memory.current_size_in_pages = leb_read(&d->pc, 32);
				if (flags & 0x1)
				{
					d->memory.maximum_size_in_pages = leb_read(&d->pc, 32);
					if (d->memory.maximum_size_in_pages > DWAC_MAX_NOF_PAGES)
					{
						snprintf(d->exception, sizeof(d->exception), "0x%x", d->memory.maximum_size_in_pages);
						d->memory.maximum_size_in_pages = DWAC_MAX_NOF_PAGES;
						return DWAC_TO_MUCH_MEMORY_REQUESTED;
					}
				}
				else
				{
					d->memory.maximum_size_in_pages = DWAC_MAX_NOF_PAGES;
				}

				D("Memory: nof pages %u, page_size %u, total in bytes %zu\n", d->memory.current_size_in_pages, DWAC_PAGE_SIZE, (size_t) wa_get_mem_size(d));
				break;
			}
			case 6: // [1] 5.5.9. Global Section
			{
				// See also Import(2) section where globals can be added by finding their initial value in a DLL/SO.
				// Here the value is found by running some code.
				assert(d->globals.size == 0);
				uint32_t nof_globals = leb_read(&d->pc, 32);
				if (nof_globals > max_nof) {return DWAC_TO_MANY_GLOBALS;}
				dwac_linear_storage_64_grow_if_needed(&d->globals, nof_globals);
				for (uint32_t i = 0; i < nof_globals; i++)
				{
					// Same allocation Import of global above

					// [1] 5.3.10. Global Types
					//     Global types are encoded by their value type and a flag for their mutability.
					uint32_t globaltype = leb_read(&d->pc, 32); // Or should it be leb_read_signed 33 bit?
					/*int mutable =*/leb_read(&d->pc, 1);

					// Run the init_expr to get global value, it will get pushed to stack.
					long r = run_init_expr(p, d, globaltype, section_len);
					if (r != DWAC_OK)	{return r;}

					char tmp[64];
					dwac_value_and_type_to_string(tmp, sizeof(tmp), &TOP(d), globaltype);
					D("Global %u 0x%x %s\n", i, globaltype, tmp);

					// Now take the value from stack.
					int64_t v = POP_I64(d);
					dwac_linear_storage_64_push(&d->globals, v);
				}
				d->pc.pos = section_begin + section_len;
				break;
			}
			case 7: // Export Section
			case 8: // Start Section
				// Taken care of by prog so skip these now.
				d->pc.pos += section_len;
				break;
			case 9: // [1] 5.5.12. Element Section
			{
				#ifdef PARSE_ELEMENTS_IN_DATA
				// The initial contents of a table is uninitialized. Element segments can be
				// used to initialize a subrange of a table from a static vector of elements.
				uint32_t nof_elements = leb_read(&d->pc, 32);
				if (nof_globals > max_nof) {return DWAC_TO_MANY_ELEMENTS;}
				for (uint32_t i = 0; i < nof_elements; i++)
				{
					{
						const uint32_t index = leb_read(&d->pc, 32);
						if (index != 0)
						{
							snprintf(d->exception, sizeof(d->exception), "Only one table is supported 0x%x\n", index);
							return DWAC_ONLY_ONE_TABLE_IS_SUPPORTED;
						}
					}

					// Run the init_expr to get offset into table on the stack.
					run_init_expr(p, d, DWAC_I32, section_len);

					size_t offset = POP_I32(d);

					uint32_t nof_entries = leb_read(&d->pc, 32);

					dwac_linear_storage_64_grow_if_needed(&p->func_table, offset + nof_entries);

					for (uint32_t j = 0; j < nof_entries; ++j)
					{
						const uint64_t v = leb_read(&d->pc, 64);
						dwac_linear_storage_64_set(&p->func_table, offset + j, v);
					}
				}
				#else
				d->pc.pos += section_len;
				#endif
				break;
			}
			case 10: // Code Section
				// Taken care of by prog so skip this now.
				d->pc.pos += section_len;
				break;
			case 11: // [1] 5.5.14. Data Section
			{
				uint32_t nof_data_segments = leb_read(&d->pc, 32);
				if (nof_data_segments > max_nof) {return DWAC_TO_MANY_DATA_SEGMENTS;}
				for (uint32_t s = 0; s < nof_data_segments; s++)
				{
					uint32_t mem = leb_read(&d->pc, 32);
					if (mem != 0)
					{
						// In the current version of WebAssembly, at most one memory may be defined or imported
						// in a single module, so all valid active data segments have a memory value of 0.
						snprintf(d->exception, sizeof(d->exception), "Only 1 memory is supported");
						return DWAC_ONLY_ONE_MEMORY_IS_SUPPORTED;
					}

					// Run the init_expr to get the offset onto stack.
					run_init_expr(p, d, DWAC_I32, section_len);
					uint32_t offset = POP_U32(d);

					uint32_t size = leb_read(&d->pc, 32);

					if (offset + size > wa_get_mem_size(d))
					{
						snprintf(d->exception, sizeof(d->exception), "Memory of of range 0x%x 0x%x 0x%x 0x%x\n", offset, size, DWAC_PAGE_SIZE,
								wa_get_mem_size(d));
						return DWAC_MEMORY_OUT_OF_RANGE;
					}

					// Copy the data.
					uint8_t *ptr = translate_addr_grow_if_needed(d, offset, size);
					memcpy(ptr, d->pc.array + d->pc.pos, size);
					d->pc.pos += size;
				}
				break;
			}
			case 12: // Data Count Section
				d->pc.pos += section_len;
				break;

			default:
				snprintf(d->exception, sizeof(d->exception), "Section %d unimplemented\n", id);
				return DWAC_UNKNOWN_SECTION;

				d->pc.pos += section_len;
		}
		if (d->pc.pos != (section_begin + section_len))
		{
			snprintf(d->exception, sizeof(d->exception), "Data section did not add up.\n");
			return DWAC_MISALLIGNED_SECTION;
		}
	}

	if (p->bytecodes.errors)
	{
		snprintf(d->exception, sizeof(d->exception), "LEB128 decoding failed\n");
		return DWAC_LEB_DECODE_FAILED;
	}

	// Ref [1]: Chapter 2.5.9. Start Function
	// The start component of a module declares the function index of a start function that
	// is automatically invoked when the module is instantiated, after tables and memories have
	// been initialized.
	if (p->start_function_idx != INVALID_FUNCTION_INDEX)
	{
		if (p->start_function_idx < p->funcs_vector.nof_imported)
		{
			snprintf(d->exception, sizeof(d->exception), "Can't setup imported function as start function %d\n", p->start_function_idx);
			return DWAC_IMPORTED_FUNC_AS_START;
		}
		else
		{
			dwac_setup_function_call(p, d, p->start_function_idx);
		}
	}

	return DWAC_OK;
}

void dwac_push_value_i64(dwac_data *d, int64_t v)
{
	PUSH_I64(d, v);
}

int32_t dwac_pop_value_i64(dwac_data *d)
{
	return POP_I64(d);
}


// little endian
static void put32(uint8_t *ptr, uint32_t v)
{
	ptr[0] = v;
	ptr[1] = v >> 8;
	ptr[2] = v >> 16;
	ptr[3] = v >> 24;
}

uint32_t wa_get_command_line_arguments_size(uint32_t argc, const char **argv)
{
	uint32_t tot_arg_size = 0x10 + argc * 4;
	for (int i = 0; i < argc; ++i)
	{
		tot_arg_size += strlen(argv[i]) + 1;
	}
	return tot_arg_size;
}

long dwac_set_command_line_arguments(dwac_data *d, uint32_t argc, const char **argv)
{
	int arg_size_in_bytes = wa_get_command_line_arguments_size(argc, argv);
	if (arg_size_in_bytes >= (0x100000000LL - DWAC_ARGUMENTS_BASE)) {return DWAC_TO_MUCH_ARGUMENTS;}
	dwac_linear_storage_8_grow_if_needed(&d->memory.arguments, arg_size_in_bytes);
	assert(d->memory.arguments.size >= wa_get_command_line_arguments_size(argc, argv));

	const uint32_t memory_reserved_by_compiler = DWAC_ARGUMENTS_BASE;

	uint32_t arg_pos = memory_reserved_by_compiler + (4 * argc);

	// If main have argument such as in "int main (int argc, char** args)"
	// Then we here provide some input, zero arguments and a null pointer.
	// Will place command line arguments in topmost memory.
	dwac_push_value_i64(d, argc);
	dwac_push_value_i64(d, memory_reserved_by_compiler); // Pointer to the array with pointers.

	for (int i = 0; i < argc; ++i)
	{
		put32(d->memory.arguments.array + (4 * i), arg_pos);
		const uint32_t n = strlen(argv[i]);
		uint8_t *ptr = translate_addr_grow_if_needed(d, arg_pos, n);
		memcpy(ptr, argv[i], n);
		arg_pos += n + 1;
	}
	return DWAC_OK;
}

long dwac_call_exported_function(const dwac_prog *p, dwac_data *d, uint32_t func_idx)
{
	long r = dwac_setup_function_call(p, d, func_idx);
	if (r) {return r;}
	return dwac_tick(p, d);
}

// Environment shall call this to register all available functions.
// That is functions the WebAsm program can import.
void dwac_register_function(dwac_prog *p, const char *name, dwac_func_ptr ptr)
{
	dwac_hash_list_put(&p->available_functions_list, name, ptr);
}

long long dwac_total_memory_usage(dwac_data *d)
{
	return d->memory.lower_mem.capacity +
	(d->memory.upper_mem.end - d->memory.upper_mem.begin) +
	d->memory.arguments.capacity +
	(d->globals.capacity * 8) +
	(d->block_stack.capacity * sizeof(dwac_block_stack_entry)) +
	DWAC_STACK_SIZE * 8 +
	d->pc.nof;
}

long dwac_report_result(const dwac_prog *p, dwac_data *d, const dwac_function *f, FILE* log)
{
	D("report_result\n");
	assert(log);
	long ret_val = 0;
	// If the called function had a return value it should be on the stack.
	// Log the values on stack.
	fprintf(log, "Stack: %u\n", d->sp + DWAC_SP_OFFSET);
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

#ifdef LOG_FUNC_NAMES
void dwac_log_block_stack(const dwac_prog *p, dwac_data *d)
{
	printf("call stack:\n");
	for(;;)
	{
		dwac_block_stack_entry *e = dwac_linear_storage_size_pop(&d->block_stack);
		if (e == NULL) {break;}
		switch(e->block_type_code)
		{
			case dwac_block_type_internal_func:
			case dwac_block_type_imported_func:
			{
				printf("%4d %s\n", e->func_info.func_idx, dwac_get_func_name(p, e->func_info.func_idx));
				break;
			}
			default:
				break;
		}
	}
	if (p->func_names.size == 0)
	{
		printf("Recompile guest app with '-g' option for call stack with names:\n");
	}
}
#else
void dwac_log_block_stack(const dwac_prog *p, dwac_data *d)
{
	printf("Hint: Recompile dwac/dwae with LOG_FUNC_NAMES macro to display call stack.\n");
}
#endif


void dwac_prog_deinit(dwac_prog *p)
{
	D("dwac_prog_deinit\n");

	#ifdef LOG_FUNC_NAMES
	dwac_linear_storage_size_deinit(&p->func_names);
	#endif

	dwac_linear_storage_size_deinit(&p->function_types_vector);

	for (uint32_t i = 0; i < p->funcs_vector.nof_imported; i++)
	{
		dwac_function *f = &p->funcs_vector.functions_array[i];
		assert(f->block_type_code == dwac_block_type_imported_func);
	}
	for (uint32_t i = p->funcs_vector.nof_imported; i < p->funcs_vector.total_nof; i++)
	{
		dwac_function *f = &p->funcs_vector.functions_array[i];
		assert(f->block_type_code == dwac_block_type_internal_func);
	}
	DWAC_ST_FREE(p->funcs_vector.functions_array);

	dwac_hash_list_deinit(&p->exported_functions_list);

	#ifdef PROG_GLOBAL_SUPPORT
	DWAC_ST_FREE(p->globals.array_of_func_types);
	#endif
	dwac_hash_list_deinit(&p->available_functions_list);
	dwac_linear_storage_64_deinit(&p->func_table);
}

/*static void* copy_alloc(const void* optr)
 {
 size_t s = st_size(optr);
 void* nptr = ST_MALLOC(s);
 memcpy(nptr, optr, s);
 return nptr;
 }*/

/*WaData * wa_data_deep_copy_alloc(WaData *o)
 {
 WaData *n = ST_MALLOC(sizeof(WaData));
 memcpy(n, o, sizeof(WaData));

 n->globals.ptr = copy_alloc(o->globals.ptr);
 n->table.entries = copy_alloc(o->table.entries);
 return n;
 }

 void wa_data_serialize(DbfSerializer *s, const WaData *d)
 {
 // TODO: Not really correct to take entire struct as a blob since struct padding is not standardized.
 // This is more of a place holder.
 DbfSerializerArray(s, d->globals.ptr, d->table.nof, sizeof(WaStackValue));
 DbfSerializerArray64(s, d->table.entries, d->table.size);
 DbfSerializerBytes(s, s, sizeof(WaData));
 }

 void wa_data_deserialize(DbfUnserializer *u, const WaData *d)
 {
 // TODO
 DbfUnserializerBytes(u, d->globals.ptr, st_size(d->globals.ptr));
 DbfUnserializerBytes(u, d->table.entries, s);
 DbfUnserializerBytes(u, s, sizeof(WaData));
 }
 */

void dwac_data_init(dwac_data *d)
{
	D("dwac_data_init\n");
	memset(d, 0, sizeof(dwac_data));

	D("sizeof(wa_value_type) %zu\n", sizeof(dwac_value_type));

	// Put some magic number in the far end of stack.
	// For performance reasons we don't check for stack overflow at every push or pop.
	// This way we can get some indication if a stack overflow happens.
	d->stack[DWAC_STACK_SIZE - 1].s64 = WA_MAGIC_STACK_VALUE;

	dwac_linear_storage_64_init(&d->globals);
	dwac_linear_storage_8_init(&d->memory.lower_mem);
	dwac_virtual_storage_init(&d->memory.upper_mem);
	dwac_linear_storage_size_init(&d->block_stack, sizeof(dwac_block_stack_entry));

	d->memory.arguments.size = 0;
	d->memory.arguments.array = NULL;

	// Initialize stack pointers.
	d->sp = DWAC_SP_INITIAL; // Not zero but -1 here (CPU optimize from ref [3]).
	d->fp = STACK_SIZE(d);
	d->block_stack.size = 0;
	D("wa_data_init 0x%x 0x%x 0x%llx\n", d->fp, d->sp, (long long)d->pc.pos);

}

void dwac_data_deinit(dwac_data *d, FILE* log)
{
	D("dwac_data_deinit\n");
	assert(d->exception[sizeof(d->exception)-1]==0);

	if (log) {
		fprintf(log, "Memory usage: %zu + %zu + %zu  +  %zu + %zu + %u + %zu\n",
			d->memory.lower_mem.capacity,
			d->memory.upper_mem.end - d->memory.upper_mem.begin,
			d->memory.arguments.capacity,
			d->globals.capacity * 8,
			d->block_stack.capacity * sizeof(dwac_block_stack_entry),
			DWAC_STACK_SIZE * 8,
			d->pc.nof);}

	dwac_linear_storage_64_deinit(&d->globals);

	dwac_linear_storage_8_deinit(&d->memory.arguments);
	dwac_linear_storage_size_deinit(&d->block_stack);
	dwac_linear_storage_8_deinit(&d->memory.lower_mem);
	dwac_virtual_storage_deinit(&d->memory.upper_mem);

	memset(d, 0, sizeof(dwac_data));
}

void dwac_prog_init(dwac_prog *p)
{
	D("dwac_prog_init\n");
	memset(p, 0, sizeof(dwac_prog));
	dwac_hash_list_init(&p->available_functions_list);
	dwac_hash_list_init(&p->exported_functions_list);
	dwac_linear_storage_64_init(&p->func_table);
	dwac_linear_storage_size_init(&p->function_types_vector, sizeof(dwac_func_type_type));

	#ifdef LOG_FUNC_NAMES
	dwac_linear_storage_size_init(&p->func_names, DWAC_HASH_LIST_MAX_KEY_SIZE+1);
	#endif

}
