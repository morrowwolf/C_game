
#include "window.h"

int WindowHandler(HINSTANCE hInstance, int iCmdShow)
{
    TCHAR szAppName[] = TEXT("C Game");
    WNDCLASS wndclass;

    wndclass.style = 0;
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

    long windowStyleFlags = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    RECT windowRect;
    windowRect.left = 0;
    windowRect.top = 0;
    windowRect.right = DEFAULT_SCREEN_SIZE_X;
    windowRect.bottom = DEFAULT_SCREEN_SIZE_Y;
    AdjustWindowRect(&windowRect, windowStyleFlags, FALSE);

    SCREEN->windowHandle = CreateWindow(szAppName, TEXT("C Game"), windowStyleFlags,
                                        CW_USEDEFAULT, CW_USEDEFAULT, windowRect.right - windowRect.left,
                                        windowRect.bottom - windowRect.top, NULL, NULL, hInstance, NULL);

    Directx_Init();

    ShowCursor(FALSE);
    ShowWindow(SCREEN->windowHandle, iCmdShow);
    UpdateWindow(SCREEN->windowHandle);

    short lastMessage = TRUE;
    MSG msg;
    msg.wParam = 1;

    while (!SCREEN->exiting)
    {
        while (!SCREEN->exiting && lastMessage)
        {
            if (WaitForSingleObject(hUpdateWindowTimer, 0) == WAIT_OBJECT_0)
            {
                Task *task;
                MemoryManager_AllocateMemory((void **)&task, sizeof(Task), MEMORY_MANAGER_FLAG_NONE);
                task->task = (void (*)(void *))Directx_SetupRender;
                task->taskArgument = NULL;

                Task_PushSystemTask(task);

                if (SCREEN->nextFrameRefreshTime.QuadPart == 0)
                {
                    FILETIME currentTime;
                    GetSystemTimeAsFileTime(&currentTime);
                    SCREEN->nextFrameRefreshTime.LowPart = currentTime.dwLowDateTime;
                    SCREEN->nextFrameRefreshTime.HighPart = currentTime.dwHighDateTime;
                }

                SCREEN->nextFrameRefreshTime.QuadPart += DEFAULT_FRAME_REFRESH_RATE;

                SetWaitableTimer(hUpdateWindowTimer, &SCREEN->nextFrameRefreshTime, 0, NULL, NULL, 0);
            }

            if (WaitForSingleObject(MEMORY_MANAGER->memoryCleanupTimer, 0) == WAIT_OBJECT_0)
            {
                Task *task;
                MemoryManager_AllocateMemory((void **)&task, sizeof(Task), MEMORY_MANAGER_FLAG_NONE);
                task->task = (void (*)(void *))MemoryManager_Cleanup;
                task->taskArgument = NULL;

                Task_PushGarbageTask(task);

                if (MEMORY_MANAGER->nextMemoryCleanupTime.QuadPart == 0)
                {
                    FILETIME currentTime;
                    GetSystemTimeAsFileTime(&currentTime);
                    MEMORY_MANAGER->nextMemoryCleanupTime.LowPart = currentTime.dwLowDateTime;
                    MEMORY_MANAGER->nextMemoryCleanupTime.HighPart = currentTime.dwHighDateTime;
                }

                MEMORY_MANAGER->nextMemoryCleanupTime.QuadPart += MEMORY_MANAGER_CLEANUP_INTERVAL;

                SetWaitableTimer(MEMORY_MANAGER->memoryCleanupTimer, &MEMORY_MANAGER->nextMemoryCleanupTime, 0, NULL, NULL, 0);
            }

            if ((lastMessage = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)))
            {
                if (msg.message == WM_QUIT)
                {
                    SCREEN->exiting = TRUE;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        MsgWaitForMultipleObjects(1, &hUpdateWindowTimer, FALSE, INFINITE, QS_ALLINPUT);
        lastMessage = TRUE;
    }

    WaitForSingleObject(SCREEN->fenceEvent, INFINITE);

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

        if (!GAMESTATE->debugMode)
        {
            GAMESTATE->running = GAME_PAUSED;
        }

        return 0;

    case WM_MOUSEMOVE:
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        GAMESTATE->mousePosition.x = GET_X_LPARAM(lParam);
        GAMESTATE->mousePosition.y = clientRect.bottom - GET_Y_LPARAM(lParam);
        return 0;

    case WM_TIMECHANGE:
        GAMESTATE->nextTickTime.QuadPart = 0;
        SCREEN->nextFrameRefreshTime.QuadPart = 0;
        return 0;

    case WM_PAINT:
        ValidateRgn(hWnd, NULL);

        return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

void HandleNonGameKeys(UINT_PTR keyCode)
{
    if (keyCode == VK_F2)
    {
        GAMESTATE->debugMode = (GAMESTATE->debugMode + 1) % 2;
    }

    if (keyCode == VK_ESCAPE)
    {
        GAMESTATE->running = (GAMESTATE->running + 1) % 2;
    }

    SetEvent(GAMESTATE->keyEvent);
    ResetEvent(GAMESTATE->keyEvent);
}
