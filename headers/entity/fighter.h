#pragma once
#ifndef FIGHTER_H_
#define FIGHTER_H_

#include "../common_defines.h"
#include "../entity.h"

void SpawnPlayerFighter();

void SetupFighterVertices(Entity *);

void OnDeathFighter(Entity *);

void OnTickKeyAcceleration(Entity *);

#endif
