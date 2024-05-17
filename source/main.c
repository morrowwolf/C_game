
#include "../headers/main.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(szCmdLine);

	GAMESTATE = malloc(sizeof(Gamestate));
	list_init(&GAMESTATE->entities, NULL);
	list_init(&GAMESTATE->deadEntities, list_free_on_remove);
	list_init(&GAMESTATE->asteroids, NULL);
	list_init(&GAMESTATE->fighters, NULL);
	ZeroMemory(&GAMESTATE->keys, sizeof(GAMESTATE->keys));
	GAMESTATE->runningEntityID = 0;

#ifdef CPU_GRAPHICS

	SCREEN = malloc(sizeof(Screen));

	SCREEN->windowHandleInitializedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	SCREEN->currentBufferUsed = 0;

	int i;
	for (i = 0; i < BUFFER_THREAD_COUNT; i++)
	{
		SCREEN->bufferDrawingMutexes[i] = CreateMutex(NULL, FALSE, NULL);
		SCREEN->bufferRedrawSemaphores[i] = CreateSemaphore(NULL, 0, 1, NULL);

		// Freed by the handler
		BufferArgs *bufferArgs = malloc(sizeof(BufferArgs));
		bufferArgs->bufferId = i;
		CreateThread(NULL, 0, BufferHandler, bufferArgs, 0, NULL);
	}

#endif

	CreateThread(NULL, 0, GamestateHandler, NULL, 0, NULL);

	// Freed by the handler
	WindowHandlerArgs *windowHandlerArgs = malloc(sizeof(WindowHandlerArgs));
	windowHandlerArgs->hInstance = hInstance;
	windowHandlerArgs->iCmdShow = iCmdShow;
	HANDLE windowThread = CreateThread(NULL, 0, WindowHandler, windowHandlerArgs, 0, NULL);

	WaitForSingleObject(windowThread, INFINITE);

	exit(0);
}
