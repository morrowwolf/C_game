#include "../headers/entity.h"

void ZeroAndInitEntity(Entity **entity)
{
    *entity = malloc(sizeof(Entity));
    ZeroMemory(*entity, sizeof(Entity));
    List_Init(&(*entity)->baseVertices, List_FreeOnRemove);
    List_Init(&(*entity)->rotationOffsetVertices, List_FreeOnRemove);
    List_Init(&(*entity)->onCollision, NULL);
    List_Init(&(*entity)->onDeath, NULL);
    List_Insert(&(*entity)->onDeath, EntityDeath);
    List_Init(&(*entity)->onDraw, NULL);
    List_Init(&(*entity)->onTick, NULL);
    (*entity)->onDestroy = EntityDestroy;
    (*entity)->entityNumber = GAMESTATE->runningEntityID;
    (*entity)->alive = ENTITY_ALIVE;
    GAMESTATE->runningEntityID += 1;
}

void EntityDeath(Entity *entity)
{
    if (entity->alive == ENTITY_DEAD)
    {
        return;
    }

    entity->alive = ENTITY_DEAD;
    ReadWriteLock_GetWritePermission(GAMESTATE->deadEntities.readWriteLock);
    List_Insert(&GAMESTATE->deadEntities, entity);
    ReadWriteLock_ReleaseWritePermission(GAMESTATE->deadEntities.readWriteLock);
}

void EntityDestroy(Entity *entity)
{
    ReadWriteLock_GetWritePermission(GAMESTATE->entities.readWriteLock);
    List_RemoveElementWithMatchingData(&GAMESTATE->entities, entity);
    ReadWriteLock_ReleaseWritePermission(GAMESTATE->entities.readWriteLock);
    List_Destroy(&entity->baseVertices);
    List_Destroy(&entity->rotationOffsetVertices);
    List_Destroy(&entity->onCollision);
    List_Destroy(&entity->onDeath);
    List_Destroy(&entity->onDraw);
    List_Destroy(&entity->onTick);
    free(entity);
}

void CalculateAndSetRotationOffsetVertices(Entity *entity)
{
    if (entity->baseVertices.length == entity->rotationOffsetVertices.length)
    {
        ListElmt *referenceElementBaseVertices = entity->baseVertices.head;
        ListElmt *referenceElementRotationOffsetVertices = entity->rotationOffsetVertices.head;
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
        List_Clear(&entity->rotationOffsetVertices);

        ListElmt *referenceElementBaseVertices = entity->baseVertices.head;
        while (referenceElementBaseVertices != NULL)
        {
            Point *referencePointVertices = referenceElementBaseVertices->data;
            Point *newRotationOffsetPoint = malloc(sizeof(Point));

            newRotationOffsetPoint->x = CalculateXPointRotation(referencePointVertices, entity->rotation);
            newRotationOffsetPoint->y = CalculateYPointRotation(referencePointVertices, entity->rotation);

            List_Insert(&entity->rotationOffsetVertices, newRotationOffsetPoint);

            referenceElementBaseVertices = referenceElementBaseVertices->next;
        }
    }
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

        settingUpEntity->rotationSpeed = fmod(randomDouble, MAX_RANDOM_ROTATION_SPEED);

    } while ((settingUpEntity->rotationSpeed > -MIN_RANDOM_ROTATION_SPEED && settingUpEntity->rotationSpeed < MIN_RANDOM_ROTATION_SPEED) || isnan(settingUpEntity->rotationSpeed));
}
#undef MIN_RANDOM_ROTATION_SPEED
#undef MAX_RANDOM_ROTATION_SPEED

void SetupLocationCenterOfScreen(Entity *settingUpEntity)
{
    settingUpEntity->location.x = DEFAULT_SCREEN_SIZE_X / 2.0;
    settingUpEntity->location.y = DEFAULT_SCREEN_SIZE_Y / 2.0;
}

#define EXTRA_OFFSCREEN_LOCATION_SPACE 30
void SetupLocationEdgeOfScreen(Entity *settingUpEntity)
{
    short deltaX;
    short deltaY;

    RANDOMIZE(deltaX);
    RANDOMIZE(deltaY);

    if (settingUpEntity->velocity.x > 0)
    {
        if (deltaX >= 0)
        {
            settingUpEntity->location.x = -EXTRA_OFFSCREEN_LOCATION_SPACE;
            settingUpEntity->location.y = abs(deltaY) % DEFAULT_SCREEN_SIZE_Y;
        }
        else
        {
            if (settingUpEntity->velocity.y >= 0)
            {
                settingUpEntity->location.x = abs(deltaX) % DEFAULT_SCREEN_SIZE_X;
                settingUpEntity->location.y = -EXTRA_OFFSCREEN_LOCATION_SPACE;
            }
            else
            {
                settingUpEntity->location.x = abs(deltaX) % DEFAULT_SCREEN_SIZE_X;
                settingUpEntity->location.y = DEFAULT_SCREEN_SIZE_Y + EXTRA_OFFSCREEN_LOCATION_SPACE;
            }
        }
    }
    else
    {
        if (deltaX <= 0)
        {
            settingUpEntity->location.x = DEFAULT_SCREEN_SIZE_X + EXTRA_OFFSCREEN_LOCATION_SPACE;
            settingUpEntity->location.y = abs(deltaY) % DEFAULT_SCREEN_SIZE_Y;
        }
        else
        {
            if (settingUpEntity->velocity.y >= 0)
            {
                settingUpEntity->location.x = abs(deltaX) % DEFAULT_SCREEN_SIZE_X;
                settingUpEntity->location.y = -EXTRA_OFFSCREEN_LOCATION_SPACE;
            }
            else
            {
                settingUpEntity->location.x = abs(deltaX) % DEFAULT_SCREEN_SIZE_X;
                settingUpEntity->location.y = DEFAULT_SCREEN_SIZE_Y + EXTRA_OFFSCREEN_LOCATION_SPACE;
            }
        }
    }
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

    ListIterator *onDeathIterator;
    ListIterator_Init(&onDeathIterator, &entity->onDeath, ReadWriteLock_Read);
    void (*onDeath)(Entity *);
    while (ListIterator_Next(onDeathIterator, (void **)&onDeath))
    {
        onDeath(entity);
    }
    ListIterator_Destroy(onDeathIterator);
}

void OnDrawVertexLines(Entity *entity, HDC *hdc)
{
    ListElmt *referenceElement = entity->rotationOffsetVertices.head;
    Point *referencePoint = referenceElement->data;
    MoveToEx(*hdc, round(entity->location.x + referencePoint->x), round(entity->location.y + referencePoint->y), NULL);

    referenceElement = referenceElement->next;
    while (referenceElement != NULL)
    {
        referencePoint = referenceElement->data;
        LineTo(*hdc, round(entity->location.x + referencePoint->x), round(entity->location.y + referencePoint->y));
        referenceElement = referenceElement->next;
    }

    referenceElement = entity->rotationOffsetVertices.head;
    referencePoint = referenceElement->data;
    LineTo(*hdc, round(entity->location.x + referencePoint->x), round(entity->location.y + referencePoint->y));

#ifdef DEBUG
    TCHAR buffer[16];
    _stprintf(buffer, TEXT("%d"), (int)entity->entityNumber);
    // The text offset for this font is wonky so
    // location is more accurate when slightly higher and left
    TextOut(*hdc, entity->location.x - 3, entity->location.y + 4, buffer, _tcslen(buffer));
#endif
}

void OnTickCheckCollision(Entity *entity)
{
    List entitiesPotentialCollision;
    List_Init(&entitiesPotentialCollision, NULL);

    ListIterator *entitiesIterator;
    ListIterator_Init(&entitiesIterator, &GAMESTATE->entities, ReadWriteLock_Read);
    Entity *referenceOtherEntity;
    while (ListIterator_Next(entitiesIterator, (void **)&referenceOtherEntity))
    {
        if (referenceOtherEntity == entity)
        {
            continue;
        }
        else
        {
            double referenceOtherEntityDistance = max(fabs(referenceOtherEntity->location.x - entity->location.x),
                                                      fabs(referenceOtherEntity->location.y - entity->location.y));
            if (referenceOtherEntityDistance < (entity->radius + referenceOtherEntity->radius))
            {
                List_Insert(&entitiesPotentialCollision, referenceOtherEntity);
            }
        }
    }
    ListIterator_Destroy(entitiesIterator);

    if (entitiesPotentialCollision.length < 1)
    {
        List_Destroy(&entitiesPotentialCollision);

#ifdef DEBUG
        entity->colliding = FALSE;
#endif

        return;
    }

    List entitiesColliding;
    List_Init(&entitiesColliding, NULL);

    ListIterator *entitiesPotentialCollisionIterator;
    ListIterator_Init(&entitiesPotentialCollisionIterator, &entitiesPotentialCollision, ReadWriteLock_Read);
    while (ListIterator_Next(entitiesPotentialCollisionIterator, (void **)&referenceOtherEntity))
    {

        ListIterator *entityVerticesIterator;
        ListIterator_Init(&entityVerticesIterator, &entity->rotationOffsetVertices, ReadWriteLock_Read);
        Point *referenceEntityVertex;
        while (ListIterator_Next(entityVerticesIterator, (void **)&referenceEntityVertex))
        {
            Point entityPoint;
            entityPoint.x = referenceEntityVertex->x + entity->location.x;
            entityPoint.y = referenceEntityVertex->y + entity->location.y;

            double otherEntityLocationToEntityPoint = max(fabs(referenceOtherEntity->location.x - entityPoint.x),
                                                          fabs(referenceOtherEntity->location.y - entityPoint.y));

            if (otherEntityLocationToEntityPoint > referenceOtherEntity->radius)
            {
                continue;
            }

            ListIterator *otherEntityVerticesIterator;
            ListIterator_Init(&otherEntityVerticesIterator, &referenceOtherEntity->rotationOffsetVertices, ReadWriteLock_Read);
            Point *referenceOtherEntityVertex;
            while (ListIterator_Next(otherEntityVerticesIterator, (void **)&referenceOtherEntityVertex))
            {
                Point otherEntityPointOne;
                otherEntityPointOne.x = referenceOtherEntityVertex->x + referenceOtherEntity->location.x;
                otherEntityPointOne.y = referenceOtherEntityVertex->y + referenceOtherEntity->location.y;

                Point otherEntityPointTwo;
                if (ListIterator_AtTail(otherEntityVerticesIterator))
                {
                    ListIterator_GetHead(otherEntityVerticesIterator, (void **)&referenceOtherEntityVertex);
                    otherEntityPointTwo.x = referenceOtherEntityVertex->x + referenceOtherEntity->location.x;
                    otherEntityPointTwo.y = referenceOtherEntityVertex->y + referenceOtherEntity->location.y;
                }
                else
                {
                    ListIterator_Next(otherEntityVerticesIterator, (void **)&referenceOtherEntityVertex);
                    otherEntityPointTwo.x = referenceOtherEntityVertex->x + referenceOtherEntity->location.x;
                    otherEntityPointTwo.y = referenceOtherEntityVertex->y + referenceOtherEntity->location.y;
                    ListIterator_Prev(otherEntityVerticesIterator, (void **)&referenceOtherEntityVertex);
                }

                double diffXOtherEntityLocationAndEntityPoint = referenceOtherEntity->location.x - entityPoint.x;
                double diffYOtherEntityLocationAndEntityPoint = referenceOtherEntity->location.y - entityPoint.y;

                double angleToEntityPoint = atan2(diffYOtherEntityLocationAndEntityPoint, diffXOtherEntityLocationAndEntityPoint);
                double angleToOtherEntityPointOne = atan2(referenceOtherEntity->location.y - otherEntityPointOne.y, referenceOtherEntity->location.x - otherEntityPointOne.x);
                double angleToOtherEntityPointTwo = atan2(referenceOtherEntity->location.y - otherEntityPointTwo.y, referenceOtherEntity->location.x - otherEntityPointTwo.x);

                if ((fabs(angleToOtherEntityPointOne - angleToOtherEntityPointTwo) > M_PI) ? isInBetween(angleToEntityPoint, angleToOtherEntityPointOne, angleToOtherEntityPointTwo) : !isInBetween(angleToEntityPoint, angleToOtherEntityPointOne, angleToOtherEntityPointTwo))
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

                double numerator = (slopeOfEntityLine * referenceOtherEntity->location.x) + (-1.0 * referenceOtherEntity->location.y) + lineConstant;

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

                List_Insert(&entitiesColliding, referenceOtherEntity);
                break;
            }
            ListIterator_Destroy(otherEntityVerticesIterator);
        }
        ListIterator_Destroy(entityVerticesIterator);
    }
    ListIterator_Destroy(entitiesPotentialCollisionIterator);

    if (entitiesColliding.length < 1)
    {
        List_Destroy(&entitiesPotentialCollision);
        List_Destroy(&entitiesColliding);

#ifdef DEBUG
        entity->colliding = FALSE;
#endif

        return;
    }

#ifdef DEBUG
    entity->colliding = TRUE;
#endif

    ListIterator *entitiesCollidingIterator;
    ListIterator_Init(&entitiesCollidingIterator, &entitiesColliding, ReadWriteLock_Read);
    while (ListIterator_Next(entitiesCollidingIterator, (void **)&referenceOtherEntity))
    {
        ListIterator *onCollisionIterator;
        ListIterator_Init(&onCollisionIterator, &entity->onCollision, ReadWriteLock_Read);
        void (*referenceOnCollision)(Entity *, Entity *);
        while (ListIterator_Next(onCollisionIterator, (void **)&referenceOnCollision))
        {
            referenceOnCollision(entity, referenceOtherEntity);
        }
        ListIterator_Destroy(onCollisionIterator);

        ListIterator_Init(&onCollisionIterator, &referenceOtherEntity->onCollision, ReadWriteLock_Read);
        while (ListIterator_Next(onCollisionIterator, (void **)&referenceOnCollision))
        {
            referenceOnCollision(referenceOtherEntity, entity);
        }
        ListIterator_Destroy(onCollisionIterator);
    }
    ListIterator_Destroy(entitiesCollidingIterator);

    List_Destroy(&entitiesPotentialCollision);
    List_Destroy(&entitiesColliding);
}

/// @brief Checks if a number is in between two other numbers. Markers can be in either order.
/// @param primary The number that is to be checked.
/// @param firstMarker The first number to check if in between.
/// @param secondMarker The second number to check if in between.
/// @return Returns 1 if primary is in between firstMarker and secondMarker. Returns 0 otherwise.
int isInBetween(double primary, double firstMarker, double secondMarker)
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

void OnTickRotation(Entity *entity)
{
    if (entity->rotationSpeed == 0.0)
    {
        return;
    }

    entity->rotation += entity->rotationSpeed;

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

void OnTickVelocity(Entity *entity)
{
    entity->location.x += entity->velocity.x;
    entity->location.y += entity->velocity.y;

    if (entity->location.x > DEFAULT_SCREEN_SIZE_X + EXTRA_OFFSCREEN_LOCATION_SPACE)
    {
        entity->location.x = 0.0;
    }
    else if (entity->location.x < 0.0 - EXTRA_OFFSCREEN_LOCATION_SPACE)
    {
        entity->location.x = DEFAULT_SCREEN_SIZE_X;
    }

    if (entity->location.y > DEFAULT_SCREEN_SIZE_Y + EXTRA_OFFSCREEN_LOCATION_SPACE)
    {
        entity->location.y = 0.0;
    }
    else if (entity->location.y < 0.0 - EXTRA_OFFSCREEN_LOCATION_SPACE)
    {
        entity->location.y = DEFAULT_SCREEN_SIZE_Y;
    }
}
#undef EXTRA_OFFSCREEN_LOCATION_SPACE

void List_DestroyEntityOnRemove(void *data)
{
    Entity *entity = data;
    entity->onDestroy(entity);
}
