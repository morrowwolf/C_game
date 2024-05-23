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
// TODO: Transfer all ListElmt iterators to use this type instead

void ListIterator_Init(ListIterator **, List *, ReadWriteLock_Type);
void ListIterator_Destroy(ListIterator *);

short ListIterator_Next(ListIterator *, void **);
short ListIterator_Prev(ListIterator *, void **);

void ListIterator_GetHead(ListIterator *, void **);
void ListIterator_GetTail(ListIterator *, void **);

short ListIterator_AtHead(ListIterator *);
short ListIterator_AtTail(ListIterator *);

#endif
