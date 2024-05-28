#pragma once
#ifndef WINDOW_H_
#define WINDOW_H_

#include "globals.h"
#include "graphics/GPUHandler.h"

int WindowHandler(HINSTANCE hInstance, int iCmdShow);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void WndProcHandlePaint(HWND, HDC);
void HandleNonGameKeys(UINT_PTR);

#endif
