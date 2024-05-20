#pragma once
#ifndef WINDOW_H_
#define WINDOW_H_

#include "common_defines.h"

typedef struct
{
    HINSTANCE hInstance;
    int iCmdShow;
} WindowHandlerArgs;
DWORD WINAPI WindowHandler(LPVOID);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

typedef struct
{
    HWND windowHandle;
    HANDLE windowHandleInitializedEvent;
    unsigned short currentBufferUsed;

    HANDLE bufferHDCSetupMutexes[BUFFER_THREAD_COUNT];
    HANDLE bufferDrawingMutexes[BUFFER_THREAD_COUNT];
    HANDLE bufferRedrawSemaphores[BUFFER_THREAD_COUNT];
    HDC bufferMemDCs[BUFFER_THREAD_COUNT];
} Screen;

extern Screen *SCREEN;

#endif
