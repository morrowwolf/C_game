#pragma once
#ifndef LIST_ITERATOR_ATOMIC_H_
#define LIST_ITERATOR_ATOMIC_H_

#include "list_iterator.h"

typedef struct ListIteratorThread
{
    List *listToIterate;
    ListElmt *currentListElement;

    unsigned __int32 currentIteration;
    unsigned __int32 iterationStarted;

    CRITICAL_SECTION criticalSection;
} ListIteratorThread;

void ListIteratorThread_Init(ListIteratorThread *, List *);
void ListIteratorThread_Destroy(ListIteratorThread *);

short ListIteratorThread_Next(ListIteratorThread *, void **);

#endif
