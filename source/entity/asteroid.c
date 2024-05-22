#include "../../headers/entity/asteroid.h"

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

    settingUpEntity->onDestroy = AsteroidDestroy;
    List_Insert(&settingUpEntity->onDraw, OnDrawVertexLines);
    List_Insert(&settingUpEntity->onTick, OnTickRotation);
    List_Insert(&settingUpEntity->onTick, OnTickVelocity);

    List_Insert(&GAMESTATE->asteroids, settingUpEntity);
}

void AsteroidDestroy(Entity *entity)
{
    ReadWriteLock_GetWritePermission(GAMESTATE->asteroids.readWriteLock);
    List_RemoveElementWithMatchingData(&GAMESTATE->asteroids, entity);
    ReadWriteLock_ReleaseWritePermission(GAMESTATE->asteroids.readWriteLock);

    EntityDestroy(entity);
}

#define DEFAULT_AXIS_LENGTH 16
#define DEFAULT_AXIS_VARIATION 8
#define AXIS_VARIATION_CALCULATION(value) \
    (value % DEFAULT_AXIS_VARIATION)
void SetupAsteroidVertices(Entity *settingUpEntity)
{
    short deltaX;
    short deltaY;

    RANDOMIZE(deltaX);
    RANDOMIZE(deltaY);

    Point *baseVertex = malloc(sizeof(Point));

    baseVertex->x = DEFAULT_AXIS_LENGTH + AXIS_VARIATION_CALCULATION(deltaX);
    baseVertex->y = 0 + AXIS_VARIATION_CALCULATION(deltaY);

    List_Insert(&settingUpEntity->baseVertices, baseVertex);

    RANDOMIZE(deltaX);
    RANDOMIZE(deltaY);

    baseVertex = malloc(sizeof(Point));

    baseVertex->x = 0 + AXIS_VARIATION_CALCULATION(deltaX);
    baseVertex->y = DEFAULT_AXIS_LENGTH + AXIS_VARIATION_CALCULATION(deltaY);

    List_Insert(&settingUpEntity->baseVertices, baseVertex);

    RANDOMIZE(deltaX);
    RANDOMIZE(deltaY);

    baseVertex = malloc(sizeof(Point));

    baseVertex->x = -DEFAULT_AXIS_LENGTH + AXIS_VARIATION_CALCULATION(deltaX);
    baseVertex->y = 0 + AXIS_VARIATION_CALCULATION(deltaY);

    List_Insert(&settingUpEntity->baseVertices, baseVertex);

    RANDOMIZE(deltaX);
    RANDOMIZE(deltaY);

    baseVertex = malloc(sizeof(Point));

    baseVertex->x = 0 + AXIS_VARIATION_CALCULATION(deltaX);
    baseVertex->y = -DEFAULT_AXIS_LENGTH + AXIS_VARIATION_CALCULATION(deltaY);

    List_Insert(&settingUpEntity->baseVertices, baseVertex);

    CalculateCentroidAndAlignVertices(settingUpEntity);
    CalculateAndSetRotationOffsetVertices(settingUpEntity);
}

#undef DEFAULT_AXIS_LENGTH
#undef DEFAULT_AXIS_VARIATION
#undef AXIS_VARIATION_CALCULATION
