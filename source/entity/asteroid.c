#include "asteroid.h"

void SpawnAsteroid()
{
    Entity *settingUpEntity;

    ZeroAndInitEntity(&settingUpEntity);

    SetupRandomVelocity(settingUpEntity);
    SetupRandomRotation(settingUpEntity);
    SetupRandomRotationSpeed(settingUpEntity);
    SetupLocationAsteroid(settingUpEntity);
    SetupVerticesAsteroid(settingUpEntity);
    SetupRadius(settingUpEntity);

    settingUpEntity->onDestroy = AsteroidDestroy;
    List_Insert(&settingUpEntity->onMovementWithLocationLock, OnMovementWithLocationLockGameEdgeCheckAsteroid);
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
    if (!ReadWriteLock_TryGetWritePermission(&GAMESTATE->asteroids, (void **)&asteroids))
    {
        Task *task;
        MemoryManager_AllocateMemory((void **)&task, sizeof(Task));
        task->task = (void (*)(void *))AsteroidDestroy;
        task->taskArgument = entity;

        Task_PushGarbageTask(task);

        return;
    }

    List_RemoveElementWithMatchingData(asteroids, entity);

    ReadWriteLock_ReleaseWritePermission(&GAMESTATE->asteroids, (void **)&asteroids);

    EntityDestroy(entity);
}

#define EXTRA_OFFSCREEN_LOCATION_SPACE 600
void SetupLocationAsteroid(Entity *settingUpEntity)
{
    short deltaX;
    short deltaY;

    RANDOMIZE(deltaX);
    RANDOMIZE(deltaY);

    Point *location;
    ReadWriteLock_GetWritePermission(&settingUpEntity->location, (void **)&location);

    if (settingUpEntity->velocity.x > 0)
    {
        if (deltaX >= 0)
        {
            location->x = MAX_GAME_SPACE_LEFT - EXTRA_OFFSCREEN_LOCATION_SPACE;
            location->y = deltaY % (MAX_GAME_SPACE_HEIGHT / 2);
        }
        else
        {
            location->x = deltaX % (MAX_GAME_SPACE_WIDTH / 2);

            if (settingUpEntity->velocity.y >= 0)
            {
                location->y = MAX_GAME_SPACE_BOTTOM - EXTRA_OFFSCREEN_LOCATION_SPACE;
            }
            else
            {
                location->y = MAX_GAME_SPACE_TOP + EXTRA_OFFSCREEN_LOCATION_SPACE;
            }
        }
    }
    else
    {
        if (deltaX <= 0)
        {
            location->x = MAX_GAME_SPACE_RIGHT + EXTRA_OFFSCREEN_LOCATION_SPACE;
            location->y = deltaY % (MAX_GAME_SPACE_HEIGHT / 2);
        }
        else
        {
            location->x = deltaX % (MAX_GAME_SPACE_WIDTH / 2);

            if (settingUpEntity->velocity.y >= 0)
            {
                location->y = MAX_GAME_SPACE_BOTTOM - EXTRA_OFFSCREEN_LOCATION_SPACE;
            }
            else
            {
                location->y = MAX_GAME_SPACE_TOP + EXTRA_OFFSCREEN_LOCATION_SPACE;
            }
        }
    }

    ReadWriteLock_ReleaseWritePermission(&settingUpEntity->location, (void **)&location);
}

#define DEFAULT_AXIS_LENGTH 16
#define DEFAULT_AXIS_VARIATION 8
#define AXIS_VARIATION_CALCULATION(value) \
    (value % DEFAULT_AXIS_VARIATION)
void SetupVerticesAsteroid(Entity *settingUpEntity)
{
    short deltaX;
    short deltaY;

    RANDOMIZE(deltaX);
    RANDOMIZE(deltaY);

    Point *baseVertex;
    MemoryManager_AllocateMemory((void **)&baseVertex, sizeof(Point));

    baseVertex->x = DEFAULT_AXIS_LENGTH + AXIS_VARIATION_CALCULATION(deltaX);
    baseVertex->y = 0 + AXIS_VARIATION_CALCULATION(deltaY);

    List_Insert(&settingUpEntity->baseVertices, baseVertex);

    RANDOMIZE(deltaX);
    RANDOMIZE(deltaY);

    MemoryManager_AllocateMemory((void **)&baseVertex, sizeof(Point));

    baseVertex->x = 0 + AXIS_VARIATION_CALCULATION(deltaX);
    baseVertex->y = DEFAULT_AXIS_LENGTH + AXIS_VARIATION_CALCULATION(deltaY);

    List_Insert(&settingUpEntity->baseVertices, baseVertex);

    RANDOMIZE(deltaX);
    RANDOMIZE(deltaY);

    MemoryManager_AllocateMemory((void **)&baseVertex, sizeof(Point));

    baseVertex->x = -DEFAULT_AXIS_LENGTH + AXIS_VARIATION_CALCULATION(deltaX);
    baseVertex->y = 0 + AXIS_VARIATION_CALCULATION(deltaY);

    List_Insert(&settingUpEntity->baseVertices, baseVertex);

    RANDOMIZE(deltaX);
    RANDOMIZE(deltaY);

    MemoryManager_AllocateMemory((void **)&baseVertex, sizeof(Point));

    baseVertex->x = 0 + AXIS_VARIATION_CALCULATION(deltaX);
    baseVertex->y = -DEFAULT_AXIS_LENGTH + AXIS_VARIATION_CALCULATION(deltaY);

    List_Insert(&settingUpEntity->baseVertices, baseVertex);

    CalculateCentroidAndAlignVertices(settingUpEntity);
    CalculateAndSetRotationOffsetVertices(settingUpEntity);
}

#undef DEFAULT_AXIS_LENGTH
#undef DEFAULT_AXIS_VARIATION
#undef AXIS_VARIATION_CALCULATION

void OnMovementWithLocationLockGameEdgeCheckAsteroid(Entity *entity, Point *location)
{

    __int8 outsideOfArea = FALSE;

    if (location->x < MAX_GAME_SPACE_LEFT)
    {
        outsideOfArea = TRUE;
    }
    else if (location->x > MAX_GAME_SPACE_RIGHT)
    {
        outsideOfArea = TRUE;
    }

    if (location->y < MAX_GAME_SPACE_BOTTOM)
    {
        outsideOfArea = TRUE;
    }
    else if (location->y > MAX_GAME_SPACE_TOP)
    {
        outsideOfArea = TRUE;
    }

    if (outsideOfArea)
    {
        ListElmt *tempListElement;
        if (!List_GetElementWithMatchingData(&entity->onTick, &tempListElement, OnTickExpire))
        {
            entity->lifetime = SECONDS_TO_TICKS(30);
            List_Insert(&entity->onTick, OnTickExpire);
        }
    }
    else
    {
        List_RemoveElementWithMatchingData(&entity->onTick, OnTickExpire);
    }
}
