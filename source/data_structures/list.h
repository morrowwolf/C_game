#pragma once
#ifndef DLIST_H
#define DLIST_H

#include <windows.h>

void MemoryManager_AllocateMemory(void **passbackPointer, unsigned int size);
void MemoryManager_DeallocateMemory(void **memoryPointer, unsigned int size);

typedef struct ListElmt_
{
    // Using void* to hold function pointers is apparently undefined behavior
    // So far it's been working so... if it stops working we'll need to get creative
    void *data;
    struct ListElmt_ *next;
    struct ListElmt_ *prev;
} ListElmt;

typedef struct List_
{
    unsigned int length;
    void (*destroy)(void *data);
    ListElmt *head;
    ListElmt *tail;
} List;

void List_Init(List *list, void (*destroy)(void *data));
void List_Clear(List *list);

int List_Insert(List *list, const void *data);
int List_InsertAt(List *list, const void *data, int position);
int List_InsertNext(List *list, ListElmt *element, const void *data);
int List_InsertElementNext(List *list, ListElmt *element, ListElmt *newElement);
int List_InsertPrevious(List *list, ListElmt *element, const void *data);

int List_RemovePosition(List *list, int position);
int List_RemoveElement(List *list, ListElmt *element);
int List_RemoveElementWithMatchingData(List *list, void *data);

int List_GetDataPosition(List *list, void *data);
short List_GetDataAtPosition(List *list, void **data, unsigned int position);

int List_GetElementPosition(List *list, ListElmt *element);
short List_GetElementAtPosition(List *, ListElmt **, unsigned int);
short List_GetElementWithMatchingData(List *, ListElmt **, void *);

void List_GetAsArray(List *list, void **returnedArray);

void List_FreeOnRemove(void *data);
void List_CloseHandleOnRemove(void *data);

#endif
