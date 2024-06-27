#pragma once
#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#define UNICODE
#define _UNICODE

#include <windows.h>
#include "data_structures/list.h"
#include "data_structures/read_write_lock.h"

#define HEAP_ALLOC_OPTIONS (HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY)

typedef struct MemoryManager
{
    HANDLE heap;
    CRITICAL_SECTION memorySizeInfosCriticalSection;
    List memorySizeInfos; // List of MemorySizeInformation
} MemoryManager;

extern MemoryManager *MEMORY_MANAGER;

typedef struct MemorySizeInfo
{
    unsigned int memorySize;

    List storedUnusedMemoryChunks; // Stack of void* pointers to unused memory
    unsigned int amountOfUsedMemoryChunks;
} MemorySizeInformation;

void MemoryManager_Initialize();
void MemoryManager_Destroy();

void MemoryManager_AllocateMemory(void **passbackPointer, unsigned int size);
void MemoryManager_DeallocateMemory(void **memoryPointer, unsigned int size);

void List_MemorySizeInfosOnRemove(void *data);

#endif
