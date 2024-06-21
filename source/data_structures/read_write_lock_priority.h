#pragma once
#ifndef READ_WRITE_LOCK_PRIORITY_H_
#define READ_WRITE_LOCK_PRIORITY_H_

#include "read_write_lock.h"

typedef struct ReadWriteLockPriority
{
    union
    {
        // NOLINTNEXTLINE
        struct ReadWriteLock;
        ReadWriteLock readWriteLock;
    };

    HANDLE priorityEvent;
} ReadWriteLockPriority;

typedef ReadWriteLockPriority RWLP_List;

void ReadWriteLockPriority_Init(ReadWriteLockPriority *readWriteLockPriority, void *protectedData);
void ReadWriteLockPriority_Destroy(ReadWriteLockPriority *readWriteLockPriority);

void ReadWriteLockPriority_GetWritePermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData);
__int8 ReadWriteLockPriority_GetWritePermissionTimeout(ReadWriteLockPriority *readWriteLockPriority, void **protectedData, unsigned int timeout);
void ReadWriteLockPriority_ReleaseWritePermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData);

void ReadWriteLockPriority_GetPriorityReadPermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData);

void ReadWriteLockPriority_GetReadPermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData);
void ReadWriteLockPriority_ReleaseReadPermission(ReadWriteLockPriority *readWriteLockPriority, void **protectedData);

#endif
