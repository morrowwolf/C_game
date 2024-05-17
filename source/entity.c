#include "../headers/entity.h"

void ZeroAndInitEntity(Entity **entity)
{
    *entity = malloc(sizeof(Entity));
    ZeroMemory(*entity, sizeof(Entity));
    list_init(&(*entity)->vertices, list_free_on_remove);
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
    ListElmt *referenceElement = entity->vertices.head;
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
    entity->radius = largestDistance * 0.75;
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

    ListElmt *referenceElement;

    if (entity->rotationOffsetVertices.length > 0)
    {
        referenceElement = entity->rotationOffsetVertices.head;
    }
    else
    {
        referenceElement = entity->vertices.head;
    }

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

#ifdef DEBUG
    TCHAR buffer[16];
    _stprintf(buffer, TEXT("%d"), entity->entityNumber);
    TextOut(*hdc, entity->location.x, entity->location.y, buffer, _tcslen(buffer));
#endif
}

void OnTickCheckCollision(Entity *entity)
{
    Entity *closestEntity = NULL;
    double closestEntityDistance = 0;
    ListElmt *referenceElement = GAMESTATE->entities.head;
    while (referenceElement != NULL)
    {
        Entity *referenceEntity = referenceElement->data;
        if (referenceEntity == entity || referenceEntity->alive == ENTITY_DEAD)
        {
            referenceElement = referenceElement->next;
            continue;
        }
        else if (closestEntity == NULL)
        {
            closestEntity = referenceEntity;
            closestEntityDistance = max(fabs(closestEntity->location.x - entity->location.x),
                                        fabs(closestEntity->location.y - entity->location.y));
        }
        else
        {
            double referenceEntityDistance = max(fabs(referenceEntity->location.x - entity->location.x),
                                                 fabs(referenceEntity->location.y - entity->location.y));
            if (closestEntityDistance > referenceEntityDistance)
            {
                closestEntity = referenceEntity;
                closestEntityDistance = referenceEntityDistance;
            }
        }
        referenceElement = referenceElement->next;
    }

    if (closestEntity == NULL)
    {
        return;
    }

    if (closestEntityDistance > (entity->radius + closestEntity->radius))
    {
        return;
    }

    int collisionReturn = COLLISION_CONTINUE;
    referenceElement = entity->onCollision.head;
    while (referenceElement != NULL && collisionReturn != COLLISION_OVER)
    {
        collisionReturn = ((int (*)(Entity *, Entity *))referenceElement->data)(entity, closestEntity);
        referenceElement = referenceElement->next;
    }

    if (collisionReturn != COLLISION_CONTINUE)
    {
        return;
    }

    referenceElement = entity->onCollision.head;
    while (referenceElement != NULL && collisionReturn != COLLISION_OVER)
    {
        collisionReturn = ((int (*)(Entity *, Entity *))referenceElement->data)(closestEntity, entity);
        referenceElement = referenceElement->next;
    }
}

void OnTickRotation(Entity *entity)
{
    if (entity->rotationSpeed != 0.0)
    {
        entity->rotation += entity->rotationSpeed;

        if (entity->rotation < 0.0)
        {
            entity->rotation += 2.0 * M_PI;
        }
        else if (entity->rotation > 2 * M_PI)
        {
            entity->rotation -= 2.0 * M_PI;
        }

        if (entity->rotation == 0.0)
        {
            list_clear(&entity->rotationOffsetVertices);
        }
        else if (entity->vertices.length == entity->rotationOffsetVertices.length)
        {
            ListElmt *referenceElementVertices = entity->vertices.head;
            ListElmt *referenceElementRotationOffsetVertices = entity->rotationOffsetVertices.head;
            while (referenceElementVertices != NULL && referenceElementRotationOffsetVertices != NULL)
            {
                Point *referencePointVertices = referenceElementVertices->data;
                Point *referencePointRotationOffsetVertices = referenceElementRotationOffsetVertices->data;

                referencePointRotationOffsetVertices->x = CalculateXPointRotation(referencePointVertices, entity->rotation);
                referencePointRotationOffsetVertices->y = CalculateYPointRotation(referencePointVertices, entity->rotation);

                referenceElementVertices = referenceElementVertices->next;
                referenceElementRotationOffsetVertices = referenceElementRotationOffsetVertices->next;
            }
        }
        else
        {
            list_clear(&entity->rotationOffsetVertices);

            ListElmt *referenceElementVertices = entity->vertices.head;
            while (referenceElementVertices != NULL)
            {
                Point *referencePointVertices = referenceElementVertices->data;
                Point *newRotationOffsetPoint = malloc(sizeof(Point));

                newRotationOffsetPoint->x = CalculateXPointRotation(referencePointVertices, entity->rotation);
                newRotationOffsetPoint->y = CalculateYPointRotation(referencePointVertices, entity->rotation);

                list_insert(&entity->rotationOffsetVertices, newRotationOffsetPoint);

                referenceElementVertices = referenceElementVertices->next;
            }
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
