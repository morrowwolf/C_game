
#include "../../headers/entity/fighter.h"

void SpawnPlayerFighter()
{
    Entity *settingUpEntity;

    ZeroAndInitEntity(&settingUpEntity);

    SetupLocationCenterOfScreen(settingUpEntity);

    SetupFighterVertices(settingUpEntity);
    SetupRadius(settingUpEntity);

    list_insert(&settingUpEntity->onCollision, OnCollisionDeath);
    list_insert(&settingUpEntity->onDeath, OnDeathFighter);
    list_insert(&settingUpEntity->onDraw, OnDrawVertexLines);
    list_insert(&settingUpEntity->onTick, OnTickCheckCollision);
    list_insert(&settingUpEntity->onTick, OnTickKeyAcceleration);
    list_insert(&settingUpEntity->onTick, OnTickRotation);
    list_insert(&settingUpEntity->onTick, OnTickVelocity);

    list_insert(&GAMESTATE->fighters, settingUpEntity);
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

    list_insert(&settingUpEntity->baseVertices, baseVertex);

    baseVertex = malloc(sizeof(Point));

    baseVertex->x = -5;
    baseVertex->y = -5;

    list_insert(&settingUpEntity->baseVertices, baseVertex);

    baseVertex = malloc(sizeof(Point));

    baseVertex->x = -2;
    baseVertex->y = 0;

    list_insert(&settingUpEntity->baseVertices, baseVertex);

    baseVertex = malloc(sizeof(Point));

    baseVertex->x = -5;
    baseVertex->y = 5;

    list_insert(&settingUpEntity->baseVertices, baseVertex);

    CalculateCentroidAndAlignVertices(settingUpEntity);
    CalculateAndSetRotationOffsetVertices(settingUpEntity);
}

void OnDeathFighter(Entity *entity)
{
    // list_remove_element_with_matching_data(&GAMESTATE->fighters, entity);
    EntityDeath(entity);
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

        if (GAMESTATE->keys['E'])
        {
            entity->velocity.x += THRUSTER_ACCELERATION * cos(entity->rotation + M_PI_2);
            entity->velocity.y += THRUSTER_ACCELERATION * sin(entity->rotation + M_PI_2);
        }

        if (GAMESTATE->keys['S'])
        {
            entity->velocity.x += THRUSTER_ACCELERATION * cos(entity->rotation + M_PI);
            entity->velocity.y += THRUSTER_ACCELERATION * sin(entity->rotation + M_PI);
        }

        if (GAMESTATE->keys['Q'])
        {
            entity->velocity.x += THRUSTER_ACCELERATION * cos(entity->rotation + M_PI + M_PI_2);
            entity->velocity.y += THRUSTER_ACCELERATION * sin(entity->rotation + M_PI + M_PI_2);
        }
    }

    if (GAMESTATE->keys['A'])
    {
        entity->rotationSpeed = -ROTATION_ACCELERATION;
    }

    if (GAMESTATE->keys['D'])
    {
        entity->rotationSpeed = +ROTATION_ACCELERATION;
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
