#include "game_memory.h"

#include "db.h"
#include "dictionary.h"
#include "memory.h"
#include "memory_defs.h"
#include "memory_manager.h"

namespace fallout {

#if !defined(__WII__)
static void* gameMemoryMalloc(size_t size);
#else
static void* gameMemoryMalloc(size_t size, const char* file, int line);
#endif
static void* gameMemoryRealloc(void* ptr, size_t newSize);
static void gameMemoryFree(void* ptr);

// 0x44B250
int gameMemoryInit()
{
#if !defined(__WII__)
    dictionarySetMemoryProcs(internal_malloc, internal_realloc, internal_free);
#else
    dictionarySetMemoryProcs(internal_malloc_managed, internal_realloc, internal_free);
#endif
    memoryManagerSetProcs(gameMemoryMalloc, gameMemoryRealloc, gameMemoryFree);

    return 0;
}

// 0x44B294
#if !defined(__WII__)
static void* gameMemoryMalloc(size_t size)
#else
static void* gameMemoryMalloc(size_t size, const char* file, int line)
#endif
{
#if !defined(__WII__)
    return internal_malloc(size);
#else
    return internal_malloc_managed(size, file, line);
#endif
}

// 0x44B29C
static void* gameMemoryRealloc(void* ptr, size_t newSize)
{
    return internal_realloc(ptr, newSize);
}

// 0x44B2A4
static void gameMemoryFree(void* ptr)
{
    internal_free(ptr);
}

} // namespace fallout
