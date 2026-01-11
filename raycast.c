#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#include <math.h>
#include "raycast.h"
#include "chunks.h"
#include "input.h"
#include "chunkLoaderManager.h"
#include "render.h"

void raycastFromCamera() {
    // DDA - digital differential analyzer, used for raycasting

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
    Block *hitBlock = NULL;
    int hitBlockFace;
    while (dist < maxDist) {
        // walking step
        if (rayLengthX < rayLengthY && rayLengthX < rayLengthZ) {
            // x step
            voxelX += stepX;
            dist = rayLengthX;
            rayLengthX += rayUnitStepSizeX;

            if (stepX == 1) {
                hitBlockFace = FACE_LEFT;
            } else {
                hitBlockFace = FACE_RIGHT;
            }
        } else if (rayLengthY < rayLengthZ) {
            // y step
            voxelY += stepY;
            dist = rayLengthY;
            rayLengthY += rayUnitStepSizeY;
            if (stepY == 1) {
                hitBlockFace = FACE_BOTTOM;
            } else {
                hitBlockFace = FACE_TOP;
            }
        } else {
            // z step
            voxelZ += stepZ;
            dist = rayLengthZ;
            rayLengthZ += rayUnitStepSizeZ;
            if (stepZ == 1) {
                hitBlockFace = FACE_BACK;
            } else {
                hitBlockFace = FACE_FRONT;
            }
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
            hitBlock = &result->chunkEntry->blocks[index];
            break;
        }
    }

    if (hitBlock == NULL) {
        if (selectedBlockToRender.meshQuad != NULL) {
            free(selectedBlockToRender.meshQuad);
        }
        selectedBlockToRender.meshQuad = NULL;
        return;
    }

    printf("Starting the mesh quad selections with %f and %d\n", hitBlock->x, voxelX);
    selectedBlockToRender.meshQuad = malloc(sizeof(MeshQuad));

    selectedBlockToRender.meshQuad->x = hitBlock->x + voxelX;
    selectedBlockToRender.meshQuad->y = hitBlock->y + voxelY;
    selectedBlockToRender.meshQuad->z = hitBlock->z + voxelZ;
    selectedBlockToRender.meshQuad->width = BlockWidthX; 
    // technically putting the same value isn't fully adapting but it would always be the same so it works
    selectedBlockToRender.meshQuad->height = BlockWidthX;
    selectedBlockToRender.meshQuad->blockType = hitBlock->blockType;
    selectedBlockToRender.meshQuad->faceType = hitBlockFace;
    printf("Ending the mesh quad selections\n");

}