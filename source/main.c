
#include "../headers/main.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(szCmdLine);

	GAMESTATE = calloc(1, sizeof(Gamestate));
	List_Init(&GAMESTATE->entities, NULL);
	List_Init(&GAMESTATE->deadEntities, List_DestroyEntityOnRemove);
	List_Init(&GAMESTATE->asteroids, NULL);
	List_Init(&GAMESTATE->fighters, NULL);
	GAMESTATE->running = TRUE;
	GAMESTATE->keyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

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

	WindowHandler(hInstance, iCmdShow);

	exit(0);
}
