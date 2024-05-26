
#include "../headers/tasks.h"

DWORD WINAPI TaskHandler(LPVOID lpParam)
{
    TaskHandlerArgs *taskHandlerArgs = (TaskHandlerArgs *)lpParam;
    unsigned int taskHandlerID = taskHandlerArgs->taskHandlerID;

    free(taskHandlerArgs);

    HANDLE tasksQueuedEvent;
    List_GetDataAtPosition(&TASKSTATE->tasksQueuedSyncEvents, &tasksQueuedEvent, taskHandlerID);

    HANDLE tasksCompleteEvent;
    List_GetDataAtPosition(&TASKSTATE->tasksCompleteSyncEvents, &tasksCompleteEvent, taskHandlerID);

    while (!GAMESTATE->exiting)
    {
        WaitForSingleObject(tasksQueuedEvent, INFINITE);

        List *taskQueue;
        Task *task = NULL;
        ReadWriteLock_GetWritePermission(&TASKSTATE->taskQueue, (void **)&taskQueue);

        if (taskQueue->length > 0)
        {
            task = taskQueue->head->data;
            List_RemoveElement(taskQueue, taskQueue->head);
        }

        ReadWriteLock_ReleaseWritePermission(&TASKSTATE->taskQueue, (void **)&taskQueue);

        if (task == NULL)
        {
            ResetEvent(tasksQueuedEvent);
            SetEvent(tasksCompleteEvent);
            continue;
        }

        task->task(task->taskArguments);
        free(task);
    }

    SetEvent(tasksCompleteEvent);

    return 0;
}

/// @brief Queues a Task to the task system.
/// If you need to queue more than one task you should use Task_QueueTasks(List *) instead.
/// @param task The Task to queue.
void Task_QueueTask(Task *task)
{
    List *taskQueue;
    ReadWriteLock_GetWritePermission(&TASKSTATE->taskQueue, (void **)&taskQueue);

    List_Insert(taskQueue, task);

    ReadWriteLock_ReleaseWritePermission(&TASKSTATE->taskQueue, (void **)&taskQueue);

    ListIterator tasksQueuedEventsIterator;
    ListIterator_Init(&tasksQueuedEventsIterator, &TASKSTATE->tasksQueuedSyncEvents);
    HANDLE tasksQueuedSyncEvent;
    while (ListIterator_Next(&tasksQueuedEventsIterator, (void **)&tasksQueuedSyncEvent))
    {
        SetEvent(tasksQueuedSyncEvent);
        ResetEvent(tasksQueuedSyncEvent);
    }
}

/// @brief Queues a list of Task to the task system.
/// @param list The list of Task to queue.
void Task_QueueTasks(List *list)
{
    List *taskQueue;
    ReadWriteLock_GetWritePermission(&TASKSTATE->taskQueue, (void **)&taskQueue);

    ListIterator listIterator;
    ListIterator_Init(&listIterator, list);
    Task *task;
    while (ListIterator_Next(&listIterator, (void **)&task))
    {
        List_Insert(taskQueue, task);
    }

    ReadWriteLock_ReleaseWritePermission(&TASKSTATE->taskQueue, (void **)&taskQueue);

    ListIterator tasksQueuedEventsIterator;
    ListIterator_Init(&tasksQueuedEventsIterator, &TASKSTATE->tasksQueuedSyncEvents);
    HANDLE tasksQueuedSyncEvent;
    while (ListIterator_Next(&tasksQueuedEventsIterator, (void **)&tasksQueuedSyncEvent))
    {
        SetEvent(tasksQueuedSyncEvent);
    }
}
