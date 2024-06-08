#include "asteroid.h"

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
    List_Insert(&settingUpEntity->onRender, OnRenderUpdate);
    List_Insert(&settingUpEntity->onTick, OnTickHandleMovement);

    List *asteroids;
    ReadWriteLock_GetWritePermission(&GAMESTATE->asteroids, (void **)&asteroids);

    List_Insert(asteroids, settingUpEntity);

    ReadWriteLock_ReleaseWritePermission(&GAMESTATE->asteroids, (void **)&asteroids);

    EntitySpawn(settingUpEntity);
}

void AsteroidDestroy(Entity *entity)
{
    List *asteroids;
    ReadWriteLock_GetWritePermission(&GAMESTATE->asteroids, (void **)&asteroids);

    List_RemoveElementWithMatchingData(asteroids, entity);

    ReadWriteLock_ReleaseWritePermission(&GAMESTATE->asteroids, (void **)&asteroids);

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
