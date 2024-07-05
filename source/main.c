
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

	Stack_Init(&TASKSTATE->systemTaskStack, NULL);
	Stack_Init(&TASKSTATE->gamestateTaskStack, NULL);
	Stack_Init(&TASKSTATE->garbageTaskStack, NULL);

	List_Init(&TASKSTATE->gamestateTasksCompleteSyncEvents, List_CloseHandleOnRemove);
	List_Init(&TASKSTATE->tasksQueuedSyncEvents, List_CloseHandleOnRemove);
	List_Init(&TASKSTATE->systemTasksPushedSyncEvents, List_CloseHandleOnRemove);
	List_Init(&TASKSTATE->gamestateTasksPushedSyncEvents, List_CloseHandleOnRemove);
	List_Init(&TASKSTATE->garbageTasksPushedSyncEvents, List_CloseHandleOnRemove);

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	List threadHandles;
	List_Init(&threadHandles, NULL);
	HANDLE threadHandle;

	for (unsigned int i = 0; i < (systemInfo.dwNumberOfProcessors - 1); i++)
	{
		List_Insert(&TASKSTATE->gamestateTasksCompleteSyncEvents, CreateEvent(NULL, TRUE, TRUE, NULL));
		List_Insert(&TASKSTATE->tasksQueuedSyncEvents, CreateEvent(NULL, TRUE, FALSE, NULL));

		List_Insert(&TASKSTATE->systemTasksPushedSyncEvents, CreateEvent(NULL, TRUE, FALSE, NULL));
		List_Insert(&TASKSTATE->gamestateTasksPushedSyncEvents, CreateEvent(NULL, TRUE, FALSE, NULL));
		List_Insert(&TASKSTATE->garbageTasksPushedSyncEvents, CreateEvent(NULL, TRUE, FALSE, NULL));

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

	Stack tempStack;
	Stack_Init(&tempStack, NULL);
	Task *task;

	while (ListIterator_Next(&entitiesIterator, (void **)&entity))
	{
		while (TASKSTATE->garbageTaskStack.length >= 1)
		{
			Stack_Pop(&TASKSTATE->garbageTaskStack, (void **)&task);

			if (task->taskArgument == entity)
			{
				MemoryManager_DeallocateMemory((void **)&task, sizeof(Task));
			}
			else
			{
				Stack_Push(&tempStack, task);
			}
		}

		while (tempStack.length >= 1)
		{
			Stack_Pop(&tempStack, (void **)&task);
			Stack_Push(&TASKSTATE->garbageTaskStack, task);
		}

		entity->onDestroy(entity);

		// Reset the iterator since we have likely modified the list.
		ListIterator_Init(&entitiesIterator, entities);
	}

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

	while (TASKSTATE->garbageTaskStack.length >= 1)
	{
		Stack_Pop(&TASKSTATE->garbageTaskStack, (void **)&task);

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

	TASKSTATE->garbageTaskStack.destroy = Stack_DeallocateTaskOnPop;
	Stack_Clear(&TASKSTATE->garbageTaskStack);

	TASKSTATE->gamestateTaskStack.destroy = Stack_DeallocateTaskOnPop;
	Stack_Clear(&TASKSTATE->gamestateTaskStack);

	TASKSTATE->systemTaskStack.destroy = Stack_DeallocateTaskOnPop;
	Stack_Clear(&TASKSTATE->systemTaskStack);

	List_Clear(&TASKSTATE->garbageTasksPushedSyncEvents);
	List_Clear(&TASKSTATE->gamestateTasksPushedSyncEvents);
	List_Clear(&TASKSTATE->systemTasksPushedSyncEvents);
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

void Stack_DeallocateTaskOnPop(void *data)
{
	Task *task = (Task *)data;
	MemoryManager_DeallocateMemory((void **)&task, sizeof(Task));
}
