#pragma once
#ifndef ATOMIC_STACK_H
#define ATOMIC_STACK_H

#include <windows.h>

void MemoryManager_AllocateMemory(void **passbackPointer, unsigned int size, unsigned long flags);
void MemoryManager_DeallocateMemory(void **memoryPointer, unsigned int size);

typedef struct StackElmt
{
    void *data;
    struct StackElmt *next;
} StackElmt;

typedef struct Stack
{
    unsigned long length;
    void (*destroy)(void *data);
    StackElmt *head;

    CRITICAL_SECTION stackCriticalSection;
} Stack;

void Stack_Init(Stack *stack, void (*destroy)(void *data));

void Stack_Clear(Stack *stack);

void Stack_Push(Stack *stack, void *data);
void Stack_Pop(Stack *stack, void **data);

#endif
