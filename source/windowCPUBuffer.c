
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

    while (!GAMESTATE->exiting)
    {
        WaitForSingleObject(SCREEN->bufferDrawingMutexes[id], INFINITE);

        RECT rect = {0, 0, DEFAULT_SCREEN_SIZE_X, DEFAULT_SCREEN_SIZE_Y};

        FillRect(bufferDC, &rect, GetStockObject(BLACK_BRUSH));

        List *entities;
        ReadWriteLock_GetReadPermission(&GAMESTATE->entities, (void **)&entities);
        ListIterator entitiesIterator;
        ListIterator_Init(&entitiesIterator, entities);
        Entity *entity;
        while (ListIterator_Next(&entitiesIterator, (void **)(&entity)))
        {
            if (entity->alive == ENTITY_DEAD)
            {
                continue;
            }

            ListIterator onDrawIterator;
            ListIterator_Init(&onDrawIterator, &entity->onDraw);
            void (*referenceOnDraw)(Entity *, HDC *);
            while (ListIterator_Next(&onDrawIterator, (void **)(&referenceOnDraw)))
            {
                referenceOnDraw(entity, &bufferDC);
            }
        }
        ReadWriteLock_ReleaseReadPermission(&GAMESTATE->entities, (void **)&entities);

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

        List *fighters;
        ReadWriteLock_GetReadPermission(&GAMESTATE->fighters, (void **)&fighters);
        if (fighters->length > 0)
        {
            if (((Entity *)(fighters.head->data))->colliding)
            {
                TextOut(bufferDC, 48, DEFAULT_SCREEN_SIZE_Y - 50, TEXT("Fighter colliding"), _tcslen(TEXT("Fighter colliding")));
            }
            else
            {
                TextOut(bufferDC, 48, DEFAULT_SCREEN_SIZE_Y - 50, TEXT("Fighter not colliding"), _tcslen(TEXT("Fighter not colliding")));
            }
        }
        ReadWriteLock_ReleaseReadPermission(&GAMESTATE->fighters, (void **)&fighters);

        formattingRect.right = 100;
        formattingRect.top = DEFAULT_SCREEN_SIZE_Y - 70;
        formattingRect.left = 48;
        formattingRect.bottom = DEFAULT_SCREEN_SIZE_Y - 80;

        _stprintf(buffer, TEXT("Mouse: (%d, %d)"), (int)GAMESTATE->mousePosition.x, (int)GAMESTATE->mousePosition.y);
        DrawText(bufferDC, buffer, _tcslen(buffer), &formattingRect, (DT_INTERNAL_FLAGS & (~DT_CENTER)) | DT_LEFT);

#endif
        if (!GAMESTATE->running)
        {
            formattingRect.right = DEFAULT_SCREEN_SIZE_X;
            formattingRect.top = DEFAULT_SCREEN_SIZE_Y - 40;
            formattingRect.left = 0;
            formattingRect.bottom = DEFAULT_SCREEN_SIZE_Y - 60;

            _stprintf(buffer, TEXT("Game Paused [ESC]"));
            DrawText(bufferDC, buffer, _tcslen(buffer), &formattingRect, DT_INTERNAL_FLAGS);
        }

        GdiFlush();

        ReleaseMutex(SCREEN->bufferDrawingMutexes[id]);

        WaitForSingleObject(SCREEN->bufferRedrawSemaphores[id], INFINITE);
    }

    SelectObject(bufferDC, original);
    DeleteDC(bufferDC);
    return 0;
}
