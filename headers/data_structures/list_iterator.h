#pragma once
#ifndef LIST_ITERATOR_H_
#define LIST_ITERATOR_HH

#include "list.h"

typedef struct
{
    List *listToIterate;
    ListElmt *currentListElement;
    unsigned int currentIteration;
    ReadWriteLock_Type type;
} ListIterator;

void ListIterator_Init(ListIterator **, List *, ReadWriteLock_Type);
void ListIterator_Destroy(ListIterator *);

short ListIterator_Next(ListIterator *listIterator, void **data);
short ListIterator_Prev(ListIterator *listIterator, void **data);

#endif
