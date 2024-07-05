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

__int8 Task_HandleTaskQueue(Stack *checkedTaskStack);

void Task_PushSystemTask(Task *task);
void Task_PushSystemTasks(List *list);

void Task_PushGamestateTask(Task *task);
void Task_PushGamestateTasks(List *list);

void Task_PushGarbageTask(Task *task);
void Task_PushGarbageTasks(List *list);

#endif
