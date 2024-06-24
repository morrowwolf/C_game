
#include "gamestate.h"

DWORD WINAPI GamestateHandler(LPVOID lpParam)
{
    UNREFERENCED_PARAMETER(lpParam);

    HANDLE hTimer;

    hTimer = CreateWaitableTimer(NULL, TRUE, NULL);

    // We must make a promise that we do not remove anything from
    // the entities global list while this is being iterated in a tick
    // We do this via the tickProcessing variable in the GAMESTATE global variable
    ListIteratorThread *entitiesIteratorThread = malloc(sizeof(ListIteratorThread));
    ListIteratorThread_Init(entitiesIteratorThread, GAMESTATE->entities.protectedData);

    void *arrayOfTasksCompleteSyncEvents;
    List_GetAsArray(&TASKSTATE->gamestateTasksCompleteSyncEvents, (void **)&arrayOfTasksCompleteSyncEvents);
    unsigned int syncEventCount = TASKSTATE->gamestateTasksCompleteSyncEvents.length;

    while (!SCREEN->exiting)
    {
        while (!GAMESTATE->running && !SCREEN->exiting)
        {
            // I think there's a very slight chance we could miss an escape key here
            // but I really don't want to lock running as it could hang the window.
            // Possibly we could give it to task handler so we don't have to worry?
            // The overhead may not be worth it.
            WaitForSingleObject(GAMESTATE->keyEvent, INFINITE);

            GAMESTATE->nextTickTime.QuadPart = 0;
        }

        FILETIME fileTime;
        GetSystemTimeAsFileTime(&fileTime);

        ULARGE_INTEGER startTime;
        startTime.LowPart = fileTime.dwLowDateTime;
        startTime.HighPart = fileTime.dwHighDateTime;

        entitiesIteratorThread->iterationStarted = FALSE;

        List tasksToQueue;
        List_Init(&tasksToQueue, NULL);

        Task *task = malloc(sizeof(Task));
        task->task = (void (*)(void *))Gamestate_FighterSpawn;
        task->taskArgument = NULL;

        List_Insert(&tasksToQueue, task);

        task = malloc(sizeof(Task));
        task->task = (void (*)(void *))Gamestate_AsteroidSpawn;
        task->taskArgument = NULL;

        List_Insert(&tasksToQueue, task);

        for (unsigned __int32 i = 0; i < TASKSTATE->totalTaskThreads; i++)
        {
            task = malloc(sizeof(Task));
            task->task = (void (*)(void *))Gamestate_EntitiesOnTick;
            task->taskArgument = entitiesIteratorThread;

            List_Insert(&tasksToQueue, task);
        }

        List *entities;
        ReadWriteLockPriority_GetPriorityReadPermission(&GAMESTATE->entities, (void **)&entities);

        InterlockedExchange((volatile long *)&GAMESTATE->tickProcessing, TRUE);

        ReadWriteLockPriority_ReleaseReadPermission(&GAMESTATE->entities, (void **)&entities);

        Task_QueueGamestateTasks(&tasksToQueue);
        List_Clear(&tasksToQueue);

        WaitForMultipleObjects(syncEventCount, arrayOfTasksCompleteSyncEvents, TRUE, INFINITE);

        InterlockedExchange((volatile long *)&GAMESTATE->tickProcessing, FALSE);

        GetSystemTimeAsFileTime(&fileTime);

        ULARGE_INTEGER endTime;
        endTime.LowPart = fileTime.dwLowDateTime;
        endTime.HighPart = fileTime.dwHighDateTime;

        if (GAMESTATE->nextTickTime.QuadPart == 0)
        {
            GAMESTATE->nextTickTime.QuadPart = endTime.QuadPart;
        }

        GAMESTATE->lastTickTimeDifference.QuadPart = endTime.QuadPart - startTime.QuadPart;

        GAMESTATE->nextTickTime.QuadPart += DEFAULT_TICK_RATE;

        LARGE_INTEGER nextTickTime;
        nextTickTime.QuadPart = (__int64)GAMESTATE->nextTickTime.QuadPart;

        SetWaitableTimer(hTimer, &nextTickTime, 0, NULL, NULL, 0);

        WaitForSingleObject(hTimer, INFINITE);

        GAMESTATE->tickCount++;
    }

    ListIteratorThread_Destroy(entitiesIteratorThread);
    free(entitiesIteratorThread);

    return 0;
}

void Gamestate_EntitiesOnTick(ListIteratorThread *entitiesListIteratorThread)
{
    Entity *entity;
    if (!ListIteratorThread_Next(entitiesListIteratorThread, (void **)&entity))
    {
        return;
    }

    if (entity->alive == ENTITY_ALIVE)
    {
        ListIterator onTickIterator;
        ListIterator_Init(&onTickIterator, &entity->onTick);
        void (*onTick)(Entity *);
        while (ListIterator_Next(&onTickIterator, (void **)&onTick))
        {
            onTick(entity);
        }
    }

    Task *task = malloc(sizeof(Task));
    task->task = (void (*)(void *))Gamestate_EntitiesOnTick;
    task->taskArgument = entitiesListIteratorThread;

    Task_QueueGamestateTask(task);
}

#define ASTEROID_SPAWN_DELAY 60
void Gamestate_AsteroidSpawn()
{
    static volatile long asteroidSpawnDelayCounter = 0;

    List *asteroids;
    ReadWriteLock_GetReadPermission(&GAMESTATE->asteroids, (void **)&asteroids);

    unsigned int asteroidCount = asteroids->length;

    ReadWriteLock_ReleaseReadPermission(&GAMESTATE->asteroids, (void **)&asteroids);

    if (asteroidCount < MAX_ASTEROIDS && InterlockedExchangeAcquire(&asteroidSpawnDelayCounter, asteroidSpawnDelayCounter) >= ASTEROID_SPAWN_DELAY)
    {
        SpawnAsteroid();

        InterlockedCompareExchangeRelease(&asteroidSpawnDelayCounter, 0, asteroidSpawnDelayCounter);
        return;
    }

    InterlockedIncrementRelease(&asteroidSpawnDelayCounter);
}

void Gamestate_FighterSpawn()
{
    List *fighters;
    ReadWriteLock_GetReadPermission(&GAMESTATE->fighters, (void **)&fighters);

    unsigned int fighterCount = fighters->length;

    ReadWriteLock_ReleaseReadPermission(&GAMESTATE->fighters, (void **)&fighters);

    if (fighterCount < MAX_FIGHTERS)
    {
        SpawnPlayerFighter();
    }
}
