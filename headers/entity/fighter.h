#pragma once
#ifndef FIGHTER_H_
#define FIGHTER_H_

#include "../entity.h"
#include "bullet.h"

void SpawnPlayerFighter();
void FighterDestroy(Entity *);

void SetupFighterVertices(Entity *);

void OnTickKeyAcceleration(Entity *);
void OnTickKeyFireBullet(Entity *);
void OnTickReduceFireDelay(Entity *);

#endif
