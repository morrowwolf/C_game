
#include "../headers/main.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(szCmdLine);

	GAMESTATE = calloc(1, sizeof(Gamestate));
	List *entities = malloc(sizeof(List));
	List *deadEntities = malloc(sizeof(List));
	List *asteroids = malloc(sizeof(List));
	List *fighters = malloc(sizeof(List));
	List_Init(entities, NULL);
	List_Init(deadEntities, List_DestroyEntityOnRemove);
	List_Init(asteroids, NULL);
	List_Init(fighters, NULL);
	ReadWriteLock_Init(&GAMESTATE->entities, entities);
	ReadWriteLock_Init(&GAMESTATE->deadEntities, deadEntities);
	ReadWriteLock_Init(&GAMESTATE->asteroids, asteroids);
	ReadWriteLock_Init(&GAMESTATE->fighters, fighters);
#ifdef DEBUG
	GAMESTATE->debugMode = TRUE;
#endif
	GAMESTATE->running = TRUE;
	GAMESTATE->keyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	List threadHandles;
	List_Init(&threadHandles, NULL);
	HANDLE threadHandle;

	SCREEN = calloc(1, sizeof(Screen));
	SCREEN->screenWidth = DEFAULT_SCREEN_SIZE_X;
	SCREEN->screenHeight = DEFAULT_SCREEN_SIZE_Y;

	TASKSTATE = calloc(1, sizeof(TaskState));

	List *taskQueue = malloc(sizeof(List));
	List_Init(taskQueue, NULL);
	ReadWriteLock_Init(&TASKSTATE->taskQueue, taskQueue);

	List_Init(&TASKSTATE->tasksCompleteSyncEvents, NULL);
	List_Init(&TASKSTATE->tasksQueuedSyncEvents, NULL);

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	for (unsigned int i = 0; i < (systemInfo.dwNumberOfProcessors - 1); i++)
	{
		List_Insert(&TASKSTATE->tasksCompleteSyncEvents, CreateEvent(NULL, TRUE, TRUE, NULL));
		List_Insert(&TASKSTATE->tasksQueuedSyncEvents, CreateEvent(NULL, TRUE, FALSE, NULL));

		TaskHandlerArgs *taskHandlerArgs = calloc(1, sizeof(TaskHandlerArgs));
		taskHandlerArgs->taskHandlerID = i;

		threadHandle = CreateThread(NULL, 0, TaskHandler, taskHandlerArgs, 0, NULL);
		List_Insert(&threadHandles, threadHandle);
	}

	threadHandle = CreateThread(NULL, 0, GamestateHandler, NULL, 0, NULL);
	List_Insert(&threadHandles, threadHandle);

	WindowHandler(hInstance, iCmdShow);

	//
	// Clean up:
	//
	SetEvent(GAMESTATE->keyEvent);

	ListIterator tasksQueuedEventsIterator;
	ListIterator_Init(&tasksQueuedEventsIterator, &TASKSTATE->tasksQueuedSyncEvents);
	HANDLE tasksQueuedSyncEvent;
	while (ListIterator_Next(&tasksQueuedEventsIterator, (void **)&tasksQueuedSyncEvent))
	{
		SetEvent(tasksQueuedSyncEvent);
	}

	void *arrayOfThreadHandles;
	List_GetAsArray(&threadHandles, &arrayOfThreadHandles);

	WaitForMultipleObjects(threadHandles.length, arrayOfThreadHandles, TRUE, INFINITE);

	free(arrayOfThreadHandles);
	List_Clear(&threadHandles);

	ReadWriteLock_GetWritePermission(&TASKSTATE->taskQueue, (void **)&taskQueue);
	List_Clear(taskQueue);
	free(taskQueue);
	ReadWriteLock_Destroy(&TASKSTATE->taskQueue);

	List_Clear(&TASKSTATE->tasksQueuedSyncEvents);
	List_Clear(&TASKSTATE->tasksCompleteSyncEvents);

	free(TASKSTATE);

	free(SCREEN);

	// All threads should be done at this point so we're just cleaning up.
	ListIterator entitiesIterator;
	ListIterator_Init(&entitiesIterator, entities);
	Entity *entity;
	while (ListIterator_Next(&entitiesIterator, (void **)&entity))
	{
		List_Insert(deadEntities, entity);
	}

	List_Clear(deadEntities);
	List_Clear(entities);
	List_Clear(asteroids);
	List_Clear(fighters);

	free(deadEntities);
	free(entities);
	free(asteroids);
	free(fighters);

	ReadWriteLock_Destroy(&GAMESTATE->entities);
	ReadWriteLock_Destroy(&GAMESTATE->deadEntities);
	ReadWriteLock_Destroy(&GAMESTATE->asteroids);
	ReadWriteLock_Destroy(&GAMESTATE->fighters);

	CloseHandle(GAMESTATE->keyEvent);

	free(GAMESTATE);

#ifdef DEBUG
	TCHAR buffer[64];
	_stprintf(buffer, TEXT("Memory leak status: (%s)"), _CrtDumpMemoryLeaks() ? TEXT("TRUE") : TEXT("FALSE"));

	OutputDebugString(buffer);
#endif

	exit(0);
}
