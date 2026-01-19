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
    int faceType = -1;

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

    selectedBlockToRender.meshQuad->x = voxelX * BlockWidthX;
    selectedBlockToRender.meshQuad->y = voxelY * BlockHeightY;
    selectedBlockToRender.meshQuad->z = voxelZ * BlockLengthZ;
    selectedBlockToRender.meshQuad->width = BlockWidthX;
    selectedBlockToRender.meshQuad->height = BlockHeightY;
    selectedBlockToRender.meshQuad->faceType = faceType;
    selectedBlockToRender.meshQuad->blockType = hitBlock->blockType;

    selectedBlockToRender.localX = localX;
    selectedBlockToRender.localY = localY;
    selectedBlockToRender.localZ = localZ;
    selectedBlockToRender.chunk = curChunk;
}

