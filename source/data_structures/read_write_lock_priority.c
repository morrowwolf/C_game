
#include "read_write_lock_priority.h"

void ReadWriteLockPriority_Init(ReadWriteLockPriority *readWriteLockPriority, void *protectedData)
{
    ReadWriteLock_Init(&readWriteLockPriority->readWriteLock, protectedData);

    Signal_Init(&readWriteLockPriority->prioritySignal, SIGNAL_OFF);
}

void ReadWriteLockPriority_Destroy(ReadWriteLockPriority *readWriteLockPriority)
{
    ReadWriteLock_Destroy(&readWriteLockPriority->readWriteLock);
}

void ReadWriteLockPriority_GetWritePermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData)
{
    do
    {
        Signal_WaitForSignal(&readWriteLockPriority->prioritySignal, SIGNAL_OFF);
    } while (!TryAcquireSRWLockExclusive(&readWriteLockPriority->SRWLock));

    (*protectedData) = readWriteLockPriority->protectedData;
}

__int8 ReadWriteLockPriority_TryGetWritePermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData)
{
    if (!Signal_TrySignal(&readWriteLockPriority->prioritySignal, SIGNAL_OFF))
    {
        return FALSE;
    }

    if (!TryAcquireSRWLockExclusive(&readWriteLockPriority->SRWLock))
    {
        return FALSE;
    }

    (*protectedData) = readWriteLockPriority->protectedData;

    return TRUE;
}

void ReadWriteLockPriority_ReleaseWritePermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData)
{
    (*protectedData) = NULL;

    ReleaseSRWLockExclusive(&readWriteLockPriority->SRWLock);
}

void ReadWriteLockPriority_GetPriorityReadPermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData)
{
    Signal_SetSignal(&readWriteLockPriority->prioritySignal, SIGNAL_ON);

    AcquireSRWLockShared(&readWriteLockPriority->SRWLock);

    Signal_SetSignal(&readWriteLockPriority->prioritySignal, SIGNAL_OFF);

    (*protectedData) = readWriteLockPriority->protectedData;
}

void ReadWriteLockPriority_GetReadPermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData)
{
    do
    {
        Signal_WaitForSignal(&readWriteLockPriority->prioritySignal, SIGNAL_OFF);
    } while (!TryAcquireSRWLockShared(&readWriteLockPriority->SRWLock));

    (*protectedData) = readWriteLockPriority->protectedData;
}

void ReadWriteLockPriority_ReleaseReadPermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData)
{
    (*protectedData) = NULL;

    ReleaseSRWLockShared(&readWriteLockPriority->SRWLock);
}
