
#include "main.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(szCmdLine);

	// Memory manager init
	MemoryManager_Initialize();

	// Screen init
	MemoryManager_AllocateMemory((void **)&SCREEN, sizeof(Screen));
	SCREEN->screenWidth = DEFAULT_SCREEN_SIZE_X;
	SCREEN->screenHeight = DEFAULT_SCREEN_SIZE_Y;
	SCREEN->screenRadius = sqrt(pow(SCREEN->screenWidth, 2) + pow(SCREEN->screenHeight, 2));
	SCREEN->screenLocation.x = abs(MAX_GAME_SPACE_LEFT) - abs(MAX_GAME_SPACE_RIGHT);
	SCREEN->screenLocation.y = abs(MAX_GAME_SPACE_TOP) - abs(MAX_GAME_SPACE_BOTTOM);
	SCREEN->preRenderSetupMutex = CreateMutex(NULL, FALSE, NULL);

	// Taskstate init
	MemoryManager_AllocateMemory((void **)&TASKSTATE, sizeof(TaskState));

	List *systemTaskQueue;
	MemoryManager_AllocateMemory((void **)&systemTaskQueue, sizeof(List));
	List_Init(systemTaskQueue, NULL);
	ReadWriteLock_Init(&TASKSTATE->systemTaskQueue, systemTaskQueue);

	List *gamestateTaskQueue;
	MemoryManager_AllocateMemory((void **)&gamestateTaskQueue, sizeof(List));
	List_Init(gamestateTaskQueue, NULL);
	ReadWriteLock_Init(&TASKSTATE->gamestateTaskQueue, gamestateTaskQueue);

	List *garbageTaskQueue;
	MemoryManager_AllocateMemory((void **)&garbageTaskQueue, sizeof(List));
	List_Init(garbageTaskQueue, NULL);
	ReadWriteLock_Init(&TASKSTATE->garbageTaskQueue, garbageTaskQueue);

	List_Init(&TASKSTATE->gamestateTasksCompleteSyncEvents, List_CloseHandleOnRemove);
	List_Init(&TASKSTATE->tasksQueuedSyncEvents, List_CloseHandleOnRemove);
	List_Init(&TASKSTATE->systemTasksQueuedSyncEvents, List_CloseHandleOnRemove);
	List_Init(&TASKSTATE->gamestateTasksQueuedSyncEvents, List_CloseHandleOnRemove);
	List_Init(&TASKSTATE->garbageTasksQueuedSyncEvents, List_CloseHandleOnRemove);

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	List threadHandles;
	List_Init(&threadHandles, NULL);
	HANDLE threadHandle;

	for (unsigned int i = 0; i < (systemInfo.dwNumberOfProcessors - 1); i++)
	{
		List_Insert(&TASKSTATE->gamestateTasksCompleteSyncEvents, CreateEvent(NULL, TRUE, TRUE, NULL));
		List_Insert(&TASKSTATE->tasksQueuedSyncEvents, CreateEvent(NULL, TRUE, FALSE, NULL));

		List_Insert(&TASKSTATE->systemTasksQueuedSyncEvents, CreateEvent(NULL, TRUE, FALSE, NULL));
		List_Insert(&TASKSTATE->gamestateTasksQueuedSyncEvents, CreateEvent(NULL, TRUE, FALSE, NULL));
		List_Insert(&TASKSTATE->garbageTasksQueuedSyncEvents, CreateEvent(NULL, TRUE, FALSE, NULL));

		TaskHandlerArgs *taskHandlerArgs;
		MemoryManager_AllocateMemory((void **)&taskHandlerArgs, sizeof(TaskHandlerArgs));
		taskHandlerArgs->taskHandlerID = i;

		threadHandle = CreateThread(NULL, 0, TaskHandler, taskHandlerArgs, 0, NULL);
		List_Insert(&threadHandles, threadHandle);

		TASKSTATE->totalTaskThreads++;
	}

	// Gamestate init
	MemoryManager_AllocateMemory((void **)&GAMESTATE, sizeof(Gamestate));

	List *entities;
	MemoryManager_AllocateMemory((void **)&entities, sizeof(List));
	List *deadEntities;
	MemoryManager_AllocateMemory((void **)&deadEntities, sizeof(List));
	List *asteroids;
	MemoryManager_AllocateMemory((void **)&asteroids, sizeof(List));
	List *fighters;
	MemoryManager_AllocateMemory((void **)&fighters, sizeof(List));
	List_Init(entities, NULL);
	List_Init(deadEntities, NULL);
	List_Init(asteroids, NULL);
	List_Init(fighters, NULL);
	ReadWriteLockPriority_Init(&GAMESTATE->entities, entities);
	ReadWriteLock_Init(&GAMESTATE->deadEntities, deadEntities);
	ReadWriteLock_Init(&GAMESTATE->asteroids, asteroids);
	ReadWriteLock_Init(&GAMESTATE->fighters, fighters);
#ifdef DEBUG
	GAMESTATE->debugMode = TRUE;
#endif
	GAMESTATE->running = TRUE;
	GAMESTATE->keyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

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

	while (WaitForMultipleObjects(threadHandles.length, arrayOfThreadHandles, TRUE, 10) == WAIT_TIMEOUT)
	{
		ListIterator gamestateTasksCompleteEventsIterator;
		ListIterator_Init(&gamestateTasksCompleteEventsIterator, &TASKSTATE->gamestateTasksCompleteSyncEvents);
		HANDLE gamestateTasksCompleteEvent;
		while (ListIterator_Next(&gamestateTasksCompleteEventsIterator, (void **)&gamestateTasksCompleteEvent))
		{
			SetEvent(gamestateTasksCompleteEvent);
		}
	}

	MemoryManager_DeallocateMemory(&arrayOfThreadHandles, threadHandles.length * sizeof(HANDLE));
	List_Clear(&threadHandles);

	// All threads should be done at this point so we're just cleaning up.

	ListIterator entitiesIterator;
	ListIterator_Init(&entitiesIterator, entities);
	Entity *entity;

	ReadWriteLock_GetWritePermission(&TASKSTATE->garbageTaskQueue, (void **)&garbageTaskQueue);

	ListIterator garbageTaskQueueIterator;
	ListIterator_Init(&garbageTaskQueueIterator, garbageTaskQueue);
	Task *task;

	while (ListIterator_Next(&entitiesIterator, (void **)&entity))
	{
		while (ListIterator_Next(&garbageTaskQueueIterator, (void **)&task))
		{
			if (task->taskArgument == entity)
			{
				List_RemoveElementWithMatchingData(garbageTaskQueue, task);
				MemoryManager_DeallocateMemory((void **)&task, sizeof(Task));

				// Reset the iterator since we modified the list.
				ListIterator_Init(&garbageTaskQueueIterator, garbageTaskQueue);
			}
		}
		entity->onDestroy(entity);

		// Reset the iterator since we have likely modified the list.
		ListIterator_Init(&entitiesIterator, entities);
	}

	ReadWriteLock_ReleaseWritePermission(&TASKSTATE->garbageTaskQueue, (void **)&garbageTaskQueue);

	List_Clear(deadEntities);
	List_Clear(entities);
	List_Clear(asteroids);
	List_Clear(fighters);

	MemoryManager_DeallocateMemory((void **)&deadEntities, sizeof(List));
	MemoryManager_DeallocateMemory((void **)&entities, sizeof(List));
	MemoryManager_DeallocateMemory((void **)&asteroids, sizeof(List));
	MemoryManager_DeallocateMemory((void **)&fighters, sizeof(List));

	ReadWriteLockPriority_Destroy(&GAMESTATE->entities);
	ReadWriteLock_Destroy(&GAMESTATE->deadEntities);
	ReadWriteLock_Destroy(&GAMESTATE->asteroids);
	ReadWriteLock_Destroy(&GAMESTATE->fighters);

	CloseHandle(GAMESTATE->keyEvent);

	MemoryManager_DeallocateMemory((void **)&GAMESTATE, sizeof(Gamestate));

	ReadWriteLock_GetWritePermission(&TASKSTATE->garbageTaskQueue, (void **)&garbageTaskQueue);

	ListIterator_Init(&garbageTaskQueueIterator, garbageTaskQueue);

	while (ListIterator_Next(&garbageTaskQueueIterator, (void **)&task))
	{
		if (task->taskArgument != NULL)
		{
			task->task(task->taskArgument);
		}
		else
		{
			void (*castedTask)() = (void (*)())task->task;
			castedTask();
		}
		MemoryManager_DeallocateMemory((void **)&task, sizeof(Task));
	}

	List_Clear(garbageTaskQueue);
	MemoryManager_DeallocateMemory((void **)&garbageTaskQueue, sizeof(List));
	ReadWriteLock_Destroy(&TASKSTATE->garbageTaskQueue);

	ReadWriteLock_GetWritePermission(&TASKSTATE->gamestateTaskQueue, (void **)&gamestateTaskQueue);

	ListIterator gamestateTaskQueueIterator;
	ListIterator_Init(&gamestateTaskQueueIterator, gamestateTaskQueue);

	while (ListIterator_Next(&gamestateTaskQueueIterator, (void **)&task))
	{
		MemoryManager_DeallocateMemory((void **)&task, sizeof(Task));
	}

	List_Clear(gamestateTaskQueue);
	MemoryManager_DeallocateMemory((void **)&gamestateTaskQueue, sizeof(List));
	ReadWriteLock_Destroy(&TASKSTATE->gamestateTaskQueue);

	ReadWriteLock_GetWritePermission(&TASKSTATE->systemTaskQueue, (void **)&systemTaskQueue);

	ListIterator systemTaskQueueIterator;
	ListIterator_Init(&systemTaskQueueIterator, systemTaskQueue);

	while (ListIterator_Next(&systemTaskQueueIterator, (void **)&task))
	{
		MemoryManager_DeallocateMemory((void **)&task, sizeof(Task));
	}

	List_Clear(systemTaskQueue);
	MemoryManager_DeallocateMemory((void **)&systemTaskQueue, sizeof(List));
	ReadWriteLock_Destroy(&TASKSTATE->systemTaskQueue);

	List_Clear(&TASKSTATE->garbageTasksQueuedSyncEvents);
	List_Clear(&TASKSTATE->gamestateTasksQueuedSyncEvents);
	List_Clear(&TASKSTATE->systemTasksQueuedSyncEvents);
	List_Clear(&TASKSTATE->tasksQueuedSyncEvents);
	List_Clear(&TASKSTATE->gamestateTasksCompleteSyncEvents);

	MemoryManager_DeallocateMemory((void **)&TASKSTATE, sizeof(TaskState));

	CloseHandle(SCREEN->preRenderSetupMutex);
	CloseHandle(SCREEN->fenceEvent);
	ReleaseDirectxObjects();
	MemoryManager_DeallocateMemory((void **)&SCREEN, sizeof(Screen));

	MemoryManager_Destroy();

#ifdef DEBUG
	TCHAR buffer[64];
	_stprintf(buffer, TEXT("Memory leak status: (%s)"), _CrtDumpMemoryLeaks() ? TEXT("TRUE") : TEXT("FALSE"));

	OutputDebugString(buffer);
#endif

	exit(0);
}
