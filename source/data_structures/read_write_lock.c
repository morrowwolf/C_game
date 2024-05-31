
#include "../../headers/data_structures/read_write_lock.h"

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

/**
 * @brief Gets read permission with a timeout from the specified ReadWriteLock.
 *
 * This function attempts to acquire read permission from the specified ReadWriteLock within the given timeout period.
 * If the read permission is acquired successfully, the protected data associated with the lock is returned through the `protectedData` parameter.
 *
 * @param readWriteLock A pointer to the ReadWriteLock from which to acquire read permission.
 * @param protectedData A pointer to a variable that will hold the protected data associated with the lock if read permission is acquired successfully.
 * @param timeout The maximum time (in milliseconds) to wait for acquiring read permission.
 *
 * @return A value indicating the result of the operation:
 *         - TRUE if read permission is acquired successfully.
 *         - FALSE if an error occurs or the timeout period is exceeded.
 */
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

// TODO: This should just use a pointer to an array of PermissionRequests and the size
//  Using a list here is overhead that is not required
short ReadWriteLock_GetMultiplePermissions(List *listOfPermissions, unsigned int timeout)
{
    ListIterator listOfPermissionsIterator;
    ListIterator_Init(&listOfPermissionsIterator, listOfPermissions);

    ReadWriteLock_PermissionRequest *permissionRequest;
    while (ListIterator_Next(&listOfPermissionsIterator, (void **)&permissionRequest))
    {
        if (permissionRequest->permissionType == ReadWriteLock_Read)
        {
            if (ReadWriteLock_GetReadPermissionTimeout(permissionRequest->readWriteLock, &permissionRequest->returnedData, timeout) == FALSE)
            {
                while (ListIterator_Prev(&listOfPermissionsIterator, (void **)&permissionRequest))
                {
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
                while (ListIterator_Prev(&listOfPermissionsIterator, (void **)&permissionRequest))
                {
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

void ReadWriteLock_ReleaseMultiplePermissions(List *listOfPermissions)
{
    ListIterator listOfPermissionsIterator;
    ListIterator_Init(&listOfPermissionsIterator, listOfPermissions);

    ReadWriteLock_PermissionRequest *permissionRequest;
    while (ListIterator_Next(&listOfPermissionsIterator, (void **)&permissionRequest))
    {
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

void List_DestroyReadWriteLockPermissionRequestOnRemove(void *data)
{
    ReadWriteLock_PermissionRequest *permissionRequest = (ReadWriteLock_PermissionRequest *)data;
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

    free(permissionRequest);
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
