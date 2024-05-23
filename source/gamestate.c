
#include "../headers/gamestate.h"

DWORD WINAPI GamestateHandler(LPVOID lpParam)
{
    UNREFERENCED_PARAMETER(lpParam);

    // TODO:
    // SYSTEM_INFO systemInfo;
    // GetSystemInfo(&systemInfo);

    // TCHAR buffer[32];
    // _stprintf(buffer, TEXT("%d"), systemInfo.dwNumberOfProcessors);
    // OutputDebugString(buffer);

    HANDLE hTimer;
    LARGE_INTEGER liDueTime;
    liDueTime.QuadPart = -40000LL;

    hTimer = CreateWaitableTimer(NULL, TRUE, NULL);

    unsigned short asteroidSpawnDelayCounter = 0;

    while (TRUE)
    {
        while (!GAMESTATE->running)
        {
            WaitForSingleObject(GAMESTATE->keyEvent, INFINITE);
        }

        SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0);

        ListIterator *entitiesIterator;
        ListIterator_Init(&entitiesIterator, &GAMESTATE->entities, ReadWriteLock_Read);
        Entity *referenceEntity;
        while (ListIterator_Next(entitiesIterator, (void **)&referenceEntity))
        {
            ListIterator *onTickIterator;
            ListIterator_Init(&onTickIterator, &referenceEntity->onTick, ReadWriteLock_Read);
            void (*referenceOnTick)(Entity *);
            while (ListIterator_Next(onTickIterator, (void **)&referenceOnTick))
            {
                referenceOnTick(referenceEntity);
            }
            ListIterator_Destroy(onTickIterator);
        }
        ListIterator_Destroy(entitiesIterator);

        if (GAMESTATE->fighters.length < MAX_FIGHTERS)
        {
            SpawnPlayerFighter();
        }

        if (GAMESTATE->asteroids.length < MAX_ASTEROIDS && asteroidSpawnDelayCounter >= 100 + (GAMESTATE->asteroids.length * 4))
        {
            SpawnAsteroid();
            asteroidSpawnDelayCounter = 0;
        }
        asteroidSpawnDelayCounter++;

        // The following should always be last:
        List_Clear(&GAMESTATE->deadEntities);

        WaitForSingleObject(hTimer, INFINITE);
    }

    exit(0);
}

// DWORD WINAPI GamestateHelper(LPVOID lpParam){}
