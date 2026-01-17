#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#include <math.h>
#include "raycast.h"
#include "chunks.h"
#include "input.h"
#include "chunkLoaderManager.h"
#include "render.h"

// void raycastFromCamera() {
//     // DDA - digital differential analyzer, used for raycasting

//     int playerChunkX = (int)floor(CameraX / (ChunkWidthX * BlockWidthX));
//     int playerChunkZ = (int)floor(CameraZ / (ChunkLengthZ * BlockLengthZ));
    
//     uint64_t chunkKey  = packChunkKey(playerChunkX, playerChunkZ);
//     BucketEntry *result = getHashmapEntry(chunkKey);
    
//     if (result == NULL) { return; }
    
//     float directionVectorLength = sqrtf(
//         PlayerDirX*PlayerDirX + 
//         PlayerDirY*PlayerDirY + 
//         PlayerDirZ*PlayerDirZ
//     );
    

//     GLfloat normCameraDirX = PlayerDirX / directionVectorLength;
//     GLfloat normCameraDirY = PlayerDirY / directionVectorLength;
//     GLfloat normCameraDirZ = PlayerDirZ / directionVectorLength;

//     float rayUnitStepSizeX = fabs(1.0f / normCameraDirX);
//     float rayUnitStepSizeY = fabs(1.0f / normCameraDirY);
//     float rayUnitStepSizeZ = fabs(1.0f / normCameraDirZ);

//     float rayX = CameraX;
//     float rayY = CameraY;
//     float rayZ = CameraZ;
//     int voxelX = floor(rayX/BlockWidthX);
//     int voxelY = floor(rayY/BlockHeightY);
//     int voxelZ = floor(rayZ/BlockLengthZ);

//     int stepX, stepY, stepZ;
//     float rayLengthX, rayLengthY, rayLengthZ;
//     float voxelWorldX = voxelX * BlockWidthX;
//     float voxelWorldY = voxelY * BlockHeightY;
//     float voxelWorldZ = voxelZ * BlockLengthZ;

//     if (normCameraDirX > 0) {
//         stepX = 1;
//         rayLengthX = (voxelWorldX + BlockWidthX - rayX) * rayUnitStepSizeX;
//     } else {
//         stepX = -1;
//         rayLengthX = (rayX - voxelWorldX) * rayUnitStepSizeX;
//     }

//     if (normCameraDirY > 0) {
//         stepY = 1;
//         rayLengthY = (voxelWorldY + BlockHeightY - rayY) * rayUnitStepSizeY;
//     } else {
//         stepY = -1;
//         rayLengthY = (rayY - voxelWorldY) * rayUnitStepSizeY;
//     }

//     if (normCameraDirZ > 0) {
//         stepZ = 1;
//         rayLengthZ = (voxelWorldZ + BlockLengthZ - rayZ) * rayUnitStepSizeZ;
//     } else {
//         stepZ = -1;
//         rayLengthZ = (rayZ - voxelWorldZ) * rayUnitStepSizeZ;
//     }


    
//     float dist     = 0.0;
//     float maxDist  = 16.0;
//     Block *hitBlock = NULL;
//     int hitBlockFace;
//     while (dist < maxDist) {
//         int originalChunkX = floor((float)voxelX / ChunkWidthX);
//         int originalChunkZ = floor((float)voxelZ / ChunkLengthZ);

//         // walking step
//         if (rayLengthX < rayLengthY && rayLengthX < rayLengthZ) {
//             // x step
//             voxelX += stepX;
//             dist = rayLengthX;
//             rayLengthX += rayUnitStepSizeX;

//             if (stepX == 1) {
//                 hitBlockFace = FACE_LEFT;
//             } else {
//                 hitBlockFace = FACE_RIGHT;
//             }
//         } else if (rayLengthY < rayLengthZ) {
//             // y step
//             voxelY += stepY;
//             dist = rayLengthY;
//             rayLengthY += rayUnitStepSizeY;
//             if (stepY == 1) {
//                 hitBlockFace = FACE_BOTTOM;
//             } else {
//                 hitBlockFace = FACE_TOP;
//             }
//         } else {
//             // z step
//             voxelZ += stepZ;
//             dist = rayLengthZ;
//             rayLengthZ += rayUnitStepSizeZ;
//             if (stepZ == 1) {
//                 hitBlockFace = FACE_BACK;
//             } else {
//                 hitBlockFace = FACE_FRONT;
//             }
//         }

//         int localX = voxelX - originalChunkX * ChunkWidthX ;
//         int localZ = voxelZ - originalChunkZ * ChunkLengthZ;
//         int localY = voxelY;

//         if (localY < 0 || localY >= ChunkHeightY) {
//             continue;
//         }

//         int index = localX +
//         ChunkWidthX * localZ +
//         (ChunkWidthX * ChunkLengthZ) * localY;

//         uint64_t chunkKey  = packChunkKey(
//             floor((float)(voxelX) / ChunkWidthX), floor((float)(voxelZ) / ChunkLengthZ));
//         BucketEntry *result = getHashmapEntry(chunkKey);

//         if (result == NULL) {
//             break;
//         }

//         if (!result->chunkEntry->blocks[index].isAir) {
//             // hit the block
//             hitBlock = &result->chunkEntry->blocks[index];
//             break;
//         }
//     }

//     if (hitBlock == NULL) {
//         if (selectedBlockToRender.meshQuad != NULL) {
//             free(selectedBlockToRender.meshQuad);
//         }
//         selectedBlockToRender.meshQuad = NULL;
//         return;
//     }

//     printf("Starting the mesh quad selections with %f and %d\n", hitBlock->x, voxelX);
//     if (selectedBlockToRender.meshQuad == NULL) {
//         selectedBlockToRender.meshQuad = malloc(sizeof(MeshQuad));
//     }


//     selectedBlockToRender.meshQuad->x = voxelX * BlockWidthX;
//     selectedBlockToRender.meshQuad->y = voxelY * BlockHeightY + 0.01;
//     selectedBlockToRender.meshQuad->z = voxelZ * BlockLengthZ;

//     selectedBlockToRender.meshQuad->width = BlockWidthX; 
//     // technically putting the same value isn't fully adapting but it would always be the same so it works
//     selectedBlockToRender.meshQuad->height = BlockHeightY;
//     selectedBlockToRender.meshQuad->blockType = hitBlock->blockType;
//     selectedBlockToRender.meshQuad->faceType = hitBlockFace;
//     printf("Ending the mesh quad selections\n");

// }

void raycastFromCamera() {
    float dirLen = sqrtf(PlayerDirX*PlayerDirX + PlayerDirY*PlayerDirY + PlayerDirZ*PlayerDirZ);

    GLfloat normDirX = PlayerDirX / dirLen;
    GLfloat normDirY = PlayerDirY / dirLen;
    GLfloat normDirZ = PlayerDirZ / dirLen;

    float rayX = CameraX;
    float rayY = CameraY;
    float rayZ = CameraZ;

    int voxelX = (int)floor(rayX / BlockWidthX);
    int voxelY = (int)floor(rayY / BlockHeightY);
    int voxelZ = (int)floor(rayZ / BlockLengthZ);
    rayX = CameraX - 0.0001f;
    rayY = CameraY - 0.0001f;
    rayZ = CameraZ - 0.0001f;


    int stepX = (normDirX > 0) ? 1 : -1;
    int stepY = (normDirY > 0) ? 1 : -1;
    int stepZ = (normDirZ > 0) ? 1 : -1;

    float tMaxX = (stepX > 0)
    ? ((voxelX + 1) * BlockWidthX - rayX) / normDirX
    : (rayX - voxelX * BlockWidthX) / -normDirX;

float tMaxY = (stepY > 0)
    ? ((voxelY + 1) * BlockHeightY - rayY) / normDirY
    : (rayY - voxelY * BlockHeightY) / -normDirY;

float tMaxZ = (stepZ > 0)
    ? ((voxelZ + 1) * BlockLengthZ - rayZ) / normDirZ
    : (rayZ - voxelZ * BlockLengthZ) / -normDirZ;


    float tDeltaX = (normDirX != 0.0f) ? BlockWidthX / fabs(normDirX) : 99999999999.0f;
    float tDeltaY = (normDirY != 0.0f) ? BlockHeightY / fabs(normDirY) : 99999999999.0f;
    float tDeltaZ = (normDirZ != 0.0f) ? BlockLengthZ / fabs(normDirZ) : 99999999999.0f;


    float maxDist = 16.0f;
    float dist = 0.0f;
    Block* hitBlock = NULL;
    int hitFace;
    printf("begin with player at %f %f %f\n", 
    CameraX, CameraY, CameraZ);
    while (dist < maxDist) {
        // step along shortest tMax
        if (tMaxX <= tMaxY && tMaxX <= tMaxZ) {
            voxelX += stepX;
            dist = tMaxX;
            tMaxX += tDeltaX;
            hitFace = (stepX > 0) ? FACE_LEFT : FACE_RIGHT;
            printf("Step x with %d\n", stepX);
        }
        else if (tMaxY <= tMaxZ) {
            voxelY += stepY;
            dist = tMaxY;
            tMaxY += tDeltaY;
            hitFace = (stepY > 0) ? FACE_BOTTOM : FACE_TOP;
            printf("Step y with %d\n", stepY);
        }
        else {
            voxelZ += stepZ;
            dist = tMaxZ;
            tMaxZ += tDeltaZ;
            hitFace = (stepZ > 0) ? FACE_BACK : FACE_FRONT;
            printf("Step z with %d\n", stepZ);
        }
        printf("now at %d %d %d\n", 
        voxelX, voxelY, voxelZ);

        int chunkX = voxelX / ChunkWidthX;
        int chunkZ = voxelZ / ChunkLengthZ;
        int localX = voxelX - chunkX * ChunkWidthX;
        int localZ = voxelZ - chunkZ * ChunkLengthZ;
        int localY = voxelY;

        uint64_t chunkKey = packChunkKey(chunkX, chunkZ);
        BucketEntry* result = getHashmapEntry(chunkKey);
        if (!result) break;

        Chunk* curChunk = result->chunkEntry;
        int index = localX + ChunkWidthX * localZ + (ChunkWidthX * ChunkLengthZ) * localY;

        
        if (index >= ChunkWidthX*ChunkLengthZ*ChunkHeightY || index < 0) {
            // out of bounds
            continue;
        }

        if (!curChunk->blocks[index].isAir) {
            hitBlock = &curChunk->blocks[index];
            break;
        }
    }

    if (!hitBlock) {
        if (selectedBlockToRender.meshQuad) free(selectedBlockToRender.meshQuad);
        selectedBlockToRender.meshQuad = NULL;
        return;
    }

    // if (hitFace == FACE_LEFT) {
    //     voxelX -= 1;
    // } else if (hitFace == FACE_BOTTOM) {
    //     voxelY -= 1;
    // } else if (hitFace == FACE_BACK) {
    //     voxelZ -= 1;
    // }

    if (!selectedBlockToRender.meshQuad) {
        selectedBlockToRender.meshQuad = malloc(sizeof(MeshQuad));
    }
    printf("hit block at %f %f %f, and player at %f %f %f\n", 
    voxelX * BlockWidthX, voxelY * BlockHeightY, voxelZ * BlockLengthZ, 
    CameraX, CameraY, CameraZ);
    
    selectedBlockToRender.meshQuad->x = voxelX * BlockWidthX;
    selectedBlockToRender.meshQuad->y = (voxelY) * BlockHeightY;
    selectedBlockToRender.meshQuad->z = voxelZ * BlockLengthZ;
    selectedBlockToRender.meshQuad->width = BlockWidthX;
    selectedBlockToRender.meshQuad->height = BlockHeightY;
    selectedBlockToRender.meshQuad->blockType = hitBlock->blockType;
    selectedBlockToRender.meshQuad->faceType = hitFace;
}
