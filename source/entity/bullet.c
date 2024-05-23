
#include "../../headers/entity/bullet.h"

void SpawnFiredBullet(Entity *firingEntity)
{
    Entity *settingUpEntity;

    ZeroAndInitEntity(&settingUpEntity);
    settingUpEntity->lifetime = 60;

    SetupBulletLocation(settingUpEntity, firingEntity);
    SetupBulletVelocity(settingUpEntity, firingEntity);
    SetupBulletVertices(settingUpEntity);
    SetupRadius(settingUpEntity);

    List_Insert(&settingUpEntity->onCollision, OnCollisionDeath);
    List_Insert(&settingUpEntity->onCollision, OnCollisionKill);
    List_Insert(&settingUpEntity->onDeath, OnDeathBullet);
    List_Insert(&settingUpEntity->onDraw, OnDrawVertexLines);
    List_Insert(&settingUpEntity->onTick, OnTickCheckCollision);
    List_Insert(&settingUpEntity->onTick, OnTickRotation);
    List_Insert(&settingUpEntity->onTick, OnTickVelocity);
    List_Insert(&settingUpEntity->onTick, OnTickExpire);

    List_Insert(&GAMESTATE->entities, settingUpEntity);
}

#define EXTRA_RADIUS_MULTIPLIER 1.8
void SetupBulletLocation(Entity *settingUpEntity, Entity *firingEntity)
{
    settingUpEntity->location.x = firingEntity->location.x + (fabs(firingEntity->velocity.x) + (firingEntity->radius * EXTRA_RADIUS_MULTIPLIER)) * cos(firingEntity->rotation);
    settingUpEntity->location.y = firingEntity->location.y + (fabs(firingEntity->velocity.y) + (firingEntity->radius * EXTRA_RADIUS_MULTIPLIER)) * sin(firingEntity->rotation);
}
#undef EXTRA_RADIUS_MULTIPLIER

#define BULLET_SPEED 5
void SetupBulletVelocity(Entity *settingUpEntity, Entity *firingEntity)
{
    settingUpEntity->velocity.x = firingEntity->velocity.x + (BULLET_SPEED * cos(firingEntity->rotation));
    settingUpEntity->velocity.y = firingEntity->velocity.y + (BULLET_SPEED * sin(firingEntity->rotation));
}
#undef BULLET_SPEED

void SetupBulletVertices(Entity *settingUpEntity)
{
    Point *vertex = malloc(sizeof(Point));
    vertex->x = 1;
    vertex->y = 1;

    List_Insert(&settingUpEntity->baseVertices, vertex);

    vertex = malloc(sizeof(Point));
    vertex->x = -1;
    vertex->y = 1;

    List_Insert(&settingUpEntity->baseVertices, vertex);

    vertex = malloc(sizeof(Point));
    vertex->x = -1;
    vertex->y = -1;

    List_Insert(&settingUpEntity->baseVertices, vertex);

    vertex = malloc(sizeof(Point));
    vertex->x = 1;
    vertex->y = -1;

    List_Insert(&settingUpEntity->baseVertices, vertex);

    CalculateCentroidAndAlignVertices(settingUpEntity);
    CalculateAndSetRotationOffsetVertices(settingUpEntity);
}

void OnCollisionKill(Entity *entity, Entity *collidingEntity)
{
    UNREFERENCED_PARAMETER(entity);

    ListIterator *onDeathIterator;
    ListIterator_Init(&onDeathIterator, &collidingEntity->onDeath, ReadWriteLock_Read);
    void (*onDeath)(Entity *);
    while (ListIterator_Next(onDeathIterator, (void **)&onDeath))
    {
        onDeath(collidingEntity);
    }
    ListIterator_Destroy(onDeathIterator);
}

void OnDeathBullet(Entity *entity)
{
    EntityDeath(entity);
}

void OnTickExpire(Entity *entity)
{
    entity->lifetime -= 1;
    if (entity->lifetime <= 0)
    {
        ListIterator *onDeathIterator;
        ListIterator_Init(&onDeathIterator, &entity->onDeath, ReadWriteLock_Read);
        void (*onDeath)(Entity *);
        while (ListIterator_Next(onDeathIterator, (void **)&onDeath))
        {
            onDeath(entity);
        }
        ListIterator_Destroy(onDeathIterator);
    }
}
