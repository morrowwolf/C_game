#pragma once
#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <windows.h>

typedef struct MemoryPool
{
    unsigned int memoryChunkSize;
    unsigned int amountOfMemoryChunks;
    unsigned int maxAmountOfMemoryChunks;

    void **memoryPool;
} MemoryPool;

void MemoryPool_Initialize(MemoryPool *memoryPool, unsigned int memoryChunkSize, unsigned int initialMaxMemoryChunks);
void MemoryPool_Destroy(MemoryPool *memoryPool);

short MemoryPool_StoreMemory(MemoryPool *memoryPool, void *memoryPointer);
short MemoryPool_TakeMemory(MemoryPool *memoryPool, void **passbackPointer);

void MemoryPool_ResizePool(MemoryPool *memoryPool, unsigned int newMaxMemoryChunks);

#endif
