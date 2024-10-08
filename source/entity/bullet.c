
#include "bullet.h"

void SpawnFiredBullet(Entity *firingEntity)
{
    Entity *settingUpEntity;

    ZeroAndInitEntity(&settingUpEntity);
    settingUpEntity->lifetime = 60;

    SetupBulletLocation(settingUpEntity, firingEntity);
    SetupBulletRotation(settingUpEntity, firingEntity);
    SetupBulletVelocity(settingUpEntity, firingEntity);
    SetupBulletVertices(settingUpEntity);
    SetupRadius(settingUpEntity);

    List_Insert(&settingUpEntity->onCollision, OnCollisionDeath);
    List_Insert(&settingUpEntity->onCollision, OnCollisionKill);
    List_Insert(&settingUpEntity->onRender, OnRenderUpdate);
    List_Insert(&settingUpEntity->onTick, OnTickHandleMovement);
    List_Insert(&settingUpEntity->onTick, OnTickExpire);

    EntitySpawn(settingUpEntity);
}

#define EXTRA_RADIUS_MULTIPLIER 1.8
void SetupBulletLocation(Entity *settingUpEntity, Entity *firingEntity)
{
    Point *settingUpEntityLocation;
    ReadWriteLock_GetWritePermission(&settingUpEntity->location, (void **)&settingUpEntityLocation);

    Point *firingEntityLocation;
    ReadWriteLock_GetReadPermission(&firingEntity->location, (void **)&firingEntityLocation);

    settingUpEntityLocation->x = firingEntityLocation->x + (fabs(firingEntity->velocity.x * 2.0) + (firingEntity->radius * EXTRA_RADIUS_MULTIPLIER)) * cos(firingEntity->rotation);
    settingUpEntityLocation->y = firingEntityLocation->y + (fabs(firingEntity->velocity.y * 2.0) + (firingEntity->radius * EXTRA_RADIUS_MULTIPLIER)) * sin(firingEntity->rotation);

    ReadWriteLock_ReleaseReadPermission(&firingEntity->location, (void **)&firingEntityLocation);

    ReadWriteLock_ReleaseWritePermission(&settingUpEntity->location, (void **)&settingUpEntityLocation);
}
#undef EXTRA_RADIUS_MULTIPLIER

void SetupBulletRotation(Entity *settingUpEntity, Entity *firingEntity)
{
    settingUpEntity->rotation = firingEntity->rotation;
}

#define BULLET_SPEED 8
void SetupBulletVelocity(Entity *settingUpEntity, Entity *firingEntity)
{
    settingUpEntity->velocity.x = (firingEntity->velocity.x * 0.5) + (BULLET_SPEED * cos(firingEntity->rotation));
    settingUpEntity->velocity.y = (firingEntity->velocity.y * 0.5) + (BULLET_SPEED * sin(firingEntity->rotation));
}
#undef BULLET_SPEED

void SetupBulletVertices(Entity *settingUpEntity)
{
    Point *vertex;
    MemoryManager_AllocateMemory((void **)&vertex, sizeof(Point), MEMORY_MANAGER_FLAG_NONE);
    vertex->x = 1;
    vertex->y = 1;

    List_Insert(&settingUpEntity->baseVertices, vertex);

    MemoryManager_AllocateMemory((void **)&vertex, sizeof(Point), MEMORY_MANAGER_FLAG_NONE);
    vertex->x = -1;
    vertex->y = 1;

    List_Insert(&settingUpEntity->baseVertices, vertex);

    MemoryManager_AllocateMemory((void **)&vertex, sizeof(Point), MEMORY_MANAGER_FLAG_NONE);
    vertex->x = -1;
    vertex->y = -1;

    List_Insert(&settingUpEntity->baseVertices, vertex);

    MemoryManager_AllocateMemory((void **)&vertex, sizeof(Point), MEMORY_MANAGER_FLAG_NONE);
    vertex->x = 1;
    vertex->y = -1;

    List_Insert(&settingUpEntity->baseVertices, vertex);

    CalculateCentroidAndAlignVertices(settingUpEntity);
    CalculateAndSetRotationOffsetVertices(settingUpEntity);
}

void OnCollisionKill(Entity *entity, Entity *collidingEntity)
{
    UNREFERENCED_PARAMETER(entity);

    ListIterator onDeathIterator;
    ListIterator_Init(&onDeathIterator, &collidingEntity->onDeath);
    void (*onDeath)(Entity *);
    while (ListIterator_Next(&onDeathIterator, (void **)&onDeath))
    {
        onDeath(collidingEntity);
    }
}
