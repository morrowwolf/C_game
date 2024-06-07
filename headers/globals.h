#pragma once
#ifndef GLOBALS_H_
#define GLOBALS_H_

#define DEBUG

#define _USE_MATH_DEFINES

#define UNICODE
#define _UNICODE

#include "helpers.h"
#include <Windows.h>
#include <windowsx.h>
#include <winerror.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <math.h>
#include <tchar.h>
#include <stdio.h>
#include "graphics/D3DX12_globals.h"
#include "data_structures/list.h"
#include "data_structures/list_iterator.h"
#include "data_structures/read_write_lock.h"

// https://learn.microsoft.com/en-us/cpp/c-runtime-library/type-checking-crt?view=msvc-170
#ifdef DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#pragma comment(lib, "bcrypt.lib")

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

#define MAX_ASTEROIDS 128
#define MAX_FIGHTERS 1
#define DEFAULT_SCREEN_SIZE_X 1600
#define DEFAULT_SCREEN_SIZE_Y 900

#define DT_INTERNAL_FLAGS (DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP)

#define RANDOMIZE(randomVal)                     \
    BCryptGenRandom(NULL,                        \
                    (unsigned char *)&randomVal, \
                    sizeof(randomVal),           \
                    BCRYPT_USE_SYSTEM_PREFERRED_RNG)

#define SIGNOF(value) \
    ((value >= 0) ? 1 : -1)

#define HANDLE_HRESULT(result)                                                                            \
    if (FAILED(result))                                                                                   \
    {                                                                                                     \
        TCHAR buffer[256];                                                                                \
        _stprintf(buffer, TEXT("Failure at %s:%d.\nHRESULT: 0x%08X."), TEXT(__FILE__), __LINE__, result); \
        OutputDebugString(buffer);                                                                        \
        MessageBox(SCREEN->windowHandle, buffer, NULL, MB_OK);                                            \
        exit(EXIT_FAILURE);                                                                               \
    }

#define HANDLE_ERROR(result)                                                                                    \
    if (result == NULL)                                                                                         \
    {                                                                                                           \
        TCHAR buffer[256];                                                                                      \
        _stprintf(buffer, TEXT("Failure at %s:%d.\nERROR: 0x%08X."), TEXT(__FILE__), __LINE__, GetLastError()); \
        OutputDebugString(buffer);                                                                              \
        MessageBox(SCREEN->windowHandle, buffer, NULL, MB_OK);                                                  \
        exit(EXIT_FAILURE);                                                                                     \
    }

#define MINIMUM_FLOAT_DIFFERENCE 0.0000001

typedef struct Point
{
    double x;
    double y;
} Point, Vector;

typedef struct Gamestate
{
    __int8 debugMode;

// 10000ULL is 1 ms in 100ns intervals
// 156250ULL is 64 ticks per second
// Given we are going into subtick territory it may be best
// to lower this to 32 ticks per second later
#define DEFAULT_TICK_RATE 156250ULL
    unsigned __int64 tickCount;
    ULARGE_INTEGER nextTickTime;
    ULARGE_INTEGER lastTickTimeDifference;

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

typedef struct TaskState
{
    RWL_List taskQueue;
    List tasksCompleteSyncEvents;
    List tasksQueuedSyncEvents;
} TaskState;

TaskState *TASKSTATE;

typedef struct Screen
{
    HWND windowHandle;

    WCHAR assetsPath[512];

    float aspectRatio;
    unsigned int screenWidth;
    unsigned int screenHeight;

    D3D12_VIEWPORT viewport;
    D3D12_RECT scissorRect;

    ID3D12Device *device;
    IDXGISwapChain3 *swapChain;

    ID3D12PipelineState *pipelineState;

    ID3D12CommandQueue *commandQueue;
    ID3D12CommandAllocator *commandAllocator;
    ID3D12GraphicsCommandList *commandList;

    ID3D12RootSignature *rootSignature;

    ID3D12DescriptorHeap *rtvHeap;
#define FRAME_COUNT 2
    ID3D12Resource *renderTargets[FRAME_COUNT];

    unsigned int rtvDescriptorSize;

    unsigned int frameIndex;
    HANDLE fenceEvent;
    ID3D12Fence *fence;
    unsigned __int64 fenceValue;

    // About 256 frames per second, exact 256 frames would be 390625ULL
#define DEFAULT_FRAME_REFRESH_RATE 39063ULL
    ULARGE_INTEGER nextFrameRefreshTime;

} Screen;

Screen *SCREEN;

#endif
