#ifndef BULLET_H_
#define BULLET_H_

#include "../entity.h"

void SpawnFiredBullet(Entity *);

void SetupBulletLocation(Entity *, Entity *);
void SetupBulletRotation(Entity *, Entity *);
void SetupBulletVelocity(Entity *, Entity *);
void SetupBulletVertices(Entity *);

void OnCollisionKill(Entity *, Entity *);

#endif
