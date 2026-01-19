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
    float dirLen = sqrtf(PlayerDirX*PlayerDirX + PlayerDirY*PlayerDirY + PlayerDirZ*PlayerDirZ);
    GLfloat normDirX = PlayerDirX / dirLen;
    GLfloat normDirY = PlayerDirY / dirLen;
    GLfloat normDirZ = PlayerDirZ / dirLen;

    const float step = 0.05f;      // small enough to not skip voxels
    const float maxDist = 16.0f;

    float rayX = CameraX; 
    float rayY = CameraY; 
    float rayZ = CameraZ; 


    Block* hitBlock = NULL;
    int voxelX, voxelY, voxelZ;
    int localX, localY, localZ;
    int hitFace;

    Chunk* curChunk = NULL;
    for (float dist = 0; dist < maxDist; dist += step) {
        voxelX = (int)round(rayX / BlockWidthX);
        voxelY = (int)round(rayY / BlockHeightY);
        voxelZ = (int)round(rayZ / BlockLengthZ);


        int chunkX = (voxelX >= 0) ? voxelX / ChunkWidthX : (voxelX - (ChunkWidthX-1)) / ChunkWidthX;
        int chunkZ = (voxelZ >= 0) ? voxelZ / ChunkLengthZ : (voxelZ - (ChunkLengthZ-1)) / ChunkLengthZ;

        localX = voxelX - chunkX * ChunkWidthX;
        localY = voxelY;
        localZ = voxelZ - chunkZ * ChunkLengthZ;

        uint64_t chunkKey = packChunkKey(chunkX, chunkZ);
        BucketEntry* result = getHashmapEntry(chunkKey);
        if (!result) break;

        curChunk = result->chunkEntry;
        int index = localX + ChunkWidthX * localZ + (ChunkWidthX*ChunkLengthZ)*localY;
        if (index < 0 || index >= ChunkWidthX*ChunkLengthZ*ChunkHeightY) {
            rayX += step * normDirX;
            rayY += step * normDirY;
            rayZ += step * normDirZ;
            continue;
        }

        if (!curChunk->blocks[index].isAir) {
            hitBlock = &curChunk->blocks[index];

            // determine which face the ray entered from
            float prevRayX = rayX - step*normDirX;
            float prevRayY = rayY - step*normDirY;
            float prevRayZ = rayZ - step*normDirZ;

            int prevX = (int)round(prevRayX);
            int prevY = (int)round(prevRayY);
            int prevZ = (int)round(prevRayZ);

            int curX = (int)round(rayX);
            int curY = (int)round(rayY);
            int curZ = (int)round(rayZ);

            if (curX != prevX) {
                hitFace = (normDirX > 0) ? FACE_LEFT : FACE_RIGHT;
            }
            else if (curY != prevY) {
                hitFace = (normDirY > 0) ? FACE_BOTTOM : FACE_TOP;
            }
            else if (curZ != prevZ) {
                hitFace = (normDirZ > 0) ? FACE_BACK : FACE_FRONT;
            }
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
        selectedBlockToRender.active = 0;
        return;
    }

    selectedBlockToRender.active = 1;

    selectedBlockToRender.worldX = voxelX * BlockWidthX;
    selectedBlockToRender.worldY = voxelY * BlockHeightY;
    selectedBlockToRender.worldZ = voxelZ * BlockLengthZ;

    selectedBlockToRender.localX = localX;
    selectedBlockToRender.localY = localY;
    selectedBlockToRender.localZ = localZ;

    selectedBlockToRender.hitFace = hitFace;

    selectedBlockToRender.chunk = curChunk;
}

