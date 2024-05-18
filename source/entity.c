#include "../headers/entity.h"

void ZeroAndInitEntity(Entity **entity)
{
    *entity = malloc(sizeof(Entity));
    ZeroMemory(*entity, sizeof(Entity));
    list_init(&(*entity)->baseVertices, list_free_on_remove);
    list_init(&(*entity)->rotationOffsetVertices, list_free_on_remove);
    list_init(&(*entity)->onCollision, NULL);
    list_init(&(*entity)->onDeath, NULL);
    list_init(&(*entity)->onDraw, NULL);
    list_init(&(*entity)->onTick, NULL);
    (*entity)->entityNumber = GAMESTATE->runningEntityID;
    (*entity)->alive = ENTITY_ALIVE;
    GAMESTATE->runningEntityID += 1;
    list_insert(&GAMESTATE->entities, *entity);
}

void EntityDeath(Entity *entity)
{
    entity->alive = ENTITY_DEAD;
    // list_remove_element_with_matching_data(&GAMESTATE->entities, entity);
    // list_insert(&GAMESTATE->deadEntities, entity);
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
        list_clear(&entity->rotationOffsetVertices);

        ListElmt *referenceElementBaseVertices = entity->baseVertices.head;
        while (referenceElementBaseVertices != NULL)
        {
            Point *referencePointVertices = referenceElementBaseVertices->data;
            Point *newRotationOffsetPoint = malloc(sizeof(Point));

            newRotationOffsetPoint->x = CalculateXPointRotation(referencePointVertices, entity->rotation);
            newRotationOffsetPoint->y = CalculateYPointRotation(referencePointVertices, entity->rotation);

            list_insert(&entity->rotationOffsetVertices, newRotationOffsetPoint);

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

void SetupRandomVelocity(Entity *settingUpEntity)
{
    double randomDouble;

    do
    {
        RANDOMIZE(randomDouble);

        settingUpEntity->velocity.x = fmod(randomDouble, 2.0);

    } while ((settingUpEntity->velocity.x > -0.1 && settingUpEntity->velocity.x < 0.1) || isnan(settingUpEntity->velocity.x));

    do
    {
        RANDOMIZE(randomDouble);

        settingUpEntity->velocity.y = fmod(randomDouble, 2.0);

    } while ((settingUpEntity->velocity.y > -0.1 && settingUpEntity->velocity.y < 0.1) || isnan(settingUpEntity->velocity.y));
}

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

void SetupRandomRotationSpeed(Entity *settingUpEntity)
{
    double randomDouble;

    do
    {
        RANDOMIZE(randomDouble);

        settingUpEntity->rotationSpeed = fmod(randomDouble, 0.1);

    } while ((settingUpEntity->rotationSpeed > -0.01 && settingUpEntity->rotationSpeed < 0.01) || isnan(settingUpEntity->rotationSpeed));
}

void SetupLocationCenterOfScreen(Entity *settingUpEntity)
{
    settingUpEntity->location.x = DEFAULT_SCREEN_SIZE_X / 2.0;
    settingUpEntity->location.y = DEFAULT_SCREEN_SIZE_Y / 2.0;
}

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
            settingUpEntity->location.x = 1;
            settingUpEntity->location.y = abs(deltaY) % DEFAULT_SCREEN_SIZE_Y;
        }
        else
        {
            if (settingUpEntity->velocity.y >= 0)
            {
                settingUpEntity->location.x = abs(deltaX) % DEFAULT_SCREEN_SIZE_X;
                settingUpEntity->location.y = 1;
            }
            else
            {
                settingUpEntity->location.x = abs(deltaX) % DEFAULT_SCREEN_SIZE_X;
                settingUpEntity->location.y = DEFAULT_SCREEN_SIZE_Y - 1;
            }
        }
    }
    else
    {
        if (deltaX <= 0)
        {
            settingUpEntity->location.x = DEFAULT_SCREEN_SIZE_X - 1;
            settingUpEntity->location.y = abs(deltaY) % DEFAULT_SCREEN_SIZE_Y;
        }
        else
        {
            if (settingUpEntity->velocity.y >= 0)
            {
                settingUpEntity->location.x = abs(deltaX) % DEFAULT_SCREEN_SIZE_X;
                settingUpEntity->location.y = 1;
            }
            else
            {
                settingUpEntity->location.x = abs(deltaX) % DEFAULT_SCREEN_SIZE_X;
                settingUpEntity->location.y = DEFAULT_SCREEN_SIZE_Y - 1;
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

int OnCollisionDeath(Entity *entity, Entity *collidingEntity)
{
    UNREFERENCED_PARAMETER(collidingEntity);

    if (!entity->alive)
    {
        return COLLISION_CONTINUE;
    }

    ListElmt *referenceElement = entity->onDeath.head;
    while (referenceElement != NULL)
    {
        ((void (*)(Entity *))referenceElement->data)(entity);
        referenceElement = referenceElement->next;
    }

    return COLLISION_CONTINUE;
}

void OnDrawVertexLines(Entity *entity, HDC *hdc)
{
    unsigned short counter = 0;
    POINT convertedVertices[MAX_VERTICES + 1];

    ListElmt *referenceElement = entity->rotationOffsetVertices.head;

    while (referenceElement != NULL)
    {
        Point *referencePoint = referenceElement->data;
        convertedVertices[counter].x = round(entity->location.x + referencePoint->x);
        convertedVertices[counter].y = round(entity->location.y + referencePoint->y);
        referenceElement = referenceElement->next;
        counter++;
    }

    // NOLINTNEXTLINE
    convertedVertices[counter].x = convertedVertices[0].x;
    convertedVertices[counter].y = convertedVertices[0].y;

    Polyline(*hdc, convertedVertices, counter + 1);

    // TODO: Might be worth just using LineTo and MoveEx instead of Polyline due
    // to having to convert to POINT rather than Point

#ifdef DEBUG
    TCHAR buffer[16];
    _stprintf(buffer, TEXT("%d"), entity->entityNumber);
    TextOut(*hdc, entity->location.x, entity->location.y, buffer, _tcslen(buffer));
#endif
}

void OnTickCheckCollision(Entity *entity)
{
    List entitiesPotentialCollision;
    list_init(&entitiesPotentialCollision, NULL);

    ListElmt *referenceElement = GAMESTATE->entities.head;
    while (referenceElement != NULL)
    {
        Entity *referenceEntity = referenceElement->data;
        if (referenceEntity == entity || referenceEntity->alive == ENTITY_DEAD)
        {
            referenceElement = referenceElement->next;
            continue;
        }
        else
        {
            double referenceEntityDistance = max(fabs(referenceEntity->location.x - entity->location.x),
                                                 fabs(referenceEntity->location.y - entity->location.y));
            if (referenceEntityDistance < (entity->radius + referenceEntity->radius))
            {
                list_insert(&entitiesPotentialCollision, referenceEntity);
            }
        }
        referenceElement = referenceElement->next;
    }

    if (entitiesPotentialCollision.length < 1)
    {
        list_clear(&entitiesPotentialCollision);

#ifdef DEBUG
        entity->colliding = FALSE;
#endif

        return;
    }

    List entitiesColliding;
    list_init(&entitiesColliding, NULL);

    referenceElement = entitiesPotentialCollision.head;
    while (referenceElement != NULL)
    {
        Entity *referenceEntity = referenceElement->data;

        ListElmt *referenceElementVertexOtherEntity = referenceEntity->rotationOffsetVertices.head;
        while (referenceElementVertexOtherEntity != NULL)
        {
            Point otherEntityPointPrimary;
            otherEntityPointPrimary.x = ((Point *)referenceElementVertexOtherEntity->data)->x + referenceEntity->location.x;
            otherEntityPointPrimary.y = ((Point *)referenceElementVertexOtherEntity->data)->y + referenceEntity->location.y;
            Point otherEntityPointSecondary;
            if (referenceElementVertexOtherEntity->next == NULL)
            {
                otherEntityPointSecondary.x = ((Point *)referenceEntity->rotationOffsetVertices.head->data)->x + referenceEntity->location.x;
                otherEntityPointSecondary.y = ((Point *)referenceEntity->rotationOffsetVertices.head->data)->y + referenceEntity->location.y;
            }
            else
            {
                otherEntityPointSecondary.x = ((Point *)referenceElementVertexOtherEntity->next->data)->x + referenceEntity->location.x;
                otherEntityPointSecondary.y = ((Point *)referenceElementVertexOtherEntity->next->data)->y + referenceEntity->location.y;
            }

            double angleToPointSecondary = atan2(otherEntityPointPrimary.y - otherEntityPointSecondary.y, otherEntityPointPrimary.x - otherEntityPointSecondary.x);

            ListElmt *referenceElementVertexEntity = entity->rotationOffsetVertices.head;
            while (referenceElementVertexEntity != NULL)
            {
                Point entityPointOne;
                entityPointOne.x = ((Point *)referenceElementVertexEntity->data)->x + entity->location.x;
                entityPointOne.y = ((Point *)referenceElementVertexEntity->data)->y + entity->location.y;
                Point entityPointTwo;
                if (referenceElementVertexEntity->next == NULL)
                {
                    entityPointTwo.x = ((Point *)entity->rotationOffsetVertices.head->data)->x + entity->location.x;
                    entityPointTwo.y = ((Point *)entity->rotationOffsetVertices.head->data)->y + entity->location.y;
                }
                else
                {
                    entityPointTwo.x = ((Point *)referenceElementVertexEntity->next->data)->x + entity->location.x;
                    entityPointTwo.y = ((Point *)referenceElementVertexEntity->next->data)->y + entity->location.y;
                }

                double angleToPointOne = atan2(otherEntityPointPrimary.y - entityPointOne.y, otherEntityPointPrimary.x - entityPointOne.x);
                double angleToPointTwo = atan2(otherEntityPointPrimary.y - entityPointTwo.y, otherEntityPointPrimary.x - entityPointTwo.x);

                if (fabs(angleToPointOne - angleToPointTwo) > M_PI)
                {
                    if (!isInBetween(angleToPointSecondary, angleToPointOne, angleToPointTwo))
                    {
                        referenceElementVertexEntity = referenceElementVertexEntity->next;
                        continue;
                    }
                }
                else
                {
                    if (isInBetween(angleToPointSecondary, angleToPointOne, angleToPointTwo))
                    {
                        referenceElementVertexEntity = referenceElementVertexEntity->next;
                        continue;
                    }
                }

                double topEquation = ((entityPointOne.y - otherEntityPointPrimary.y) * (entityPointTwo.x - entityPointOne.x)) - ((entityPointOne.x - otherEntityPointPrimary.x) * (entityPointTwo.y - entityPointOne.y));
                double lengthOfEntityLine = sqrt(pow(entityPointTwo.x - entityPointOne.x, 2.0) + pow(entityPointTwo.y - entityPointOne.y, 2));

                double intersectionLocation = topEquation / pow(lengthOfEntityLine, 2.0);

                double distanceBetweenIntersectAndOtherEntityPointPrimary = fabs(intersectionLocation) * lengthOfEntityLine;

                double lengthOfOtherEntityLine = sqrt(pow(otherEntityPointSecondary.x - otherEntityPointPrimary.x, 2.0) + pow(otherEntityPointSecondary.y - otherEntityPointPrimary.y, 2));

                if (lengthOfOtherEntityLine < distanceBetweenIntersectAndOtherEntityPointPrimary)
                {
                    referenceElementVertexEntity = referenceElementVertexEntity->next;
                    continue;
                }

                list_insert(&entitiesColliding, referenceEntity);
                break;
            }

            referenceElementVertexOtherEntity = referenceElementVertexOtherEntity->next;
        }

        referenceElement = referenceElement->next;
    }

    if (entitiesColliding.length < 1)
    {
        list_clear(&entitiesPotentialCollision);
        list_clear(&entitiesColliding);

#ifdef DEBUG
        entity->colliding = FALSE;
#endif

        return;
    }

#ifdef DEBUG
    entity->colliding = TRUE;
#endif

    int collisionReturn = COLLISION_CONTINUE;
    referenceElement = entity->onCollision.head;
    while (referenceElement != NULL && collisionReturn != COLLISION_OVER)
    {
        ListElmt *referenceElementOtherEntity = entitiesColliding.head;
        while (referenceElementOtherEntity != NULL && collisionReturn != COLLISION_OVER)
        {
            Entity *otherEntity = referenceElementOtherEntity->data;
            collisionReturn = ((int (*)(Entity *, Entity *))referenceElement->data)(entity, otherEntity);
            referenceElementOtherEntity = referenceElementOtherEntity->next;
        }
        referenceElement = referenceElement->next;
    }

    if (collisionReturn != COLLISION_CONTINUE)
    {
        list_clear(&entitiesPotentialCollision);
        list_clear(&entitiesColliding);
        return;
    }

    referenceElement = entity->onCollision.head;
    while (referenceElement != NULL && collisionReturn != COLLISION_OVER)
    {
        ListElmt *referenceElementOtherEntity = entitiesColliding.head;
        while (referenceElementOtherEntity != NULL && collisionReturn != COLLISION_OVER)
        {
            Entity *otherEntity = referenceElementOtherEntity->data;
            collisionReturn = ((int (*)(Entity *, Entity *))referenceElement->data)(entity, otherEntity);
            referenceElementOtherEntity = referenceElementOtherEntity->next;
        }
        referenceElement = referenceElement->next;
    }

    list_clear(&entitiesPotentialCollision);
    list_clear(&entitiesColliding);
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
    else if (firstMarker > primary || secondMarker < primary)
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

    if (entity->rotation < 0.0)
    {
        entity->rotation += 2.0 * M_PI;
    }
    else if (entity->rotation > 2 * M_PI)
    {
        entity->rotation -= 2.0 * M_PI;
    }

    CalculateAndSetRotationOffsetVertices(entity);
}

void OnTickVelocity(Entity *entity)
{
    entity->location.x += entity->velocity.x;
    entity->location.y += entity->velocity.y;

    if (entity->location.x > DEFAULT_SCREEN_SIZE_X)
    {
        entity->location.x = 0.0;
    }
    else if (entity->location.x < 0.0)
    {
        entity->location.x = DEFAULT_SCREEN_SIZE_X;
    }

    if (entity->location.y > DEFAULT_SCREEN_SIZE_Y)
    {
        entity->location.y = 0.0;
    }
    else if (entity->location.y < 0.0)
    {
        entity->location.y = DEFAULT_SCREEN_SIZE_Y;
    }
}
