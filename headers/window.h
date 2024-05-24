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
    unsigned short currentBufferUsed;

    HANDLE bufferHDCSetupMutexes[BUFFER_THREAD_COUNT];
    HANDLE bufferDrawingMutexes[BUFFER_THREAD_COUNT];
    HANDLE bufferRedrawSemaphores[BUFFER_THREAD_COUNT];
    HDC bufferMemDCs[BUFFER_THREAD_COUNT];
} Screen;

extern Screen *SCREEN;

#endif
