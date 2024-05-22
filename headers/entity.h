#pragma once
#ifndef ENTITY_H_
#define ENTITY_H_

#include "common_defines.h"

struct EntityInternal;

typedef struct EntityInternal Entity;

struct EntityInternal
{
    unsigned long long entityNumber;
#define ENTITY_ALIVE 1
#define ENTITY_DEAD 0
    short alive; // ENTITY_ALIVE if alive, ENTITY_DEAD if dead
    Point location;
    Vector velocity;
    double rotation;
    double rotationSpeed;
    double radius;

    List baseVertices;           // List of Point
    List rotationOffsetVertices; // List of Point

    // Other than onDestroy we must promise we never modify the
    // Entities global list (whether add or remove) in these functions
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

void OnTickCheckCollision(Entity *);
int isInBetween(double, double, double);

void OnTickRotation(Entity *);

void OnTickVelocity(Entity *);

void List_DestroyEntityOnRemove(void *);
#endif
