
#include "stack.h"

void Stack_Init(Stack *stack, void (*destroy)(void *data))
{
    stack->length = 0;
    stack->destroy = destroy;
    stack->head = NULL;

    InitializeCriticalSection(&stack->stackCriticalSection);
}

void Stack_Clear(Stack *stack)
{
    EnterCriticalSection(&stack->stackCriticalSection);
    void *data;
    while (stack->length > 0)
    {
        Stack_Pop(stack, &data);
    }
    LeaveCriticalSection(&stack->stackCriticalSection);
}

void Stack_Push(Stack *stack, void *data)
{
    StackElmt *newElement;
    MemoryManager_AllocateMemory((void **)&newElement, sizeof(StackElmt));
    newElement->data = data;

    EnterCriticalSection(&stack->stackCriticalSection);

    newElement->next = stack->head;

    stack->head = newElement;
    stack->length++;

    LeaveCriticalSection(&stack->stackCriticalSection);
}

void Stack_Pop(Stack *stack, void **data)
{
    EnterCriticalSection(&stack->stackCriticalSection);

    if (stack->length == 0)
    {
        (*data) = NULL;
        LeaveCriticalSection(&stack->stackCriticalSection);
        return;
    }

    StackElmt *element = stack->head;
    (*data) = element->data;

    stack->head = element->next;
    stack->length--;

    LeaveCriticalSection(&stack->stackCriticalSection);

    if (stack->destroy != NULL)
    {
        stack->destroy(element->data);
    }

    MemoryManager_DeallocateMemory((void **)&element, sizeof(StackElmt));
}
