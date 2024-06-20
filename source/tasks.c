
#include "tasks.h"

DWORD WINAPI TaskHandler(LPVOID lpParam)
{
    TaskHandlerArgs *taskHandlerArgs = (TaskHandlerArgs *)lpParam;
    unsigned int taskHandlerID = taskHandlerArgs->taskHandlerID;

    free(taskHandlerArgs);

    HANDLE tasksQueuedEvent;
    List_GetDataAtPosition(&TASKSTATE->tasksQueuedSyncEvents, &tasksQueuedEvent, taskHandlerID);

    HANDLE systemTasksQueuedEvent;
    List_GetDataAtPosition(&TASKSTATE->systemTasksQueuedSyncEvents, &systemTasksQueuedEvent, taskHandlerID);

    HANDLE gamestateTasksQueuedEvent;
    List_GetDataAtPosition(&TASKSTATE->gamestateTasksQueuedSyncEvents, &gamestateTasksQueuedEvent, taskHandlerID);

    HANDLE garbageTasksQueuedEvent;
    List_GetDataAtPosition(&TASKSTATE->garbageTasksQueuedSyncEvents, &garbageTasksQueuedEvent, taskHandlerID);

    HANDLE gamestateTaskCompleteEvent;
    List_GetDataAtPosition(&TASKSTATE->gamestateTasksCompleteSyncEvents, &gamestateTaskCompleteEvent, taskHandlerID);

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
                SetEvent(gamestateTaskCompleteEvent);
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

    SetEvent(gamestateTaskCompleteEvent);

    return 0;
}

__int8 Task_HandleTaskQueue(RWL_List *checkedTaskQueue)
{
    List *taskQueue;
    Task *task;

    if (!ReadWriteLock_GetWritePermissionTimeout(checkedTaskQueue, (void **)&taskQueue, 1))
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
    free(task);
    return TRUE;
}

/// @brief Queues a Task to the task system.
/// If you need to queue more than one task you should use Task_QueueGamestateTasks(List *) instead.
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
