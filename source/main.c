
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

	List *systemTaskQueue = malloc(sizeof(List));
	List_Init(systemTaskQueue, NULL);
	ReadWriteLock_Init(&TASKSTATE->systemTaskQueue, systemTaskQueue);

	List *gamestateTaskQueue = malloc(sizeof(List));
	List_Init(gamestateTaskQueue, NULL);
	ReadWriteLock_Init(&TASKSTATE->gamestateTaskQueue, gamestateTaskQueue);

	List *garbageTaskQueue = malloc(sizeof(List));
	List_Init(garbageTaskQueue, NULL);
	ReadWriteLock_Init(&TASKSTATE->garbageTaskQueue, garbageTaskQueue);

	List_Init(&TASKSTATE->gamestateTasksCompleteSyncEvents, List_CloseHandleOnRemove);
	List_Init(&TASKSTATE->tasksQueuedSyncEvents, List_CloseHandleOnRemove);
	List_Init(&TASKSTATE->systemTasksQueuedSyncEvents, List_CloseHandleOnRemove);
	List_Init(&TASKSTATE->gamestateTasksQueuedSyncEvents, List_CloseHandleOnRemove);
	List_Init(&TASKSTATE->garbageTasksQueuedSyncEvents, List_CloseHandleOnRemove);

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	for (unsigned int i = 0; i < (systemInfo.dwNumberOfProcessors - 1); i++)
	{
		List_Insert(&TASKSTATE->gamestateTasksCompleteSyncEvents, CreateEvent(NULL, TRUE, TRUE, NULL));
		List_Insert(&TASKSTATE->tasksQueuedSyncEvents, CreateEvent(NULL, TRUE, FALSE, NULL));

		List_Insert(&TASKSTATE->systemTasksQueuedSyncEvents, CreateEvent(NULL, TRUE, FALSE, NULL));
		List_Insert(&TASKSTATE->gamestateTasksQueuedSyncEvents, CreateEvent(NULL, TRUE, FALSE, NULL));
		List_Insert(&TASKSTATE->garbageTasksQueuedSyncEvents, CreateEvent(NULL, TRUE, FALSE, NULL));

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

	ReadWriteLock_GetWritePermission(&TASKSTATE->garbageTaskQueue, (void **)&garbageTaskQueue);
	List_Clear(garbageTaskQueue);
	free(garbageTaskQueue);
	ReadWriteLock_Destroy(&TASKSTATE->garbageTaskQueue);

	ReadWriteLock_GetWritePermission(&TASKSTATE->gamestateTaskQueue, (void **)&gamestateTaskQueue);
	List_Clear(gamestateTaskQueue);
	free(gamestateTaskQueue);
	ReadWriteLock_Destroy(&TASKSTATE->gamestateTaskQueue);

	ReadWriteLock_GetWritePermission(&TASKSTATE->systemTaskQueue, (void **)&systemTaskQueue);
	List_Clear(systemTaskQueue);
	free(systemTaskQueue);
	ReadWriteLock_Destroy(&TASKSTATE->systemTaskQueue);

	List_Clear(&TASKSTATE->garbageTasksQueuedSyncEvents);
	List_Clear(&TASKSTATE->gamestateTasksQueuedSyncEvents);
	List_Clear(&TASKSTATE->systemTasksQueuedSyncEvents);
	List_Clear(&TASKSTATE->tasksQueuedSyncEvents);
	List_Clear(&TASKSTATE->gamestateTasksCompleteSyncEvents);

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
