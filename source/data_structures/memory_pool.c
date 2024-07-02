
#include "memory_pool.h"

void MemoryPool_Initialize(MemoryPool *memoryPool, unsigned int memoryChunkSize, unsigned int initialMaxMemoryChunks)
{
    memoryPool->memoryChunkSize = memoryChunkSize;
    memoryPool->amountOfMemoryChunks = 0;
    memoryPool->maxAmountOfMemoryChunks = initialMaxMemoryChunks;

    memoryPool->memoryPool = calloc(initialMaxMemoryChunks, memoryChunkSize);
}

void MemoryPool_Destroy(MemoryPool *memoryPool)
{
    while (memoryPool->amountOfMemoryChunks > 0)
    {
        memoryPool->amountOfMemoryChunks--;
        free(memoryPool->memoryPool[memoryPool->amountOfMemoryChunks]);
    }
    free(memoryPool->memoryPool);
}

short MemoryPool_StoreMemory(MemoryPool *memoryPool, void *memoryPointer)
{
    if (memoryPool->amountOfMemoryChunks >= memoryPool->maxAmountOfMemoryChunks)
    {
        return FALSE;
    }

    memoryPool->memoryPool[memoryPool->amountOfMemoryChunks] = memoryPointer;
    memoryPool->amountOfMemoryChunks++;

    return TRUE;
}

short MemoryPool_TakeMemory(MemoryPool *memoryPool, void **passbackPointer)
{
    if (memoryPool->amountOfMemoryChunks == 0)
    {
        return FALSE;
    }

    memoryPool->amountOfMemoryChunks--;
    (*passbackPointer) = memoryPool->memoryPool[memoryPool->amountOfMemoryChunks];

    return TRUE;
}

void MemoryPool_ResizePool(MemoryPool *memoryPool, unsigned int newMaxMemoryChunks)
{
    while (newMaxMemoryChunks < memoryPool->amountOfMemoryChunks)
    {
        memoryPool->amountOfMemoryChunks--;
        free(memoryPool->memoryPool[memoryPool->amountOfMemoryChunks]);
    }

    memoryPool->memoryPool = realloc(memoryPool->memoryPool, newMaxMemoryChunks * memoryPool->memoryChunkSize);
    memoryPool->maxAmountOfMemoryChunks = newMaxMemoryChunks;
}
