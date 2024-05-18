#ifndef ENTITY_H_
#define ENTITY_H_

#include "common_defines.h"

struct EntityInternal;

typedef struct EntityInternal Entity;

struct EntityInternal
{
    unsigned int entityNumber;
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

    List onCollision; // List of int (*)(Entity *, Entity *)
    List onDeath;     // List of void (*)(Entity *)
    List onDraw;      // List of void (*)(Entity *, HDC *)
    List onTick;      // List of void (*)(Entity *)

#ifdef DEBUG
    int colliding;
#endif
};

void ZeroAndInitEntity(Entity **);
void EntityDeath(Entity *);

void CalculateAndSetRotationOffsetVertices(Entity *entity);
double CalculateXPointRotation(Point *offsetLocation, double rotation);
double CalculateYPointRotation(Point *offsetLocation, double rotation);

void SetupRandomVelocity(Entity *);
void SetupRandomRotation(Entity *);
void SetupRandomRotationSpeed(Entity *);

void SetupLocationCenterOfScreen(Entity *);
void SetupLocationEdgeOfScreen(Entity *);

void SetupRadius(Entity *);

int OnCollisionDeath(Entity *, Entity *);

void OnDrawVertexLines(Entity *, HDC *);

#define COLLISION_CONTINUE_SELF_ONLY 2
#define COLLISION_CONTINUE 1
#define COLLISION_OVER 0
void OnTickCheckCollision(Entity *);
int isInBetween(double primary, double firstMarker, double secondMarker);

void OnTickRotation(Entity *);

void OnTickVelocity(Entity *);

#endif
