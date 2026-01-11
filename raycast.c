#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#include <math.h>
#include "raycast.h"
#include "chunks.h"
#include "input.h"
#include "chunkLoaderManager.h"

void raycastFromCamera() {
    // DDA - digital differential analyzer

    int playerChunkX = (int)floor(CameraX / (ChunkWidthX * BlockWidthX));
    int playerChunkZ = (int)floor(CameraZ / (ChunkLengthZ * BlockLengthZ));
    
    uint64_t chunkKey  = packChunkKey(playerChunkX, playerChunkZ);
    BucketEntry *result = getHashmapEntry(chunkKey);
    
    if (result == NULL) { return; }
    
    float directionVectorLength = sqrtf(
        PlayerDirX*PlayerDirX + 
        PlayerDirY*PlayerDirY + 
        PlayerDirZ*PlayerDirZ
    );
    

    GLfloat normCameraDirX = PlayerDirX / directionVectorLength;
    GLfloat normCameraDirY = PlayerDirY / directionVectorLength;
    GLfloat normCameraDirZ = PlayerDirZ / directionVectorLength;

    float rayUnitStepSizeX = fabs(1.0f / normCameraDirX);
    float rayUnitStepSizeY = fabs(1.0f / normCameraDirY);
    float rayUnitStepSizeZ = fabs(1.0f / normCameraDirZ);


    
    float rayX = CameraX;
    float rayY = CameraY;
    float rayZ = CameraZ;
    int voxelX = floor(rayX);
    int voxelY = floor(rayY);
    int voxelZ = floor(rayZ);

    int stepX, stepY, stepZ;
    float rayLengthX, rayLengthY, rayLengthZ;
    if (normCameraDirX > 0) {
        stepX = 1;
        rayLengthX = (voxelX+1 - rayX) * rayUnitStepSizeX;
    } else {
        stepX = -1;
        rayLengthX = (rayX - voxelX) * rayUnitStepSizeX;
    }

    if (normCameraDirY > 0) {
        stepY = 1;
        rayLengthY = (voxelY+1 - rayY) * rayUnitStepSizeY;
    } else {
        stepY = -1;
        rayLengthY = (rayY - voxelY) * rayUnitStepSizeY;
    }

    if (normCameraDirZ > 0) {
        stepZ = 1;
        rayLengthZ = (voxelZ+1 - rayZ) * rayUnitStepSizeZ;
    } else {
        stepZ = -1;
        rayLengthZ = (rayZ - voxelZ) * rayUnitStepSizeZ;
    }

    
    float dist     = 0.0;
    float maxDist  = 16.0;
    int originalChunkX = floor((float)voxelX / ChunkWidthX);
    int originalChunkZ = floor((float)voxelZ / ChunkLengthZ);
    while (dist < maxDist) {
        // walking step
        if (rayLengthX < rayLengthY && rayLengthX < rayLengthZ) {
            // x step
            voxelX += stepX;
            dist = rayLengthX;
            rayLengthX += rayUnitStepSizeX;
        } else if (rayLengthY < rayLengthZ) {
            // y step
            voxelY += stepY;
            dist = rayLengthY;
            rayLengthY += rayUnitStepSizeY;
        } else {
            // z step
            voxelZ += stepZ;
            dist = rayLengthZ;
            rayLengthZ += rayUnitStepSizeZ;
        }

        int localX = voxelX - originalChunkX * ChunkWidthX;
        int localZ = voxelZ - originalChunkZ * ChunkLengthZ;
        int localY = voxelY;

        int index = localX +
        ChunkWidthX * localZ +
        (ChunkWidthX * ChunkLengthZ) * localY;

        uint64_t chunkKey  = packChunkKey(
            floor((float)(voxelX) / ChunkWidthX), floor((float)(voxelZ) / ChunkLengthZ));
        BucketEntry *result = getHashmapEntry(chunkKey);

        if (result == NULL) {
            break;
        }

        if (!result->chunkEntry->blocks[index].isAir) {
            // hit the block
            printf("hit at dist %f\n", dist);
            break;
        }
    }
}