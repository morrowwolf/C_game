
#include "memory_manager.h"

MemoryManager *MEMORY_MANAGER;

void MemoryManager_Initialize()
{
    HANDLE processHeap = GetProcessHeap();

    MEMORY_MANAGER = HeapAlloc(processHeap, HEAP_ALLOC_OPTIONS, sizeof(MemoryManager));
    MEMORY_MANAGER->heap = processHeap;

    InitializeCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);

    List_Init(&MEMORY_MANAGER->memorySizeInfos, List_MemorySizeInfosOnRemove);

    MemorySizeInformation *memoryManagerSizeInfo = HeapAlloc(MEMORY_MANAGER->heap, HEAP_ALLOC_OPTIONS, sizeof(MemorySizeInformation));
    memoryManagerSizeInfo->memorySize = sizeof(MemoryManager);
    List_Init(&memoryManagerSizeInfo->storedUnusedMemoryChunks, NULL);
    memoryManagerSizeInfo->amountOfUsedMemoryChunks = 1;

    ListElmt *memoryManagerSizeInfoListElement = HeapAlloc(MEMORY_MANAGER->heap, HEAP_ALLOC_OPTIONS, sizeof(ListElmt));
    memoryManagerSizeInfoListElement->data = memoryManagerSizeInfo;

    List_InsertElementNext(&MEMORY_MANAGER->memorySizeInfos, NULL, memoryManagerSizeInfoListElement);

    MemorySizeInformation *memorySizeInfoSizeInfo = HeapAlloc(MEMORY_MANAGER->heap, HEAP_ALLOC_OPTIONS, sizeof(MemorySizeInformation));
    memorySizeInfoSizeInfo->memorySize = sizeof(MemorySizeInformation);
    List_Init(&memorySizeInfoSizeInfo->storedUnusedMemoryChunks, NULL);
    memorySizeInfoSizeInfo->amountOfUsedMemoryChunks = 4;

    ListElmt *memorySizeInfoSizeInfoListElement = HeapAlloc(MEMORY_MANAGER->heap, HEAP_ALLOC_OPTIONS, sizeof(ListElmt));
    memorySizeInfoSizeInfoListElement->data = memorySizeInfoSizeInfo;

    List_InsertElementNext(&MEMORY_MANAGER->memorySizeInfos, MEMORY_MANAGER->memorySizeInfos.tail, memorySizeInfoSizeInfoListElement);

    MemorySizeInformation *listSizeInfo = HeapAlloc(MEMORY_MANAGER->heap, HEAP_ALLOC_OPTIONS, sizeof(MemorySizeInformation));
    listSizeInfo->memorySize = sizeof(List);
    List_Init(&listSizeInfo->storedUnusedMemoryChunks, NULL);
    listSizeInfo->amountOfUsedMemoryChunks = 0;

    ListElmt *listSizeInfoListElement = HeapAlloc(MEMORY_MANAGER->heap, HEAP_ALLOC_OPTIONS, sizeof(ListElmt));
    listSizeInfoListElement->data = listSizeInfo;

    List_InsertElementNext(&MEMORY_MANAGER->memorySizeInfos, MEMORY_MANAGER->memorySizeInfos.tail, listSizeInfoListElement);

    MemorySizeInformation *listElmtSizeInfo = HeapAlloc(MEMORY_MANAGER->heap, HEAP_ALLOC_OPTIONS, sizeof(MemorySizeInformation));
    listElmtSizeInfo->memorySize = sizeof(ListElmt);
    List_Init(&listElmtSizeInfo->storedUnusedMemoryChunks, NULL);
    listElmtSizeInfo->amountOfUsedMemoryChunks = 4;

    ListElmt *listElmtSizeInfoListElmt = HeapAlloc(MEMORY_MANAGER->heap, HEAP_ALLOC_OPTIONS, sizeof(ListElmt));
    listElmtSizeInfoListElmt->data = listElmtSizeInfo;

    List_InsertElementNext(&MEMORY_MANAGER->memorySizeInfos, MEMORY_MANAGER->memorySizeInfos.tail, listElmtSizeInfoListElmt);
}

void MemoryManager_Destroy()
{
    EnterCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);

    List_Clear(&MEMORY_MANAGER->memorySizeInfos);

    DeleteCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);

    HeapFree(MEMORY_MANAGER->heap, 0, MEMORY_MANAGER);
}

void MemoryManager_AllocateMemory(void **passbackPointer, unsigned int size)
{
    EnterCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);

    ListIterator memorySizeInfosIterator;
    ListIterator_Init(&memorySizeInfosIterator, &MEMORY_MANAGER->memorySizeInfos);
    MemorySizeInformation *memorySizeInfo;

    while (ListIterator_Next(&memorySizeInfosIterator, (void **)&memorySizeInfo))
    {
        if (memorySizeInfo->memorySize != size)
        {
            continue;
        }

        List *memoryChunks = &memorySizeInfo->storedUnusedMemoryChunks;

        ListElmt *memoryChunkElement = memoryChunks->tail;

        if (memoryChunkElement != NULL)
        {
            (*passbackPointer) = memoryChunkElement->data;
            List_RemoveElement(memoryChunks, memoryChunkElement);

            memorySizeInfo->amountOfUsedMemoryChunks += 1;

            LeaveCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);
            return;
        }

        void *memoryChunk = HeapAlloc(MEMORY_MANAGER->heap, HEAP_ALLOC_OPTIONS, size);

        (*passbackPointer) = memoryChunk;

        memorySizeInfo->amountOfUsedMemoryChunks += 1;

        LeaveCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);
        return;
    }

    MemorySizeInformation *newMemorySizeInfo;
    MemoryManager_AllocateMemory((void **)&newMemorySizeInfo, sizeof(MemorySizeInformation));
    newMemorySizeInfo->memorySize = size;
    List_Init(&newMemorySizeInfo->storedUnusedMemoryChunks, NULL);
    newMemorySizeInfo->amountOfUsedMemoryChunks = 1;
    (*passbackPointer) = HeapAlloc(MEMORY_MANAGER->heap, HEAP_ALLOC_OPTIONS, size);

    List_Insert(&MEMORY_MANAGER->memorySizeInfos, newMemorySizeInfo);

    LeaveCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);
}

void MemoryManager_DeallocateMemory(void **memoryPointer, unsigned int size)
{
    EnterCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);

    ListIterator memorySizeInfosIterator;
    ListIterator_Init(&memorySizeInfosIterator, &MEMORY_MANAGER->memorySizeInfos);
    MemorySizeInformation *memorySizeInfo;

    while (ListIterator_Next(&memorySizeInfosIterator, (void **)&memorySizeInfo))
    {
        if (memorySizeInfo->memorySize != size)
        {
            continue;
        }

        if (memorySizeInfo->storedUnusedMemoryChunks.length >= (memorySizeInfo->amountOfUsedMemoryChunks * 0.5))
        {
            HeapFree(MEMORY_MANAGER->heap, 0, (*memoryPointer));
            memorySizeInfo->amountOfUsedMemoryChunks -= 1;
            LeaveCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);
            return;
        }

        List_Insert(&memorySizeInfo->storedUnusedMemoryChunks, (*memoryPointer));
        memorySizeInfo->amountOfUsedMemoryChunks -= 1;
        LeaveCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);
        return;
    }

    // There should never be memory deallocated via this that isn't allocated via MemoryManager_AllocateMemory
    abort();
}

void List_MemorySizeInfosOnRemove(void *data)
{
    MemorySizeInformation *memorySizeInfo = (MemorySizeInformation *)data;
    ListIterator memoryChunksIterator;
    ListIterator_Init(&memoryChunksIterator, &memorySizeInfo->storedUnusedMemoryChunks);
    void *memoryChunk;

    while (ListIterator_Next(&memoryChunksIterator, (void **)&memoryChunk))
    {
        HeapFree(MEMORY_MANAGER->heap, 0, memoryChunk);
    }

    HeapFree(MEMORY_MANAGER->heap, 0, memorySizeInfo);
}
