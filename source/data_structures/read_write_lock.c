
#include "../../headers/data_structures/read_write_lock.h"

void ReadWriteLock_Init(ReadWriteLock **readWriteLock)
{
    *readWriteLock = malloc(sizeof(ReadWriteLock));
    ZeroMemory(*readWriteLock, sizeof(ReadWriteLock));

    (*readWriteLock)->writeSemaphore = CreateSemaphore(NULL, MAX_WRITERS, MAX_WRITERS, NULL);
    (*readWriteLock)->readSemaphore = CreateSemaphore(NULL, MAX_READERS, MAX_READERS, NULL);
}

void ReadWriteLock_Destroy(ReadWriteLock *readWriteLock)
{
    CloseHandle(readWriteLock->writeSemaphore);
    CloseHandle(readWriteLock->readSemaphore);
    free(readWriteLock);
}

void ReadWriteLock_GetWritePermission(ReadWriteLock *readWriteLock)
{
    WaitForSingleObject(readWriteLock->writeSemaphore, INFINITE);
    int i;
    for (i = 0; i < MAX_READERS; i++)
    {
        WaitForSingleObject(readWriteLock->readSemaphore, INFINITE);
    }
}

void ReadWriteLock_ReleaseWritePermission(ReadWriteLock *readWriteLock)
{
    ReleaseSemaphore(readWriteLock->readSemaphore, MAX_READERS, NULL);
    ReleaseSemaphore(readWriteLock->readSemaphore, MAX_WRITERS, NULL);
}

void ReadWriteLock_GetReadPermission(ReadWriteLock *readWriteLock)
{
    WaitForSingleObject(readWriteLock->writeSemaphore, INFINITE);
    WaitForSingleObject(readWriteLock->readSemaphore, INFINITE);
    ReleaseSemaphore(readWriteLock->writeSemaphore, 1, NULL);
}

void ReadWriteLock_ReleaseReadPermission(ReadWriteLock *readWriteLock)
{
    ReleaseSemaphore(readWriteLock->readSemaphore, 1, NULL);
}
