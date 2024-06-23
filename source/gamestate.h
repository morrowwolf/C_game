#pragma once
#ifndef GAMESTATE_H_
#define GAMESTATE_H_

#include "globals.h"
#include "entity.h"
#include "entity/asteroid.h"
#include "entity/fighter.h"
#include "tasks.h"

DWORD WINAPI GamestateHandler(LPVOID);

void Gamestate_ResetGamestateCompleteEvents();

void Gamestate_EntitiesOnTick(ListIteratorThread *);

void Gamestate_AsteroidSpawn();
void Gamestate_FighterSpawn();

#endif
