#pragma once
#ifndef READ_WRITE_LOCK_H_
#define READ_WRITE_LOCK_H_

#include <Windows.h>

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

#endif
