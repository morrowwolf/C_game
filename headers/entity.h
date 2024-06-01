#pragma once
#ifndef ENTITY_H_
#define ENTITY_H_

#include "globals.h"
#include "tasks.h"

struct EntityInternal;

typedef struct EntityInternal Entity;

struct EntityInternal
{
    unsigned long long entityNumber;
#define ENTITY_ALIVE 1
#define ENTITY_DEAD 0
    volatile long alive; // ENTITY_ALIVE if alive, ENTITY_DEAD if dead

    RWL_Point location;
    Vector velocity;
    Vector velocityThisTick;

    double rotation;
    double rotationVelocity;
    double rotationVelocityThisTick;

    double radius;

    int lifetime;
    int activationDelay;
    int fireDelay;

    List baseVertices;               // List of Point
    RWL_List rotationOffsetVertices; // List of Point

    List onCollision;            // List of int (*)(Entity *, Entity *)
    List onDeath;                // List of void (*)(Entity *)
    void (*onDestroy)(Entity *); // onDestroy would release itself, everyone gets one
    List onDraw;                 // List of void (*)(Entity *, HDC *)
    List onTick;                 // List of void (*)(Entity *)

#ifdef DEBUG
    int colliding;
#endif
};

void ZeroAndInitEntity(Entity **);
void EntityDeath(Entity *);
void EntityDestroy(Entity *);
void EntitySpawn(Entity *);

void CalculateAndSetRotationOffsetVertices(Entity *);
double CalculateXPointRotation(Point *, double);
double CalculateYPointRotation(Point *, double);

void CalculateCentroidAndAlignVertices(Entity *);

void SetupRandomVelocity(Entity *);
void SetupRandomRotation(Entity *);
void SetupRandomRotationSpeed(Entity *);

void SetupLocationCenterOfScreen(Entity *);
void SetupLocationEdgeOfScreen(Entity *);

void SetupRadius(Entity *);

void OnCollisionDeath(Entity *, Entity *);

void OnDrawVertexLines(Entity *, HDC *);

void OnTickHandleMovement(Entity *);

void HandleMovementCollisionCheck(Entity *);
typedef struct
{
    Entity *entity;
    Point location;
    List vertices;
} CollisionDataHolder;

void CheckCollisionDataHolder_Init(CollisionDataHolder *, Entity *, Point *, List *);
void CheckCollisionDataHolder_Destroy(CollisionDataHolder *);
void List_DestroyCollisionDataHolderOnRemove(void *);
int IsInBetween(double, double, double);

void HandleMovementRotation(Entity *);

void HandleMovementVelocity(Entity *);

void List_DestroyEntityOnRemove(void *);
#endif
