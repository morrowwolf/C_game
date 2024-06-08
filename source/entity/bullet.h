#ifndef BULLET_H_
#define BULLET_H_

#include "../entity.h"

void SpawnFiredBullet(Entity *firingEntity);

void SetupBulletLocation(Entity *settingUpEntity, Entity *firingEntity);
void SetupBulletRotation(Entity *settingUpEntity, Entity *firingEntity);
void SetupBulletVelocity(Entity *settingUpEntity, Entity *firingEntity);
void SetupBulletVertices(Entity *settingUpEntity);

void OnCollisionKill(Entity *entity, Entity *collidingEntity);

void OnDeathBullet(Entity *entity);

void OnTickExpire(Entity *entity);

#endif
