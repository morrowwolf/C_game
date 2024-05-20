#pragma once
#ifndef DLIST_H
#define DLIST_H

#include <stdlib.h>

typedef struct ListElmt_
{
    void *data;
    struct ListElmt_ *prev;
    struct ListElmt_ *next;
} ListElmt;

typedef struct List_
{
    int length;
    void (*destroy)(void *data);
    ListElmt *head;
    ListElmt *tail;
} List;

void list_init(List *list, void (*destroy)(void *data));
void list_clear(List *list);

int list_insert(List *list, const void *data);
int list_insert_at(List *list, const void *data, int position);
int list_ins_next(List *list, ListElmt *element, const void *data);
int list_insert_prev(List *list, ListElmt *element, const void *data);

int list_remove_position(List *list, int position);
int list_remove_element(List *list, ListElmt *element);
int list_remove_element_with_matching_data(List *list, void *data);

int list_get_element_position(List *list, ListElmt *element);
int list_get_data_position(List *list, void *data);
int list_get_element_at_position(List *list, ListElmt **element, int position);
int list_get_element_with_matching_data(List *list, ListElmt **element, void *data);

void list_free_on_remove(void *data);

#endif
