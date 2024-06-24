
#include "read_write_lock.h"

void ReadWriteLock_Init(ReadWriteLock *readWriteLock, void *protectedData)
{
    ZeroMemory(readWriteLock, sizeof(ReadWriteLock));

    InitializeSRWLock(&readWriteLock->SRWLock);
    readWriteLock->protectedData = protectedData;
}

/// @brief Closes the semaphore handles and frees the memory for a ReadWriteLock.
/// Insure you handle the protected data and have a write lock before calling this.
/// @param readWriteLock The ReadWriteLock to be destroyed
void ReadWriteLock_Destroy(ReadWriteLock *readWriteLock)
{
    UNREFERENCED_PARAMETER(readWriteLock);
    // Nothing for now. We may no longer need this.
}

void ReadWriteLock_GetWritePermission(ReadWriteLock *readWriteLock, void **protectedData)
{
    AcquireSRWLockExclusive(&readWriteLock->SRWLock);

    (*protectedData) = readWriteLock->protectedData;
}

short ReadWriteLock_TryGetWritePermission(ReadWriteLock *readWriteLock, void **protectedData)
{
    if (TryAcquireSRWLockExclusive(&readWriteLock->SRWLock))
    {
        (*protectedData) = readWriteLock->protectedData;
        return TRUE;
    }

    return FALSE;
}

void ReadWriteLock_ReleaseWritePermission(ReadWriteLock *readWriteLock, void **protectedData)
{
    (*protectedData) = NULL;

    ReleaseSRWLockExclusive(&readWriteLock->SRWLock);
}

void ReadWriteLock_GetReadPermission(ReadWriteLock *readWriteLock, void **protectedData)
{
    AcquireSRWLockShared(&readWriteLock->SRWLock);

    (*protectedData) = readWriteLock->protectedData;
}

short ReadWriteLock_TryGetReadPermission(ReadWriteLock *readWriteLock, void **protectedData)
{
    if (TryAcquireSRWLockShared(&readWriteLock->SRWLock))
    {
        (*protectedData) = readWriteLock->protectedData;
        return TRUE;
    }

    return FALSE;
}

void ReadWriteLock_ReleaseReadPermission(ReadWriteLock *readWriteLock, void **protectedData)
{
    (*protectedData) = NULL;

    ReleaseSRWLockShared(&readWriteLock->SRWLock);
}

short ReadWriteLock_GetMultiplePermissions(ReadWriteLock_PermissionRequest *pointerToPermissionRequests,
                                           unsigned int amountOfPermissionRequests)
{
    ReadWriteLock_PermissionRequest *permissionRequest;
    for (unsigned int i = 0; i < amountOfPermissionRequests; i++)
    {
        permissionRequest = &pointerToPermissionRequests[i];
        if (permissionRequest->permissionType == ReadWriteLock_Read)
        {
            if (ReadWriteLock_TryGetReadPermission(permissionRequest->readWriteLock, &permissionRequest->returnedData) == FALSE)
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
            if (ReadWriteLock_TryGetWritePermission(permissionRequest->readWriteLock, &permissionRequest->returnedData) == FALSE)
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
