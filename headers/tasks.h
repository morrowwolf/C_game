#pragma once
#ifndef TASKS_H_
#define TASKS_H_

#include "globals.h"

typedef struct
{
    void (*task)(void *);
    void *taskArgument;
} Task;

typedef struct
{
    unsigned int taskHandlerID;
} TaskHandlerArgs;

DWORD WINAPI TaskHandler(LPVOID);

__int8 Task_HandleTaskQueue(RWL_List *checkedTaskQueue);

void Task_QueueTask(RWL_List *queueingTaskQueue, List *syncEvents, Task *task);
void Task_QueueTasks(RWL_List *queueingTaskQueue, List *syncEvents, List *list);

extern TaskState *TASKSTATE;

#endif
