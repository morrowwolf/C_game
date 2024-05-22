
#include "../headers/windowCPUBuffer.h"

DWORD WINAPI BufferHandler(LPVOID lpParam)
{
    BufferArgs *bufferArgs = (BufferArgs *)lpParam;
    unsigned short id = bufferArgs->bufferId;

    free(bufferArgs);

    WaitForSingleObject(SCREEN->windowHandleInitializedEvent, INFINITE);

    HDC screenDC = GetDC(SCREEN->windowHandle);

    HDC bufferDC = CreateCompatibleDC(screenDC);
    SCREEN->bufferMemDCs[id] = bufferDC;

    HBITMAP bufferBitmap = CreateCompatibleBitmap(screenDC,
                                                  DEFAULT_SCREEN_SIZE_X,
                                                  DEFAULT_SCREEN_SIZE_Y);

    HBITMAP oldBitmap = SelectObject(bufferDC, bufferBitmap);
    // NOLINTNEXTLINE
    HGDIOBJ original = SelectObject(bufferDC, GetStockObject(WHITE_PEN));

    SetMapMode(bufferDC, MM_ANISOTROPIC);
    SetWindowExtEx(bufferDC, DEFAULT_SCREEN_SIZE_X, DEFAULT_SCREEN_SIZE_Y, NULL);
    SetViewportExtEx(bufferDC, DEFAULT_SCREEN_SIZE_X, -DEFAULT_SCREEN_SIZE_Y, NULL);
    SetViewportOrgEx(bufferDC, 0, DEFAULT_SCREEN_SIZE_Y, NULL);

    SetTextAlign(bufferDC, GetTextAlign(bufferDC) & (~TA_BASELINE | ~TA_CENTER));
    SetTextColor(bufferDC, RGB(255, 255, 255));
    SetBkMode(bufferDC, TRANSPARENT);
    SetBkColor(bufferDC, RGB(0, 0, 0));

    DeleteObject(oldBitmap);

    ReleaseDC(SCREEN->windowHandle, screenDC);

    while (TRUE)
    {
        WaitForSingleObject(SCREEN->bufferDrawingMutexes[id], INFINITE);

        RECT rect = {0, 0, DEFAULT_SCREEN_SIZE_X, DEFAULT_SCREEN_SIZE_Y};

        FillRect(bufferDC, &rect, GetStockObject(BLACK_BRUSH));

        ListIterator *entitiesIterator;
        ListIterator_Init(&entitiesIterator, &GAMESTATE->entities, ReadWriteLock_Read);
        Entity *referenceEntity;
        while (ListIterator_Next(entitiesIterator, (void **)(&referenceEntity)))
        {
            ListIterator *onDrawIterator;
            ListIterator_Init(&onDrawIterator, &referenceEntity->onDraw, ReadWriteLock_Read);
            void (*referenceOnDraw)(Entity *, HDC *);
            while (ListIterator_Next(onDrawIterator, (void **)(&referenceOnDraw)))
            {
                referenceOnDraw(referenceEntity, &bufferDC);
            }
            ListIterator_Destroy(onDrawIterator);
        }
        ListIterator_Destroy(entitiesIterator);

        TCHAR buffer[32];
        RECT formattingRect;

#ifdef DEBUG
        int i;
        for (i = 48; i < 90; i++)
        {
            if (i > 57 && i < 65)
            {
                continue;
            }
            _stprintf(buffer, TEXT("%c"), i);
            TextOut(bufferDC, i + (12 * (i - 48)), DEFAULT_SCREEN_SIZE_Y - 10, buffer, _tcslen(buffer));
            _stprintf(buffer, TEXT("%d"), GAMESTATE->keys[i]);
            TextOut(bufferDC, i + (12 * (i - 48)), DEFAULT_SCREEN_SIZE_Y - 30, buffer, _tcslen(buffer));
        }

        if (GAMESTATE->fighters.length > 0)
        {
            if (((Entity *)(GAMESTATE->fighters.head->data))->colliding)
            {
                TextOut(bufferDC, 12, 40, TEXT("Fighter colliding"), _tcslen(TEXT("Fighter colliding")));
            }
            else
            {
                TextOut(bufferDC, 12, 40, TEXT("Fighter not colliding"), _tcslen(TEXT("Fighter not colliding")));
            }
        }
#endif
        formattingRect.right = DEFAULT_SCREEN_SIZE_X;
        formattingRect.top = DEFAULT_SCREEN_SIZE_Y - 20;
        formattingRect.left = 0;
        formattingRect.bottom = DEFAULT_SCREEN_SIZE_Y - 30;

        _stprintf(buffer, TEXT("%d"), GAMESTATE->asteroids.length);
        DrawText(bufferDC, buffer, _tcslen(buffer), &formattingRect, DT_INTERNAL_FLAGS);

        if (!GAMESTATE->running)
        {
            formattingRect.right = DEFAULT_SCREEN_SIZE_X;
            formattingRect.top = DEFAULT_SCREEN_SIZE_Y - 40;
            formattingRect.left = 0;
            formattingRect.bottom = DEFAULT_SCREEN_SIZE_Y - 60;

            _stprintf(buffer, TEXT("Game Paused [ESC]"), GAMESTATE->asteroids.length);
            DrawText(bufferDC, buffer, _tcslen(buffer), &formattingRect, DT_INTERNAL_FLAGS);
        }

        GdiFlush();

        ReleaseMutex(SCREEN->bufferDrawingMutexes[id]);

        WaitForSingleObject(SCREEN->bufferRedrawSemaphores[id], INFINITE);
    }

    SelectObject(bufferDC, original);
    DeleteDC(bufferDC);
    exit(0);
}
