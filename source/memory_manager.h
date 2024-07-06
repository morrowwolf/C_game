#pragma once
#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include "globals.h"
#include "data_structures/memory_pool.h"
#include "tasks.h"

#define MEMORY_MANAGER_FLAG_NONE (1 << 0)
#define MEMORY_MANAGER_FLAG_NO_STORE (1 << 1)

typedef struct MemoryManager
{
    CRITICAL_SECTION memorySizeInfosCriticalSection;
    List memorySizeInfos; // List of MemorySizeInformation

    unsigned long long lastCleanupTick;

#define MEMORY_MANAGER_CLEANUP_INTERVAL SECOND_IN_HUNDREDNANOSECONDS
    HANDLE memoryCleanupTimer;
    LARGE_INTEGER nextMemoryCleanupTime;
} MemoryManager;

extern MemoryManager *MEMORY_MANAGER;

typedef struct MemorySizeInfo
{
    MemoryPool memoryPool; // Pool of void* pointers to unused memory
    unsigned int amountOfUsedMemoryChunks;

    unsigned int allocatedCount;
    unsigned int deallocatedCount;
    unsigned int failedToStoreCount;

    unsigned long flags;
} MemorySizeInformation;

void MemoryManager_Initialize();
void MemoryManager_Destroy();

void MemoryManager_AllocateMemory(void **passbackPointer, unsigned int size, unsigned long flags);
void MemoryManager_DeallocateMemory(void **memoryPointer, unsigned int size);

void MemoryManager_Cleanup();

void List_MemorySizeInfosOnRemove(void *data);

void List_DeallocatePointOnRemove(Point *data);

#endif
