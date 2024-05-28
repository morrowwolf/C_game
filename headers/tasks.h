#pragma once
#ifndef TASKS_H_
#define TASKS_H_

#include "globals.h"

typedef struct
{
    void (*task)(void *);
    void *taskArguments;
} Task;

typedef struct
{
    unsigned int taskHandlerID;
} TaskHandlerArgs;

DWORD WINAPI TaskHandler(LPVOID);

void Task_QueueTask(Task *);
void Task_QueueTasks(List *);

extern TaskState *TASKSTATE;

#endif
