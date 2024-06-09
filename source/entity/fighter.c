
#include "fighter.h"

void SpawnPlayerFighter()
{
    Entity *settingUpEntity;

    ZeroAndInitEntity(&settingUpEntity);

    SetupLocationCenterOfScreen(settingUpEntity);

    SetupFighterVertices(settingUpEntity);
    SetupRadius(settingUpEntity);

    List_Insert(&settingUpEntity->onCollision, OnCollisionDeath);
    settingUpEntity->onDestroy = FighterDestroy;
    List_Insert(&settingUpEntity->onRender, OnRenderUpdate);
    List_Insert(&settingUpEntity->onTick, OnTickHandleMovement);
    List_Insert(&settingUpEntity->onTick, OnTickReduceFireDelay);
    List_Insert(&settingUpEntity->onTick, OnTickKeyAcceleration);
    List_Insert(&settingUpEntity->onTick, OnTickKeyFireBullet);

    List *fighters;
    ReadWriteLock_GetWritePermission(&GAMESTATE->fighters, (void **)&fighters);

    List_Insert(fighters, settingUpEntity);

    ReadWriteLock_ReleaseWritePermission(&GAMESTATE->fighters, (void **)&fighters);

    EntitySpawn(settingUpEntity);
}

void FighterDestroy(Entity *entity)
{
    List *fighters;
    if (!ReadWriteLock_GetWritePermissionTimeout(&GAMESTATE->fighters, (void **)&fighters, 5))
    {
        Task *task = calloc(1, sizeof(Task));
        task->task = (void (*)(void *))FighterDestroy;
        task->taskArgument = entity;

        Task_QueueTask(&TASKSTATE->garbageTaskQueue, &TASKSTATE->garbageTasksQueuedSyncEvents, task);

        return;
    }

    List_RemoveElementWithMatchingData(fighters, entity);

    ReadWriteLock_ReleaseWritePermission(&GAMESTATE->fighters, (void **)&fighters);

    EntityDestroy(entity);
}

void SetupFighterVertices(Entity *settingUpEntity)
{
    short deltaX;
    short deltaY;

    RANDOMIZE(deltaX);
    RANDOMIZE(deltaY);

    Point *baseVertex = malloc(sizeof(Point));

    baseVertex->x = 5;
    baseVertex->y = 0;

    List_Insert(&settingUpEntity->baseVertices, baseVertex);

    baseVertex = malloc(sizeof(Point));

    baseVertex->x = -5;
    baseVertex->y = -5;

    List_Insert(&settingUpEntity->baseVertices, baseVertex);

    baseVertex = malloc(sizeof(Point));

    baseVertex->x = -2;
    baseVertex->y = 0;

    List_Insert(&settingUpEntity->baseVertices, baseVertex);

    baseVertex = malloc(sizeof(Point));

    baseVertex->x = -5;
    baseVertex->y = 5;

    List_Insert(&settingUpEntity->baseVertices, baseVertex);

    CalculateCentroidAndAlignVertices(settingUpEntity);
    CalculateAndSetRotationOffsetVertices(settingUpEntity);
}

#define MAIN_DRIVE_ACCELERATION 0.05
#define THRUSTER_ACCELERATION 0.025
#define ROTATION_ACCELERATION 0.075
#define MAX_FIGHTER_VELOCITY 8.0
void OnTickKeyAcceleration(Entity *entity)
{

    if (GAMESTATE->keys['C'])
    {
        if (entity->velocity.x == 0.0)
        {
            entity->velocity.y -= THRUSTER_ACCELERATION * SIGNOF(entity->velocity.y);
            if (fabs(entity->velocity.y) <= THRUSTER_ACCELERATION)
            {
                entity->velocity.y = 0.0;
            }
        }
        else if (entity->velocity.y == 0.0)
        {
            entity->velocity.x -= THRUSTER_ACCELERATION * SIGNOF(entity->velocity.x);
            if (fabs(entity->velocity.x) <= THRUSTER_ACCELERATION)
            {
                entity->velocity.x = 0.0;
            }
        }
        else
        {
            double velocityAngle = atan2(entity->velocity.y, entity->velocity.x);

            double delta = THRUSTER_ACCELERATION * cos(velocityAngle + M_PI);
            entity->velocity.x += delta;
            if (fabs(entity->velocity.x) <= fabs(delta))
            {
                entity->velocity.x = 0.0;
            }

            delta = THRUSTER_ACCELERATION * sin(velocityAngle + M_PI);
            entity->velocity.y += delta;
            if (fabs(entity->velocity.y) <= fabs(delta))
            {
                entity->velocity.y = 0.0;
            }
        }
    }
    else
    {
        if (GAMESTATE->keys[VK_SHIFT])
        {
            entity->velocity.x += MAIN_DRIVE_ACCELERATION * cos(entity->rotation);
            entity->velocity.y += MAIN_DRIVE_ACCELERATION * sin(entity->rotation);
        }

        if (GAMESTATE->keys['W'])
        {
            entity->velocity.x += THRUSTER_ACCELERATION * cos(entity->rotation);
            entity->velocity.y += THRUSTER_ACCELERATION * sin(entity->rotation);
        }

        if (GAMESTATE->keys['Q'])
        {
            entity->velocity.x += THRUSTER_ACCELERATION * cos(entity->rotation + M_PI_2);
            entity->velocity.y += THRUSTER_ACCELERATION * sin(entity->rotation + M_PI_2);
        }

        if (GAMESTATE->keys['S'])
        {
            entity->velocity.x += THRUSTER_ACCELERATION * cos(entity->rotation + M_PI);
            entity->velocity.y += THRUSTER_ACCELERATION * sin(entity->rotation + M_PI);
        }

        if (GAMESTATE->keys['E'])
        {
            entity->velocity.x += THRUSTER_ACCELERATION * cos(entity->rotation + M_PI + M_PI_2);
            entity->velocity.y += THRUSTER_ACCELERATION * sin(entity->rotation + M_PI + M_PI_2);
        }
    }

    double velocityMagnitude = sqrt(pow(entity->velocity.x, 2) + pow(entity->velocity.y, 2));

    if (velocityMagnitude > MAX_FIGHTER_VELOCITY)
    {
        double tempAngle = atan2(entity->velocity.y, entity->velocity.x);
        entity->velocity.x = MAX_FIGHTER_VELOCITY * cos(tempAngle);
        entity->velocity.y = MAX_FIGHTER_VELOCITY * sin(tempAngle);
    }

    if (GAMESTATE->keys['A'])
    {
        entity->rotationVelocity = +ROTATION_ACCELERATION;
    }

    if (GAMESTATE->keys['D'])
    {
        entity->rotationVelocity = -ROTATION_ACCELERATION;
    }

    if (!GAMESTATE->keys['A'] && !GAMESTATE->keys['D'])
    {
        entity->rotationVelocity -= ROTATION_ACCELERATION * SIGNOF(entity->rotationVelocity);
        if (fabs(entity->rotationVelocity) <= ROTATION_ACCELERATION)
        {
            entity->rotationVelocity = 0.0;
        }
    }
}
#undef MAX_FIGHTER_VELOCITY
#undef MAIN_DRIVE_ACCELERATION
#undef THRUSTER_ACCELERATION
#undef ROTATION_ACCELERATION

#define FIGHTER_FIRE_DELAY 8
void OnTickKeyFireBullet(Entity *entity)
{
    if (GAMESTATE->keys[VK_SPACE])
    {
        if (entity->fireDelay <= 0)
        {
            SpawnFiredBullet(entity);
            entity->fireDelay = FIGHTER_FIRE_DELAY;
        }
    }
}
#undef FIGHTER_FIRE_DELAY

void OnTickReduceFireDelay(Entity *entity)
{
    if (entity->fireDelay > 0)
    {
        entity->fireDelay -= 1;
    }
}
