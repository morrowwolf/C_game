
#include "read_write_lock.h"

void ReadWriteLock_Init(ReadWriteLock *readWriteLock, void *data)
{
    ZeroMemory(readWriteLock, sizeof(ReadWriteLock));

    readWriteLock->writeSemaphore = CreateSemaphore(NULL, MAX_WRITERS, MAX_WRITERS, NULL);
    readWriteLock->readSemaphore = CreateSemaphore(NULL, MAX_READERS, MAX_READERS, NULL);
    readWriteLock->protectedData = data;
}

/// @brief Closes the semaphore handles and frees the memory for a ReadWriteLock.
/// Insure you handle the protected data and have a write lock before calling this.
/// @param readWriteLock The ReadWriteLock to be destroyed
void ReadWriteLock_Destroy(ReadWriteLock *readWriteLock)
{
    CloseHandle(readWriteLock->writeSemaphore);
    CloseHandle(readWriteLock->readSemaphore);
}

void ReadWriteLock_GetWritePermission(ReadWriteLock *readWriteLock, void **protectedData)
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

    (*protectedData) = readWriteLock->protectedData;

#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(readWriteLock);
#endif
}

short ReadWriteLock_GetWritePermissionTimeout(ReadWriteLock *readWriteLock, void **protectedData, unsigned int timeout)
{
#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(readWriteLock);
#endif

    if (WaitForSingleObject(readWriteLock->writeSemaphore, timeout) != WAIT_OBJECT_0)
    {
        return FALSE;
    }

    int i;
    for (i = 0; i < MAX_READERS; i++)
    {
        if (WaitForSingleObject(readWriteLock->readSemaphore, timeout) != WAIT_OBJECT_0)
        {
            while (i > 0)
            {
                ReleaseSemaphore(readWriteLock->readSemaphore, 1, NULL);
                i--;
            }
            ReleaseSemaphore(readWriteLock->writeSemaphore, 1, NULL);
            return FALSE;
        }
    }

    (*protectedData) = readWriteLock->protectedData;
    return TRUE;

#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(readWriteLock);
#endif
}

void ReadWriteLock_ReleaseWritePermission(ReadWriteLock *readWriteLock, void **protectedData)
{
#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(readWriteLock);
#endif

    (*protectedData) = NULL;

    ReleaseSemaphore(readWriteLock->readSemaphore, MAX_READERS, NULL);
    ReleaseSemaphore(readWriteLock->writeSemaphore, MAX_WRITERS, NULL);

#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(readWriteLock);
#endif
}

void ReadWriteLock_GetReadPermission(ReadWriteLock *readWriteLock, void **protectedData)
{
#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(readWriteLock);
#endif

    WaitForSingleObject(readWriteLock->writeSemaphore, INFINITE);
    WaitForSingleObject(readWriteLock->readSemaphore, INFINITE);
    ReleaseSemaphore(readWriteLock->writeSemaphore, 1, NULL);

    (*protectedData) = readWriteLock->protectedData;

#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(readWriteLock);
#endif
}

short ReadWriteLock_GetReadPermissionTimeout(ReadWriteLock *readWriteLock, void **protectedData, unsigned int timeout)
{
#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(readWriteLock);
#endif

    if (WaitForSingleObject(readWriteLock->writeSemaphore, timeout) != WAIT_OBJECT_0)
    {
        return FALSE;
    }

    if (WaitForSingleObject(readWriteLock->readSemaphore, timeout) != WAIT_OBJECT_0)
    {
        return FALSE;
    }

    ReleaseSemaphore(readWriteLock->writeSemaphore, 1, NULL);

    (*protectedData) = readWriteLock->protectedData;
    return TRUE;

#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(readWriteLock);
#endif
}

void ReadWriteLock_ReleaseReadPermission(ReadWriteLock *readWriteLock, void **protectedData)
{
#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(readWriteLock);
#endif

    (*protectedData) = NULL;

    ReleaseSemaphore(readWriteLock->readSemaphore, 1, NULL);

#ifdef DEBUG_SEMAPHORES
    SemaphoreDebugOutput(readWriteLock);
#endif
}

short ReadWriteLock_GetMultiplePermissions(ReadWriteLock_PermissionRequest *pointerToPermissionRequests,
                                           unsigned int amountOfPermissionRequests,
                                           unsigned int timeout)
{

    ReadWriteLock_PermissionRequest *permissionRequest;
    for (unsigned int i = 0; i < amountOfPermissionRequests; i++)
    {
        permissionRequest = &pointerToPermissionRequests[i];
        if (permissionRequest->permissionType == ReadWriteLock_Read)
        {
            if (ReadWriteLock_GetReadPermissionTimeout(permissionRequest->readWriteLock, &permissionRequest->returnedData, timeout) == FALSE)
            {
                for (unsigned int j = 1; j <= i; j++)
                {
                    permissionRequest = &pointerToPermissionRequests[i - j];
                    if (permissionRequest->permissionType == ReadWriteLock_Read)
                    {
                        ReadWriteLock_ReleaseReadPermission(permissionRequest->readWriteLock, &permissionRequest->returnedData);
                    }
                    else if (permissionRequest->permissionType == ReadWriteLock_Write)
                    {
                        ReadWriteLock_ReleaseWritePermission(permissionRequest->readWriteLock, &permissionRequest->returnedData);
                    }
                }
                return FALSE;
            }
        }
        else if (permissionRequest->permissionType == ReadWriteLock_Write)
        {
            if (ReadWriteLock_GetWritePermissionTimeout(permissionRequest->readWriteLock, &permissionRequest->returnedData, timeout) == FALSE)
            {
                for (unsigned int j = 1; j <= i; j++)
                {
                    permissionRequest = &pointerToPermissionRequests[i - j];
                    if (permissionRequest->permissionType == ReadWriteLock_Read)
                    {
                        ReadWriteLock_ReleaseReadPermission(permissionRequest->readWriteLock, &permissionRequest->returnedData);
                    }
                    else if (permissionRequest->permissionType == ReadWriteLock_Write)
                    {
                        ReadWriteLock_ReleaseWritePermission(permissionRequest->readWriteLock, &permissionRequest->returnedData);
                    }
                }
                return FALSE;
            }
        }
        else
        {
            abort();
        }
    }

    return TRUE;
}

void ReadWriteLock_ReleaseMultiplePermissions(ReadWriteLock_PermissionRequest *pointerToPermissionRequests,
                                              unsigned int amountOfPermissionRequests)
{
    ReadWriteLock_PermissionRequest *permissionRequest;
    for (unsigned int i = 0; i < amountOfPermissionRequests; i++)
    {
        permissionRequest = &pointerToPermissionRequests[i];
        if (permissionRequest->permissionType == ReadWriteLock_Read)
        {
            ReadWriteLock_ReleaseReadPermission(permissionRequest->readWriteLock, &permissionRequest->returnedData);
        }
        else if (permissionRequest->permissionType == ReadWriteLock_Write)
        {
            ReadWriteLock_ReleaseWritePermission(permissionRequest->readWriteLock, &permissionRequest->returnedData);
        }
        else
        {
            abort();
        }
    }
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

        TCHAR buffer[256];

        if (Status == ERROR_SUCCESS)
        {
            _stprintf(buffer, TEXT("Thread ID: %lu Read Semaphore ID: %p CurrentCount: %lu"), GetCurrentThreadId(), readWriteLock->readSemaphore, BasicInfo.CurrentCount);
        }

        Status = NtQuerySemaphore(readWriteLock->writeSemaphore, 0,
                                  &BasicInfo, sizeof(SEMAPHORE_BASIC_INFORMATION), NULL);

        if (Status == ERROR_SUCCESS)
        {
            _stprintf(buffer, TEXT("%s Write Semaphore ID: %p CurrentCount: %lu\n"), buffer, readWriteLock->writeSemaphore, BasicInfo.CurrentCount);

            OutputDebugString(buffer);
        }
    }
}
#endif
