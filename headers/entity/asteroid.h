#pragma once
#ifndef ASTEROID_H_
#define ASTEROID_H_

#include "../common_defines.h"
#include "../entity.h"

void SpawnAsteroid();
void OnDeathAsteroid(Entity *entity);

void SetupAsteroidVertices(Entity *);

void OnCollisionDeathAsteroid(Entity *, Entity *);

#endif
