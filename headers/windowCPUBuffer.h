#pragma once
#ifndef WINDOWCPUBUFFER_H_
#define WINDOWCPUBUFFER_H_

#include "common_defines.h"
#include "window.h"
#include "gamestate.h"

#ifdef CPU_GRAPHICS

typedef struct
{
    unsigned short bufferId;
} BufferArgs;
DWORD WINAPI BufferHandler(LPVOID);

#endif

#endif
