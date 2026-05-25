#ifndef PLAYER_H
#define PLAYER_H
#include <GL/glu.h>
#include <GL/glut.h>
#include <stdint.h>
#include "chunks.h"
#include "vectors.h"

typedef struct Player {
    Vec3 position;
    Vec3 velocity;
    int isOnGround;
    int isInWater;
    float width;
    float height;
} Player;

Block *blockAtPosition(int voxelX, int voxelY, int voxelZ);
int isSolidVoxel(int voxelX, int voxelY, int voxelZ);
float playerHalfWidth(Player* player);
int playerCollides(Player* player);
void updatePlayerPhysics(Player *player);

#endif