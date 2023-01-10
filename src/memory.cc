#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"

#if defined(__WII__)
#include <ogc/machine/processor.h>
#include <ogc/mutex.h>
#include <ogc/system.h>
#include <ogcsys.h>

#include "color.h"
#include "svga.h"
#include "text_font.h"
#include "window.h"
#include "window_manager.h"

#endif

namespace fallout {

// A special value that denotes a beginning of a memory block data.
#define MEMORY_BLOCK_HEADER_GUARD (0xFEEDFACE)

// A special value that denotes an ending of a memory block data.
#define MEMORY_BLOCK_FOOTER_GUARD (0xBEEFCAFE)

// A header of a memory block.
typedef struct MemoryBlockHeader {
    // Size of the memory block including header and footer.
    size_t size;

    // See [MEMORY_BLOCK_HEADER_GUARD].
    int guard;

#if defined(__WII__)
    // File name where the memory block was allocated.
    char* file;

    // Line number where the memory block was allocated.
    int line;
#endif
} MemoryBlockHeader;

#if defined(__WII__)
typedef struct FileMemoryStats {
    char* file;
    size_t size;
} FileMemoryStats;

static FileMemoryStats gFileMemoryStats[256];
#endif

// A footer of a memory block.
typedef struct MemoryBlockFooter {
    // See [MEMORY_BLOCK_FOOTER_GUARD].
    int guard;
} MemoryBlockFooter;

#if defined(__WII__)
u8* MEM2_start = (u8*)0x90200000;
u8* MEM2_end = (u8*)0x93200000;

// strategy: if there is enough space in MEM2, allocate there, otherwise allocate in MEM1
#define ALIGN(x, a) (((x) + ((a)-1)) & ~((a)-1))

bool gAddressOccupied[0x3000000 / 32];

void* MEM2_init(void* start, void* end, u32 size)
{
    u32* p = (u32*)ALIGN((u32)start, 32);
    u32* startp = p;
    u32* endp = (u32*)ALIGN((u32)end, 32);
    u32* endp2 = (u32*)((u32)endp - size);

    while (p < endp2) {
        *p++ = 0;
        gAddressOccupied[(u32)p / 32] = false;
    }

    return (void*)startp;
}

void* MEM2_alloc(u32 size)
{
    static void* p = NULL;

    if (p == NULL) {
        p = MEM2_init(MEM2_start, MEM2_end, 48 * 1024 * 1024);
    }

    u32* startp = (u32*)ALIGN((u32)p, 32);

    while (startp < (u32*)MEM2_end) {
        if (gAddressOccupied[(u32)startp / 32] == false) {
            // check if there is enough space
            u32* endp = (u32*)((u32)startp + size);
            u32* endp2 = (u32*)ALIGN((u32)endp, 32);
            bool enoughSpace = true;
            for (u32* p = startp; p < endp2; p++) {
                if (gAddressOccupied[(u32)p / 32] == true) {
                    enoughSpace = false;
                    break;
                }
            }
            if (enoughSpace) {
                // mark as occupied
                for (u32* p = startp; p < endp2; p++) {
                    gAddressOccupied[(u32)p / 32] = true;
                }
                p = (void*)endp2;
                return (void*)startp;
            }
        }
        startp += 32;
    }

    return (void*)malloc(size);
}

void MEM2_free(void* ptr)
{
    // if ptr is not in MEM2, just free it
    if (ptr < MEM2_start) {
        free(ptr);
        return;
    }

    u32* startp = (u32*)ALIGN((u32)ptr, 32);
    u32* endp = (u32*)((u32)startp + 32);
    u32* endp2 = (u32*)ALIGN((u32)endp, 32);
    for (u32* p = startp; p < endp2; p++) {
        gAddressOccupied[(u32)p / 32] = false;
    }
}

void* MEM2_realloc(void* ptr, size_t size)
{
    void* newPtr = MEM2_alloc(size);
    if (newPtr == NULL) {
        return NULL;
    }
    memcpy(newPtr, ptr, size);
    MEM2_free(ptr);
    return newPtr;
}
#endif

#if !defined(__WII__)
static void* memoryBlockMallocImpl(size_t size);
#else
static void* memoryBlockMallocImpl(size_t size, const char* file, int line);
#endif
static void* memoryBlockReallocImpl(void* ptr, size_t size);
static void memoryBlockFreeImpl(void* ptr);
static void memoryBlockPrintStats();
#if !defined(__WII__)
static void* mem_prep_block(void* block, size_t size);
#else
static void* mem_prep_block(void* block, size_t size, const char* file, int line);
#endif
static void memoryBlockValidate(void* block);

// 0x51DED0
#if !defined(__WII__)
static MallocProc* gMallocProc = memoryBlockMallocImpl;
#else
static void* (*gMallocProc)(size_t size, const char* file, int line) = memoryBlockMallocImpl;
#endif

// 0x51DED4
static ReallocProc* gReallocProc = memoryBlockReallocImpl;

// 0x51DED8
static FreeProc* gFreeProc = memoryBlockFreeImpl;

// 0x51DEDC
static int gMemoryBlocksCurrentCount = 0;

// 0x51DEE0
static int gMemoryBlockMaximumCount = 0;

// 0x51DEE4
static size_t gMemoryBlocksCurrentSize = 0;

// 0x51DEE8
static size_t gMemoryBlocksMaximumSize = 0;

// 0x4C5A80
char* internal_strdup(const char* string)
{
    char* copy = NULL;
    if (string != NULL) {
        copy = (char*)internal_malloc(strlen(string) + 1);
        strcpy(copy, string);
    }
    return copy;
}

#if !defined(__WII__)
// 0x4C5AD0
void* internal_malloc(size_t size)
{
    return gMallocProc(size);
}
#else
void* internal_malloc_managed(size_t size, const char* file, int line)
{
    return gMallocProc(size, file, line);
}
#endif

// 0x4C5AD8
#if !defined(__WII__)
static void* memoryBlockMallocImpl(size_t size)
#else
static void* memoryBlockMallocImpl(size_t size, const char* file, int line)
#endif
{
    void* ptr = NULL;

    if (size != 0) {
        size += sizeof(MemoryBlockHeader) + sizeof(MemoryBlockFooter);
        size += sizeof(int) - size % sizeof(int);
// #if !defined(__WII__)
#if 1
        unsigned char* block = (unsigned char*)malloc(size);
#else
        unsigned char* block = (unsigned char*)MEM2_alloc(size);
#endif
        if (block != NULL) {
            // NOTE: Uninline.
#if defined(__WII__)
            ptr = mem_prep_block(block, size, file, line);
#else
            ptr = mem_prep_block(block, size);
#endif

            gMemoryBlocksCurrentCount++;
            if (gMemoryBlocksCurrentCount > gMemoryBlockMaximumCount) {
                gMemoryBlockMaximumCount = gMemoryBlocksCurrentCount;
            }

            gMemoryBlocksCurrentSize += size;
            if (gMemoryBlocksCurrentSize > gMemoryBlocksMaximumSize) {
                gMemoryBlocksMaximumSize = gMemoryBlocksCurrentSize;
            }
        }
    }

    return ptr;
}

// 0x4C5B50
void* internal_realloc(void* ptr, size_t size)
{
    return gReallocProc(ptr, size);
}

// 0x4C5B58
static void* memoryBlockReallocImpl(void* ptr, size_t size)
{
    if (ptr != NULL) {
        unsigned char* block = (unsigned char*)ptr - sizeof(MemoryBlockHeader);

        MemoryBlockHeader* header = (MemoryBlockHeader*)block;
        size_t oldSize = header->size;
#if defined(__WII__)
        for (int i = 0; i < 128; i++) {
            if (gFileMemoryStats[i].file == header->file) {
                gFileMemoryStats[i].size -= oldSize;
                break;
            }
        }
#endif

        gMemoryBlocksCurrentSize -= oldSize;

        memoryBlockValidate(block);

        if (size != 0) {
            size += sizeof(MemoryBlockHeader) + sizeof(MemoryBlockFooter);
            size += sizeof(int) - size % sizeof(int);
        }
// #if !defined(__WII__)
#if 1
        unsigned char* newBlock = (unsigned char*)realloc(block, size);
#else
        unsigned char* newBlock = (unsigned char*)MEM2_realloc(block, size);
#endif
        if (newBlock != NULL) {
            gMemoryBlocksCurrentSize += size;
            if (gMemoryBlocksCurrentSize > gMemoryBlocksMaximumSize) {
                gMemoryBlocksMaximumSize = gMemoryBlocksCurrentSize;
            }

            // NOTE: Uninline.
#if defined(__WII__)
            ptr = mem_prep_block(newBlock, size, __FILE__, __LINE__);
#else
            ptr = mem_prep_block(newBlock, size);
#endif
        } else {
            if (size != 0) {
                gMemoryBlocksCurrentSize += oldSize;

                debugPrint("%s,%u: ", __FILE__, __LINE__); // "Memory.c", 195
                debugPrint("Realloc failure.\n");
            } else {
                gMemoryBlocksCurrentCount--;
            }
            ptr = NULL;
        }
    } else {
#if defined(__WII_)
        ptr = gMallocProc(size, __FILE__, __LINE__);
#else
        ptr = gMallocProc(size);
#endif
    }

    return ptr;
}

// 0x4C5C24
void internal_free(void* ptr)
{
    gFreeProc(ptr);
}

// 0x4C5C2C
static void memoryBlockFreeImpl(void* ptr)
{
    if (ptr != NULL) {
        void* block = (unsigned char*)ptr - sizeof(MemoryBlockHeader);
        MemoryBlockHeader* header = (MemoryBlockHeader*)block;

        memoryBlockValidate(block);
#if defined(__WII__)
        for (int i = 0; i < 128; i++) {
            if (gFileMemoryStats[i].file == header->file) {
                gFileMemoryStats[i].size -= header->size;
                break;
            }
        }
#endif

        gMemoryBlocksCurrentSize -= header->size;
        gMemoryBlocksCurrentCount--;
// #if !defined(__WII__)
#if 1
        free(block);
#else
        MEM2_free(block);
#endif
    }
}

// NOTE: Not used.
//
// 0x4C5C5C
static void memoryBlockPrintStats()
{
    if (gMallocProc == memoryBlockMallocImpl) {
        debugPrint("Current memory allocated: %6d blocks, %9u bytes total\n", gMemoryBlocksCurrentCount, gMemoryBlocksCurrentSize);
        debugPrint("Max memory allocated:     %6d blocks, %9u bytes total\n", gMemoryBlockMaximumCount, gMemoryBlocksMaximumSize);
    }
}

#if defined(__WII__)
void print_memory_stats()
{
#if 0
    // show stats on screen (pos: 0, 440, size: 640x20)

    char buf[256];
    sprintf(buf, "MEM: %6d blocks, %9u bytes total\n", gMemoryBlocksCurrentCount, gMemoryBlocksCurrentSize);

    int x = 0;
    int y = 440;
    int w = 640;
    int h = 20;

    u8 pix[640 * 20];
    memset(pix, 0, sizeof(pix));

    fontDrawText(pix, buf, 640, 640, _colorTable[32767]);

    _scr_blit(pix, 640, 20, 0, 0, w, h, x, y);

    // printf("%s\n", buf);

    // sprintf(buf, "MEM_MAX: %6d blocks, %9u bytes total\n", gMemoryBlockMaximumCount, gMemoryBlocksMaximumSize);

    // fontDrawText(pix, buf, 640, 640, _colorTable[32767]);

    // _scr_blit(pix, 640, 480, 0, 0, w, h, x, y + 20);

    printf("%s\n", buf);

    // print file memory stats

    char* fileWithMostMemory = NULL;
    int mostMemory = 0;
    for (int i = 0; i < 128; i++) {
        if (gFileMemoryStats[i].file != NULL && gFileMemoryStats[i].size != 0) {
            sprintf(buf, "%s: %9u bytes\n", gFileMemoryStats[i].file, gFileMemoryStats[i].size);
            printf("%s\n", buf);

            if (gFileMemoryStats[i].size > mostMemory) {
                mostMemory = gFileMemoryStats[i].size;
                fileWithMostMemory = gFileMemoryStats[i].file;
            }
        }
    }

    if (fileWithMostMemory != NULL) {
        sprintf(buf, "MOST MALLOCS: %s (%u bytes)\n", fileWithMostMemory, mostMemory);
        fontDrawText(pix, buf, 640, 640, _colorTable[32767]);
        _scr_blit(pix, 640, 20, 0, 0, w, h, x, y + 20);
    }
#endif
}
#endif

// NOTE: Inlined.
//
// 0x4C5CC4
#if !defined(__WII__)
static void* mem_prep_block(void* block, size_t size)
#else
static void* mem_prep_block(void* block, size_t size, const char* file, int line)
#endif
{
    MemoryBlockHeader* header;
    MemoryBlockFooter* footer;

    header = (MemoryBlockHeader*)block;
    header->guard = MEMORY_BLOCK_HEADER_GUARD;
    header->size = size;

#if defined(__WII__)

    bool fileMemoryInfoInitialized = false;
    for (int i = 0; i < 128; i++) {
        if (strcmp(gFileMemoryStats[i].file, file) == 0) {
            gFileMemoryStats[i].size += size;
            fileMemoryInfoInitialized = true;
            header->file = gFileMemoryStats[i].file;
            header->line = line;
            break;
        }
    }
    if (!fileMemoryInfoInitialized) {
        for (int i = 0; i < 128; i++) {
            if (gFileMemoryStats[i].file == NULL) {
                gFileMemoryStats[i].file = strdup(file);
                gFileMemoryStats[i].size = size;
                header->file = gFileMemoryStats[i].file;
                header->line = line;
                break;
            }
        }
    }
#endif

    footer = (MemoryBlockFooter*)((unsigned char*)block + size - sizeof(*footer));
    footer->guard = MEMORY_BLOCK_FOOTER_GUARD;

    return (unsigned char*)block + sizeof(*header);
}

// Validates integrity of the memory block.
//
// [block] is a pointer to the the memory block itself, not it's data.
//
// 0x4C5CE4
static void memoryBlockValidate(void* block)
{
    MemoryBlockHeader* header = (MemoryBlockHeader*)block;
    if (header->guard != MEMORY_BLOCK_HEADER_GUARD) {
        debugPrint("Memory header stomped.\n");
    }

    MemoryBlockFooter* footer = (MemoryBlockFooter*)((unsigned char*)block + header->size - sizeof(MemoryBlockFooter));
    if (footer->guard != MEMORY_BLOCK_FOOTER_GUARD) {
        debugPrint("Memory footer stomped.\n");
    }
}

} // namespace fallout
