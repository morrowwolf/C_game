
#include "../headers/gamestate.h"

DWORD WINAPI GamestateHandler(LPVOID lpParam)
{
    UNREFERENCED_PARAMETER(lpParam);

    HANDLE hTimer;
    LARGE_INTEGER liDueTime;
    liDueTime.QuadPart = -40000LL;

    hTimer = CreateWaitableTimer(NULL, TRUE, NULL);

    short asteroidSpawnDelayCounter = 0;

    while (TRUE)
    {
        SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0);

        ListElmt *referenceElementEntities = GAMESTATE->entities.head;
        while (referenceElementEntities != NULL)
        {
            Entity *referenceEntity = referenceElementEntities->data;
            ListElmt *referenceElementOnTick = referenceEntity->onTick.head;
            while (referenceElementOnTick != NULL)
            {
                ((void (*)(Entity *))referenceElementOnTick->data)(referenceEntity);
                referenceElementOnTick = referenceElementOnTick->next;
            }
            referenceElementEntities = referenceElementEntities->next;
        }

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
