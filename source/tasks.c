
#include "tasks.h"

DWORD WINAPI TaskHandler(LPVOID lpParam)
{
    TaskHandlerArgs *taskHandlerArgs = (TaskHandlerArgs *)lpParam;
    unsigned int taskHandlerID = taskHandlerArgs->taskHandlerID;

    MemoryManager_DeallocateMemory((void **)&taskHandlerArgs, sizeof(TaskHandlerArgs));

    HANDLE tasksQueuedEvent;
    List_GetDataAtPosition(&TASKSTATE->tasksQueuedSyncEvents, &tasksQueuedEvent, taskHandlerID);

    HANDLE systemTasksQueuedEvent;
    List_GetDataAtPosition(&TASKSTATE->systemTasksQueuedSyncEvents, &systemTasksQueuedEvent, taskHandlerID);

    HANDLE gamestateTasksQueuedEvent;
    List_GetDataAtPosition(&TASKSTATE->gamestateTasksQueuedSyncEvents, &gamestateTasksQueuedEvent, taskHandlerID);

    HANDLE garbageTasksQueuedEvent;
    List_GetDataAtPosition(&TASKSTATE->garbageTasksQueuedSyncEvents, &garbageTasksQueuedEvent, taskHandlerID);

    HANDLE gamestateTasksCompleteEvent;
    List_GetDataAtPosition(&TASKSTATE->gamestateTasksCompleteSyncEvents, &gamestateTasksCompleteEvent, taskHandlerID);

    while (!SCREEN->exiting)
    {
        WaitForSingleObject(tasksQueuedEvent, INFINITE);

        if (WaitForSingleObject(systemTasksQueuedEvent, 0) == WAIT_OBJECT_0)
        {
            if (Task_HandleTaskQueue(&TASKSTATE->systemTaskQueue))
            {
                continue;
            }
            else
            {
                ResetEvent(systemTasksQueuedEvent);
            }
        }

        if (WaitForSingleObject(gamestateTasksQueuedEvent, 0) == WAIT_OBJECT_0)
        {
            if (Task_HandleTaskQueue(&TASKSTATE->gamestateTaskQueue))
            {
                continue;
            }
            else
            {
                ResetEvent(gamestateTasksQueuedEvent);
                SetEvent(gamestateTasksCompleteEvent);
            }
        }

        if (WaitForSingleObject(garbageTasksQueuedEvent, 0) == WAIT_OBJECT_0)
        {
            if (Task_HandleTaskQueue(&TASKSTATE->garbageTaskQueue))
            {
                continue;
            }
            else
            {
                ResetEvent(garbageTasksQueuedEvent);
            }
        }

        ResetEvent(tasksQueuedEvent);
    }

    SetEvent(gamestateTasksCompleteEvent);

    return 0;
}

__int8 Task_HandleTaskQueue(RWL_List *checkedTaskQueue)
{
    List *taskQueue;
    Task *task;

    if (!ReadWriteLock_TryGetWritePermission(checkedTaskQueue, (void **)&taskQueue))
    {
        return TRUE;
    }

    if (taskQueue->length <= 0u)
    {
        ReadWriteLock_ReleaseWritePermission(checkedTaskQueue, (void **)&taskQueue);
        return FALSE;
    }

    task = taskQueue->head->data;
    List_RemoveElement(taskQueue, taskQueue->head);

    ReadWriteLock_ReleaseWritePermission(checkedTaskQueue, (void **)&taskQueue);

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
    return TRUE;
}

/// @brief Queues a Task to the task system.
/// If you need to queue more than one task you should use Task_QueueTasks(List *) instead.
/// Do *not* use this for gamestate tasks.
/// They have an extra step and instead should use Task_QueueGamestateTask(Task *);
/// @param task The Task to queue.
void Task_QueueTask(RWL_List *queueingTaskQueue, List *syncEvents, Task *task)
{
    List *taskQueue;
    ReadWriteLock_GetWritePermission(queueingTaskQueue, (void **)&taskQueue);

    List_Insert(taskQueue, task);

    ReadWriteLock_ReleaseWritePermission(queueingTaskQueue, (void **)&taskQueue);

    ListIterator tasksQueuedEventsIterator;
    ListIterator_Init(&tasksQueuedEventsIterator, syncEvents);
    HANDLE tasksQueuedSyncEvent;
    while (ListIterator_Next(&tasksQueuedEventsIterator, (void **)&tasksQueuedSyncEvent))
    {
        SetEvent(tasksQueuedSyncEvent);
    }

    ListIterator_Init(&tasksQueuedEventsIterator, &TASKSTATE->tasksQueuedSyncEvents);
    while (ListIterator_Next(&tasksQueuedEventsIterator, (void **)&tasksQueuedSyncEvent))
    {
        SetEvent(tasksQueuedSyncEvent);
    }
}

/// @brief Queues a list of Task to the task system.
/// Do *not* use this for gamestate tasks.
/// They have an extra step and instead should use Task_QueueGamestateTask(Task *);
/// @param list The list of Task to queue.
void Task_QueueTasks(RWL_List *queueingTaskQueue, List *syncEvents, List *list)
{
    List *taskQueue;
    ReadWriteLock_GetWritePermission(queueingTaskQueue, (void **)&taskQueue);

    ListIterator listIterator;
    ListIterator_Init(&listIterator, list);
    Task *task;
    while (ListIterator_Next(&listIterator, (void **)&task))
    {
        List_Insert(taskQueue, task);
    }

    ReadWriteLock_ReleaseWritePermission(queueingTaskQueue, (void **)&taskQueue);

    ListIterator tasksQueuedEventsIterator;
    ListIterator_Init(&tasksQueuedEventsIterator, syncEvents);
    HANDLE tasksQueuedSyncEvent;
    while (ListIterator_Next(&tasksQueuedEventsIterator, (void **)&tasksQueuedSyncEvent))
    {
        SetEvent(tasksQueuedSyncEvent);
    }

    ListIterator_Init(&tasksQueuedEventsIterator, &TASKSTATE->tasksQueuedSyncEvents);
    while (ListIterator_Next(&tasksQueuedEventsIterator, (void **)&tasksQueuedSyncEvent))
    {
        SetEvent(tasksQueuedSyncEvent);
    }
}

void Task_QueueGamestateTask(Task *task)
{
    ListIterator gamestateTasksCompleteEventIterator;
    ListIterator_Init(&gamestateTasksCompleteEventIterator, &TASKSTATE->gamestateTasksCompleteSyncEvents);
    HANDLE gamestateTasksCompleteEvent;
    while (ListIterator_Next(&gamestateTasksCompleteEventIterator, &gamestateTasksCompleteEvent))
    {
        ResetEvent(gamestateTasksCompleteEvent);
    }

    Task_QueueTask(&TASKSTATE->gamestateTaskQueue, &TASKSTATE->gamestateTasksQueuedSyncEvents, task);
}

void Task_QueueGamestateTasks(List *list)
{
    ListIterator gamestateTasksCompleteEventIterator;
    ListIterator_Init(&gamestateTasksCompleteEventIterator, &TASKSTATE->gamestateTasksCompleteSyncEvents);
    HANDLE gamestateTasksCompleteEvent;
    while (ListIterator_Next(&gamestateTasksCompleteEventIterator, &gamestateTasksCompleteEvent))
    {
        ResetEvent(gamestateTasksCompleteEvent);
    }

    Task_QueueTasks(&TASKSTATE->gamestateTaskQueue, &TASKSTATE->gamestateTasksQueuedSyncEvents, list);
}
