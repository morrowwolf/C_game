#pragma once
#ifndef READ_WRITE_LOCK_H_
#define READ_WRITE_LOCK_H_

#include <Windows.h>
#include "list.h"
#include "list_iterator.h"

typedef struct ReadWriteLock
{
    SRWLOCK SRWLock;

    void *protectedData;
} ReadWriteLock;

typedef ReadWriteLock RWL_List;
typedef ReadWriteLock RWL_Point;

void ReadWriteLock_Init(ReadWriteLock *readWriteLock, void *protectedData);
void ReadWriteLock_Destroy(ReadWriteLock *readWriteLock);

void ReadWriteLock_GetWritePermission(ReadWriteLock *readWriteLock, void **protectedData);
short ReadWriteLock_TryGetWritePermission(ReadWriteLock *readWriteLock, void **protectedData);
void ReadWriteLock_ReleaseWritePermission(ReadWriteLock *readWriteLock, void **protectedData);

void ReadWriteLock_GetReadPermission(ReadWriteLock *readWriteLock, void **protectedData);
short ReadWriteLock_TryGetReadPermission(ReadWriteLock *readWriteLock, void **protectedData);
void ReadWriteLock_ReleaseReadPermission(ReadWriteLock *readWriteLock, void **protectedData);

enum ReadWriteLock_PermissionType
{
    ReadWriteLock_Read,
    ReadWriteLock_Write
};

typedef struct
{
    enum ReadWriteLock_PermissionType permissionType;
    ReadWriteLock *readWriteLock;
    void *returnedData;
} ReadWriteLock_PermissionRequest;

short ReadWriteLock_GetMultiplePermissions(ReadWriteLock_PermissionRequest *, unsigned int);
void ReadWriteLock_ReleaseMultiplePermissions(ReadWriteLock_PermissionRequest *, unsigned int);

#endif
