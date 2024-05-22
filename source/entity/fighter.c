
#include "../../headers/entity/fighter.h"

void SpawnPlayerFighter()
{
    Entity *settingUpEntity;

    ZeroAndInitEntity(&settingUpEntity);

    SetupLocationCenterOfScreen(settingUpEntity);

    SetupFighterVertices(settingUpEntity);
    SetupRadius(settingUpEntity);

    List_Insert(&settingUpEntity->onCollision, OnCollisionDeath);
    settingUpEntity->onDestroy = FighterDestroy;
    List_Insert(&settingUpEntity->onDraw, OnDrawVertexLines);
    List_Insert(&settingUpEntity->onTick, OnTickCheckCollision);
    List_Insert(&settingUpEntity->onTick, OnTickReduceFireDelay);
    List_Insert(&settingUpEntity->onTick, OnTickKeyAcceleration);
    List_Insert(&settingUpEntity->onTick, OnTickKeyFireBullet);
    List_Insert(&settingUpEntity->onTick, OnTickRotation);
    List_Insert(&settingUpEntity->onTick, OnTickVelocity);

    List_Insert(&GAMESTATE->fighters, settingUpEntity);
    List_Insert(&GAMESTATE->entities, settingUpEntity);
}

void FighterDestroy(Entity *entity)
{
    ReadWriteLock_GetWritePermission(GAMESTATE->fighters.readWriteLock);
    List_RemoveElementWithMatchingData(&GAMESTATE->fighters, entity);
    ReadWriteLock_ReleaseWritePermission(GAMESTATE->fighters.readWriteLock);

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

    if (GAMESTATE->keys['A'])
    {
        entity->rotationSpeed = +ROTATION_ACCELERATION;
    }

    if (GAMESTATE->keys['D'])
    {
        entity->rotationSpeed = -ROTATION_ACCELERATION;
    }

    if (!GAMESTATE->keys['A'] && !GAMESTATE->keys['D'])
    {
        entity->rotationSpeed -= ROTATION_ACCELERATION * SIGNOF(entity->rotationSpeed);
        if (fabs(entity->rotationSpeed) <= ROTATION_ACCELERATION)
        {
            entity->rotationSpeed = 0.0;
        }
    }
}
#undef MAIN_DRIVE_ACCELERATION
#undef THRUSTER_ACCELERATION
#undef ROTATION_ACCELERATION

#define FIGHTER_FIRE_DELAY 5
void OnTickKeyFireBullet(Entity *entity)
{
    if (GAMESTATE->keys[VK_SPACE])
    {
        if (entity->fireDelay <= 0)
        {
            SpawnFiredBullet(entity);
            entity->fireDelay = 5;
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
