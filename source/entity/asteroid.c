#include "../../headers/entity/asteroid.h"

#define DEFAULT_AXIS_LENGTH 16
#define DEFAULT_AXIS_VARIATION 8
#define AXIS_VARIATION_CALCULATION(value) \
    ((value % DEFAULT_AXIS_VARIATION) * SIGNOF(value))

void SpawnAsteroid()
{

    Entity *settingUpEntity;

    ZeroAndInitEntity(&settingUpEntity);

    SetupRandomVelocity(settingUpEntity);
    SetupRandomRotation(settingUpEntity);
    SetupRandomRotationSpeed(settingUpEntity);
    SetupLocationEdgeOfScreen(settingUpEntity);

    SetupAsteroidVertices(settingUpEntity);
    SetupRadius(settingUpEntity);

    // list_insert(&settingUpEntity->onCollision, OnCollisionDeath);
    list_insert(&settingUpEntity->onDeath, OnDeathAsteroid);
    list_insert(&settingUpEntity->onDraw, OnDrawVertexLines);
    // list_insert(&settingUpEntity->onTick, OnTickCheckCollision);
    list_insert(&settingUpEntity->onTick, OnTickRotation);
    list_insert(&settingUpEntity->onTick, OnTickVelocity);

    list_insert(&GAMESTATE->asteroids, settingUpEntity);
}

void OnDeathAsteroid(Entity *entity)
{
    list_remove_element_with_matching_data(&GAMESTATE->asteroids, entity);
    EntityDeath(entity);
}

void SetupAsteroidVertices(Entity *settingUpEntity)
{
    short deltaX;
    short deltaY;

    RANDOMIZE(deltaX);
    RANDOMIZE(deltaY);

    Point *vertex = malloc(sizeof(Point));

    vertex->x = DEFAULT_AXIS_LENGTH + AXIS_VARIATION_CALCULATION(deltaX);
    vertex->y = 0 + AXIS_VARIATION_CALCULATION(deltaY);

    list_insert(&settingUpEntity->vertices, vertex);

    RANDOMIZE(deltaX);
    RANDOMIZE(deltaY);

    vertex = malloc(sizeof(Point));

    vertex->x = 0 + AXIS_VARIATION_CALCULATION(deltaX);
    vertex->y = DEFAULT_AXIS_LENGTH + AXIS_VARIATION_CALCULATION(deltaY);

    list_insert(&settingUpEntity->vertices, vertex);

    RANDOMIZE(deltaX);
    RANDOMIZE(deltaY);

    vertex = malloc(sizeof(Point));

    vertex->x = -DEFAULT_AXIS_LENGTH + AXIS_VARIATION_CALCULATION(deltaX);
    vertex->y = 0 + AXIS_VARIATION_CALCULATION(deltaY);

    list_insert(&settingUpEntity->vertices, vertex);

    RANDOMIZE(deltaX);
    RANDOMIZE(deltaY);

    vertex = malloc(sizeof(Point));

    vertex->x = 0 + AXIS_VARIATION_CALCULATION(deltaX);
    vertex->y = -DEFAULT_AXIS_LENGTH + AXIS_VARIATION_CALCULATION(deltaY);

    list_insert(&settingUpEntity->vertices, vertex);
}

#undef defaultAxisLength
#undef defaultAxisVariation
