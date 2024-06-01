
#include "../headers/gamestate.h"

DWORD WINAPI GamestateHandler(LPVOID lpParam)
{
    UNREFERENCED_PARAMETER(lpParam);

    HANDLE hTimer;

    hTimer = CreateWaitableTimer(NULL, TRUE, NULL);

    HANDLE *arrayOfTasksCompleteSyncEvents;
    List_GetAsArray(&TASKSTATE->tasksCompleteSyncEvents, (void **)&arrayOfTasksCompleteSyncEvents);
    unsigned short syncEventCount = TASKSTATE->tasksCompleteSyncEvents.length;

    unsigned short asteroidSpawnDelayCounter = 0;

    while (!GAMESTATE->exiting)
    {
        while (!GAMESTATE->running && !GAMESTATE->exiting)
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

        List tasksToQueue;
        List_Init(&tasksToQueue, NULL);

        List *entities;
        ReadWriteLock_GetReadPermission(&GAMESTATE->entities, (void **)&entities);

        ListIterator entitiesIterator;
        ListIterator_Init(&entitiesIterator, entities);
        Entity *entity;
        while (ListIterator_Next(&entitiesIterator, (void **)&entity))
        {
            if (entity->alive == ENTITY_DEAD)
            {
                continue;
            }

            ListIterator onTickIterator;
            ListIterator_Init(&onTickIterator, &entity->onTick);
            void (*referenceOnTick)(Entity *);
            while (ListIterator_Next(&onTickIterator, (void **)&referenceOnTick))
            {
                Task *task = malloc(sizeof(Task));
                task->task = (void (*)(void *))referenceOnTick;
                task->taskArguments = entity;
                List_Insert(&tasksToQueue, task);
            }
        }

        ReadWriteLock_ReleaseReadPermission(&GAMESTATE->entities, (void **)&entities);

        Task_QueueTasks(&tasksToQueue);
        List_Clear(&tasksToQueue);

        List *fighters;
        ReadWriteLock_GetReadPermission(&GAMESTATE->fighters, (void **)&fighters);

        unsigned int fighterCount = fighters->length;

        ReadWriteLock_ReleaseReadPermission(&GAMESTATE->fighters, (void **)&fighters);

        if (fighterCount < MAX_FIGHTERS)
        {
            SpawnPlayerFighter();
        }

        List *asteroids;
        ReadWriteLock_GetReadPermission(&GAMESTATE->asteroids, (void **)&asteroids);

        unsigned int asteroidCount = asteroids->length;

        ReadWriteLock_ReleaseReadPermission(&GAMESTATE->asteroids, (void **)&asteroids);

        if (asteroidCount < MAX_ASTEROIDS && asteroidSpawnDelayCounter >= 100 + (asteroidCount * 4))
        {
            SpawnAsteroid();
            asteroidSpawnDelayCounter = 0;
        }
        asteroidSpawnDelayCounter++;

        WaitForMultipleObjects(syncEventCount, arrayOfTasksCompleteSyncEvents, TRUE, INFINITE);

        for (unsigned short i = 0; i < syncEventCount; i++)
        {
            ResetEvent(arrayOfTasksCompleteSyncEvents[i]);
        }

        // The following should always be last:
        List *deadEntities;
        ReadWriteLock_GetReadPermission(&GAMESTATE->deadEntities, (void **)&deadEntities);

        List_Clear(deadEntities);

        ReadWriteLock_ReleaseReadPermission(&GAMESTATE->deadEntities, (void **)&deadEntities);

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

    free(arrayOfTasksCompleteSyncEvents);

    return 0;
}
