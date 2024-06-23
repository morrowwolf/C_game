
#include "list_iterator_thread.h"

void ListIteratorThread_Init(ListIteratorThread *listIteratorThread, List *listToIterate)
{
    ZeroMemory(listIteratorThread, sizeof(ListIteratorThread));

    listIteratorThread->listToIterate = listToIterate;
    InitializeCriticalSection(&listIteratorThread->criticalSection);
}

void ListIteratorThread_Destroy(ListIteratorThread *listIteratorThread)
{
    DeleteCriticalSection(&listIteratorThread->criticalSection);
}

short ListIteratorThread_Next(ListIteratorThread *listIteratorThread, void **data)
{
    EnterCriticalSection(&listIteratorThread->criticalSection);

    if (listIteratorThread->currentListElement == NULL)
    {
        if (listIteratorThread->iterationStarted == TRUE)
        {
            LeaveCriticalSection(&listIteratorThread->criticalSection);
            return FALSE;
        }

        listIteratorThread->currentListElement = listIteratorThread->listToIterate->head;
        listIteratorThread->currentIteration = 0;
        listIteratorThread->iterationStarted = TRUE;
    }
    else
    {
        listIteratorThread->currentListElement = listIteratorThread->currentListElement->next;
        listIteratorThread->currentIteration += 1;
    }

    if (listIteratorThread->currentListElement == NULL)
    {
        LeaveCriticalSection(&listIteratorThread->criticalSection);
        return FALSE;
    }

    (*data) = listIteratorThread->currentListElement->data;

    LeaveCriticalSection(&listIteratorThread->criticalSection);
    return TRUE;
}
