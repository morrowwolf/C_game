#pragma once
#ifndef HELPER_H_
#define HELPER_H_

#define UNICODE 1
#define _UNICODE 1

#define _USE_MATH_DEFINES 1

// #define DEBUG 1

// https://learn.microsoft.com/en-us/cpp/c-runtime-library/type-checking-crt?view=msvc-170
#ifdef DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <Windows.h>
#include <windowsx.h>
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

#define DT_INTERNAL_FLAGS (DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP)

#define RANDOMIZE(randomVal)                     \
    BCryptGenRandom(NULL,                        \
                    (unsigned char *)&randomVal, \
                    sizeof(randomVal),           \
                    BCRYPT_USE_SYSTEM_PREFERRED_RNG)

#define SIGNOF(value) \
    ((value >= 0) ? 1 : -1)

#define MINIMUM_FLOAT_DIFFERENCE 0.0000001

typedef struct
{
    double x;
    double y;
} Point, Vector;

typedef struct
{
    unsigned short exiting; // If the overall process is ending

    RWL_List taskQueue; // list of Task

    volatile unsigned __int64 runningEntityID;
#define GAME_PAUSED 0
#define GAME_RUNNING 1
    unsigned short running; // If the gamestate should be processing
    RWL_List entities;      // list of Entity
    RWL_List deadEntities;  // list of Entity, holds memory references to free at end of gamestate cycle
    RWL_List asteroids;     // list of Entity
    RWL_List fighters;      // list of Entity

    HANDLE keyEvent;
    BYTE keys[256]; // 1 is active, 0 is inactive | https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

    BYTE mouseButtons[1]; // TODO
    Point mousePosition;  // This is screen location not world location

} Gamestate;

Gamestate *GAMESTATE;

typedef struct
{
    RWL_List taskQueue;
    List tasksCompleteSyncEvents;
    List tasksQueuedSyncEvents;
} TaskState;

TaskState *TASKSTATE;

#endif
