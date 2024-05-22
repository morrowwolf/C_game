
#include "../../headers/data_structures/list_iterator.h"

void ListIterator_Init(ListIterator **listIterator, List *listToIterate, ReadWriteLock_Type type)
{

    *listIterator = malloc(sizeof(ListIterator));
    ZeroMemory(*listIterator, sizeof(ListIterator));

    (*listIterator)->listToIterate = listToIterate;
    (*listIterator)->type = type;

    switch (type)
    {
    case ReadWriteLock_Write:
        ReadWriteLock_GetWritePermission(listToIterate->readWriteLock);
        break;

    case ReadWriteLock_Read:
        ReadWriteLock_GetReadPermission(listToIterate->readWriteLock);
        break;

    default:
        abort();
        break;
    }
}

void ListIterator_Destroy(ListIterator *listIterator)
{
    switch (listIterator->type)
    {
    case ReadWriteLock_Write:
        ReadWriteLock_ReleaseWritePermission(listIterator->listToIterate->readWriteLock);
        break;

    case ReadWriteLock_Read:
        ReadWriteLock_ReleaseReadPermission(listIterator->listToIterate->readWriteLock);
        break;

    default:
        abort();
        break;
    }
    free(listIterator);
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
