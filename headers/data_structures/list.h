#pragma once
#ifndef DLIST_H
#define DLIST_H

#include <stdlib.h>
#include "read_write_lock.h"

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
    int length;
    void (*destroy)(void *data);
    ListElmt *head;
    ListElmt *tail;
    ReadWriteLock *readWriteLock;
} List;

void List_Init(List *list, void (*destroy)(void *data));
void List_Clear(List *list);
void List_Destroy(List *list);

int List_Insert(List *list, const void *data);
int List_InsertAt(List *list, const void *data, int position);
int List_InsertNext(List *list, ListElmt *element, const void *data);
int List_InsertPrevious(List *list, ListElmt *element, const void *data);

int List_RemovePosition(List *list, int position);
int List_RemoveElement(List *list, ListElmt *element);
int List_RemoveElementWithMatchingData(List *list, void *data);

int List_GetElementPosition(List *list, ListElmt *element);
int List_GetDataPosition(List *list, void *data);
int List_GetElementAtPosition(List *list, ListElmt **element, int position);
int List_GetElementWithMatchingData(List *list, ListElmt **element, void *data);

void List_FreeOnRemove(void *data);

#endif
