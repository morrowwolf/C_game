
#include "list_iterator.h"

void ListIterator_Init(ListIterator *listIterator, List *listToIterate)
{
    ZeroMemory(listIterator, sizeof(ListIterator));

    listIterator->listToIterate = listToIterate;
}

short ListIterator_Next(ListIterator *listIterator, void **data)
{
    if (listIterator->currentListElement == NULL)
    {
        listIterator->currentListElement = listIterator->listToIterate->head;
        listIterator->currentIteration = 0;
    }
    else
    {
        listIterator->currentListElement = listIterator->currentListElement->next;
        listIterator->currentIteration = listIterator->currentIteration + 1;
    }

    if (listIterator->currentListElement == NULL)
    {
        return FALSE;
    }

    (*data) = listIterator->currentListElement->data;

    return TRUE;
}

short ListIterator_Prev(ListIterator *listIterator, void **data)
{
    if (listIterator->currentListElement == NULL)
    {
        listIterator->currentListElement = listIterator->listToIterate->tail;
        listIterator->currentIteration = listIterator->listToIterate->length - 1;
    }
    else
    {
        listIterator->currentListElement = listIterator->currentListElement->prev;
        listIterator->currentIteration = listIterator->currentIteration - 1;
        if (listIterator->currentListElement == NULL)
        {
            return FALSE;
        }
    }

    (*data) = listIterator->currentListElement->data;

    return TRUE;
}

void ListIterator_GetHead(ListIterator *listIterator, void **data)
{
    (*data) = listIterator->listToIterate->head->data;
}

void ListIterator_GetTail(ListIterator *listIterator, void **data)
{
    (*data) = listIterator->listToIterate->tail->data;
}

short ListIterator_AtHead(ListIterator *listIterator)
{
    if (listIterator->currentIteration == 0)
    {
        return TRUE;
    }

    return FALSE;
}

short ListIterator_AtTail(ListIterator *listIterator)
{
    if (listIterator->currentIteration == listIterator->listToIterate->length - 1)
    {
        return TRUE;
    }

    return FALSE;
}
