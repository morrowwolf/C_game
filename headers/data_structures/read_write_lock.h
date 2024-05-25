#pragma once
#ifndef READ_WRITE_LOCK_H_
#define READ_WRITE_LOCK_H_

// #define DEBUG_SEMAPHORES 1

#include <Windows.h>
#include "list.h"
#include "list_iterator.h"

#ifdef DEBUG_SEMAPHORES
#include <tchar.h>
#include <stdio.h>
#endif

typedef struct
{
#define MAX_WRITERS 1
    HANDLE writeSemaphore;

#define MAX_READERS 20
    HANDLE readSemaphore;

    void *protectedData;
} ReadWriteLock;

typedef ReadWriteLock RWL_List;
typedef ReadWriteLock RWL_Point;

void ReadWriteLock_Init(ReadWriteLock *, void *);
void ReadWriteLock_Destroy(ReadWriteLock *);

void ReadWriteLock_GetWritePermission(ReadWriteLock *, void **);
void ReadWriteLock_ReleaseWritePermission(ReadWriteLock *, void **);

void ReadWriteLock_GetReadPermission(ReadWriteLock *, void **);
void ReadWriteLock_ReleaseReadPermission(ReadWriteLock *, void **);

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

short ReadWriteLock_GetMultiplePermissions(List *, unsigned int);
void ReadWriteLock_ReleaseMultiplePermissions(List *);
void List_DestroyReadWriteLockPermissionRequestOnRemove(void *);

#ifdef DEBUG_SEMAPHORES
typedef LONG NTSTATUS;

typedef NTSTATUS(NTAPI *_NtQuerySemaphore)(
    HANDLE SemaphoreHandle,
    DWORD SemaphoreInformationClass,
    PVOID SemaphoreInformation,
    ULONG SemaphoreInformationLength,
    PULONG ReturnLength OPTIONAL);

typedef struct _SEMAPHORE_BASIC_INFORMATION
{
    ULONG CurrentCount;
    ULONG MaximumCount;
} SEMAPHORE_BASIC_INFORMATION;

void SemaphoreDebugOutput(ReadWriteLock *);
#endif

#endif
