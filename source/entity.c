#include "../headers/entity.h"

void ZeroAndInitEntity(Entity **entity)
{
    *entity = malloc(sizeof(Entity));
    ZeroMemory(*entity, sizeof(Entity));

    Point *location = calloc(1, sizeof(Point));
    ReadWriteLock_Init(&(*entity)->location, location);

    List_Init(&(*entity)->baseVertices, List_FreeOnRemove);

    List *rotationOffsetVertices = calloc(1, sizeof(List));
    List_Init(rotationOffsetVertices, List_FreeOnRemove);
    ReadWriteLock_Init(&(*entity)->rotationOffsetVertices, rotationOffsetVertices);

    List_Init(&(*entity)->onCollision, NULL);
    List_Init(&(*entity)->onDeath, NULL);
    List_Insert(&(*entity)->onDeath, EntityDeath);
    List_Init(&(*entity)->onDraw, NULL);
    List_Init(&(*entity)->onTick, NULL);
    (*entity)->onDestroy = EntityDestroy;

    (*entity)->entityNumber = GAMESTATE->runningEntityID;
    (*entity)->alive = ENTITY_ALIVE;
    InterlockedIncrement64((__int64 *)&GAMESTATE->runningEntityID);
}

void EntityDeath(Entity *entity)
{
    if (!InterlockedCompareExchange(&entity->alive, ENTITY_DEAD, ENTITY_ALIVE))
    {
        return;
    }

    List *deadEntities;
    ReadWriteLock_GetWritePermission(&GAMESTATE->deadEntities, (void **)&deadEntities);

    List_Insert(deadEntities, entity);

    ReadWriteLock_ReleaseWritePermission(&GAMESTATE->deadEntities, (void **)&deadEntities);
}

void EntityDestroy(Entity *entity)
{
    List *entities;
    ReadWriteLock_GetWritePermission(&GAMESTATE->entities, (void **)&entities);

    List_RemoveElementWithMatchingData(entities, entity);

    ReadWriteLock_ReleaseWritePermission(&GAMESTATE->entities, (void **)&entities);

    Point *location;
    ReadWriteLock_GetWritePermission(&entity->location, (void **)&location);
    free(location);
    ReadWriteLock_Destroy(&entity->location);

    List_Clear(&entity->baseVertices);

    List *rotationOffsetVertices;
    ReadWriteLock_GetWritePermission(&entity->rotationOffsetVertices, (void **)&rotationOffsetVertices);
    List_Clear(rotationOffsetVertices);
    free(rotationOffsetVertices);
    ReadWriteLock_Destroy(&entity->rotationOffsetVertices);

    List_Clear(&entity->onCollision);
    List_Clear(&entity->onDeath);
    List_Clear(&entity->onDraw);
    List_Clear(&entity->onTick);
    free(entity);
}

void EntitySpawn(Entity *settingUpEntity)
{
    List *entities;
    ReadWriteLock_GetWritePermission(&GAMESTATE->entities, (void **)&entities);

    List_Insert(entities, settingUpEntity);

    ReadWriteLock_ReleaseWritePermission(&GAMESTATE->entities, (void **)&entities);
}

void CalculateAndSetRotationOffsetVertices(Entity *entity)
{
    List *rotationOffsetVertices;
    ReadWriteLock_GetWritePermission(&entity->rotationOffsetVertices, (void **)&rotationOffsetVertices);

    if (entity->baseVertices.length == rotationOffsetVertices->length)
    {
        ListElmt *referenceElementBaseVertices = entity->baseVertices.head;
        ListElmt *referenceElementRotationOffsetVertices = rotationOffsetVertices->head;
        while (referenceElementBaseVertices != NULL && referenceElementRotationOffsetVertices != NULL)
        {
            Point *referencePointVertices = referenceElementBaseVertices->data;
            Point *referencePointRotationOffsetVertices = referenceElementRotationOffsetVertices->data;

            referencePointRotationOffsetVertices->x = CalculateXPointRotation(referencePointVertices, entity->rotation);
            referencePointRotationOffsetVertices->y = CalculateYPointRotation(referencePointVertices, entity->rotation);

            referenceElementBaseVertices = referenceElementBaseVertices->next;
            referenceElementRotationOffsetVertices = referenceElementRotationOffsetVertices->next;
        }
    }
    else
    {
        List_Clear(rotationOffsetVertices);

        ListElmt *referenceElementBaseVertices = entity->baseVertices.head;
        while (referenceElementBaseVertices != NULL)
        {
            Point *referencePointVertices = referenceElementBaseVertices->data;
            Point *newRotationOffsetPoint = malloc(sizeof(Point));

            newRotationOffsetPoint->x = CalculateXPointRotation(referencePointVertices, entity->rotation);
            newRotationOffsetPoint->y = CalculateYPointRotation(referencePointVertices, entity->rotation);

            List_Insert(rotationOffsetVertices, newRotationOffsetPoint);

            referenceElementBaseVertices = referenceElementBaseVertices->next;
        }
    }

    ReadWriteLock_ReleaseWritePermission(&entity->rotationOffsetVertices, (void **)&rotationOffsetVertices);
}

double CalculateXPointRotation(Point *offsetLocation, double rotation)
{
    return (offsetLocation->x * cos(rotation)) - (offsetLocation->y * sin(rotation));
}

double CalculateYPointRotation(Point *offsetLocation, double rotation)
{
    return (offsetLocation->x * sin(rotation)) + (offsetLocation->y * cos(rotation));
}

/// @brief Finds the centroid of the polygon based on its vertices and adjusts the vertices based on it.
/// Entity must have at least three vertices and must be defined so as its area != 0.
/// @param entity The entity holding vertex data.
void CalculateCentroidAndAlignVertices(Entity *entity)
{
    if (entity->baseVertices.length < 3)
    {
        return;
    }

    unsigned int i;
    unsigned int j;
    double tempArea = 0.0;
    double tempX = 0.0;
    double tempY = 0.0;

    // Unused for now, possibly use for weighted interactions?
    // double area;

    for (i = entity->baseVertices.length - 1, j = 0; j < entity->baseVertices.length; i = j, j++)
    {
        double areaOfCurrentI;
        ListElmt *elementAtI;
        ListElmt *elementAtJ;

        List_GetElementAtPosition(&entity->baseVertices, &elementAtI, i);
        List_GetElementAtPosition(&entity->baseVertices, &elementAtJ, j);

        Point *pointAtI = ((Point *)elementAtI->data);
        Point *pointAtJ = ((Point *)elementAtJ->data);

        areaOfCurrentI = (pointAtI->x * pointAtJ->y) - (pointAtJ->x * pointAtI->y);

        tempArea += areaOfCurrentI;
        tempX += (pointAtJ->x + pointAtI->x) * areaOfCurrentI;
        tempY += (pointAtJ->y + pointAtI->y) * areaOfCurrentI;
    }

    // area = tempArea / 2;

    if (tempArea == 0)
    {
        return;
    }

    double centroidX = tempX / (3.0 * tempArea);
    double centroidY = tempY / (3.0 * tempArea);

    ListElmt *referenceElement = entity->baseVertices.head;
    while (referenceElement != NULL)
    {
        Point *referencePoint = referenceElement->data;
        referencePoint->x -= centroidX;
        referencePoint->y -= centroidY;

        referenceElement = referenceElement->next;
    }
}

#define MAX_RANDOM_VELOCITY 2.0
#define MIN_RANDOM_VELOCITY 0.1
void SetupRandomVelocity(Entity *settingUpEntity)
{
    double randomDouble;

    do
    {
        RANDOMIZE(randomDouble);

        settingUpEntity->velocity.x = fmod(randomDouble, MAX_RANDOM_VELOCITY);

    } while ((settingUpEntity->velocity.x > -MIN_RANDOM_VELOCITY && settingUpEntity->velocity.x < MIN_RANDOM_VELOCITY) || isnan(settingUpEntity->velocity.x));

    do
    {
        RANDOMIZE(randomDouble);

        settingUpEntity->velocity.y = fmod(randomDouble, MAX_RANDOM_VELOCITY);

    } while ((settingUpEntity->velocity.y > -MIN_RANDOM_VELOCITY && settingUpEntity->velocity.y < MIN_RANDOM_VELOCITY) || isnan(settingUpEntity->velocity.y));
}
#undef MIN_RANDOM_VELOCITY
#undef MAX_RANDOM_VELOCITY

void SetupRandomRotation(Entity *settingUpEntity)
{
    double randomDouble;

    do
    {
        RANDOMIZE(randomDouble);

        settingUpEntity->rotation = fmod(randomDouble, 2.0 * M_PI);

    } while (isnan(settingUpEntity->rotation));

    CalculateAndSetRotationOffsetVertices(settingUpEntity);
}

#define MAX_RANDOM_ROTATION_SPEED 0.05
#define MIN_RANDOM_ROTATION_SPEED 0.01
void SetupRandomRotationSpeed(Entity *settingUpEntity)
{
    double randomDouble;

    do
    {
        RANDOMIZE(randomDouble);

        settingUpEntity->rotationVelocity = fmod(randomDouble, MAX_RANDOM_ROTATION_SPEED);

    } while ((settingUpEntity->rotationVelocity > -MIN_RANDOM_ROTATION_SPEED && settingUpEntity->rotationVelocity < MIN_RANDOM_ROTATION_SPEED) || isnan(settingUpEntity->rotationVelocity));
}
#undef MIN_RANDOM_ROTATION_SPEED
#undef MAX_RANDOM_ROTATION_SPEED

void SetupLocationCenterOfScreen(Entity *settingUpEntity)
{
    Point *location;
    ReadWriteLock_GetWritePermission(&settingUpEntity->location, (void **)&location);

    location->x = DEFAULT_SCREEN_SIZE_X / 2.0;
    location->y = DEFAULT_SCREEN_SIZE_Y / 2.0;

    ReadWriteLock_ReleaseWritePermission(&settingUpEntity->location, (void **)&location);
}

#define EXTRA_OFFSCREEN_LOCATION_SPACE 30
void SetupLocationEdgeOfScreen(Entity *settingUpEntity)
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
            location->x = -EXTRA_OFFSCREEN_LOCATION_SPACE;
            location->y = abs(deltaY) % DEFAULT_SCREEN_SIZE_Y;
        }
        else
        {
            if (settingUpEntity->velocity.y >= 0)
            {
                location->x = abs(deltaX) % DEFAULT_SCREEN_SIZE_X;
                location->y = -EXTRA_OFFSCREEN_LOCATION_SPACE;
            }
            else
            {
                location->x = abs(deltaX) % DEFAULT_SCREEN_SIZE_X;
                location->y = DEFAULT_SCREEN_SIZE_Y + EXTRA_OFFSCREEN_LOCATION_SPACE;
            }
        }
    }
    else
    {
        if (deltaX <= 0)
        {
            location->x = DEFAULT_SCREEN_SIZE_X + EXTRA_OFFSCREEN_LOCATION_SPACE;
            location->y = abs(deltaY) % DEFAULT_SCREEN_SIZE_Y;
        }
        else
        {
            if (settingUpEntity->velocity.y >= 0)
            {
                location->x = abs(deltaX) % DEFAULT_SCREEN_SIZE_X;
                location->y = -EXTRA_OFFSCREEN_LOCATION_SPACE;
            }
            else
            {
                location->x = abs(deltaX) % DEFAULT_SCREEN_SIZE_X;
                location->y = DEFAULT_SCREEN_SIZE_Y + EXTRA_OFFSCREEN_LOCATION_SPACE;
            }
        }
    }

    ReadWriteLock_ReleaseWritePermission(&settingUpEntity->location, (void **)&location);
}

void SetupRadius(Entity *entity)
{
    double largestDistance = 0;
    ListElmt *referenceElement = entity->baseVertices.head;
    while (referenceElement != NULL)
    {
        Point *referencePoint = referenceElement->data;
        double maximumNumber = max(fabs(referencePoint->x), fabs(referencePoint->y));
        if (largestDistance < maximumNumber)
        {
            largestDistance = maximumNumber;
        }
        referenceElement = referenceElement->next;
    }
    entity->radius = largestDistance;
}

void OnCollisionDeath(Entity *entity, Entity *collidingEntity)
{
    UNREFERENCED_PARAMETER(collidingEntity);

    ListIterator onDeathIterator;
    ListIterator_Init(&onDeathIterator, &entity->onDeath);
    void (*onDeath)(Entity *);
    while (ListIterator_Next(&onDeathIterator, (void **)&onDeath))
    {
        onDeath(entity);
    }
}

void OnDrawVertexLines(Entity *entity, HDC *hdc)
{
    Point *referenceLocation;
    ReadWriteLock_GetReadPermission(&entity->location, (void **)&referenceLocation);

    Point location;
    location.x = referenceLocation->x;
    location.y = referenceLocation->y;

    ReadWriteLock_ReleaseReadPermission(&entity->location, (void **)&referenceLocation);

    List *rotationOffsetVertices;
    ReadWriteLock_GetReadPermission(&entity->rotationOffsetVertices, (void **)&rotationOffsetVertices);

    ListIterator rotationOffsetVerticesIterator;
    ListIterator_Init(&rotationOffsetVerticesIterator, rotationOffsetVertices);
    Point *point;

    ListIterator_Next(&rotationOffsetVerticesIterator, (void **)&point);
    MoveToEx(*hdc, round(location.x + point->x), round(location.y + point->y), NULL);

    while (ListIterator_Next(&rotationOffsetVerticesIterator, (void **)&point))
    {
        LineTo(*hdc, round(location.x + point->x), round(location.y + point->y));
    }

    ListIterator_Next(&rotationOffsetVerticesIterator, (void **)&point);
    LineTo(*hdc, round(location.x + point->x), round(location.y + point->y));

    ReadWriteLock_ReleaseReadPermission(&entity->rotationOffsetVertices, (void **)&rotationOffsetVertices);

#ifdef DEBUG
    TCHAR buffer[16];
    _stprintf(buffer, TEXT("%d"), (int)entity->entityNumber);
    // The text offset for this font is wonky so
    // location is more accurate when slightly higher and left
    TextOut(*hdc, location.x - 3, location.y + 4, buffer, _tcslen(buffer));
#endif
}

void OnTickHandleMovement(Entity *entity)
{
    if (fabs(entity->velocityThisTick.x) <= MINIMUM_FLOAT_DIFFERENCE && fabs(entity->velocityThisTick.y) <= MINIMUM_FLOAT_DIFFERENCE && fabs(entity->rotationVelocityThisTick) <= MINIMUM_FLOAT_DIFFERENCE)
    {
        entity->velocityThisTick.x = entity->velocity.x;
        entity->velocityThisTick.y = entity->velocity.y;
        entity->rotationVelocityThisTick = entity->rotationVelocity;
    }

    HandleMovementVelocity(entity);
    HandleMovementRotation(entity);
    HandleMovementCollisionCheck(entity);

    if (fabs(entity->velocityThisTick.x) > MINIMUM_FLOAT_DIFFERENCE || fabs(entity->velocityThisTick.y) > MINIMUM_FLOAT_DIFFERENCE || fabs(entity->rotationVelocityThisTick) > MINIMUM_FLOAT_DIFFERENCE)
    {
        Task *task = calloc(1, sizeof(Task));
        task->task = (void (*)(void *))OnTickHandleMovement;
        task->taskArguments = entity;

        List *taskQueue;
        ReadWriteLock_GetWritePermission(&TASKSTATE->taskQueue, (void **)&taskQueue);

        List_Insert(taskQueue, task);

        ReadWriteLock_ReleaseWritePermission(&TASKSTATE->taskQueue, (void **)&taskQueue);
    }
}

void HandleMovementCollisionCheck(Entity *entity)
{
    List otherEntitiesPotentialCollision;
    List_Init(&otherEntitiesPotentialCollision, List_DestroyCollisionDataHolderOnRemove);

    Point *referenceEntityLocation;
    ReadWriteLock_GetReadPermission(&entity->location, (void **)&referenceEntityLocation);

    Point entityLocation;
    entityLocation.x = referenceEntityLocation->x;
    entityLocation.y = referenceEntityLocation->y;

    ReadWriteLock_ReleaseReadPermission(&entity->location, (void **)&referenceEntityLocation);

    List *referenceEntityRotationOffsetVertices;
    ReadWriteLock_GetReadPermission(&entity->rotationOffsetVertices, (void **)&referenceEntityRotationOffsetVertices);

    List entityRotationOffsetVertices;
    List_Init(&entityRotationOffsetVertices, List_FreeOnRemove);

    ListIterator referenceEntityRotationOffsetVerticesIterator;
    ListIterator_Init(&referenceEntityRotationOffsetVerticesIterator, referenceEntityRotationOffsetVertices);
    Point *referenceEntityRotationOffsetVertex;
    while (ListIterator_Next(&referenceEntityRotationOffsetVerticesIterator, (void **)&referenceEntityRotationOffsetVertex))
    {
        Point *entityRotationOffsetVertex = malloc(sizeof(Point));
        entityRotationOffsetVertex->x = referenceEntityRotationOffsetVertex->x;
        entityRotationOffsetVertex->y = referenceEntityRotationOffsetVertex->y;

        List_Insert(&entityRotationOffsetVertices, entityRotationOffsetVertex);
    }

    ReadWriteLock_ReleaseReadPermission(&entity->rotationOffsetVertices, (void **)&referenceEntityRotationOffsetVertices);

    List *entities;
    ReadWriteLock_GetReadPermission(&GAMESTATE->entities, (void **)&entities);

    ListIterator entitiesIterator;
    ListIterator_Init(&entitiesIterator, entities);
    Entity *otherEntity;
    while (ListIterator_Next(&entitiesIterator, (void **)&otherEntity))
    {
        if (otherEntity == entity)
        {
            continue;
        }
        else
        {
            List permissionRequests;
            List_Init(&permissionRequests, List_DestroyReadWriteLockPermissionRequestOnRemove);

            ReadWriteLock_PermissionRequest *otherEntityLocationPermissionRequest = calloc(1, sizeof(ReadWriteLock_PermissionRequest));
            otherEntityLocationPermissionRequest->readWriteLock = &otherEntity->location;
            otherEntityLocationPermissionRequest->permissionType = ReadWriteLock_Read;

            List_Insert(&permissionRequests, otherEntityLocationPermissionRequest);

            ReadWriteLock_PermissionRequest *otherEntityVerticesPermissionRequest = calloc(1, sizeof(ReadWriteLock_PermissionRequest));
            otherEntityVerticesPermissionRequest->readWriteLock = &otherEntity->rotationOffsetVertices;
            otherEntityVerticesPermissionRequest->permissionType = ReadWriteLock_Read;

            List_Insert(&permissionRequests, otherEntityVerticesPermissionRequest);

            while (!ReadWriteLock_GetMultiplePermissions(&permissionRequests, 5))
            {
                if (GAMESTATE->exiting)
                {
                    List_Clear(&entityRotationOffsetVertices);
                    List_Clear(&otherEntitiesPotentialCollision);
                    List_Clear(&permissionRequests);
                    return;
                }
            }

            Point *referenceOtherEntityLocation = ((ReadWriteLock_PermissionRequest *)(permissionRequests.head->data))->returnedData;
            double otherEntityDistance = max(fabs(referenceOtherEntityLocation->x - entityLocation.x),
                                             fabs(referenceOtherEntityLocation->y - entityLocation.y));

            if (otherEntityDistance > (entity->radius + otherEntity->radius))
            {
                List_Clear(&permissionRequests);
                continue;
            }

            CollisionDataHolder *collisionDataHolder = calloc(1, sizeof(CollisionDataHolder));
            CheckCollisionDataHolder_Init(collisionDataHolder, otherEntity, referenceOtherEntityLocation, ((ReadWriteLock_PermissionRequest *)(permissionRequests.tail->data))->returnedData);

            List_Insert(&otherEntitiesPotentialCollision, collisionDataHolder);
            List_Clear(&permissionRequests);
        }
    }

    ReadWriteLock_ReleaseReadPermission(&GAMESTATE->entities, (void **)&entities);

    if (otherEntitiesPotentialCollision.length < 1)
    {
        List_Clear(&entityRotationOffsetVertices);
        List_Clear(&otherEntitiesPotentialCollision);

#ifdef DEBUG
        entity->colliding = FALSE;
#endif

        return;
    }

    List entitiesColliding;
    List_Init(&entitiesColliding, NULL);

    ListIterator otherEntitiesPotentialCollisionIterator;
    ListIterator_Init(&otherEntitiesPotentialCollisionIterator, &otherEntitiesPotentialCollision);
    CollisionDataHolder *otherEntityDataHolder;
    while (ListIterator_Next(&otherEntitiesPotentialCollisionIterator, (void **)&otherEntityDataHolder))
    {

        ListIterator entityVerticesIterator;
        ListIterator_Init(&entityVerticesIterator, &entityRotationOffsetVertices);
        Point *entityVertex;
        while (ListIterator_Next(&entityVerticesIterator, (void **)&entityVertex))
        {

            Point entityPoint;
            entityPoint.x = entityVertex->x + entityLocation.x;
            entityPoint.y = entityVertex->y + entityLocation.y;

            double otherEntityLocationToEntityPoint = max(fabs(otherEntityDataHolder->location.x - entityPoint.x),
                                                          fabs(otherEntityDataHolder->location.y - entityPoint.y));

            if (otherEntityLocationToEntityPoint > otherEntityDataHolder->entity->radius)
            {
                continue;
            }

            ListIterator otherEntityVerticesIterator;
            ListIterator_Init(&otherEntityVerticesIterator, &otherEntityDataHolder->vertices);
            Point *referenceOtherEntityVertex;
            while (ListIterator_Next(&otherEntityVerticesIterator, (void **)&referenceOtherEntityVertex))
            {
                Point otherEntityPointOne;
                otherEntityPointOne.x = referenceOtherEntityVertex->x + otherEntityDataHolder->location.x;
                otherEntityPointOne.y = referenceOtherEntityVertex->y + otherEntityDataHolder->location.y;

                Point otherEntityPointTwo;
                if (ListIterator_AtTail(&otherEntityVerticesIterator))
                {
                    ListIterator_GetHead(&otherEntityVerticesIterator, (void **)&referenceOtherEntityVertex);
                    otherEntityPointTwo.x = referenceOtherEntityVertex->x + otherEntityDataHolder->location.x;
                    otherEntityPointTwo.y = referenceOtherEntityVertex->y + otherEntityDataHolder->location.y;
                }
                else
                {
                    ListIterator_Next(&otherEntityVerticesIterator, (void **)&referenceOtherEntityVertex);
                    otherEntityPointTwo.x = referenceOtherEntityVertex->x + otherEntityDataHolder->location.x;
                    otherEntityPointTwo.y = referenceOtherEntityVertex->y + otherEntityDataHolder->location.y;
                    ListIterator_Prev(&otherEntityVerticesIterator, (void **)&referenceOtherEntityVertex);
                }

                double diffXOtherEntityLocationAndEntityPoint = otherEntityDataHolder->location.x - entityPoint.x;
                double diffYOtherEntityLocationAndEntityPoint = otherEntityDataHolder->location.y - entityPoint.y;

                double angleToEntityPoint = atan2(diffYOtherEntityLocationAndEntityPoint, diffXOtherEntityLocationAndEntityPoint);
                double angleToOtherEntityPointOne = atan2(otherEntityDataHolder->location.y - otherEntityPointOne.y, otherEntityDataHolder->location.x - otherEntityPointOne.x);
                double angleToOtherEntityPointTwo = atan2(otherEntityDataHolder->location.y - otherEntityPointTwo.y, otherEntityDataHolder->location.x - otherEntityPointTwo.x);

                if ((fabs(angleToOtherEntityPointOne - angleToOtherEntityPointTwo) > M_PI) ? IsInBetween(angleToEntityPoint, angleToOtherEntityPointOne, angleToOtherEntityPointTwo) : !IsInBetween(angleToEntityPoint, angleToOtherEntityPointOne, angleToOtherEntityPointTwo))
                {
                    continue;
                }

                double distanceBetweenOtherEntityLocationAndEntityPoint = max(fabs(diffXOtherEntityLocationAndEntityPoint), fabs(diffYOtherEntityLocationAndEntityPoint));

                double otherEntityPointsDiffX = otherEntityPointOne.x - otherEntityPointTwo.x;
                double otherEntityPointsDiffY = otherEntityPointOne.y - otherEntityPointTwo.y;

                double slopeOfEntityLine;
                if (otherEntityPointsDiffX == 0.0 || otherEntityPointsDiffY == 0.0)
                {
                    slopeOfEntityLine = 0.0;
                }
                else
                {
                    slopeOfEntityLine = otherEntityPointsDiffY / otherEntityPointsDiffX;
                }

                double lineConstant = otherEntityPointOne.y - (slopeOfEntityLine * otherEntityPointOne.x);

                double numerator = (slopeOfEntityLine * otherEntityDataHolder->location.x) + (-1.0 * otherEntityDataHolder->location.y) + lineConstant;

                double denominator = sqrt(pow(slopeOfEntityLine, 2.0) + 1.0);

                double distanceFromOtherEntityLocationToOtherEntityLine = fabs(numerator / denominator);

                double angleFromOtherEntityLocationToOtherEntityLine = atan2(-1 * otherEntityPointsDiffX, otherEntityPointsDiffY);

                double angleDiff = fabs(angleFromOtherEntityLocationToOtherEntityLine - angleToEntityPoint);

                if (angleDiff > M_PI)
                {
                    angleDiff = (2 * M_PI) - angleDiff;
                }

                double distanceFromOtherEntityLocationToOtherEntityLineAtCorrectAngle = fabs(distanceFromOtherEntityLocationToOtherEntityLine / cos(angleDiff));

                if (distanceFromOtherEntityLocationToOtherEntityLineAtCorrectAngle < distanceBetweenOtherEntityLocationAndEntityPoint)
                {
                    continue;
                }

                List_Insert(&entitiesColliding, otherEntityDataHolder->entity);
                break;
            }
        }
    }

    if (entitiesColliding.length < 1)
    {
        List_Clear(&entityRotationOffsetVertices);
        List_Clear(&otherEntitiesPotentialCollision);
        List_Clear(&entitiesColliding);

#ifdef DEBUG
        entity->colliding = FALSE;
#endif

        return;
    }

#ifdef DEBUG
    entity->colliding = TRUE;
#endif

    ListIterator entitiesCollidingIterator;
    ListIterator_Init(&entitiesCollidingIterator, &entitiesColliding);
    while (ListIterator_Next(&entitiesCollidingIterator, (void **)&otherEntity))
    {
        ListIterator onCollisionIterator;
        ListIterator_Init(&onCollisionIterator, &entity->onCollision);
        void (*onCollision)(Entity *, Entity *);
        while (ListIterator_Next(&onCollisionIterator, (void **)&onCollision))
        {
            onCollision(entity, otherEntity);
        }

        ListIterator_Init(&onCollisionIterator, &otherEntity->onCollision);
        while (ListIterator_Next(&onCollisionIterator, (void **)&onCollision))
        {
            onCollision(otherEntity, entity);
        }
    }

    List_Clear(&entityRotationOffsetVertices);
    List_Clear(&otherEntitiesPotentialCollision);
    List_Clear(&entitiesColliding);
}

void CheckCollisionDataHolder_Init(CollisionDataHolder *collisionDataHolder, Entity *entity, Point *location, List *verticesList)
{
    collisionDataHolder->entity = entity;

    collisionDataHolder->location.x = location->x;
    collisionDataHolder->location.y = location->y;

    List_Init(&collisionDataHolder->vertices, List_FreeOnRemove);

    ListIterator verticesListIterator;
    ListIterator_Init(&verticesListIterator, verticesList);
    Point *referenceVertex;
    while (ListIterator_Next(&verticesListIterator, (void **)&referenceVertex))
    {
        Point *copiedVertex = malloc(sizeof(Point));
        copiedVertex->x = referenceVertex->x;
        copiedVertex->y = referenceVertex->y;

        List_Insert(&collisionDataHolder->vertices, copiedVertex);
    }
}

void CheckCollisionDataHolder_Destroy(CollisionDataHolder *collisionDataHolder)
{
    List_Clear(&collisionDataHolder->vertices);
    free(collisionDataHolder);
}

void List_DestroyCollisionDataHolderOnRemove(void *data)
{
    CheckCollisionDataHolder_Destroy(data);
}

/// @brief Checks if a number is in between two other numbers. Markers can be in either order.
/// @param primary The number that is to be checked.
/// @param firstMarker The first number to check if in between.
/// @param secondMarker The second number to check if in between.
/// @return Returns 1 if primary is in between firstMarker and secondMarker. Returns 0 otherwise.
int IsInBetween(double primary, double firstMarker, double secondMarker)
{
    if (firstMarker >= secondMarker && (primary < secondMarker || primary > firstMarker))
    {
        return 0;
    }
    else if (firstMarker < secondMarker && (primary < firstMarker || primary > secondMarker))
    {
        return 0;
    }

    return 1;
}

#define MAX_ROTATION_PER_SUBTICK ((2.0 * M_PI) / 10.0)
void HandleMovementRotation(Entity *entity)
{
    if (fabs(entity->rotationVelocity) <= MINIMUM_FLOAT_DIFFERENCE || fabs(entity->rotationVelocityThisTick) <= MINIMUM_FLOAT_DIFFERENCE)
    {
        entity->rotationVelocityThisTick = 0.0;
        return;
    }

    int sign = SIGNOF(entity->rotationVelocityThisTick);
    double rotationThisSubTick = min(fabs(entity->rotationVelocityThisTick), MAX_ROTATION_PER_SUBTICK) * sign;

    entity->rotationVelocityThisTick -= rotationThisSubTick;

    if (fabs(entity->rotationVelocityThisTick) <= MINIMUM_FLOAT_DIFFERENCE)
    {
        entity->rotationVelocityThisTick = 0.0;
    }

    entity->rotation += rotationThisSubTick;

    while (entity->rotation < 0.0)
    {
        entity->rotation += 2.0 * M_PI;
    }

    while (entity->rotation > 2 * M_PI)
    {
        entity->rotation -= 2.0 * M_PI;
    }

    CalculateAndSetRotationOffsetVertices(entity);
}
#undef MAX_ROTATION_PER_SUBTICK

#define MAX_VELOCITY_PER_SUBTICK 2.0
void HandleMovementVelocity(Entity *entity)
{

    Point *entityLocation;
    ReadWriteLock_GetWritePermission(&entity->location, (void **)&entityLocation);

    if ((fabs(entity->velocity.x) <= MINIMUM_FLOAT_DIFFERENCE &&
         fabs(entity->velocity.y) <= MINIMUM_FLOAT_DIFFERENCE) ||
        (fabs(entity->velocityThisTick.x) <= MINIMUM_FLOAT_DIFFERENCE &&
         fabs(entity->velocityThisTick.y) <= MINIMUM_FLOAT_DIFFERENCE))
    {
        entity->velocityThisTick.x = 0.0;
        entity->velocityThisTick.y = 0.0;
        ReadWriteLock_ReleaseWritePermission(&entity->location, (void **)&entityLocation);
        return;
    }

    double tempAngle = atan2(entity->velocityThisTick.y, entity->velocityThisTick.x);

    int signX = SIGNOF(entity->velocityThisTick.x);
    int signY = SIGNOF(entity->velocityThisTick.y);

    double velocityXThisSubTick = min(fabs(MAX_VELOCITY_PER_SUBTICK * cos(tempAngle)), fabs(entity->velocityThisTick.x)) * signX;
    double velocityYThisSubTick = min(fabs(MAX_VELOCITY_PER_SUBTICK * sin(tempAngle)), fabs(entity->velocityThisTick.y)) * signY;

    entity->velocityThisTick.x -= velocityXThisSubTick;
    entity->velocityThisTick.y -= velocityYThisSubTick;

    if (fabs(entity->velocityThisTick.x) < MINIMUM_FLOAT_DIFFERENCE)
    {
        entity->velocityThisTick.x = 0.0;
    }

    if (fabs(entity->velocityThisTick.y) < MINIMUM_FLOAT_DIFFERENCE)
    {
        entity->velocityThisTick.y = 0.0;
    }

    entityLocation->x += velocityXThisSubTick;
    entityLocation->y += velocityYThisSubTick;

    if (entityLocation->x > DEFAULT_SCREEN_SIZE_X + EXTRA_OFFSCREEN_LOCATION_SPACE)
    {
        entityLocation->x -= DEFAULT_SCREEN_SIZE_X + EXTRA_OFFSCREEN_LOCATION_SPACE;
    }
    else if (entityLocation->x < 0.0 - EXTRA_OFFSCREEN_LOCATION_SPACE)
    {
        entityLocation->x += DEFAULT_SCREEN_SIZE_X + EXTRA_OFFSCREEN_LOCATION_SPACE;
    }

    if (entityLocation->y > DEFAULT_SCREEN_SIZE_Y + EXTRA_OFFSCREEN_LOCATION_SPACE)
    {
        entityLocation->y -= DEFAULT_SCREEN_SIZE_Y + EXTRA_OFFSCREEN_LOCATION_SPACE;
    }
    else if (entityLocation->y < 0.0 - EXTRA_OFFSCREEN_LOCATION_SPACE)
    {
        entityLocation->y += DEFAULT_SCREEN_SIZE_Y + EXTRA_OFFSCREEN_LOCATION_SPACE;
    }

    ReadWriteLock_ReleaseWritePermission(&entity->location, (void **)&entityLocation);
}
#undef MAX_VELOCITY_PER_SUBTICK
#undef EXTRA_OFFSCREEN_LOCATION_SPACE

void List_DestroyEntityOnRemove(void *data)
{
    Entity *entity = data;
    entity->onDestroy(entity);
}
