
#include "tasks.h"

static void Task_PushTask(Stack *taskStack, List *syncEvents, Task *task);
static void Task_PushTasks(Stack *taskStack, List *syncEvents, List *taskList);

DWORD WINAPI TaskHandler(LPVOID lpParam)
{
    TaskHandlerArgs *taskHandlerArgs = (TaskHandlerArgs *)lpParam;
    unsigned int taskHandlerID = taskHandlerArgs->taskHandlerID;

    MemoryManager_DeallocateMemory((void **)&taskHandlerArgs, sizeof(TaskHandlerArgs));

    HANDLE tasksQueuedEvent;
    List_GetDataAtPosition(&TASKSTATE->tasksQueuedSyncEvents, &tasksQueuedEvent, taskHandlerID);

    HANDLE systemTasksQueuedEvent;
    List_GetDataAtPosition(&TASKSTATE->systemTasksPushedSyncEvents, &systemTasksQueuedEvent, taskHandlerID);

    HANDLE gamestateTasksQueuedEvent;
    List_GetDataAtPosition(&TASKSTATE->gamestateTasksPushedSyncEvents, &gamestateTasksQueuedEvent, taskHandlerID);

    HANDLE garbageTasksQueuedEvent;
    List_GetDataAtPosition(&TASKSTATE->garbageTasksPushedSyncEvents, &garbageTasksQueuedEvent, taskHandlerID);

    HANDLE gamestateTasksCompleteEvent;
    List_GetDataAtPosition(&TASKSTATE->gamestateTasksCompleteSyncEvents, &gamestateTasksCompleteEvent, taskHandlerID);

    while (!SCREEN->exiting)
    {
        WaitForSingleObject(tasksQueuedEvent, INFINITE);

        if (WaitForSingleObject(systemTasksQueuedEvent, 0) == WAIT_OBJECT_0)
        {
            if (Task_HandleTaskQueue(&TASKSTATE->systemTaskStack))
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
            if (Task_HandleTaskQueue(&TASKSTATE->gamestateTaskStack))
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
            if (Task_HandleTaskQueue(&TASKSTATE->garbageTaskStack))
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

__int8 Task_HandleTaskQueue(Stack *checkedTaskStack)
{
    if (checkedTaskStack->length <= 0)
    {
        return FALSE;
    }

    Task *task = NULL;
    Stack_Pop(checkedTaskStack, (void **)&task);

    if (task == NULL)
    {
        return FALSE;
    }

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
/// Do *not* use this for gamestate tasks.
/// They have an extra step and instead should use Task_PushGamestateTask(Task *);
/// @param task The Task to queue.
static void Task_PushTask(Stack *taskStack, List *syncEvents, Task *task)
{
    if (task->task == NULL)
    {
        return;
    }

    Stack_Push(taskStack, task);

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
/// They have an extra step and instead should use Task_PushGamestateTask(Task *);
/// @param taskList The list of Task to queue.
static void Task_PushTasks(Stack *taskStack, List *syncEvents, List *taskList)
{
    ListIterator listIterator;
    ListIterator_Init(&listIterator, taskList);
    Task *task;
    while (ListIterator_Next(&listIterator, (void **)&task))
    {
        if (task->task == NULL)
        {
            continue;
        }

        Stack_Push(taskStack, task);
    }

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

void Task_PushSystemTask(Task *task)
{
    Task_PushTask(&TASKSTATE->systemTaskStack, &TASKSTATE->systemTasksPushedSyncEvents, task);
}

void Task_PushSystemTasks(List *taskList)
{
    Task_PushTasks(&TASKSTATE->systemTaskStack, &TASKSTATE->systemTasksPushedSyncEvents, taskList);
}

void Task_PushGamestateTask(Task *task)
{
    ListIterator gamestateTasksCompleteEventIterator;
    ListIterator_Init(&gamestateTasksCompleteEventIterator, &TASKSTATE->gamestateTasksCompleteSyncEvents);
    HANDLE gamestateTasksCompleteEvent;
    while (ListIterator_Next(&gamestateTasksCompleteEventIterator, &gamestateTasksCompleteEvent))
    {
        ResetEvent(gamestateTasksCompleteEvent);
    }

    Task_PushTask(&TASKSTATE->gamestateTaskStack, &TASKSTATE->gamestateTasksPushedSyncEvents, task);
}

void Task_PushGamestateTasks(List *taskList)
{
    ListIterator gamestateTasksCompleteEventIterator;
    ListIterator_Init(&gamestateTasksCompleteEventIterator, &TASKSTATE->gamestateTasksCompleteSyncEvents);
    HANDLE gamestateTasksCompleteEvent;
    while (ListIterator_Next(&gamestateTasksCompleteEventIterator, &gamestateTasksCompleteEvent))
    {
        ResetEvent(gamestateTasksCompleteEvent);
    }

    Task_PushTasks(&TASKSTATE->gamestateTaskStack, &TASKSTATE->gamestateTasksPushedSyncEvents, taskList);
}

void Task_PushGarbageTask(Task *task)
{
    Task_PushTask(&TASKSTATE->garbageTaskStack, &TASKSTATE->garbageTasksPushedSyncEvents, task);
}

void Task_PushGarbageTasks(List *taskList)
{
    Task_PushTasks(&TASKSTATE->garbageTaskStack, &TASKSTATE->garbageTasksPushedSyncEvents, taskList);
}
