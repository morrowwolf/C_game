#pragma once
#ifndef READ_WRITE_LOCK_H_
#define READ_WRITE_LOCK_H_

// #define DEBUG_SEMAPHORES 1

#include <Windows.h>
#ifdef DEBUG_SEMAPHORES
#include <tchar.h>
#include <stdio.h>
#endif

typedef enum
{
    ReadWriteLock_Write,
    ReadWriteLock_Read
} ReadWriteLock_Type;

typedef struct
{
#define MAX_WRITERS 1
    HANDLE writeSemaphore;

#define MAX_READERS 20
    HANDLE readSemaphore;

} ReadWriteLock;

void ReadWriteLock_Init(ReadWriteLock **);
void ReadWriteLock_Destroy(ReadWriteLock *);

void ReadWriteLock_GetWritePermission(ReadWriteLock *);
void ReadWriteLock_ReleaseWritePermission(ReadWriteLock *);

void ReadWriteLock_GetReadPermission(ReadWriteLock *);
void ReadWriteLock_ReleaseReadPermission(ReadWriteLock *);

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
