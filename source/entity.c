#include "entity.h"

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
    List_Init(&(*entity)->onMovementWithLocationLock, NULL);
    List_Init(&(*entity)->onRender, NULL);
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

    Task *task = calloc(1, sizeof(Task));
    task->task = (void (*)(void *))(entity->onDestroy);
    task->taskArgument = entity;

    Task_QueueTask(&TASKSTATE->garbageTaskQueue, &TASKSTATE->garbageTasksQueuedSyncEvents, task);
}

void EntityDestroy(Entity *entity)
{
    List *entities;
    if (!ReadWriteLockPriority_TryGetWritePermission(&GAMESTATE->entities, (void **)&entities))
    {
        Task *task = malloc(sizeof(Task));
        task->task = (void (*)(void *))EntityDestroy;
        task->taskArgument = entity;

        Task_QueueTask(&TASKSTATE->garbageTaskQueue, &TASKSTATE->garbageTasksQueuedSyncEvents, task);

        return;
    }

    if (InterlockedExchange((volatile long *)&GAMESTATE->handlingTick, GAMESTATE->handlingTick) == TRUE)
    {
        ReadWriteLockPriority_ReleaseWritePermission(&GAMESTATE->entities, (void **)&entities);

        Task *task = malloc(sizeof(Task));
        task->task = (void (*)(void *))EntityDestroy;
        task->taskArgument = entity;

        Task_QueueTask(&TASKSTATE->garbageTaskQueue, &TASKSTATE->garbageTasksQueuedSyncEvents, task);

        return;
    }

    List_RemoveElementWithMatchingData(entities, entity);

    ReadWriteLockPriority_ReleaseWritePermission(&GAMESTATE->entities, (void **)&entities);

    Point *location;
    ReadWriteLock_GetWritePermission(&entity->location, (void **)&location);
    free(location);
    ReadWriteLock_ReleaseWritePermission(&entity->location, (void **)&location);
    ReadWriteLock_Destroy(&entity->location);

    List_Clear(&entity->baseVertices);

    List *rotationOffsetVertices;
    ReadWriteLock_GetWritePermission(&entity->rotationOffsetVertices, (void **)&rotationOffsetVertices);
    List_Clear(rotationOffsetVertices);
    free(rotationOffsetVertices);
    ReadWriteLock_ReleaseWritePermission(&entity->rotationOffsetVertices, (void **)&rotationOffsetVertices);
    ReadWriteLock_Destroy(&entity->rotationOffsetVertices);

    List_Clear(&entity->onCollision);
    List_Clear(&entity->onDeath);
    List_Clear(&entity->onMovementWithLocationLock);
    List_Clear(&entity->onRender);
    List_Clear(&entity->onTick);

    EntityDestroyPartTwo(entity);
}

void EntityDestroyPartTwo(Entity *entity)
{
    if (WaitForSingleObject(SCREEN->handlingCommandListMutex, 5) != WAIT_OBJECT_0)
    {
        Task *task = calloc(1, sizeof(Task));
        task->task = (void (*)(void *))EntityDestroyPartTwo;
        task->taskArgument = entity;

        Task_QueueTask(&TASKSTATE->garbageTaskQueue, &TASKSTATE->garbageTasksQueuedSyncEvents, task);

        return;
    }

    if (WaitForSingleObject(SCREEN->fenceEvent, 0) != WAIT_OBJECT_0)
    {
        ReleaseMutex(SCREEN->handlingCommandListMutex);

        Task *task = calloc(1, sizeof(Task));
        task->task = (void (*)(void *))EntityDestroyPartTwo;
        task->taskArgument = entity;

        Task_QueueTask(&TASKSTATE->garbageTaskQueue, &TASKSTATE->garbageTasksQueuedSyncEvents, task);

        return;
    }

    RELEASE(entity->vertexBuffer);
    RELEASE(entity->indexBuffer);

    ReleaseMutex(SCREEN->handlingCommandListMutex);

    free(entity);
}

void EntitySpawn(Entity *settingUpEntity)
{
    List *entities;
    ReadWriteLockPriority_GetWritePermission(&GAMESTATE->entities, (void **)&entities);

    List_Insert(entities, settingUpEntity);

    ReadWriteLockPriority_ReleaseWritePermission(&GAMESTATE->entities, (void **)&entities);
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

void OnRenderUpdate(Entity *entity)
{

    // TODO: use root signature to give the GPU a rotation and location
    // Vertices will then be constants that we can reuse (for the most part)

    RELEASE(entity->vertexBuffer);
    RELEASE(entity->indexBuffer);

    Point *referenceLocation;
    ReadWriteLock_GetReadPermission(&entity->location, (void **)&referenceLocation);

    Point location;
    location.x = referenceLocation->x;
    location.y = referenceLocation->y;

    ReadWriteLock_ReleaseReadPermission(&entity->location, (void **)&referenceLocation);

    double tempX = location.x - SCREEN->screenLocation.x;
    double tempY = location.y - SCREEN->screenLocation.y;

    double tempRadius = sqrt(pow(tempX, 2) + pow(tempY, 2)) - entity->radius;

    if (tempRadius > SCREEN->screenRadius)
    {
        return;
    }

    List *rotationOffsetVertices;
    ReadWriteLock_GetReadPermission(&entity->rotationOffsetVertices, (void **)&rotationOffsetVertices);

    unsigned int lengthOfVerticesList = rotationOffsetVertices->length;

    D3D12_HEAP_PROPERTIES heapPropertyUpload = {
        .Type = D3D12_HEAP_TYPE_UPLOAD,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 1,
        .VisibleNodeMask = 1,
    };

    int vertexBufferSize = lengthOfVerticesList * sizeof(Vertex);

    D3D12_RESOURCE_DESC vertexBufferResource = {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = vertexBufferSize,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {.Count = 1, .Quality = 0},
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
    };

    HANDLE_HRESULT(CALL(CreateCommittedResource, SCREEN->device,
                        &heapPropertyUpload,
                        D3D12_HEAP_FLAG_NONE,
                        &vertexBufferResource,
                        D3D12_RESOURCE_STATE_GENERIC_READ,
                        NULL,
                        IID_PPV_ARGS(&entity->vertexBuffer)));

    int indexBufferSize = (lengthOfVerticesList * sizeof(unsigned int)) + sizeof(unsigned int);

    D3D12_RESOURCE_DESC indexBufferResource = {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = indexBufferSize,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {.Count = 1, .Quality = 0},
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
    };

    HANDLE_HRESULT(CALL(CreateCommittedResource, SCREEN->device,
                        &heapPropertyUpload,
                        D3D12_HEAP_FLAG_NONE,
                        &indexBufferResource,
                        D3D12_RESOURCE_STATE_GENERIC_READ,
                        NULL,
                        IID_PPV_ARGS(&entity->indexBuffer)));

    D3D12_RANGE readRange = {.Begin = 0, .End = 0};

    void *vertexBufferCPUAddress = NULL;
    HANDLE_HRESULT(CALL(Map, entity->vertexBuffer, 0, &readRange, &vertexBufferCPUAddress));

    void *indexBufferCPUAddress = NULL;
    HANDLE_HRESULT(CALL(Map, entity->indexBuffer, 0, &readRange, &indexBufferCPUAddress));

    ListIterator rotationOffsetVerticesIterator;
    ListIterator_Init(&rotationOffsetVerticesIterator, rotationOffsetVertices);
    Point *point;
    while (ListIterator_Next(&rotationOffsetVerticesIterator, (void **)&point))
    {
        float xPoint = 0;
        float yPoint = 0;
        if (SCREEN->screenEntity == entity)
        {
            xPoint = point->x / (SCREEN->screenWidth / 2.0);
            yPoint = point->y / (SCREEN->screenHeight / 2.0);
        }
        else
        {
            xPoint = (point->x + location.x - SCREEN->screenLocation.x) / (SCREEN->screenWidth / 2.0);
            yPoint = (point->y + location.y - SCREEN->screenLocation.y) / (SCREEN->screenHeight / 2.0);
        }

        Vertex vertex = {{xPoint, yPoint, 0.0f},
                         {1.0f, 1.0f, 1.0f, 1.0f}};

        CopyMemory(&((Vertex *)vertexBufferCPUAddress)[rotationOffsetVerticesIterator.currentIteration], &vertex, sizeof(vertex));

        unsigned int index = rotationOffsetVerticesIterator.currentIteration;
        CopyMemory(&((unsigned int *)indexBufferCPUAddress)[rotationOffsetVerticesIterator.currentIteration], &index, sizeof(index));
    }

    unsigned int index = 0UL;
    CopyMemory(&((unsigned int *)indexBufferCPUAddress)[lengthOfVerticesList], &index, sizeof(index));

    CALL(Unmap, entity->indexBuffer, 0, NULL);
    CALL(Unmap, entity->vertexBuffer, 0, NULL);

    ReadWriteLock_ReleaseReadPermission(&entity->rotationOffsetVertices, (void **)&rotationOffsetVertices);

    entity->vertexBufferView.BufferLocation = CALL(GetGPUVirtualAddress, entity->vertexBuffer);
    entity->vertexBufferView.StrideInBytes = sizeof(Vertex);
    entity->vertexBufferView.SizeInBytes = vertexBufferSize;

    entity->indexBufferView.BufferLocation = CALL(GetGPUVirtualAddress, entity->indexBuffer);
    entity->indexBufferView.SizeInBytes = indexBufferSize;
    entity->indexBufferView.Format = DXGI_FORMAT_R32_UINT;

    CALL(IASetIndexBuffer, SCREEN->commandList, &entity->indexBufferView);
    CALL(IASetVertexBuffers, SCREEN->commandList, 0, 1, &entity->vertexBufferView);
    CALL(DrawIndexedInstanced, SCREEN->commandList, (lengthOfVerticesList + 1), 1, 0, 0, 0);

    // TODO: Readd debugging entityID
}

void OnTickExpire(Entity *entity)
{
    entity->lifetime -= 1;
    if (entity->lifetime <= 0)
    {
        ListIterator onDeathIterator;
        ListIterator_Init(&onDeathIterator, &entity->onDeath);
        void (*onDeath)(Entity *);
        while (ListIterator_Next(&onDeathIterator, (void **)&onDeath))
        {
            onDeath(entity);
        }
    }
}

// TODO: Need initiator function so we can get the highest velocity
// and then divide up movement of every entity into the same number of subticks
// Also, stop handling movement when dead
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
        task->taskArgument = entity;

        Task_QueueGamestateTask(task);
    }
}

#define COLLISION_PERMISSION_REQUEST_SIZE 2

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
    ReadWriteLockPriority_GetReadPermission(&GAMESTATE->entities, (void **)&entities);

    ListIterator entitiesIterator;
    ListIterator_Init(&entitiesIterator, entities);
    Entity *otherEntity;
    while (ListIterator_Next(&entitiesIterator, (void **)&otherEntity))
    {
        if (otherEntity == entity)
        {
            continue;
        }

        if (InterlockedExchange(&otherEntity->alive, otherEntity->alive) == ENTITY_DEAD)
        {
            continue;
        }

        ReadWriteLock_PermissionRequest permissionRequests[COLLISION_PERMISSION_REQUEST_SIZE] = {
            {.permissionType = ReadWriteLock_Read, .readWriteLock = &otherEntity->location, .returnedData = NULL},
            {.permissionType = ReadWriteLock_Read, .readWriteLock = &otherEntity->rotationOffsetVertices, .returnedData = NULL}};

        while (!ReadWriteLock_GetMultiplePermissions(permissionRequests, COLLISION_PERMISSION_REQUEST_SIZE))
        {
            if (SCREEN->exiting)
            {
                List_Clear(&entityRotationOffsetVertices);
                List_Clear(&otherEntitiesPotentialCollision);
                return;
            }
        }

        Point *referenceOtherEntityLocation = permissionRequests[0].returnedData;
        double otherEntityDistance = max(fabs(referenceOtherEntityLocation->x - entityLocation.x),
                                         fabs(referenceOtherEntityLocation->y - entityLocation.y));

        if (otherEntityDistance > (entity->radius + otherEntity->radius))
        {
            ReadWriteLock_ReleaseMultiplePermissions(permissionRequests, COLLISION_PERMISSION_REQUEST_SIZE);
            continue;
        }

        CollisionDataHolder *collisionDataHolder = calloc(1, sizeof(CollisionDataHolder));
        CheckCollisionDataHolder_Init(collisionDataHolder, otherEntity, referenceOtherEntityLocation, permissionRequests[1].returnedData);

        List_Insert(&otherEntitiesPotentialCollision, collisionDataHolder);

        ReadWriteLock_ReleaseMultiplePermissions(permissionRequests, COLLISION_PERMISSION_REQUEST_SIZE);
    }

    ReadWriteLockPriority_ReleaseReadPermission(&GAMESTATE->entities, (void **)&entities);

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

#undef COLLISION_PERMISSION_REQUEST_SIZE

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

    ListIterator entityOnMovementWithLockIterator;
    ListIterator_Init(&entityOnMovementWithLockIterator, &entity->onMovementWithLocationLock);
    void (*onMovementWithLock)(Entity *, Point *);
    while (ListIterator_Next(&entityOnMovementWithLockIterator, (void **)&onMovementWithLock))
    {
        onMovementWithLock(entity, entityLocation);
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
