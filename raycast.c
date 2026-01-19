#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#include <math.h>
#include "raycast.h"
#include "chunks.h"
#include "input.h"
#include "chunkLoaderManager.h"
#include "render.h"

static inline int voxelFloor(float v, float size) {
    int i = (int)(v / size);
    if (v < 0 && fmodf(v, size) != 0.0f) i--;
    return i;
}




// void raycastFromCamera() {
//     // normalize direction
//     float dirLen = sqrtf(PlayerDirX*PlayerDirX + PlayerDirY*PlayerDirY + PlayerDirZ*PlayerDirZ);
//     GLfloat normDirX = PlayerDirX / dirLen;
//     GLfloat normDirY = PlayerDirY / dirLen;
//     GLfloat normDirZ = PlayerDirZ / dirLen;

//     // get the voxel the camera is inside
//     int camVoxelX = (int)floor(CameraX / BlockWidthX);
//     int camVoxelY = (int)floor(CameraY / BlockHeightY);
//     int camVoxelZ = (int)floor(CameraZ / BlockLengthZ);
//     printf("inside of %d %d %d with %f %f %f\n", camVoxelX, camVoxelY, camVoxelZ, CameraX, CameraY, CameraZ);
//     // start the ray at the CENTER of that voxel
//     float rayX = camVoxelX * BlockWidthX + 0.5f * BlockWidthX;
//     float rayY = camVoxelY * BlockHeightY + 0.5f * BlockHeightY;
//     float rayZ = camVoxelZ * BlockLengthZ + 0.5f * BlockLengthZ;

//     // small epsilon along the ray direction to avoid self-hit
//     const float epsilon = 1e-2f;
//     rayX += epsilon * normDirX;
//     rayY += epsilon * normDirY;
//     rayZ += epsilon * normDirZ;

//     // initialize voxel coordinates for DDA
//     int voxelX = (int)floor(rayX / BlockWidthX);
//     int voxelY = (int)floor(rayY / BlockHeightY);
//     int voxelZ = (int)floor(rayZ / BlockLengthZ);

//     // compute steps
//     int stepX = (normDirX > 0) ? 1 : -1;
//     int stepY = (normDirY > 0) ? 1 : -1;
//     int stepZ = (normDirZ > 0) ? 1 : -1;

//     // compute tMax and tDelta
//     float tMaxX = (stepX > 0)
//         ? ((voxelX + 1) * BlockWidthX - rayX) / normDirX
//         : (rayX - voxelX * BlockWidthX) / -normDirX;
//     float tMaxY = (stepY > 0)
//         ? ((voxelY + 1) * BlockHeightY - rayY) / normDirY
//         : (rayY - voxelY * BlockHeightY) / -normDirY;
//     float tMaxZ = (stepZ > 0)
//         ? ((voxelZ + 1) * BlockLengthZ - rayZ) / normDirZ
//         : (rayZ - voxelZ * BlockLengthZ) / -normDirZ;

//     float tDeltaX = (normDirX != 0.0f) ? BlockWidthX / fabs(normDirX) : 99999999999.0f;
//     float tDeltaY = (normDirY != 0.0f) ? BlockHeightY / fabs(normDirY) : 99999999999.0f;
//     float tDeltaZ = (normDirZ != 0.0f) ? BlockLengthZ / fabs(normDirZ) : 99999999999.0f;

//     float maxDist = 16.0f;
//     float dist = 0.0f;
//     Block* hitBlock = NULL;
//     int hitFace;
//     // printf("begin with player at %f %f %f\n", 
//     // CameraX, CameraY, CameraZ);
//     while (dist < maxDist) {
//         // step along shortest tMax
//         // printf("will step now with tvals %f %f %f\n", tMaxX, tMaxY, tMaxZ);
//         if (tMaxX <= tMaxY && tMaxX <= tMaxZ) {
//             voxelX += stepX;
//             dist = tMaxX;
//             tMaxX += tDeltaX;
//             hitFace = (stepX > 0) ? FACE_LEFT : FACE_RIGHT;
//             // printf("Step x with %d\n", stepX);
//         }
//         else if (tMaxY <= tMaxZ) {
//             voxelY += stepY;
//             dist = tMaxY;
//             tMaxY += tDeltaY;
//             hitFace = (stepY > 0) ? FACE_BOTTOM : FACE_TOP;
//             // printf("Step y with %d\n", stepY);
//         }
//         else {
//             voxelZ += stepZ;
//             dist = tMaxZ;
//             tMaxZ += tDeltaZ;
//             hitFace = (stepZ > 0) ? FACE_BACK : FACE_FRONT;
//             // printf("Step z with %d\n", stepZ);
//         }
        
//         int chunkX;
//         int chunkZ;
//         if (voxelX >= 0) {
//             chunkX = voxelX / ChunkWidthX;
//         } else {
//             chunkX = (voxelX - (ChunkWidthX - 1)) / ChunkWidthX;
//         }
//         if (voxelZ >= 0) {
//             chunkZ = voxelZ / ChunkLengthZ;
//         } else {
//             chunkZ = (voxelZ - (ChunkLengthZ - 1)) / ChunkLengthZ;
//         }

//         int localX = voxelX - chunkX * ChunkWidthX;
//         int localZ = voxelZ - chunkZ * ChunkLengthZ;
//         int localY = voxelY;
        
//         // printf("now at %d %d %d     ||    local %d %d %d\n", 
//         // voxelX, voxelY, voxelZ,
//         // localX, localY, localZ);

//         uint64_t chunkKey = packChunkKey(chunkX, chunkZ);
//         BucketEntry* result = getHashmapEntry(chunkKey);
//         if (!result) break;

//         Chunk* curChunk = result->chunkEntry;
//         int index = localX + ChunkWidthX * localZ + (ChunkWidthX * ChunkLengthZ) * localY;

        
//         if (index >= ChunkWidthX*ChunkLengthZ*ChunkHeightY || index < 0) {
//             // out of bounds
//             continue;
//         }

//         if (!curChunk->blocks[index].isAir) {
//             hitBlock = &curChunk->blocks[index];
//             break;
//         }
//     }

//     if (!hitBlock) {
//         if (selectedBlockToRender.meshQuad) free(selectedBlockToRender.meshQuad);
//         selectedBlockToRender.meshQuad = NULL;
//         return;
//     }

//     // if (hitFace == FACE_LEFT) {
//     //     voxelX -= 1;
//     // } else if (hitFace == FACE_BOTTOM) {
//     //     voxelY -= 1;
//     // } else if (hitFace == FACE_BACK) {
//     //     voxelZ -= 1;
//     // }

//     if (!selectedBlockToRender.meshQuad) {
//         selectedBlockToRender.meshQuad = malloc(sizeof(MeshQuad));
//     }
//     // printf("hit block at %f %f %f, and player at %f %f %f\n", 
//     // voxelX * BlockWidthX, voxelY * BlockHeightY, voxelZ * BlockLengthZ, 
//     // CameraX, CameraY, CameraZ);
    
//     selectedBlockToRender.meshQuad->x = voxelX * BlockWidthX;
//     selectedBlockToRender.meshQuad->y = (voxelY) * BlockHeightY;
//     selectedBlockToRender.meshQuad->z = voxelZ * BlockLengthZ;
//     selectedBlockToRender.meshQuad->width = BlockWidthX;
//     selectedBlockToRender.meshQuad->height = BlockHeightY;
//     selectedBlockToRender.meshQuad->blockType = hitBlock->blockType;
//     selectedBlockToRender.meshQuad->faceType = hitFace;
// }


void raycastFromCamera() {
    float dirLen = sqrtf(PlayerDirX*PlayerDirX + PlayerDirY*PlayerDirY + PlayerDirZ*PlayerDirZ);
    GLfloat normDirX = PlayerDirX / dirLen;
    GLfloat normDirY = PlayerDirY / dirLen;
    GLfloat normDirZ = PlayerDirZ / dirLen;

    const float step = 0.05f;      // small enough to not skip voxels
    const float maxDist = 16.0f;

    float rayX = CameraX; //+ ((normDirX >= 0) ? 1e-4f : -1e-4f);
    float rayY = CameraY; //+ ((normDirY >= 0) ? 1e-4f : -1e-4f);
    float rayZ = CameraZ; //+ ((normDirZ >= 0) ? 1e-4f : -1e-4f);


    Block* hitBlock = NULL;
    int voxelX, voxelY, voxelZ;
    int faceType = -1;

    for (float dist = 0; dist < maxDist; dist += step) {
        voxelX = (int)round(rayX / BlockWidthX);
        voxelY = (int)round(rayY / BlockHeightY);
        voxelZ = (int)round(rayZ / BlockLengthZ);


        int chunkX = (voxelX >= 0) ? voxelX / ChunkWidthX : (voxelX - (ChunkWidthX-1)) / ChunkWidthX;
        int chunkZ = (voxelZ >= 0) ? voxelZ / ChunkLengthZ : (voxelZ - (ChunkLengthZ-1)) / ChunkLengthZ;

        int localX = voxelX - chunkX * ChunkWidthX;
        int localY = voxelY;
        int localZ = voxelZ - chunkZ * ChunkLengthZ;

        uint64_t chunkKey = packChunkKey(chunkX, chunkZ);
        BucketEntry* result = getHashmapEntry(chunkKey);
        if (!result) break;

        Chunk* curChunk = result->chunkEntry;
        int index = localX + ChunkWidthX * localZ + (ChunkWidthX*ChunkLengthZ)*localY;
        if (index < 0 || index >= ChunkWidthX*ChunkLengthZ*ChunkHeightY) {
            rayX += step * normDirX;
            rayY += step * normDirY;
            rayZ += step * normDirZ;
            continue;
        }

        if (!curChunk->blocks[index].isAir) {
            hitBlock = &curChunk->blocks[index];
            break;
        }

        rayX += step * normDirX;
        rayY += step * normDirY;
        rayZ += step * normDirZ;
    }
    voxelX = (int)round(rayX / BlockWidthX);
    voxelY = (int)round(rayY / BlockHeightY);
    voxelZ = (int)round(rayZ / BlockLengthZ);
    if (!hitBlock) {
        if (selectedBlockToRender.meshQuad) free(selectedBlockToRender.meshQuad);
        selectedBlockToRender.meshQuad = NULL;
        return;
    }

    if (!selectedBlockToRender.meshQuad) {
        selectedBlockToRender.meshQuad = malloc(sizeof(MeshQuad));
    }

    // --- MODULO-BASED BOUNDARY CORRECTION ---
    // float modX = fmodf(rayX, BlockWidthX);
    // float modY = fmodf(rayY, BlockHeightY);
    // float modZ = fmodf(rayZ, BlockLengthZ);
    // if (modX < 0) modX += BlockWidthX;
    // if (modY < 0) modY += BlockHeightY;
    // if (modZ < 0) modZ += BlockLengthZ;

    // const float eps = 1e-4f;
    // if (modX < eps && normDirX > 0) voxelX--;
    // if (modX > BlockWidthX - eps && normDirX < 0) voxelX++;
    // if (modY < eps && normDirY > 0) voxelY--;
    // if (modY > BlockHeightY - eps && normDirY < 0) voxelY++;
    // if (modZ < eps && normDirZ > 0) voxelZ--;
    // if (modZ > BlockLengthZ - eps && normDirZ < 0) voxelZ++;

    printf("%f %f %f\n", rayX, rayY, rayZ);
    selectedBlockToRender.meshQuad->x = voxelX * BlockWidthX;
    selectedBlockToRender.meshQuad->y = voxelY * BlockHeightY;
    selectedBlockToRender.meshQuad->z = voxelZ * BlockLengthZ;
    selectedBlockToRender.x = rayX * BlockWidthX;
    selectedBlockToRender.y = rayY * BlockHeightY;
    selectedBlockToRender.z = rayZ * BlockLengthZ;
    selectedBlockToRender.meshQuad->width = BlockWidthX;
    selectedBlockToRender.meshQuad->height = BlockHeightY;
    selectedBlockToRender.meshQuad->faceType = faceType;
    selectedBlockToRender.meshQuad->blockType = hitBlock->blockType;
}

