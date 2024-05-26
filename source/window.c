
#include "../headers/window.h"

Screen *SCREEN;

int WindowHandler(HINSTANCE hInstance, int iCmdShow)
{
    TCHAR szAppName[] = TEXT("C Game");
    WNDCLASS wndclass;

    wndclass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = GetStockObject(BLACK_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szAppName;

    if (!RegisterClass(&wndclass))
    {
        MessageBox(NULL, TEXT("Unable to register class."), szAppName, MB_ICONERROR);

        exit(1);
    }

    HANDLE hUpdateWindowTimer;
    LARGE_INTEGER updateWindowTimerTime;
    updateWindowTimerTime.QuadPart = -40000LL;

    hUpdateWindowTimer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(hUpdateWindowTimer, &updateWindowTimerTime, 0, NULL, NULL, 0);

    long windowStyleFlags = WS_OVERLAPPEDWINDOW; // TODO: Kill free resize when we make menu

    RECT windowRect;
    windowRect.left = 0;
    windowRect.top = 0;
    windowRect.right = DEFAULT_SCREEN_SIZE_X;
    windowRect.bottom = DEFAULT_SCREEN_SIZE_Y;
    AdjustWindowRect(&windowRect, windowStyleFlags, FALSE);

    SCREEN->windowHandle = CreateWindow(szAppName, TEXT("C Game"), windowStyleFlags,
                                        CW_USEDEFAULT, CW_USEDEFAULT, windowRect.right - windowRect.left,
                                        windowRect.bottom - windowRect.top, NULL, NULL, hInstance, NULL);

    SetEvent(SCREEN->windowHandleInitializedEvent);

    HWND hWnd = SCREEN->windowHandle;

    ShowCursor(FALSE);
    ShowWindow(hWnd, iCmdShow);
    UpdateWindow(hWnd);

    short lastMessage = TRUE;
    MSG msg;
    msg.wParam = 1;

    while (!GAMESTATE->exiting)
    {
        while (!GAMESTATE->exiting && lastMessage)
        {
            if (WaitForSingleObject(hUpdateWindowTimer, 0) == WAIT_OBJECT_0)
            {
                InvalidateRect(hWnd, NULL, FALSE);
                UpdateWindow(hWnd);
                SetWaitableTimer(hUpdateWindowTimer, &updateWindowTimerTime, 0, NULL, NULL, 0);
            }

            if ((lastMessage = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)))
            {
                if (msg.message == WM_QUIT)
                {
                    GAMESTATE->exiting = TRUE;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        MsgWaitForMultipleObjects(1, &hUpdateWindowTimer, FALSE, INFINITE, QS_ALLINPUT);
        lastMessage = TRUE;
    }

    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_ERASEBKGND:
        return 0;

    case WM_KEYDOWN:
        GAMESTATE->keys[wParam] = 1;
        HandleNonGameKeys(wParam);
        return 0;

    case WM_KEYUP:
        GAMESTATE->keys[wParam] = 0;
        return 0;

    case WM_KILLFOCUS:
        ZeroMemory(&GAMESTATE->keys, sizeof(GAMESTATE->keys));

#ifndef DEBUG
        GAMESTATE->running = GAME_PAUSED;
#endif
        return 0;

    case WM_MOUSEMOVE:
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        GAMESTATE->mousePosition.x = GET_X_LPARAM(lParam);
        GAMESTATE->mousePosition.y = clientRect.bottom - GET_Y_LPARAM(lParam);
        return 0;

    case WM_TIMECHANGE:
        GAMESTATE->nextTickTime.QuadPart = 0;
        return 0;

    case WM_PAINT:

#ifdef CPU_GRAPHICS
        HDC hdc;
        PAINTSTRUCT ps;

        hdc = BeginPaint(hWnd, &ps);

        WndProcHandlePaint(hWnd, hdc);

        EndPaint(hWnd, &ps);
        return 0;
#endif
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

void WndProcHandlePaint(HWND hWnd, HDC hdc)
{
    RECT clientRect;

    GetClientRect(hWnd, &clientRect);

    SetMapMode(hdc, MM_ANISOTROPIC);
    SetWindowExtEx(hdc, DEFAULT_SCREEN_SIZE_X, DEFAULT_SCREEN_SIZE_Y, NULL);
    SetViewportExtEx(hdc, clientRect.right, -clientRect.bottom, NULL);
    SetViewportOrgEx(hdc, 0, clientRect.bottom, NULL);

    int i;

    for (i = 0; i < BUFFER_THREAD_COUNT; i++)
    {
        if (WaitForSingleObject(
                SCREEN->bufferDrawingMutexes[SCREEN->currentBufferUsed], 0) == WAIT_OBJECT_0)
        {

            BitBlt(hdc, 0, 0,
                   DEFAULT_SCREEN_SIZE_X,
                   DEFAULT_SCREEN_SIZE_Y,
                   SCREEN->bufferMemDCs[SCREEN->currentBufferUsed],
                   0, 0, SRCCOPY);

            ReleaseMutex(SCREEN->bufferDrawingMutexes[SCREEN->currentBufferUsed]);
            ReleaseSemaphore(SCREEN->bufferRedrawSemaphores[SCREEN->currentBufferUsed], 1, NULL);

            SCREEN->currentBufferUsed = (SCREEN->currentBufferUsed + 1) % BUFFER_THREAD_COUNT;
            break;
        }

        SCREEN->currentBufferUsed = (SCREEN->currentBufferUsed + 1) % BUFFER_THREAD_COUNT;
    }
}

void HandleNonGameKeys(UINT_PTR keyCode)
{
    if (keyCode == VK_ESCAPE)
    {
        GAMESTATE->running = (GAMESTATE->running + 1) % 2;
    }

    SetEvent(GAMESTATE->keyEvent);
    ResetEvent(GAMESTATE->keyEvent);
}
