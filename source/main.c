
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
	GAMESTATE->running = TRUE;
	GAMESTATE->keyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	List threadHandles;
	List_Init(&threadHandles, NULL);
	HANDLE threadHandle;

#ifdef CPU_GRAPHICS

	SCREEN = calloc(1, sizeof(Screen));

	SCREEN->windowHandleInitializedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	SCREEN->currentBufferUsed = 0;

	unsigned int i;
	for (i = 0; i < BUFFER_THREAD_COUNT; i++)
	{
		SCREEN->bufferDrawingMutexes[i] = CreateMutex(NULL, FALSE, NULL);
		SCREEN->bufferRedrawSemaphores[i] = CreateSemaphore(NULL, 0, 1, NULL);

		// Freed by the handler
		BufferArgs *bufferArgs = malloc(sizeof(BufferArgs));
		bufferArgs->bufferId = i;
		threadHandle = CreateThread(NULL, 0, BufferHandler, bufferArgs, 0, NULL);
		List_Insert(&threadHandles, threadHandle);
	}

#endif

	TASKSTATE = calloc(1, sizeof(TaskState));

	List *taskQueue = malloc(sizeof(List));
	List_Init(taskQueue, NULL);
	ReadWriteLock_Init(&TASKSTATE->taskQueue, taskQueue);

	List_Init(&TASKSTATE->tasksCompleteSyncEvents, NULL);
	List_Init(&TASKSTATE->tasksQueuedSyncEvents, NULL);

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	for (i = 0; i < (systemInfo.dwNumberOfProcessors - BUFFER_THREAD_COUNT - 1); i++)
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

	ListIterator tasksQueuedEventsIterator;
	ListIterator_Init(&tasksQueuedEventsIterator, &TASKSTATE->tasksQueuedSyncEvents);
	HANDLE tasksQueuedSyncEvent;
	while (ListIterator_Next(&tasksQueuedEventsIterator, (void **)&tasksQueuedSyncEvent))
	{
		SetEvent(tasksQueuedSyncEvent);
	}

#ifdef CPU_GRAPHICS

	for (unsigned int i = 0; i < BUFFER_THREAD_COUNT; i++)
	{
		ReleaseSemaphore(SCREEN->bufferRedrawSemaphores[i], 1, NULL);
	}

#endif

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

#ifdef CPU_GRAPHICS

	for (unsigned int i = 0; i < BUFFER_THREAD_COUNT; i++)
	{
		CloseHandle(SCREEN->bufferRedrawSemaphores[i]);
		CloseHandle(SCREEN->bufferDrawingMutexes[i]);
	}

	free(SCREEN);

#endif

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

	free(GAMESTATE);

#ifdef DEBUG
	TCHAR buffer[64];
	_stprintf(buffer, TEXT("Memory leak status: (%s)"), _CrtDumpMemoryLeaks() ? TEXT("TRUE") : TEXT("FALSE"));

	OutputDebugString(buffer);
#endif

	exit(0);
}
