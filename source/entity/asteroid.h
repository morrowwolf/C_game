#pragma once
#ifndef ASTEROID_H_
#define ASTEROID_H_

#include "../entity.h"

void SpawnAsteroid();
void AsteroidDestroy(Entity *);

void SetupLocationAsteroid(Entity *);
void SetupVerticesAsteroid(Entity *);

void OnCollisionDeathAsteroid(Entity *, Entity *);

void OnMovementWithLocationLockGameEdgeCheckAsteroid(Entity *, Point *);

#endif
