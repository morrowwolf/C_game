#pragma once
#ifndef WINDOW_H_
#define WINDOW_H_

#include "common_defines.h"

int WindowHandler(HINSTANCE hInstance, int iCmdShow);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void WndProcHandlePaint(HWND, HDC);
void HandleNonGameKeys(UINT_PTR);

typedef struct
{
    HWND windowHandle;
    HANDLE windowHandleInitializedEvent;

    // About 256 frames per second, exact 256 frames would be 390625ULL
#define DEFAULT_FRAME_REFRESH_RATE 39063ULL
    ULARGE_INTEGER nextFrameRefreshTime;

#ifdef CPU_GRAPHICS
    unsigned short currentBufferUsed;

    HANDLE bufferDrawingMutexes[BUFFER_THREAD_COUNT];
    HANDLE bufferRedrawSemaphores[BUFFER_THREAD_COUNT];
    HDC bufferMemDCs[BUFFER_THREAD_COUNT];
#endif
} Screen;

extern Screen *SCREEN;

#endif
