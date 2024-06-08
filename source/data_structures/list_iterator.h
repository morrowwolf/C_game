#pragma once
#ifndef LIST_ITERATOR_H_
#define LIST_ITERATOR_H_

#include "list.h"

typedef struct
{
    List *listToIterate;
    ListElmt *currentListElement;
    unsigned int currentIteration;
} ListIterator;

void ListIterator_Init(ListIterator *, List *);

short ListIterator_Next(ListIterator *, void **);
short ListIterator_Prev(ListIterator *, void **);

void ListIterator_GetHead(ListIterator *, void **);
void ListIterator_GetTail(ListIterator *, void **);

short ListIterator_AtHead(ListIterator *);
short ListIterator_AtTail(ListIterator *);

#endif
