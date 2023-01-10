#ifndef MEMORY_H
#define MEMORY_H

#include "memory_defs.h"

namespace fallout {

char* internal_strdup(const char* string);
#if !defined(__WII__)
void* internal_malloc(size_t size);
#else
void* internal_malloc_managed(size_t size, const char* file, int line);
#define internal_malloc(size) internal_malloc_managed(size, __FILE__, __LINE__)
#endif
void* internal_realloc(void* ptr, size_t size);
void internal_free(void* ptr);

#if defined(__WII__)
void print_memory_stats();
#endif

} // namespace fallout

#endif /* MEMORY_H */
