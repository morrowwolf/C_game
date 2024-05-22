
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
#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(readWriteLock);
#endif

    WaitForSingleObject(readWriteLock->writeSemaphore, INFINITE);
    int i;
    for (i = 0; i < MAX_READERS; i++)
    {
        WaitForSingleObject(readWriteLock->readSemaphore, INFINITE);
    }

#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(readWriteLock);
#endif
}

void ReadWriteLock_ReleaseWritePermission(ReadWriteLock *readWriteLock)
{
#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(readWriteLock);
#endif

    ReleaseSemaphore(readWriteLock->readSemaphore, MAX_READERS, NULL);
    ReleaseSemaphore(readWriteLock->writeSemaphore, MAX_WRITERS, NULL);

#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(readWriteLock);
#endif
}

void ReadWriteLock_GetReadPermission(ReadWriteLock *readWriteLock)
{
#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(readWriteLock);
#endif

    WaitForSingleObject(readWriteLock->writeSemaphore, INFINITE);
    WaitForSingleObject(readWriteLock->readSemaphore, INFINITE);
    ReleaseSemaphore(readWriteLock->writeSemaphore, 1, NULL);

#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(readWriteLock);
#endif
}

void ReadWriteLock_ReleaseReadPermission(ReadWriteLock *readWriteLock)
{
#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(readWriteLock);
#endif

    ReleaseSemaphore(readWriteLock->readSemaphore, 1, NULL);

#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(readWriteLock);
#endif
}

#ifdef DEBUG_SEMAPHORES
void SemaphoreDebugOutput(ReadWriteLock *readWriteLock)
{
    _NtQuerySemaphore NtQuerySemaphore;
    SEMAPHORE_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;

    NtQuerySemaphore = (_NtQuerySemaphore)GetProcAddress(GetModuleHandle("ntdll.dll"),
                                                         "NtQuerySemaphore");

    if (NtQuerySemaphore)
    {
        Status = NtQuerySemaphore(readWriteLock->readSemaphore, 0,
                                  &BasicInfo, sizeof(SEMAPHORE_BASIC_INFORMATION), NULL);

        if (Status == ERROR_SUCCESS)
        {
            TCHAR buffer[64];

            _stprintf(buffer, TEXT("ID: %p CurrentCount: %lu\n"), readWriteLock->readSemaphore, BasicInfo.CurrentCount);

            OutputDebugString(buffer);
        }

        Status = NtQuerySemaphore(readWriteLock->writeSemaphore, 0,
                                  &BasicInfo, sizeof(SEMAPHORE_BASIC_INFORMATION), NULL);

        if (Status == ERROR_SUCCESS)
        {
            TCHAR buffer[64];

            _stprintf(buffer, TEXT("ID: %p CurrentCount: %lu\n"), readWriteLock->writeSemaphore, BasicInfo.CurrentCount);

            OutputDebugString(buffer);
        }
    }
}
#endif
