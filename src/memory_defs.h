#ifndef MEMORY_DEFS_H
#define MEMORY_DEFS_H

#include <stddef.h>

namespace fallout {

#if !defined(__WII__)
typedef void*(MallocProc)(size_t size);
#else
typedef void*(MallocProc)(size_t size, const char* file, int line);
typedef void*(MallocUntrackedProc)(size_t size);
#endif
typedef void*(ReallocProc)(void* ptr, size_t newSize);
typedef void(FreeProc)(void* ptr);

} // namespace fallout

#endif /* MEMORY_DEFS_H */
