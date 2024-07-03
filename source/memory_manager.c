
#include "memory_manager.h"

MemoryManager *MEMORY_MANAGER;

void MemoryManager_Initialize()
{
    MEMORY_MANAGER = calloc(1, sizeof(MemoryManager));

    InitializeCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);

    MEMORY_MANAGER->memoryCleanupTimer = CreateWaitableTimer(NULL, FALSE, NULL);
    LARGE_INTEGER initialTime;
    initialTime.QuadPart = -MEMORY_MANAGER_CLEANUP_INTERVAL;

    SetWaitableTimer(MEMORY_MANAGER->memoryCleanupTimer, &initialTime, 0, NULL, NULL, 0);

    List_Init(&MEMORY_MANAGER->memorySizeInfos, List_MemorySizeInfosOnRemove);

    MemorySizeInformation *memoryManagerSizeInfo = calloc(1, sizeof(MemorySizeInformation));
    MemoryPool_Initialize(&memoryManagerSizeInfo->memoryPool, sizeof(MemoryManager), 1);
    memoryManagerSizeInfo->amountOfUsedMemoryChunks = 1;

    ListElmt *memoryManagerSizeInfoListElement = calloc(1, sizeof(ListElmt));
    memoryManagerSizeInfoListElement->data = memoryManagerSizeInfo;

    List_InsertElementNext(&MEMORY_MANAGER->memorySizeInfos, NULL, memoryManagerSizeInfoListElement);

    MemorySizeInformation *memorySizeInfoSizeInfo = calloc(1, sizeof(MemorySizeInformation));
    MemoryPool_Initialize(&memorySizeInfoSizeInfo->memoryPool, sizeof(MemorySizeInformation), 1);
    memorySizeInfoSizeInfo->amountOfUsedMemoryChunks = 4;

    ListElmt *memorySizeInfoSizeInfoListElement = calloc(1, sizeof(ListElmt));
    memorySizeInfoSizeInfoListElement->data = memorySizeInfoSizeInfo;

    List_InsertElementNext(&MEMORY_MANAGER->memorySizeInfos, MEMORY_MANAGER->memorySizeInfos.tail, memorySizeInfoSizeInfoListElement);

    MemorySizeInformation *listSizeInfo = calloc(1, sizeof(MemorySizeInformation));
    MemoryPool_Initialize(&listSizeInfo->memoryPool, sizeof(List), 1);
    listSizeInfo->amountOfUsedMemoryChunks = 0;

    ListElmt *listSizeInfoListElement = calloc(1, sizeof(ListElmt));
    listSizeInfoListElement->data = listSizeInfo;

    List_InsertElementNext(&MEMORY_MANAGER->memorySizeInfos, MEMORY_MANAGER->memorySizeInfos.tail, listSizeInfoListElement);

    MemorySizeInformation *listElmtSizeInfo = calloc(1, sizeof(MemorySizeInformation));
    MemoryPool_Initialize(&listElmtSizeInfo->memoryPool, sizeof(ListElmt), 1);
    listElmtSizeInfo->amountOfUsedMemoryChunks = 4;

    ListElmt *listElmtSizeInfoListElmt = calloc(1, sizeof(ListElmt));
    listElmtSizeInfoListElmt->data = listElmtSizeInfo;

    List_InsertElementNext(&MEMORY_MANAGER->memorySizeInfos, MEMORY_MANAGER->memorySizeInfos.tail, listElmtSizeInfoListElmt);
}

void MemoryManager_Destroy()
{
    EnterCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);

    while (MEMORY_MANAGER->memorySizeInfos.length > 0)
    {
        List_RemoveElementHardFree(&MEMORY_MANAGER->memorySizeInfos, MEMORY_MANAGER->memorySizeInfos.tail);
    }

    DeleteCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);

    free(MEMORY_MANAGER);
}

void MemoryManager_AllocateMemory(void **passbackPointer, unsigned int size)
{
    EnterCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);

    ListIterator memorySizeInfosIterator;
    ListIterator_Init(&memorySizeInfosIterator, &MEMORY_MANAGER->memorySizeInfos);
    MemorySizeInformation *memorySizeInfo;

    while (ListIterator_Next(&memorySizeInfosIterator, (void **)&memorySizeInfo))
    {
        if (memorySizeInfo->memoryPool.memoryChunkSize != size)
        {
            continue;
        }

        memorySizeInfo->allocatedCount += 1;

        void *memoryChunk = NULL;
        if (MemoryPool_TakeMemory(&memorySizeInfo->memoryPool, &memoryChunk))
        {
            (*passbackPointer) = memoryChunk;
            memorySizeInfo->amountOfUsedMemoryChunks += 1;

            LeaveCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);
            return;
        }

        (*passbackPointer) = calloc(1, size);
        memorySizeInfo->amountOfUsedMemoryChunks += 1;

        LeaveCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);
        return;
    }

    MemorySizeInformation *newMemorySizeInfo;
    MemoryManager_AllocateMemory((void **)&newMemorySizeInfo, sizeof(MemorySizeInformation));
    MemoryPool_Initialize(&newMemorySizeInfo->memoryPool, size, 1);
    newMemorySizeInfo->amountOfUsedMemoryChunks = 1;
    (*passbackPointer) = calloc(1, size);

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
        if (memorySizeInfo->memoryPool.memoryChunkSize != size)
        {
            continue;
        }

        memorySizeInfo->deallocatedCount += 1;

        if (MemoryPool_StoreMemory(&memorySizeInfo->memoryPool, (*memoryPointer)))
        {
            memorySizeInfo->amountOfUsedMemoryChunks -= 1;
            LeaveCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);
            return;
        }

        free((*memoryPointer));
        memorySizeInfo->amountOfUsedMemoryChunks -= 1;
        memorySizeInfo->failedToStoreCount += 1;
        LeaveCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);
        return;
    }

    // There should never be memory deallocated via this that isn't allocated via MemoryManager_AllocateMemory
    abort();
}

void MemoryManager_Cleanup()
{

    if (GAMESTATE->tickProcessing)
    {
        Task *task;
        MemoryManager_AllocateMemory((void **)&task, sizeof(Task));
        task->task = (void (*)(void *))MemoryManager_Cleanup;
        task->taskArgument = NULL;

        Task_QueueTask(&TASKSTATE->garbageTaskQueue, &TASKSTATE->garbageTasksQueuedSyncEvents, task);

        return;
    }

    EnterCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);

    unsigned long long currentTick = GAMESTATE->currentTick;

    if (currentTick <= MEMORY_MANAGER->lastCleanupTick)
    {
        LeaveCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);
        return;
    }

    unsigned long long deltaTicks = currentTick - MEMORY_MANAGER->lastCleanupTick;

    MEMORY_MANAGER->lastCleanupTick = currentTick;

    ListIterator memorySizeInfosIterator;
    ListIterator_Init(&memorySizeInfosIterator, &MEMORY_MANAGER->memorySizeInfos);
    MemorySizeInformation *memorySizeInfo;

    while (ListIterator_Next(&memorySizeInfosIterator, (void **)&memorySizeInfo))
    {
        // unsigned int normalizedAllocatedCount = memorySizeInfo->allocatedCount / deltaTicks;
        // unsigned int normalizedDeallocatedCount = memorySizeInfo->deallocatedCount / deltaTicks;
        unsigned int normalizedFailedToStoreCount = memorySizeInfo->failedToStoreCount / deltaTicks;

        memorySizeInfo->allocatedCount = 0;
        memorySizeInfo->deallocatedCount = 0;
        memorySizeInfo->failedToStoreCount = 0;

        if (normalizedFailedToStoreCount > 0)
        {
            MemoryPool_ResizePool(&memorySizeInfo->memoryPool, memorySizeInfo->memoryPool.maxAmountOfMemoryChunks * 2);
            continue;
        }

        // TODO: Maybe fill the pool if the allocated count is greater than deallocated and the memory pool is empty?
        // We want to avoid the pool being empty and having to allocate memory during a tick
    }

    LeaveCriticalSection(&MEMORY_MANAGER->memorySizeInfosCriticalSection);
}

void List_MemorySizeInfosOnRemove(void *data)
{
    MemorySizeInformation *memorySizeInfo = (MemorySizeInformation *)data;

    MemoryPool_Destroy(&memorySizeInfo->memoryPool);
    free(memorySizeInfo);
}

void List_DeallocatePointOnRemove(Point *data)
{
    MemoryManager_DeallocateMemory((void **)&data, sizeof(Point));
}
