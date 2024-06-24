#pragma once
#ifndef READ_WRITE_LOCK_PRIORITY_H_
#define READ_WRITE_LOCK_PRIORITY_H_

#include "read_write_lock.h"
#include "signal.h"

typedef struct ReadWriteLockPriority
{
    union
    {
        // NOLINTNEXTLINE
        struct ReadWriteLock;
        ReadWriteLock readWriteLock;
    };

    Signal prioritySignal;
} ReadWriteLockPriority;

typedef ReadWriteLockPriority RWLP_List;

void ReadWriteLockPriority_Init(ReadWriteLockPriority *readWriteLockPriority, void *protectedData);
void ReadWriteLockPriority_Destroy(ReadWriteLockPriority *readWriteLockPriority);

void ReadWriteLockPriority_GetWritePermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData);
__int8 ReadWriteLockPriority_TryGetWritePermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData);
void ReadWriteLockPriority_ReleaseWritePermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData);

void ReadWriteLockPriority_GetPriorityReadPermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData);

void ReadWriteLockPriority_GetReadPermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData);
void ReadWriteLockPriority_ReleaseReadPermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData);

#endif
