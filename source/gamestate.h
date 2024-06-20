#pragma once
#ifndef GAMESTATE_H_
#define GAMESTATE_H_

#include "globals.h"
#include "entity.h"
#include "entity/asteroid.h"
#include "entity/fighter.h"
#include "tasks.h"

DWORD WINAPI GamestateHandler(LPVOID);

typedef struct Gamestate_StartTick_Params
{
    unsigned __int32 assignedNumber;
    unsigned __int32 maxNumber;
} Gamestate_StartTick_Params;

void Gamestate_StartTick(Gamestate_StartTick_Params *params);

#endif
