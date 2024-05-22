#pragma once
#ifndef HELPER_H_
#define HELPER_H_

#define UNICODE 1
#define _UNICODE 1

#define _USE_MATH_DEFINES 1

// #define DEBUG 1

// https://learn.microsoft.com/en-us/cpp/c-runtime-library/type-checking-crt?view=msvc-170
// #define _DEBUG 1

#include <Windows.h>
#include <math.h>
#include <tchar.h>
#include <stdio.h>
#include "data_structures/list.h"
#include "data_structures/list_iterator.h"
#include "data_structures/read_write_lock.h"

#pragma comment(lib, "bcrypt.lib")

#define CPU_GRAPHICS 1

#define MAX_ASTEROIDS 128
#define MAX_FIGHTERS 1
#define DEFAULT_SCREEN_SIZE_X 1600
#define DEFAULT_SCREEN_SIZE_Y 900
#define BUFFER_THREAD_COUNT 1

#define RANDOMIZE(randomVal)                     \
    BCryptGenRandom(NULL,                        \
                    (unsigned char *)&randomVal, \
                    sizeof(randomVal),           \
                    BCRYPT_USE_SYSTEM_PREFERRED_RNG)

#define SIGNOF(value) \
    ((value >= 0) ? 1 : -1)

typedef struct
{
    double x;
    double y;
} Point, Vector;

typedef struct
{
    unsigned long long runningEntityID;
    List entities;     // list of Entity
    List deadEntities; // list of Entity, holds memory references to free at end of gamestate cycle
    List asteroids;    // list of Entity
    List fighters;     // list of Entity
    BYTE keys[256];    // 1 is active, 0 is inactive | https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
} Gamestate;

Gamestate *GAMESTATE;

#endif
