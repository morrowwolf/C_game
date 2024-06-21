
#include "read_write_lock_priority.h"

void ReadWriteLockPriority_Init(ReadWriteLockPriority *readWriteLockPriority, void *protectedData)
{
    ReadWriteLock_Init(&readWriteLockPriority->readWriteLock, protectedData);

    readWriteLockPriority->priorityEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

void ReadWriteLockPriority_Destroy(ReadWriteLockPriority *readWriteLockPriority)
{
    ReadWriteLock_Destroy(&readWriteLockPriority->readWriteLock);

    CloseHandle(readWriteLockPriority->priorityEvent);
}

void ReadWriteLockPriority_GetWritePermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData)
{
#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(&readWriteLockPriority->readWriteLock);
#endif

    while (TRUE)
    {
        WaitForSingleObject(readWriteLockPriority->writeSemaphore, INFINITE);

        unsigned int i;
        for (i = 0; i < MAX_READERS; i++)
        {
            if (WaitForMultipleObjects(2, (HANDLE[]){readWriteLockPriority->readSemaphore, readWriteLockPriority->priorityEvent}, FALSE, INFINITE) != WAIT_OBJECT_0)
            {
                while (i > 0)
                {
                    ReleaseSemaphore(readWriteLockPriority->readSemaphore, 1, NULL);
                    i--;
                }
                ReleaseSemaphore(readWriteLockPriority->writeSemaphore, 1, NULL);
                break;
            }
        }

        if (i == MAX_READERS)
        {
            break;
        }
    }

    (*protectedData) = readWriteLockPriority->protectedData;

#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(&readWriteLockPriority->readWriteLock);
#endif
}

__int8 ReadWriteLockPriority_GetWritePermissionTimeout(ReadWriteLockPriority *readWriteLockPriority, void **protectedData, unsigned int timeout)
{
#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(&readWriteLockPriority->readWriteLock);
#endif

    while (TRUE)
    {
        if (WaitForSingleObject(readWriteLockPriority->writeSemaphore, timeout) != WAIT_OBJECT_0)
        {
            return FALSE;
        }

        __int8 breakFor = FALSE;
        unsigned int i;
        for (i = 0; i < MAX_READERS; i++)
        {
            switch (WaitForMultipleObjects(2, (HANDLE[]){readWriteLockPriority->readSemaphore, readWriteLockPriority->priorityEvent}, FALSE, timeout))
            {
            case WAIT_OBJECT_0:
                continue;
            case WAIT_OBJECT_0 + 1:
                while (i > 0)
                {
                    ReleaseSemaphore(readWriteLockPriority->readSemaphore, 1, NULL);
                    i--;
                }
                ReleaseSemaphore(readWriteLockPriority->writeSemaphore, 1, NULL);
                breakFor = TRUE;
                break;
            case WAIT_TIMEOUT:
                while (i > 0)
                {
                    ReleaseSemaphore(readWriteLockPriority->readSemaphore, 1, NULL);
                    i--;
                }
                ReleaseSemaphore(readWriteLockPriority->writeSemaphore, 1, NULL);
                return FALSE;
            default:
                abort();
            }

            if (breakFor == TRUE)
            {
                break;
            }
        }

        if (i == MAX_READERS)
        {
            break;
        }
    }

    (*protectedData) = readWriteLockPriority->protectedData;

    return TRUE;
}

void ReadWriteLockPriority_ReleaseWritePermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData)
{
#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(&readWriteLockPriority->readWriteLock);
#endif

    (*protectedData) = NULL;

    ReleaseSemaphore(readWriteLockPriority->readSemaphore, MAX_READERS, NULL);
    ReleaseSemaphore(readWriteLockPriority->writeSemaphore, MAX_WRITERS, NULL);

#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(&readWriteLockPriority->readWriteLock);
#endif
}

void ReadWriteLockPriority_GetPriorityReadPermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData)
{
#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(&readWriteLockPriority->readWriteLock);
#endif

    SetEvent(readWriteLockPriority->priorityEvent);

    WaitForSingleObject(readWriteLockPriority->writeSemaphore, INFINITE);
    WaitForSingleObject(readWriteLockPriority->readSemaphore, INFINITE);
    ReleaseSemaphore(readWriteLockPriority->writeSemaphore, 1, NULL);

    ResetEvent(readWriteLockPriority->priorityEvent);

    (*protectedData) = readWriteLockPriority->protectedData;

#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(&readWriteLockPriority->readWriteLock);
#endif
}

void ReadWriteLockPriority_GetReadPermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData)
{
#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(&readWriteLockPriority->readWriteLock);
#endif

    __int8 complete = FALSE;
    while (!complete)
    {
        WaitForSingleObject(readWriteLockPriority->writeSemaphore, INFINITE);

        switch (WaitForMultipleObjects(2, (HANDLE[]){readWriteLockPriority->readSemaphore, readWriteLockPriority->priorityEvent}, FALSE, INFINITE))
        {
        case WAIT_OBJECT_0:
            ReleaseSemaphore(readWriteLockPriority->writeSemaphore, 1, NULL);
            complete = TRUE;
            break;
        default:
            ReleaseSemaphore(readWriteLockPriority->writeSemaphore, 1, NULL);
            continue;
        }
    }

    (*protectedData) = readWriteLockPriority->protectedData;

#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(&readWriteLockPriority->readWriteLock);
#endif
}

void ReadWriteLockPriority_ReleaseReadPermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData)
{
#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(&readWriteLockPriority->readWriteLock);
#endif

    (*protectedData) = NULL;

    ReleaseSemaphore(readWriteLockPriority->readSemaphore, 1, NULL);

#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(&readWriteLockPriority->readWriteLock);
#endif
}
