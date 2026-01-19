#include <GL/glu.h>
#include <GL/glut.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include "chunks.h"
#include "chunkLoaderManager.h"

ChunkMeshQuads chunkMeshQuads;
// int ChunkWidthX = 1;
// int ChunkLengthZ = 1;
// int ChunkHeightY = 10;

float BlockWidthX = 1;
float BlockHeightY = 1;
float BlockLengthZ = 1;

int DEBUG = 0;

void createChunk(Chunk *chunk, GLfloat xAdd, GLfloat zAdd, int isFirstCreation, int flag, uint64_t key)
{
    chunk->chunkStartX = xAdd;
    chunk->chunkStartZ = zAdd;

    chunk->key = key;
    chunk->hasMesh = 0;

    if (isFirstCreation)
    {
        chunk->firstQuadIndex = -1;
        chunk->lastQuadIndex = -1;
    }

    for (int x = 0; x < ChunkWidthX; x++)
    {
        for (int z = 0; z < ChunkLengthZ; z++)
        {
            if (1) {
                
            int baseHeight = 60;
            int stairHeight = baseHeight + x + z;
            if (stairHeight >= ChunkHeightY)
            {
                stairHeight = ChunkHeightY - 1;
            }
            for (int y = 0; y < ChunkHeightY; y++)
            { 
                Block *curBlock = &(chunk->blocks[x + ChunkWidthX * z + (ChunkWidthX * ChunkLengthZ) * y]);

                curBlock->x = BlockWidthX * x + xAdd;
                curBlock->z = BlockLengthZ * z + zAdd;
                curBlock->y = BlockHeightY * (y);

                // fill everything below the staircase height
                curBlock->isAir = (y > stairHeight);

                if (y == stairHeight)
                {
                    curBlock->blockType = BLOCK_TYPE_GRASS;
                }
                else if (y < stairHeight && y >= (stairHeight - 3))
                {
                    curBlock->blockType = BLOCK_TYPE_DIRT;
                }
                else
                {
                    curBlock->blockType = BLOCK_TYPE_STONE;
                }
            }

            } else {
            /////////////////////////////////////////////////////////////

            for (int y = 0; y < ChunkHeightY; y++)
            { 
                Block *curBlock = &(chunk->blocks[x + ChunkWidthX * z + (ChunkWidthX * ChunkLengthZ) * y]);

                curBlock->x = BlockWidthX * x + xAdd;
                curBlock->z = BlockLengthZ * z + zAdd;
                curBlock->y = BlockHeightY * (y);

                // fill everything below the staircase height
                int stairHeight;
                if (x != 5) {
                    curBlock->isAir = (y > 50);
                    stairHeight = 50;
                } else {
                    curBlock->isAir = (y > 58);
                    stairHeight = 58;
                }

                if (y == stairHeight)
                {
                    curBlock->blockType = BLOCK_TYPE_GRASS;
                }
                else if (y < stairHeight && y >= (stairHeight - 3))
                {
                    curBlock->blockType = BLOCK_TYPE_DIRT;
                }
                else
                {
                    curBlock->blockType = BLOCK_TYPE_STONE;
                }
            }
            
            }
        }
    }
}

void initChunkMeshingSystem()
{
    chunkMeshQuads.capacity = 16;
    chunkMeshQuads.amtQuads = 0;
    chunkMeshQuads.quads = malloc(sizeof(MeshQuad) * chunkMeshQuads.capacity);
}

void handleProgramClose()
{
    printf("\n\n\n[MEMORY INFO] Program closing -> freeing chunk quad memory\n\n\n");
    free(chunkMeshQuads.quads);
}

void generateChunkMesh(Chunk *chunk)
{
    // printf("starting mesh amts %d\n", chunkMeshQuads.amtQuads);
    int tops[ChunkWidthX][ChunkLengthZ][ChunkHeightY]    = {0}; // * X-Z plane, y fixed
    int bottoms[ChunkWidthX][ChunkLengthZ][ChunkHeightY] = {0}; // * X-Z plane, y fixed
    int lefts[ChunkHeightY][ChunkLengthZ][ChunkWidthX]   = {0}; // ? Y-Z plane, x fixed
    int rights[ChunkHeightY][ChunkLengthZ][ChunkWidthX]  = {0}; // ? Y-Z plane, x fixed
    int fronts[ChunkWidthX][ChunkHeightY][ChunkLengthZ]  = {0}; // * X-Y plane, z fixed
    int backs[ChunkWidthX][ChunkHeightY][ChunkLengthZ]   = {0}; // * X-Y plane, z fixed
    
    int64_t chunkX;
    int64_t chunkZ;
    unpackChunkKey(chunk->key, &chunkX, &chunkZ);
    
    uint64_t upChunkNeighborKey, downChunkNeighborKey, rightChunkNeighborKey, leftChunkNeighborKey;
    leftChunkNeighborKey  = packChunkKey((chunkX - 1), chunkZ);
    rightChunkNeighborKey = packChunkKey((chunkX + 1), chunkZ);
    upChunkNeighborKey    = packChunkKey(chunkX, (chunkZ - 1));
    downChunkNeighborKey  = packChunkKey(chunkX, (chunkZ + 1));

    BucketEntry *leftNeighbor  = getHashmapEntry(leftChunkNeighborKey);
    BucketEntry *rightNeighbor = getHashmapEntry(rightChunkNeighborKey);
    BucketEntry *upNeighbor    = getHashmapEntry(upChunkNeighborKey);
    BucketEntry *downNeighbor  = getHashmapEntry(downChunkNeighborKey);

    if (DEBUG)
    {
        printf("X %d and Y %d\n", chunkX, chunkZ);
        printf("Up neighbor is chunkKey = %" PRIu64 "\n", upChunkNeighborKey);
        printf("Down neighbor is chunkKey = %" PRIu64 "\n", downChunkNeighborKey);
        printf("Left neighbor is chunkKey = %" PRIu64 "\n", leftChunkNeighborKey);
        printf("Right neighbor is chunkKey = %" PRIu64 "\n", rightChunkNeighborKey);
    }

    int currentQuadIndex = chunkMeshQuads.amtQuads;

    // mask generation step (for top, bottom, left, right, front, back)
    for (int y = 0; y < ChunkHeightY; y++)
    {
        for (int x = 0; x < ChunkWidthX; x++)
        {
            for (int z = 0; z < ChunkLengthZ; z++)
            {
                int blockIndex = x + z * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ);
                int topBlockIndex = blockIndex + ChunkWidthX * ChunkLengthZ;
                int bottomBlockIndex = blockIndex - ChunkWidthX * ChunkLengthZ;
                int leftBlockIndex = blockIndex - 1;
                int rightBlockIndex = blockIndex + 1;
                int frontBlockIndex = blockIndex - ChunkWidthX;
                int backBlockIndex = blockIndex + ChunkWidthX;

                if (
                    (topBlockIndex < ChunkWidthX * ChunkLengthZ * ChunkHeightY && (!chunk->blocks[blockIndex].isAir && chunk->blocks[topBlockIndex].isAir)) ||
                    (!chunk->blocks[blockIndex].isAir && topBlockIndex >= ChunkWidthX * ChunkLengthZ * ChunkHeightY))
                {
                    // existing block
                    tops[x][z][y] = chunk->blocks[blockIndex].blockType;
                }
                else
                {
                    tops[x][z][y] = 0;
                }

                if (
                    (bottomBlockIndex >= 0 && (!chunk->blocks[blockIndex].isAir && chunk->blocks[bottomBlockIndex].isAir)) ||
                    (!chunk->blocks[blockIndex].isAir && bottomBlockIndex < 0))
                {
                    // existing block
                    bottoms[x][z][y] = chunk->blocks[blockIndex].blockType;
                }
                else
                {
                    bottoms[x][z][y] = 0;
                }

                if ((x > 0 && (!chunk->blocks[blockIndex].isAir && chunk->blocks[leftBlockIndex].isAir)) ||
                    (!chunk->blocks[blockIndex].isAir && x == 0))
                {
                    // existing block
                    lefts[y][z][x] = chunk->blocks[blockIndex].blockType;
                }
                else
                {
                    lefts[y][z][x] = 0;
                }

                if ((x < (ChunkWidthX - 1) && (!chunk->blocks[blockIndex].isAir && chunk->blocks[rightBlockIndex].isAir)) ||
                    (!chunk->blocks[blockIndex].isAir && x == (ChunkWidthX - 1)))
                {
                    // existing block
                    rights[y][z][x] = chunk->blocks[blockIndex].blockType;
                }
                else
                {
                    rights[y][z][x] = 0;
                }

                if ((z > 0 && (!chunk->blocks[blockIndex].isAir && chunk->blocks[frontBlockIndex].isAir)) ||
                    (!chunk->blocks[blockIndex].isAir && z == 0))
                {
                    // existing block
                    fronts[x][y][z] = chunk->blocks[blockIndex].blockType;
                }
                else
                {
                    fronts[x][y][z] = 0;
                }

                if ((z < (ChunkLengthZ - 1) && (!chunk->blocks[blockIndex].isAir && chunk->blocks[backBlockIndex].isAir)) ||
                    (!chunk->blocks[blockIndex].isAir && z == (ChunkLengthZ - 1)))
                {
                    // existing block
                    backs[x][y][z] = chunk->blocks[blockIndex].blockType;
                }
                else
                {
                    backs[x][y][z] = 0;
                }
            }
        }
    }

    // greedy algorithm steps
    int visitedTops[ChunkWidthX][ChunkLengthZ][ChunkHeightY] = {0};
    int width;
    int height;
    int done;
    int curBlockType;
    for (int y = 0; y < ChunkHeightY; y++)
    {
        for (int x = 0; x < ChunkWidthX; x++)
        {
            for (int z = 0; z < ChunkLengthZ; z++)
            {
                if (tops[x][z][y] == 0 || visitedTops[x][z][y])
                {
                    continue;
                }

                curBlockType = tops[x][z][y];

                width = 1;
                while (((x + width) < ChunkWidthX && tops[x + width][z][y] == curBlockType) && !visitedTops[x + width][z][y])
                {
                    width++;
                }

                height = 1;
                done = 0;
                while ((z + height) < ChunkLengthZ && !done)
                {
                    for (int dx = 0; dx < width; dx++)
                    {
                        if ((tops[x + dx][z + height][y] == 0 || visitedTops[x + dx][z + height][y]) || tops[x + dx][z + height][y] != curBlockType)
                        {
                            done = 1;
                            break;
                        }
                    }

                    if (!done)
                    {
                        height++;
                    }
                }

                for (int dz = 0; dz < height; dz++)
                {
                    for (int dx = 0; dx < width; dx++)
                    {
                        visitedTops[x + dx][z + dz][y] = 1;
                    }
                }

                if (chunkMeshQuads.amtQuads >= chunkMeshQuads.capacity)
                {
                    chunkMeshQuads.capacity *= 2;
                    chunkMeshQuads.quads = realloc(chunkMeshQuads.quads, sizeof(MeshQuad) * chunkMeshQuads.capacity);
                }

                MeshQuad *curQuad = &(chunkMeshQuads.quads[chunkMeshQuads.amtQuads]);
                curQuad->x = x + chunk->chunkStartX;
                curQuad->y = y;
                curQuad->z = z + chunk->chunkStartZ;
                curQuad->width = width;
                curQuad->height = height;
                curQuad->faceType = FACE_TOP;
                curQuad->blockType = curBlockType;

                chunkMeshQuads.amtQuads++;
                // printf("  -> %f, %f\n", x, chunk->chunkStartX);
                if (DEBUG)
                {
                    printf("[CREATED QUAD] x %f, y %f, z %f, width %d, height %d, faceType %d, blockType %d\n", curQuad->x, curQuad->y, curQuad->z, width, height, curQuad->faceType, curQuad->blockType);
                }
            }
        }
    }

    int visitedBottoms[ChunkWidthX][ChunkLengthZ][ChunkHeightY - 1] = {0};
    for (int y = 1; y < ChunkHeightY; y++)
    {
        for (int x = 0; x < ChunkWidthX; x++)
        {
            for (int z = 0; z < ChunkLengthZ; z++)
            {
                if (bottoms[x][z][y - 1] == 0 || visitedBottoms[x][z][y - 1])
                {
                    continue;
                }

                curBlockType = bottoms[x][z][y - 1];

                width = 1;
                while (((x + width) < ChunkWidthX && bottoms[x + width][z][y - 1] == curBlockType) && !visitedBottoms[x + width][z][y - 1])
                {
                    width++;
                }

                height = 1;
                done = 0;
                while ((z + height) < ChunkLengthZ && !done)
                {
                    for (int dx = 0; dx < width; dx++)
                    {
                        if ((bottoms[x + dx][z + height][y - 1] == 0 || visitedBottoms[x + dx][z + height][y - 1]) || bottoms[x + dx][z + height][y - 1] != curBlockType)
                        {
                            done = 1;
                            break;
                        }
                    }

                    if (!done)
                    {
                        height++;
                    }
                }

                for (int dz = 0; dz < height; dz++)
                {
                    for (int dx = 0; dx < width; dx++)
                    {
                        visitedBottoms[x + dx][z + dz][y - 1] = 1;
                    }
                }

                if (chunkMeshQuads.amtQuads >= chunkMeshQuads.capacity)
                {
                    chunkMeshQuads.capacity *= 2;
                    chunkMeshQuads.quads = realloc(chunkMeshQuads.quads, sizeof(MeshQuad) * chunkMeshQuads.capacity);
                }

                MeshQuad *curQuad = &(chunkMeshQuads.quads[chunkMeshQuads.amtQuads]);
                curQuad->x = x + chunk->chunkStartX;
                curQuad->y = y - 1;
                curQuad->z = z + chunk->chunkStartZ;
                curQuad->width = width;
                curQuad->height = height;
                curQuad->faceType = FACE_BOTTOM;
                curQuad->blockType = curBlockType;

                chunkMeshQuads.amtQuads++;
                if (DEBUG)
                {
                    printf("[CREATED QUAD] x %f, y %f, z %f, width %d, height %d, faceType %d, blockType %d\n", curQuad->x, curQuad->y, curQuad->z, width, height, curQuad->faceType, curQuad->blockType);
                }
            }
        }
    }

    int visitedLeft[ChunkHeightY][ChunkLengthZ][ChunkWidthX] = {0};
    for (int y = 0; y < ChunkHeightY; y++)
    {
        for (int x = 0; x < ChunkWidthX; x++)
        {
            for (int z = 0; z < ChunkLengthZ; z++)
            {
                if (lefts[y][z][x] == 0 || visitedLeft[y][z][x])
                {
                    continue;
                }

                curBlockType = lefts[y][z][x];

                if (leftNeighbor != NULL && (x == 0))
                {
                    // use ChunkWidthX-1 for the x because need to scan the right side for the left computations
                    // if the block on the left chunk's right side is solid then we don't need a face here!
                    if (!leftNeighbor->chunkEntry->blocks[ChunkWidthX - 1 + z * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ)].isAir)
                    {
                        continue;
                    }
                }

                width = 1;
                while ((((y + width) < ChunkHeightY && lefts[y + width][z][x] == curBlockType) && !visitedLeft[y + width][z][x]))
                {
                    if (leftNeighbor != NULL && (x == 0))
                    {
                        if (!leftNeighbor->chunkEntry->blocks[ChunkWidthX - 1 + z * ChunkWidthX + (y + width) * (ChunkWidthX * ChunkLengthZ)].isAir)
                        {
                            break;
                        }
                    }
                    width++;
                }

                height = 1;
                done = 0;
                while ((z + height) < ChunkLengthZ && !done)
                {
                    for (int dy = 0; dy < width; dy++)
                    {
                        if ((lefts[y + dy][z + height][x] == 0 || visitedLeft[y + dy][z + height][x]) || lefts[y + dy][z + height][x] != curBlockType)
                        {
                            done = 1;
                            break;
                        }

                        if (leftNeighbor != NULL && (x == 0))
                        {
                            if (!leftNeighbor->chunkEntry->blocks[ChunkWidthX - 1 + (z + height) * ChunkWidthX + (y + dy) * (ChunkWidthX * ChunkLengthZ)].isAir)
                            {
                                done = 1;
                                break;
                            }
                        }
                    }

                    if (!done)
                    {
                        height++;
                    }
                }

                for (int dz = 0; dz < height; dz++)
                {
                    for (int dy = 0; dy < width; dy++)
                    {
                        visitedLeft[y + dy][z + dz][x] = 1;
                    }
                }

                if (chunkMeshQuads.amtQuads >= chunkMeshQuads.capacity)
                {
                    chunkMeshQuads.capacity *= 2;
                    chunkMeshQuads.quads = realloc(chunkMeshQuads.quads, sizeof(MeshQuad) * chunkMeshQuads.capacity);
                }

                MeshQuad *curQuad = &(chunkMeshQuads.quads[chunkMeshQuads.amtQuads]);
                curQuad->x = x + chunk->chunkStartX;
                curQuad->y = y;
                curQuad->z = z + chunk->chunkStartZ;

                curQuad->width = width;
                curQuad->height = height;
                curQuad->faceType = FACE_LEFT;
                curQuad->blockType = curBlockType;

                chunkMeshQuads.amtQuads++;
                if (DEBUG)
                {
                    printf("[CREATED QUAD] x %f, y %f, z %f, width %d, height %d, faceType %d, blockType %d\n", curQuad->x, curQuad->y, curQuad->z, width, height, curQuad->faceType, curQuad->blockType);
                }
            }
        }
    }

    int visitedRight[ChunkHeightY][ChunkLengthZ][ChunkWidthX] = {0};
    for (int y = 0; y < ChunkHeightY; y++)
    {
        for (int x = 0; x < ChunkWidthX; x++)
        {
            for (int z = 0; z < ChunkLengthZ; z++)
            {
                if (rights[y][z][x] == 0 || visitedRight[y][z][x])
                {
                    continue;
                }

                curBlockType = rights[y][z][x];

                if (rightNeighbor != NULL && (x == (ChunkWidthX - 1)))
                {
                    // use 0 for the x because need to scan the left side for the right computations
                    // if the block on the right chunk's left side is solid then we don't need a face here!
                    if (!rightNeighbor->chunkEntry->blocks[0 + z * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ)].isAir)
                    {
                        continue;
                    }
                }

                width = 1;
                while (((y + width) < ChunkHeightY && rights[y + width][z][x] == curBlockType) && !visitedRight[y + width][z][x])
                {
                    if (rightNeighbor != NULL && (x == (ChunkWidthX - 1)))
                    {
                        if (!rightNeighbor->chunkEntry->blocks[0 + z * ChunkWidthX + (y + width) * (ChunkWidthX * ChunkLengthZ)].isAir)
                        {
                            break;
                        }
                    }
                    width++;
                }

                height = 1;
                done = 0;
                while ((z + height) < ChunkLengthZ && !done)
                {
                    for (int dy = 0; dy < width; dy++)
                    {
                        if ((rights[y + dy][z + height][x] == 0 || visitedRight[y + dy][z + height][x]) || rights[y + dy][z + height][x] != curBlockType)
                        {
                            done = 1;
                            break;
                        }

                        if (rightNeighbor != NULL && (x == (ChunkWidthX - 1)))
                        {
                            if (!rightNeighbor->chunkEntry->blocks[0 + (z + height) * ChunkWidthX + (y + dy) * (ChunkWidthX * ChunkLengthZ)].isAir)
                            {
                                done = 1;
                                break;
                            }
                        }
                    }

                    if (!done)
                    {
                        height++;
                    }
                }

                for (int dz = 0; dz < height; dz++)
                {
                    for (int dy = 0; dy < width; dy++)
                    {
                        visitedRight[y + dy][z + dz][x] = 1;
                    }
                }

                if (chunkMeshQuads.amtQuads >= chunkMeshQuads.capacity)
                {
                    chunkMeshQuads.capacity *= 2;
                    chunkMeshQuads.quads = realloc(chunkMeshQuads.quads, sizeof(MeshQuad) * chunkMeshQuads.capacity);
                }
                MeshQuad *curQuad = &(chunkMeshQuads.quads[chunkMeshQuads.amtQuads]);
                curQuad->x = x + chunk->chunkStartX;
                curQuad->y = y;
                curQuad->z = z + chunk->chunkStartZ;
                curQuad->width = width;
                curQuad->height = height;
                curQuad->faceType = FACE_RIGHT;
                curQuad->blockType = curBlockType;

                chunkMeshQuads.amtQuads++;
                if (DEBUG)
                {
                    printf("[CREATED QUAD] x %f, y %f, z %f, width %d, height %d, faceType %d, blockType %d\n", curQuad->x, curQuad->y, curQuad->z, width, height, curQuad->faceType, curQuad->blockType);
                }
            }
        }
    }

    int visitedFronts[ChunkWidthX][ChunkHeightY][ChunkLengthZ] = {0};
    for (int y = 0; y < ChunkHeightY; y++)
    {
        for (int x = 0; x < ChunkWidthX; x++)
        {
            for (int z = 0; z < ChunkLengthZ; z++)
            {
                if (fronts[x][y][z] == 0 || visitedFronts[x][y][z])
                {
                    continue;
                }

                curBlockType = fronts[x][y][z];

                if (upNeighbor != NULL && (z == 0))
                {
                    // use ChunkLengthZ-1 for the z because need to scan the bottom side for the up computations
                    // if the block on the up chunk's bottom side is solid then we don't need a face here!
                    if (!upNeighbor->chunkEntry->blocks[x + ((ChunkLengthZ - 1)) * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ)].isAir)
                    {
                        continue;
                    }
                }

                width = 1;
                while (((x + width) < ChunkWidthX && fronts[x + width][y][z] == curBlockType) && !visitedFronts[x + width][y][z])
                {
                    if (upNeighbor != NULL && (z == 0))
                    {
                        if (!upNeighbor->chunkEntry->blocks[(x + width) + ((ChunkLengthZ - 1)) * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ)].isAir)
                        {
                            break;
                        }
                    }
                    width++;
                }

                height = 1;
                done = 0;
                while ((y + height) < ChunkHeightY && !done)
                {
                    for (int dx = 0; dx < width; dx++)
                    {
                        if ((fronts[x + dx][y + height][z] == 0 || visitedFronts[x + dx][y + height][z]) || fronts[x + dx][y + height][z] != curBlockType)
                        {
                            done = 1;
                            break;
                        }

                        if (upNeighbor != NULL && (z == 0))
                        {
                            if (!upNeighbor->chunkEntry->blocks[(x + dx) + ((ChunkLengthZ - 1)) * ChunkWidthX + (y + height) * (ChunkWidthX * ChunkLengthZ)].isAir)
                            {
                                done = 1;
                                break;
                            }
                        }
                    }

                    if (!done)
                    {
                        height++;
                    }
                }

                for (int dy = 0; dy < height; dy++)
                {
                    for (int dx = 0; dx < width; dx++)
                    {
                        visitedFronts[x + dx][y + dy][z] = 1;
                    }
                }

                if (chunkMeshQuads.amtQuads >= chunkMeshQuads.capacity)
                {
                    chunkMeshQuads.capacity *= 2;
                    chunkMeshQuads.quads = realloc(chunkMeshQuads.quads, sizeof(MeshQuad) * chunkMeshQuads.capacity);
                }

                MeshQuad *curQuad = &(chunkMeshQuads.quads[chunkMeshQuads.amtQuads]);
                curQuad->x = x + chunk->chunkStartX;
                curQuad->y = y;
                curQuad->z = z + chunk->chunkStartZ - 1;
                curQuad->width = width;
                curQuad->height = height;
                curQuad->faceType = FACE_FRONT;
                curQuad->blockType = curBlockType;

                chunkMeshQuads.amtQuads++;
                if (DEBUG)
                {
                    printf("[CREATED QUAD] x %f, y %f, z %f, width %d, height %d, faceType %d, blockType %d\n", curQuad->x, curQuad->y, curQuad->z, width, height, curQuad->faceType, curQuad->blockType);
                }
            }
        }
    }

    int visitedBacks[ChunkWidthX][ChunkHeightY][ChunkLengthZ] = {0};
    for (int y = 0; y < ChunkHeightY; y++)
    {
        for (int x = 0; x < ChunkWidthX; x++)
        {
            for (int z = 0; z < ChunkLengthZ; z++)
            {
                if (backs[x][y][z] == 0 || visitedBacks[x][y][z])
                {
                    continue;
                }

                int curBlockType = backs[x][y][z];

                if (downNeighbor != NULL && (z == (ChunkLengthZ - 1)))
                {
                    // use 0 for the z because need to scan the top side for the bottom computations
                    // if the block on the bottom chunk's up side is solid then we don't need a face here!
                    if (!downNeighbor->chunkEntry->blocks[x + (0) * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ)].isAir)
                    {
                        continue;
                    }
                }

                width = 1;
                while (((x + width) < ChunkWidthX && backs[x + width][y][z] == curBlockType) && !visitedBacks[x + width][y][z])
                {
                    if (downNeighbor != NULL && (z == (ChunkLengthZ - 1)))
                    {
                        if (!downNeighbor->chunkEntry->blocks[(x + width) + (0) * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ)].isAir)
                        {
                            break;
                        }
                    }
                    width++;
                }

                height = 1;
                done = 0;
                while ((y + height) < ChunkHeightY && !done)
                {
                    for (int dx = 0; dx < width; dx++)
                    {
                        if ((backs[x + dx][y + height][z] == 0 || visitedBacks[x + dx][y + height][z]) || backs[x + dx][y + height][z] != curBlockType)
                        {
                            done = 1;
                            break;
                        }

                        if (downNeighbor != NULL && (z == (ChunkLengthZ - 1)))
                        {
                            if (!downNeighbor->chunkEntry->blocks[(x + dx) + (0) * ChunkWidthX + (y + height) * (ChunkWidthX * ChunkLengthZ)].isAir)
                            {
                                done = 1;
                                break;
                            }
                        }
                    }

                    if (!done)
                    {
                        height++;
                    }
                }

                for (int dy = 0; dy < height; dy++)
                {
                    for (int dx = 0; dx < width; dx++)
                    {
                        visitedBacks[x + dx][y + dy][z] = 1;
                    }
                }

                if (chunkMeshQuads.amtQuads >= chunkMeshQuads.capacity)
                {
                    chunkMeshQuads.capacity *= 2;
                    chunkMeshQuads.quads = realloc(chunkMeshQuads.quads, sizeof(MeshQuad) * chunkMeshQuads.capacity);
                }

                MeshQuad *curQuad = &(chunkMeshQuads.quads[chunkMeshQuads.amtQuads]);
                curQuad->x = x + chunk->chunkStartX;
                curQuad->y = y;
                curQuad->z = z + chunk->chunkStartZ + 1;
                curQuad->width = width;
                curQuad->height = height;
                curQuad->faceType = FACE_BACK;
                curQuad->blockType = curBlockType;

                chunkMeshQuads.amtQuads++;
                if (DEBUG)
                {
                    printf("[CREATED QUAD] x %f, y %f, z %f, width %d, height %d, faceType %d, blockType %d\n", curQuad->x, curQuad->y, curQuad->z, width, height, curQuad->faceType, curQuad->blockType);
                }
            }
        }
    }

    // printf("   -> ending mesh amts %d\n", chunkMeshQuads.amtQuads);

    chunk->firstQuadIndex = currentQuadIndex;
    chunk->lastQuadIndex = chunkMeshQuads.amtQuads-1;
    chunk->flag = CHUNK_FLAG_RENDERED_AND_LOADED;
}

void deleteChunkMesh(Chunk *chunk) {
    // need to delete the quads created from the chunks start to end quad
    // this can be done by shifting over entries and then resetting the quad count

    int firstQuadIndex = chunk->firstQuadIndex;
    int lastQuadIndex = chunk->lastQuadIndex;
    chunk->hasMesh = 0;
    chunk->flag = CHUNK_FLAG_LOADED;
    // printf("Deleting quads from %d to %d and has mesh is %d\n", firstQuadIndex, lastQuadIndex, chunk->hasMesh);
    if (firstQuadIndex == -1 && lastQuadIndex == -1) {
        return; 
    }

    int deleteAmount = (lastQuadIndex - firstQuadIndex) + 1;

    for (int quadIndex = lastQuadIndex+1; quadIndex < chunkMeshQuads.amtQuads; quadIndex++) {
        chunkMeshQuads.quads[quadIndex - deleteAmount] = chunkMeshQuads.quads[quadIndex];
    }

    chunkMeshQuads.amtQuads -= deleteAmount;
    // fix other chunks' indices
    for (int c = 0; c < chunkLoaderManager.loadedChunks.amtLoadedChunks; c++) {
        Chunk* other = chunkLoaderManager.loadedChunks.loadedChunks[c];

        if (other->firstQuadIndex > lastQuadIndex) {
            other->firstQuadIndex -= deleteAmount;
            other->lastQuadIndex  -= deleteAmount;
        }

        if (other->key == chunk->key) {
            // printf("key reset!\n");
            other->firstQuadIndex = -1;
            other->lastQuadIndex = -1;
        }
    }
    // printf("New starting quad index is at %d\n", chunkMeshQuads.amtQuads);
}
 