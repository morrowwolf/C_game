#pragma once
#ifndef ASTEROID_H_
#define ASTEROID_H_

#include "../entity.h"

void SpawnAsteroid();
void AsteroidDestroy(Entity *);

void SetupAsteroidVertices(Entity *);

void OnCollisionDeathAsteroid(Entity *, Entity *);

#endif
