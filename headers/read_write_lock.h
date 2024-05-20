#pragma once
#ifndef READ_WRITE_LOCK_H_
#define READ_WRITE_LOCK_H_

#include <Windows.h>

typedef struct
{
    HANDLE writeSemaphore;
    HANDLE readSemaphore;
} ReadWriteLock;

#endif
