
#include "../headers/window.h"

Screen *SCREEN;

DWORD WINAPI WindowHandler(LPVOID lpParam)
{
    WindowHandlerArgs *windowHandlerArgs = (WindowHandlerArgs *)lpParam;

    HINSTANCE hInstance = windowHandlerArgs->hInstance;
    int iCmdShow = windowHandlerArgs->iCmdShow;

    free(windowHandlerArgs);

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

    HWND hwnd = SCREEN->windowHandle;

    ShowCursor(FALSE);
    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    short quit = FALSE;
    MSG msg;

    while (!quit)
    {
        if (WaitForSingleObject(hUpdateWindowTimer, 0) == WAIT_OBJECT_0)
        {
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
            SetWaitableTimer(hUpdateWindowTimer, &updateWindowTimerTime, 0, NULL, NULL, 0);
        }

        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                quit = TRUE;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    exit(msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;

    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_ERASEBKGND:
        return 0;

    case WM_KEYDOWN:
        GAMESTATE->keys[wParam] = 1;
        return 0;

    case WM_KEYUP:
        GAMESTATE->keys[wParam] = 0;
        return 0;

    case WM_KILLFOCUS:
        ZeroMemory(&GAMESTATE->keys, sizeof(GAMESTATE->keys));
        return 0;

    case WM_PAINT:

#ifdef CPU_GRAPHICS
        hdc = BeginPaint(hwnd, &ps);

        GetClientRect(hwnd, &rect);

        SetMapMode(hdc, MM_ANISOTROPIC);
        SetWindowExtEx(hdc, DEFAULT_SCREEN_SIZE_X, DEFAULT_SCREEN_SIZE_Y, NULL);
        SetViewportExtEx(hdc, rect.right, -rect.bottom, NULL);
        SetViewportOrgEx(hdc, 0, rect.bottom, NULL);

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

        EndPaint(hwnd, &ps);
        return 0;
#endif
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}
