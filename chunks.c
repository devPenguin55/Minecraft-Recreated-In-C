#include <GL/glu.h>
#include <GL/glut.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include "chunks.h"
#include "chunkLoaderManager.h"
#include "noise.h"
#include "player.h"
#include "worldDiskStorage.h"
#include "input.h"

ChunkMeshQuads chunkMeshQuads;
BlockType blockRegistry[100];
Queue lightingQueue;

float BlockWidthX = 1;
float BlockHeightY = 1;
float BlockLengthZ = 1;

int DEBUG_MODE = 0;

static inline float clamp(float value, float minVal, float maxVal)
{
    if (value < minVal)
        return minVal;
    if (value > maxVal)
        return maxVal;
    return value;
}

void createChunk(Chunk *chunk, GLfloat xAdd, GLfloat zAdd, int isFirstCreation, int flag, uint64_t key)
{
    chunk->chunkStartX = xAdd;
    chunk->chunkStartZ = zAdd;

    chunk->key = key;
    chunk->hasMesh = 0;
    chunk->hasVertices = 0;
    chunk->hasWaterVertices = 0;
    chunk->firstQuadIndex = -1;
    chunk->lastQuadIndex = -1;
    chunk->triggerVertexDeletion = 0;
    chunk->triggerVertexRecreation = 0;

    chunk->firstVertex = -1;
    chunk->lastVertex = -1;
    chunk->firstWaterVertex = -1;
    chunk->lastWaterVertex = -1;

    chunk->flag = -1;

    chunk->isDirty = 0;
    chunk->lightDirty = 0;

    chunk->gpuLightIndex = -1;

    chunk->isInitialLightCreated = 0;

    chunk->lightData = calloc(1, sizeof(uint8_t) * ChunkWidthX * ChunkLengthZ * ChunkHeightY);

    static int rawHeightMap[ChunkWidthX * ChunkLengthZ];
    static int heightMap[ChunkWidthX * ChunkLengthZ];
    static int blurTmp[ChunkWidthX * ChunkLengthZ];

    // ---- PASS 1: height -- low-amplitude, long-wavelength so adjacent
    // columns differ by ~0-1 blocks most of the time (slope-friendly),
    // with an occasional bigger hill from the ridge layer and a plains
    // mask that flattens some regions out entirely ----
    for (int x = 0; x < ChunkWidthX; x++)
    {
        for (int z = 0; z < ChunkLengthZ; z++)
        {
            float worldX = BlockWidthX * x + xAdd;
            float worldZ = BlockLengthZ * z + zAdd;

            // rolling hills -- long wavelength (scale 90), modest amplitude
            float rolling = fbm2D(worldX / 90.0f, worldZ / 90.0f, 4, 2, 2.0, 5.0);

            // occasional bigger hills/mountains -- even longer wavelength so
            // its per-block gradient stays gentle despite bigger total height
            float bigHills = ridgedFbm2D(worldX / 60.0f, worldZ / 60.0f, 5, 3, 2.0, 0.5);

            // plains mask: 0 = flatten this region into a plain, 1 = let hills through
            float plainsMask = fbm2D(worldX / 320.0f, worldZ / 320.0f, 3, 2, 2.0, 3.0);
            plainsMask = clamp((plainsMask + 1.0f) * 0.5f, 0.15f, 1.0f); // keep a floor so plains aren't perfectly flat

            int h = 34;
            h += (int)(rolling * 50.0f * plainsMask);
            h += (int)(bigHills * 50.0f * plainsMask);

            rawHeightMap[x + z * ChunkWidthX] = h;
        }
    }

    // ---- PASS 2: two blur passes -- mops up whatever per-column jumps
    // the noise still produced, so slope-eligible (diff == 1) edges dominate ----
    for (int x = 0; x < ChunkWidthX; x++)
        for (int z = 0; z < ChunkLengthZ; z++)
            blurTmp[x + z * ChunkWidthX] = rawHeightMap[x + z * ChunkWidthX];

    for (int pass = 0; pass < 2; pass++)
    {
        for (int x = 0; x < ChunkWidthX; x++)
        {
            for (int z = 0; z < ChunkLengthZ; z++)
            {
                if (x == 0 || x == ChunkWidthX - 1 || z == 0 || z == ChunkLengthZ - 1)
                {
                    heightMap[x + z * ChunkWidthX] = blurTmp[x + z * ChunkWidthX];
                    continue;
                }

                int sum = 0, count = 0;
                for (int dx = -1; dx <= 1; dx++)
                    for (int dz = -1; dz <= 1; dz++)
                    {
                        sum += blurTmp[(x + dx) + (z + dz) * ChunkWidthX];
                        count++;
                    }
                heightMap[x + z * ChunkWidthX] = sum / count;
            }
        }
        for (int i = 0; i < ChunkWidthX * ChunkLengthZ; i++)
            blurTmp[i] = heightMap[i];
    }

    // ---- PASS 3: fill blocks, with widened cave band tuned for overhangs ----
    for (int x = 0; x < ChunkWidthX; x++)
    {
        for (int z = 0; z < ChunkLengthZ; z++)
        {
            int generatedBlockNoiseHeight = heightMap[x + z * ChunkWidthX];

            for (int y = 0; y < ChunkHeightY; y++)
            {
                Block *curBlock = &(chunk->blocks[x + ChunkWidthX * z + (ChunkWidthX * ChunkLengthZ) * y]);

                curBlock->blockType = -1;
                curBlock->x = BlockWidthX * x + xAdd;
                curBlock->z = BlockLengthZ * z + zAdd;
                curBlock->y = BlockHeightY * (y);
                curBlock->isSlope = 0;

                float worldX = curBlock->x;
                float worldZ = curBlock->z;

                curBlock->isAir = (y > generatedBlockNoiseHeight);

                if (!curBlock->isAir)
                {
                    if (y > 10 && y < 55)
                    {
                        int yboost = 15;

                        float base = perlinNoise3D(worldX / 22.0f, (y + yboost) / 22.0f, worldZ / 22.0f, 3);
                        float ridged = 1.0f - fabs(base);

                        // region mask so caves cluster instead of appearing uniformly
                        // everywhere -- some areas get big cave networks, others none
                        float caveRegion = fbm2D(worldX / 60.0f, worldZ / 60.0f, 3, 2, 2.0, 3.0);

                        if (caveRegion > 0.05f)
                        {
                            curBlock->isAir = (ridged > 0.85f);
                        }
                    }
                }

                if (curBlock->isAir)
                {
                    curBlock->blockType = -1;
                }
                else
                {
                    if (y == generatedBlockNoiseHeight)
                    {
                        if (generatedBlockNoiseHeight <= SEA_LEVEL + 1 && perlinNoise3D(worldX / 25.0f, (y + 5) / 25.0f, worldZ / 25.0f, 3) < 0.05)
                        {
                            curBlock->blockType = BLOCK_TYPE_RED_CLAY;
                        }
                        else if (generatedBlockNoiseHeight <= SEA_LEVEL)
                        {
                            curBlock->blockType = BLOCK_TYPE_DIRT;
                        }
                        else
                        {
                            curBlock->blockType = BLOCK_TYPE_GRASS;
                        }
                    }
                    else if (y < generatedBlockNoiseHeight && y >= generatedBlockNoiseHeight - 1)
                    {
                        curBlock->blockType = (y < SEA_LEVEL) ? BLOCK_TYPE_RED_CLAY : BLOCK_TYPE_DIRT;
                    }
                    else
                    {
                        curBlock->blockType = BLOCK_TYPE_STONE;
                    }
                }

                if (curBlock->isAir && y < SEA_LEVEL)
                {
                    curBlock->isAir = 0;
                    curBlock->blockType = BLOCK_TYPE_WATER;
                }
            }
        }
    }

    // ---- PASS 4: slope-direction pass -- corner cases now PICK a direction
    // instead of falling back to a cube step, so slope coverage dominates ----
    for (int x = 1; x < ChunkWidthX - 1; x++)
    {
        for (int z = 1; z < ChunkLengthZ - 1; z++)
        {
            int h = heightMap[x + z * ChunkWidthX];
            if (h < 0 || h >= ChunkHeightY)
                continue;

            int hEast  = heightMap[(x + 1) + z * ChunkWidthX];
            int hWest  = heightMap[(x - 1) + z * ChunkWidthX];
            int hSouth = heightMap[x + (z + 1) * ChunkWidthX];
            int hNorth = heightMap[x + (z - 1) * ChunkWidthX];

            int dir = 0;
            if (hSouth == h - 1) dir = 1;
            else if (hEast  == h - 1) dir = 2;
            else if (hNorth == h - 1) dir = 3;
            else if (hWest  == h - 1) dir = 4;

            if (dir == 0)
                continue; // flat or a >1 cliff -- stays a plain cube, that's fine

            Block *b = &(chunk->blocks[x + ChunkWidthX * z + (ChunkWidthX * ChunkLengthZ) * h]);
            if (b->isAir || b->blockType == BLOCK_TYPE_WATER || b->blockType == -1)
                continue;

            b->isSlope = dir;
        }
    }

    // ---- PASS 5: vegetation -- trees, flowers, short grass, shrubs (unchanged logic) ----
    for (int x = 0; x < ChunkWidthX; x++)
    {
        for (int z = 0; z < ChunkLengthZ; z++)
        {
            int surfaceY = -1;

            for (int y = ChunkHeightY - 2; y >= 0; y--)
            {
                Block *b = &(chunk->blocks[x + ChunkWidthX * z + (ChunkWidthX * ChunkLengthZ) * y]);
                Block *above = &(chunk->blocks[x + ChunkWidthX * z + (ChunkWidthX * ChunkLengthZ) * (y + 1)]);

                if (!b->isAir && above->isAir && b->blockType == BLOCK_TYPE_GRASS)
                {
                    surfaceY = y + 1;
                    break;
                }
            }

            if (surfaceY == -1)
                continue;
            if (x < 4 || x >= ChunkWidthX - 4 || z < 4 || z >= ChunkLengthZ - 4)
                continue;

            // Block *surfaceBlock = &(chunk->blocks[x + ChunkWidthX * z + (ChunkWidthX * ChunkLengthZ) * (surfaceY - 1)]);
            // if (surfaceBlock->isSlope)
                // continue;

            float worldX = x + chunk->chunkStartX;
            float worldZ = z + chunk->chunkStartZ;


            int treeValid = !(fbm2D(worldX / 1.5f, worldZ / 1.5f, 5, 2, 2.0, 5.0) <= 0.45f);
            int flowerValid = fbm2D(worldX / 10.f, worldZ / 10.f, 5, 2, 2.0, 5.0) > 0.37f;
            int shortGrassValid = fbm2D(worldX / 1.2f, worldZ / 10.f, 5, 2, 2.0, 5.0) > 0.30f;
            int shrubValid = fbm2D(worldX / 6.f, worldZ / 6.f, 4, 2, 2.0, 3.0) > 0.55f;

            if (flowerValid && !treeValid && !shortGrassValid)
            {
                Block *b = &(chunk->blocks[x + ChunkWidthX * z + (ChunkWidthX * ChunkLengthZ) * surfaceY]);
                b->blockType = BLOCK_TYPE_ORCHID;
                b->isAir = 0;
                continue;
            }

            if (shortGrassValid && !treeValid)
            {
                Block *b = &(chunk->blocks[x + ChunkWidthX * z + (ChunkWidthX * ChunkLengthZ) * surfaceY]);
                b->blockType = BLOCK_TYPE_SHORT_GRASS;
                b->isAir = 0;
                continue;
            }

            // if (shrubValid && !treeValid)
            // {
            //     for (int yOff = 0; yOff < 2; yOff++)
            //     {
            //         int layerY = surfaceY + yOff;
            //         if (layerY < 0 || layerY >= ChunkHeightY)
            //             continue;

            //         for (int xOff = -1; xOff <= 1; xOff++)
            //         {
            //             for (int zOff = -1; zOff <= 1; zOff++)
            //             {
            //                 if (xOff != 0 && zOff != 0 && yOff == 1)
            //                     continue;

            //                 int newX = x + xOff, newZ = z + zOff;
            //                 if (newX < 0 || newX >= ChunkWidthX || newZ < 0 || newZ >= ChunkLengthZ)
            //                     continue;

            //                 Block *b = &(chunk->blocks[newX + ChunkWidthX * newZ + (ChunkWidthX * ChunkLengthZ) * layerY]);
            //                 if (!b->isAir)
            //                     continue;

            //                 b->blockType = BLOCK_TYPE_LEAVES;
            //                 b->isAir = 0;
            //             }
            //         }
            //     }
            //     continue;
            // }

            if (!treeValid)
            continue;

            int trunkHeight = 10 + (int)(fbm2D(worldX, worldZ, 1, 1, 1, 1) * 5); // 10-14 tall trunk

            for (int yAdd = 0; yAdd < trunkHeight; yAdd++)
            {
                int newY = surfaceY + yAdd;
                if (newY >= ChunkHeightY)
                    break;

                Block *b = &(chunk->blocks[x + ChunkWidthX * z + (ChunkWidthX * ChunkLengthZ) * newY]);
                b->blockType = BLOCK_TYPE_OAK;
                b->isAir = 0;
            }

            // Christmas-tree canopy: stack of tapering rings from a wide base
            // near the bottom of the trunk up to a single point at the top.
            int canopyBaseY = surfaceY + 2;                 // leave a bit of bare trunk near the ground
            int canopyTopY  = surfaceY + trunkHeight;        // point of the tree
            int numLayers   = canopyTopY - canopyBaseY;

            if (numLayers < 3)
                numLayers = 3;

            int maxRadius = 3;

            for (int layer = 0; layer < numLayers; layer++)
            {
                int layerY = canopyBaseY + layer;
                if (layerY < 0 || layerY >= ChunkHeightY)
                    continue;

                // linear taper from maxRadius at the base to 0 at the very top
                int radius = maxRadius - (int)((float)(maxRadius) * layer / (float)(numLayers - 1) + 0.5f);
                if (radius < 0) radius = 0;

                for (int xOff = -radius; xOff <= radius; xOff++)
                {
                    for (int zOff = -radius; zOff <= radius; zOff++)
                    {
                        // diamond taper so corners round off instead of staying square
                        int manhattan = abs(xOff) + abs(zOff);
                        if (manhattan > radius + 1)
                            continue;

                        int newX = x + xOff, newZ = z + zOff;
                        if (newX < 0 || newX >= ChunkWidthX || newZ < 0 || newZ >= ChunkLengthZ)
                            continue;

                        Block *b = &(chunk->blocks[newX + ChunkWidthX * newZ + (ChunkWidthX * ChunkLengthZ) * layerY]);
                        if (b->blockType == BLOCK_TYPE_OAK)
                            continue;

                        b->blockType = BLOCK_TYPE_LEAVES;
                        b->isAir = 0;

                        int absX = abs(xOff), absZ = abs(zOff);
                        int distFromEdge = radius - ((absX > absZ) ? absX : absZ);

                        // slope any block on the outer rim, including the rounded diamond corners
                        if (radius > 0 && (distFromEdge <= 0 || manhattan == radius + 1))
                        {
                            if (absX >= absZ)
                                b->isSlope = (xOff > 0) ? 2 : 4; // east : west
                            else
                                b->isSlope = (zOff > 0) ? 1 : 3; // south : north
                        }
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

    resetLightingQueue(&lightingQueue);
}

void handleProgramClose()
{
    printf("\n\n\n[MEMORY INFO] Program closing -> freeing chunk quad memory\n\n\n");
    free(chunkMeshQuads.quads);
}


static inline int checkIfFaceValidToBeInMesh(Block *mainBlock, Block *neighborBlock)
{
    if (mainBlock->isAir)
        return 0;

    if (!mainBlock->isAir && !neighborBlock->isAir && !blockRegistry[mainBlock->blockType].isRenderSolid && !blockRegistry[neighborBlock->blockType].isRenderSolid && mainBlock->blockType != BLOCK_TYPE_WATER && neighborBlock->blockType != BLOCK_TYPE_WATER)
    {
        return 1;
    }
    if (!mainBlock->isAir && !neighborBlock->isAir && blockRegistry[mainBlock->blockType].isRenderSolid && !blockRegistry[neighborBlock->blockType].isRenderSolid && mainBlock->blockType != BLOCK_TYPE_WATER && neighborBlock->blockType != BLOCK_TYPE_WATER)
    {
        return 1;
    }

    // normal solid block
    if (mainBlock->blockType != BLOCK_TYPE_WATER)
    {
        if (neighborBlock->isAir)
            return 1;

        // show terrain against water
        if (neighborBlock->blockType == BLOCK_TYPE_WATER)
            return 1;

        if (neighborBlock->isSlope) {
            return 1;
        }

        return 0;
    }

    // water block
    if (mainBlock->blockType == BLOCK_TYPE_WATER)
    {
        // only render water surface
        if (neighborBlock->isAir)
            return 1;

        return 0;
    }

    return 0;
}



void generateChunkMesh(Chunk *chunk)
{
    // printf("starting mesh amts %d\n", chunkMeshQuads.amtQuads);
    int tops[ChunkWidthX][ChunkLengthZ][ChunkHeightY] = {0};    // * X-Z plane, y fixed
    int bottoms[ChunkWidthX][ChunkLengthZ][ChunkHeightY] = {0}; // * X-Z plane, y fixed
    int lefts[ChunkHeightY][ChunkLengthZ][ChunkWidthX] = {0};   // ? Y-Z plane, x fixed
    int rights[ChunkHeightY][ChunkLengthZ][ChunkWidthX] = {0};  // ? Y-Z plane, x fixed
    int fronts[ChunkWidthX][ChunkHeightY][ChunkLengthZ] = {0};  // * X-Y plane, z fixed
    int backs[ChunkWidthX][ChunkHeightY][ChunkLengthZ] = {0};   // * X-Y plane, z fixed

    int crosses[ChunkWidthX * ChunkLengthZ * ChunkHeightY] = {0}; // * cross blocks (like flowers)
    int slopes[ChunkWidthX * ChunkLengthZ * ChunkHeightY] = {0}; // ? slope blocks for removing the harsh cube shape

    int64_t chunkX;
    int64_t chunkZ;
    unpackChunkKey(chunk->key, &chunkX, &chunkZ);

    uint64_t upChunkNeighborKey, downChunkNeighborKey, rightChunkNeighborKey, leftChunkNeighborKey;
    leftChunkNeighborKey = packChunkKey((chunkX - 1), chunkZ);
    rightChunkNeighborKey = packChunkKey((chunkX + 1), chunkZ);
    upChunkNeighborKey = packChunkKey(chunkX, (chunkZ - 1));
    downChunkNeighborKey = packChunkKey(chunkX, (chunkZ + 1));

    BucketEntry *leftNeighbor = getHashmapEntry(leftChunkNeighborKey);
    BucketEntry *rightNeighbor = getHashmapEntry(rightChunkNeighborKey);
    BucketEntry *upNeighbor = getHashmapEntry(upChunkNeighborKey);
    BucketEntry *downNeighbor = getHashmapEntry(downChunkNeighborKey);

    // if (leftNeighbor == NULL) {
    //     printf("left neighbor null\n");
    // }
    // if (rightNeighbor == NULL) {
    //     printf("right neighbor null\n");
    // }
    // if (upNeighbor == NULL) {
    //     printf("up neighbor null\n");
    // }
    // if (downNeighbor == NULL) {
    //     printf("down neighbor null\n");
    // }

    if (DEBUG_MODE)
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

                if (!chunk->blocks[blockIndex].isAir && blockRegistry[chunk->blocks[blockIndex].blockType].isRenderCross)
                {
                    crosses[blockIndex] = 1;
                    continue;
                }

                if (!chunk->blocks[blockIndex].isAir && chunk->blocks[blockIndex].isSlope) 
                {
                    slopes[blockIndex] = chunk->blocks[blockIndex].isSlope; // isSlope carries the direction info
                    continue;
                }

                if (
                    (topBlockIndex < ChunkWidthX * ChunkLengthZ * ChunkHeightY && checkIfFaceValidToBeInMesh(&(chunk->blocks[blockIndex]), &(chunk->blocks[topBlockIndex]))) ||
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
                    (bottomBlockIndex >= 0 && checkIfFaceValidToBeInMesh(&(chunk->blocks[blockIndex]), &(chunk->blocks[bottomBlockIndex]))) ||
                    (!chunk->blocks[blockIndex].isAir && bottomBlockIndex < 0))
                {
                    // existing block
                    bottoms[x][z][y] = chunk->blocks[blockIndex].blockType;
                }
                else
                {
                    bottoms[x][z][y] = 0;
                }

                if ((x > 0 && checkIfFaceValidToBeInMesh(&(chunk->blocks[blockIndex]), &(chunk->blocks[leftBlockIndex]))) ||
                    (!chunk->blocks[blockIndex].isAir && x == 0))
                {
                    // existing block
                    lefts[y][z][x] = chunk->blocks[blockIndex].blockType;
                }
                else
                {
                    lefts[y][z][x] = 0;
                }

                if ((x < (ChunkWidthX - 1) && checkIfFaceValidToBeInMesh(&(chunk->blocks[blockIndex]), &(chunk->blocks[rightBlockIndex]))) ||
                    (!chunk->blocks[blockIndex].isAir && x == (ChunkWidthX - 1)))
                {
                    // existing block
                    rights[y][z][x] = chunk->blocks[blockIndex].blockType;
                }
                else
                {
                    rights[y][z][x] = 0;
                }

                if ((z > 0 && checkIfFaceValidToBeInMesh(&(chunk->blocks[blockIndex]), &(chunk->blocks[frontBlockIndex]))) ||
                    (!chunk->blocks[blockIndex].isAir && z == 0))
                {
                    // existing block
                    fronts[x][y][z] = chunk->blocks[blockIndex].blockType;
                }
                else
                {
                    fronts[x][y][z] = 0;
                }

                if ((z < (ChunkLengthZ - 1) && checkIfFaceValidToBeInMesh(&(chunk->blocks[blockIndex]), &(chunk->blocks[backBlockIndex]))) ||
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

                if (crosses[x + z * ChunkWidthX + y * ChunkWidthX * ChunkLengthZ] || slopes[x + z * ChunkWidthX + y * ChunkWidthX * ChunkLengthZ])
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
                if (DEBUG_MODE)
                {
                    printf("[CREATED QUAD] x %f, y %f, z %f, width %d, height %d, faceType %d, blockType %d\n", curQuad->x, curQuad->y, curQuad->z, width, height, curQuad->faceType, curQuad->blockType);
                }
            }
        }
    }

    int visitedBottoms[ChunkWidthX][ChunkLengthZ][ChunkHeightY] = {0};
    for (int y = 0; y < ChunkHeightY; y++)
    {
        for (int x = 0; x < ChunkWidthX; x++)
        {
            for (int z = 0; z < ChunkLengthZ; z++)
            {
                if (bottoms[x][z][y] == 0 || visitedBottoms[x][z][y])
                {
                    continue;
                }

                if (crosses[x + z * ChunkWidthX + y * ChunkWidthX * ChunkLengthZ] || slopes[x + z * ChunkWidthX + y * ChunkWidthX * ChunkLengthZ])
                {
                    continue;
                }

                curBlockType = bottoms[x][z][y];

                width = 1;
                while (((x + width) < ChunkWidthX && bottoms[x + width][z][y] == curBlockType) && !visitedBottoms[x + width][z][y])
                {
                    width++;
                }

                height = 1;
                done = 0;
                while ((z + height) < ChunkLengthZ && !done)
                {
                    for (int dx = 0; dx < width; dx++)
                    {
                        if ((bottoms[x + dx][z + height][y] == 0 || visitedBottoms[x + dx][z + height][y]) || bottoms[x + dx][z + height][y] != curBlockType)
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
                        visitedBottoms[x + dx][z + dz][y] = 1;
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
                curQuad->faceType = FACE_BOTTOM;
                curQuad->blockType = curBlockType;

                chunkMeshQuads.amtQuads++;
                if (DEBUG_MODE)
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

                if (crosses[x + z * ChunkWidthX + y * ChunkWidthX * ChunkLengthZ] || slopes[x + z * ChunkWidthX + y * ChunkWidthX * ChunkLengthZ])
                {
                    continue;
                }

                curBlockType = lefts[y][z][x];

                if (leftNeighbor != NULL && (x == 0))
                {
                    // use ChunkWidthX-1 for the x because need to scan the right side for the left computations
                    // if the block on the left chunk's right side is solid then we don't need a face here!
                    Block *mainBlock = &chunk->blocks[x + z * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ)];
                    Block *neighborBlock = &leftNeighbor->chunkEntry->blocks[ChunkWidthX - 1 + z * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ)];
                    if (
                        !checkIfFaceValidToBeInMesh(mainBlock, neighborBlock))
                    {
                        continue;
                    }
                }

                width = 1;
                while ((((y + width) < ChunkHeightY && lefts[y + width][z][x] == curBlockType) && !visitedLeft[y + width][z][x]))
                {
                    if (leftNeighbor != NULL && (x == 0))
                    {
                        Block *mainBlock = &chunk->blocks[x + z * ChunkWidthX + (y + width) * (ChunkWidthX * ChunkLengthZ)];
                        Block *neighborBlock = &leftNeighbor->chunkEntry->blocks[ChunkWidthX - 1 + z * ChunkWidthX + (y + width) * (ChunkWidthX * ChunkLengthZ)];
                        if (
                            !checkIfFaceValidToBeInMesh(mainBlock, neighborBlock))
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
                            Block *mainBlock = &chunk->blocks[x + (z + height) * ChunkWidthX + (y + dy) * (ChunkWidthX * ChunkLengthZ)];
                            Block *neighborBlock = &leftNeighbor->chunkEntry->blocks[ChunkWidthX - 1 + (z + height) * ChunkWidthX + (y + dy) * (ChunkWidthX * ChunkLengthZ)];
                            if (
                                !checkIfFaceValidToBeInMesh(mainBlock, neighborBlock))
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
                if (DEBUG_MODE)
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

                if (crosses[x + z * ChunkWidthX + y * ChunkWidthX * ChunkLengthZ] || slopes[x + z * ChunkWidthX + y * ChunkWidthX * ChunkLengthZ])
                {
                    continue;
                }

                curBlockType = rights[y][z][x];

                if (rightNeighbor != NULL && (x == (ChunkWidthX - 1)))
                {
                    // use 0 for the x because need to scan the left side for the right computations
                    // if the block on the right chunk's left side is solid then we don't need a face here!
                    Block *mainBlock = &chunk->blocks[x + z * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ)];
                    Block *neighborBlock = &rightNeighbor->chunkEntry->blocks[0 + z * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ)];
                    if (!checkIfFaceValidToBeInMesh(mainBlock, neighborBlock))
                    {
                        continue;
                    }
                }

                width = 1;
                while (((y + width) < ChunkHeightY && rights[y + width][z][x] == curBlockType) && !visitedRight[y + width][z][x])
                {
                    if (rightNeighbor != NULL && (x == (ChunkWidthX - 1)))
                    {
                        Block *mainBlock = &chunk->blocks[x + z * ChunkWidthX + (y + width) * (ChunkWidthX * ChunkLengthZ)];
                        Block *neighborBlock = &rightNeighbor->chunkEntry->blocks[0 + z * ChunkWidthX + (y + width) * (ChunkWidthX * ChunkLengthZ)];
                        if (!checkIfFaceValidToBeInMesh(mainBlock, neighborBlock))
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
                            Block *mainBlock = &chunk->blocks[x + (z + height) * ChunkWidthX + (y + dy) * (ChunkWidthX * ChunkLengthZ)];
                            Block *neighborBlock = &rightNeighbor->chunkEntry->blocks[0 + (z + height) * ChunkWidthX + (y + dy) * (ChunkWidthX * ChunkLengthZ)];
                            if (!checkIfFaceValidToBeInMesh(mainBlock, neighborBlock))
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
                if (DEBUG_MODE)
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

                if (crosses[x + z * ChunkWidthX + y * ChunkWidthX * ChunkLengthZ] || slopes[x + z * ChunkWidthX + y * ChunkWidthX * ChunkLengthZ])
                {
                    continue;
                }

                curBlockType = fronts[x][y][z];

                if (upNeighbor != NULL && (z == 0))
                {
                    // use ChunkLengthZ-1 for the z because need to scan the bottom side for the up computations
                    // if the block on the up chunk's bottom side is solid then we don't need a face here!
                    Block *mainBlock = &chunk->blocks[x + z * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ)];
                    Block *neighborBlock = &upNeighbor->chunkEntry->blocks[x + ((ChunkLengthZ - 1)) * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ)];
                    if (!checkIfFaceValidToBeInMesh(mainBlock, neighborBlock))
                    {
                        continue;
                    }
                }

                width = 1;
                while (((x + width) < ChunkWidthX && fronts[x + width][y][z] == curBlockType) && !visitedFronts[x + width][y][z])
                {
                    if (upNeighbor != NULL && (z == 0))
                    {
                        Block *mainBlock = &chunk->blocks[(x + width) + z * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ)];
                        Block *neighborBlock = &upNeighbor->chunkEntry->blocks[(x + width) + ((ChunkLengthZ - 1)) * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ)];
                        if (!checkIfFaceValidToBeInMesh(mainBlock, neighborBlock))
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
                            Block *mainBlock = &chunk->blocks[(x + dx) + z * ChunkWidthX + (y + height) * (ChunkWidthX * ChunkLengthZ)];
                            Block *neighborBlock = &upNeighbor->chunkEntry->blocks[(x + dx) + ((ChunkLengthZ - 1)) * ChunkWidthX + (y + height) * (ChunkWidthX * ChunkLengthZ)];
                            if (!checkIfFaceValidToBeInMesh(mainBlock, neighborBlock))
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
                if (DEBUG_MODE)
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

                if (crosses[x + z * ChunkWidthX + y * ChunkWidthX * ChunkLengthZ] || slopes[x + z * ChunkWidthX + y * ChunkWidthX * ChunkLengthZ])
                {
                    continue;
                }

                int curBlockType = backs[x][y][z];

                if (downNeighbor != NULL && (z == (ChunkLengthZ - 1)))
                {
                    // use 0 for the z because need to scan the top side for the bottom computations
                    // if the block on the bottom chunk's up side is solid then we don't need a face here!
                    Block *mainBlock = &chunk->blocks[x + z * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ)];
                    Block *neighborBlock = &downNeighbor->chunkEntry->blocks[x + (0) * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ)];
                    if (!checkIfFaceValidToBeInMesh(mainBlock, neighborBlock))
                    {
                        continue;
                    }
                }

                width = 1;
                while (((x + width) < ChunkWidthX && backs[x + width][y][z] == curBlockType) && !visitedBacks[x + width][y][z])
                {
                    if (downNeighbor != NULL && (z == (ChunkLengthZ - 1)))
                    {
                        Block *mainBlock = &chunk->blocks[(x + width) + z * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ)];
                        Block *neighborBlock = &downNeighbor->chunkEntry->blocks[(x + width) + (0) * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ)];
                        if (!checkIfFaceValidToBeInMesh(mainBlock, neighborBlock))
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
                            Block *mainBlock = &chunk->blocks[(x + dx) + z * ChunkWidthX + (y + height) * (ChunkWidthX * ChunkLengthZ)];
                            Block *neighborBlock = &downNeighbor->chunkEntry->blocks[(x + dx) + (0) * ChunkWidthX + (y + height) * (ChunkWidthX * ChunkLengthZ)];
                            if (!checkIfFaceValidToBeInMesh(mainBlock, neighborBlock))
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
                if (DEBUG_MODE)
                {
                    printf("[CREATED QUAD] x %f, y %f, z %f, width %d, height %d, faceType %d, blockType %d\n", curQuad->x, curQuad->y, curQuad->z, width, height, curQuad->faceType, curQuad->blockType);
                }
            }
        }
    }

    for (int y = 0; y < ChunkHeightY; y++)
    {
        for (int x = 0; x < ChunkWidthX; x++)
        {
            for (int z = 0; z < ChunkLengthZ; z++)
            {
                if (crosses[x + z * ChunkWidthX + y * ChunkWidthX * ChunkLengthZ])
                {
                    int blockIndex = x + z * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ);
                    Block *block = &chunk->blocks[blockIndex];

                    if (chunkMeshQuads.amtQuads >= chunkMeshQuads.capacity)
                    {
                        chunkMeshQuads.capacity *= 2;
                        chunkMeshQuads.quads = realloc(chunkMeshQuads.quads, sizeof(MeshQuad) * chunkMeshQuads.capacity);
                    }

                    MeshQuad *curQuad = &(chunkMeshQuads.quads[chunkMeshQuads.amtQuads]);
                    curQuad->x = x + chunk->chunkStartX;
                    curQuad->y = y;
                    curQuad->z = z + chunk->chunkStartZ;
                    curQuad->width = 1;
                    curQuad->height = 1;
                    curQuad->faceType = FACE_CROSS;
                    curQuad->blockType = block->blockType;

                    chunkMeshQuads.amtQuads++;
                    if (DEBUG_MODE)
                    {
                        printf("[CREATED QUAD] x %f, y %f, z %f, width %d, height %d, faceType %d, blockType %d\n", curQuad->x, curQuad->y, curQuad->z, width, height, curQuad->faceType, curQuad->blockType);
                    }
                } else if (slopes[x + z * ChunkWidthX + y * ChunkWidthX * ChunkLengthZ]) {
                    int blockIndex = x + z * ChunkWidthX + y * (ChunkWidthX * ChunkLengthZ);
                    Block *block = &chunk->blocks[blockIndex];

                    if (chunkMeshQuads.amtQuads >= chunkMeshQuads.capacity)
                    {
                        chunkMeshQuads.capacity *= 2;
                        chunkMeshQuads.quads = realloc(chunkMeshQuads.quads, sizeof(MeshQuad) * chunkMeshQuads.capacity);
                    }

                    MeshQuad *curQuad = &(chunkMeshQuads.quads[chunkMeshQuads.amtQuads]);
                    curQuad->x = x + chunk->chunkStartX;
                    curQuad->y = y;
                    curQuad->z = z + chunk->chunkStartZ;
                    curQuad->width = 1;
                    curQuad->height = 1;
                    curQuad->faceType = FACE_SLOPE;
                    curQuad->blockType = block->blockType;
                    curQuad->slopeDirection = block->isSlope; // isSlope carries the direction information

                    chunkMeshQuads.amtQuads++;
                    if (DEBUG_MODE)
                    {
                        printf("[CREATED QUAD] x %f, y %f, z %f, width %d, height %d, faceType %d, blockType %d\n", curQuad->x, curQuad->y, curQuad->z, width, height, curQuad->faceType, curQuad->blockType);
                    }
                }
            }
        }
    }

    // printf("   -> ending mesh amts %d\n", chunkMeshQuads.amtQuads);

    chunk->firstQuadIndex = currentQuadIndex;
    chunk->lastQuadIndex = chunkMeshQuads.amtQuads - 1;
    chunk->flag = CHUNK_FLAG_RENDERED_AND_LOADED;
}

void deleteChunkMesh(Chunk *chunk)
{
    // need to delete the quads created from the chunks start to end quad
    // this can be done by shifting over entries and then resetting the quad count

    int firstQuadIndex = chunk->firstQuadIndex;
    int lastQuadIndex = chunk->lastQuadIndex;
    chunk->hasMesh = 0;
    chunk->flag = CHUNK_FLAG_LOADED;

    // printf("Deleting quads from %d to %d and has mesh is %d\n", firstQuadIndex, lastQuadIndex, chunk->hasMesh);
    if (firstQuadIndex == -1 && lastQuadIndex == -1)
    {
        return;
    }

    int deleteAmount = (lastQuadIndex - firstQuadIndex) + 1;

    for (int quadIndex = lastQuadIndex + 1; quadIndex < chunkMeshQuads.amtQuads; quadIndex++)
    {
        chunkMeshQuads.quads[quadIndex - deleteAmount] = chunkMeshQuads.quads[quadIndex];
    }

    chunkMeshQuads.amtQuads -= deleteAmount;
    // fix other chunks' indices
    for (int c = 0; c < chunkLoaderManager.loadedChunks.amtLoadedChunks; c++)
    {
        Chunk *other = chunkLoaderManager.loadedChunks.loadedChunks[c];

        if (other->firstQuadIndex > lastQuadIndex)
        {
            other->firstQuadIndex -= deleteAmount;
            other->lastQuadIndex -= deleteAmount;
        }

        if (other->key == chunk->key)
        {
            // printf("key reset!\n");
            other->firstQuadIndex = -1;
            other->lastQuadIndex = -1;
        }
    }

    // printf("New starting quad index is at %d\n", chunkMeshQuads.amtQuads);
}

void resetLightingQueue(Queue *queue)
{
    queue->front = 0;
    queue->rear = 0;
    queue->capacity = 100000;
    queue->size = 0;
}

void enqueue(Queue *queue, int worldX, int worldY, int worldZ)
{
    if (queue->size >= queue->capacity)
    {
        printf("LIGHT QUEUE FULL!\n");
        exit(1);
        return;
    }

    // appends to the end of the queue
    queue->items[queue->rear].x = worldX;
    queue->items[queue->rear].y = worldY;
    queue->items[queue->rear].z = worldZ;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->size++;
}

QueueEntry *dequeue(Queue *queue)
{
    if (queue->size == 0)
    {
        return NULL;
    }

    // pops from the start of queue and returns it
    QueueEntry *result = &queue->items[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;
    return result;
}

static inline uint8_t getSkylight(uint8_t b)
{
    return (b >> 4) & 0xF;
}

static inline uint8_t getBlockLight(uint8_t b)
{
    return b & 0xF;
}

static inline void setSkylight(uint8_t *b, uint8_t s)
{
    *b = (*b & 0x0F) | ((s & 0xF) << 4);
}

static inline void setBlockLight(uint8_t *b, uint8_t s)
{
    *b = (*b & 0xF0) | (s & 0xF);
}

void propagateLightBFS(Queue *targetQueue, int isBlockLight)
{ // seed the lighting queue beforehand (note to self to reset the lighting queue first)
    Vec3 neighborShift[] = {
        {-1, 0, 0},
        {1, 0, 0},
        {0, -1, 0},
        {0, 1, 0},
        {0, 0, 1},
        {0, 0, -1}};

    uint8_t (*getLight)(uint8_t);
    void (*setLight)(uint8_t *, uint8_t);

    if (isBlockLight)
    {
        getLight = getBlockLight;
        setLight = setBlockLight;
    }
    else
    {
        getLight = getSkylight;
        setLight = setSkylight;
    }

    int nodesProcessed = 0;
    while (targetQueue->size > 0)
    {
        nodesProcessed++;
        QueueEntry *queueEntry = dequeue(targetQueue);
        if (queueEntry == NULL)
        {
            continue;
        }

        Chunk *curLightingChunk = chunkAtPosition(queueEntry->x, queueEntry->y, queueEntry->z);
        if (curLightingChunk == NULL)
        {
            continue;
        }

        curLightingChunk->lightDirty = 1;
        int blockIndex = ((int)queueEntry->x - (int)curLightingChunk->chunkStartX) + ((int)queueEntry->z - (int)curLightingChunk->chunkStartZ) * ChunkWidthX + (int)queueEntry->y * ChunkWidthX * ChunkLengthZ;
        if (blockIndex < 0 ||
            blockIndex >= ChunkWidthX * ChunkLengthZ * ChunkHeightY)
        {
            printf("BAD INDEX %d at %d %d %d\n", blockIndex, queueEntry->x, queueEntry->y, queueEntry->z);
            continue;
        }

        if (getLight(curLightingChunk->lightData[blockIndex]) == 0)
        {
            continue;
        }

        int neighborIndex;
        int traversingWorldX = queueEntry->x;
        int traversingWorldY = queueEntry->y;
        int traversingWorldZ = queueEntry->z;

        for (int i = 0; i < 6; i++)
        {
            int nx = traversingWorldX + (int)neighborShift[i].x;
            int ny = traversingWorldY + (int)neighborShift[i].y;
            int nz = traversingWorldZ + (int)neighborShift[i].z;

            if (ny < 0 || ny >= ChunkHeightY)
            {
                continue;
            }

            Chunk *neighborLightingChunk = chunkAtPosition(nx, ny, nz);
            if (neighborLightingChunk == NULL)
            {
                continue;
            }

            neighborIndex = (nx - (int)neighborLightingChunk->chunkStartX) + (nz - (int)neighborLightingChunk->chunkStartZ) * ChunkWidthX + ny * ChunkWidthX * ChunkLengthZ;

            if (neighborIndex < 0 ||
                neighborIndex >= ChunkWidthX * ChunkLengthZ * ChunkHeightY)
            {
                printf("BAD INDEX %d\n", neighborIndex);
                exit(1);
            }

            if (
                (getLight(neighborLightingChunk->lightData[neighborIndex]) >= (getLight(curLightingChunk->lightData[blockIndex]) - 1)) ||
                (!neighborLightingChunk->blocks[neighborIndex].isAir && blockRegistry[neighborLightingChunk->blocks[neighborIndex].blockType].isRenderSolid && !neighborLightingChunk->blocks[neighborIndex].isSlope))
            {
                continue;
            }
            neighborLightingChunk->lightDirty = 1;

            setLight(&neighborLightingChunk->lightData[neighborIndex], getLight(curLightingChunk->lightData[blockIndex]) - 1);
            enqueue(targetQueue, (int)neighborLightingChunk->blocks[neighborIndex].x, (int)neighborLightingChunk->blocks[neighborIndex].y, (int)neighborLightingChunk->blocks[neighborIndex].z);
        }
    }
}

void seedNeighborBorderBlockLighting(Queue *targetQueue, Chunk *chunk)
{
    const int chunkXUnit = ChunkWidthX * BlockWidthX;
    const int chunkZUnit = ChunkLengthZ * BlockLengthZ;

    const int offsets[4][2] = {
        {-1, 0},
        { 1, 0},
        { 0,-1},
        { 0, 1}
    };

    for (int n = 0; n < 4; n++)
    {
        int dx = offsets[n][0];
        int dz = offsets[n][1];

        uint64_t key = packChunkKey(
            chunk->chunkStartX / chunkXUnit + dx,
            chunk->chunkStartZ / chunkZUnit + dz
        );

        BucketEntry *entry = getHashmapEntry(key);
        if (!entry) {
            continue;
        }

        Chunk *neighbor = entry->chunkEntry;

        for (int y = 0; y < ChunkHeightY; y++)
        {
            for (int t = 0; t < ChunkWidthX; t++)
            {
                int cx, cz, nx, nz;

                if (dx == -1)
                {
                    cx = 0;
                    cz = t;
                    nx = ChunkWidthX - 1;
                    nz = t;
                }
                else if (dx == 1)
                {
                    cx = ChunkWidthX - 1;
                    cz = t;
                    nx = 0;
                    nz = t;
                }
                else if (dz == -1)
                {
                    cx = t;
                    cz = 0;
                    nx = t;
                    nz = ChunkLengthZ - 1;
                }
                else
                {
                    cx = t;
                    cz = ChunkLengthZ - 1;
                    nx = t;
                    nz = 0;
                }

                int curIndex = cx + cz * ChunkWidthX + y * ChunkWidthX * ChunkLengthZ;
                int neighIndex = nx + nz * ChunkWidthX + y * ChunkWidthX * ChunkLengthZ;

                uint8_t neighborLight = GET_BLOCK_LIGHT(neighbor->lightData[neighIndex]);

                if (neighborLight <= 1)
                    continue;

                if (GET_BLOCK_LIGHT(chunk->lightData[curIndex]) < neighborLight - 1)
                {
                    SET_BLOCK_LIGHT(chunk->lightData[curIndex], neighborLight - 1);

                    enqueue(
                        targetQueue,
                        chunk->blocks[curIndex].x,
                        chunk->blocks[curIndex].y,
                        chunk->blocks[curIndex].z
                    );
                }
            }
        }
    }
}


void seedNeighborBorderSkyLighting(Queue *targetQueue, Chunk *chunk)
{
    const int chunkXUnit = ChunkWidthX * BlockWidthX;
    const int chunkZUnit = ChunkLengthZ * BlockLengthZ;

    const int offsets[4][2] = {
        {-1, 0},
        { 1, 0},
        { 0,-1},
        { 0, 1}
    };

    for (int n = 0; n < 4; n++)
    {
        int dx = offsets[n][0];
        int dz = offsets[n][1];

        uint64_t key = packChunkKey(
            chunk->chunkStartX / chunkXUnit + dx,
            chunk->chunkStartZ / chunkZUnit + dz
        );

        BucketEntry *entry = getHashmapEntry(key);
        if (!entry) {
            continue;
        }

        Chunk *neighbor = entry->chunkEntry;

        for (int y = 0; y < ChunkHeightY; y++)
        {
            for (int t = 0; t < ChunkWidthX; t++)
            {
                int cx, cz, nx, nz;

                if (dx == -1)
                {
                    cx = 0;
                    cz = t;
                    nx = ChunkWidthX - 1;
                    nz = t;
                }
                else if (dx == 1)
                {
                    cx = ChunkWidthX - 1;
                    cz = t;
                    nx = 0;
                    nz = t;
                }
                else if (dz == -1)
                {
                    cx = t;
                    cz = 0;
                    nx = t;
                    nz = ChunkLengthZ - 1;
                }
                else
                {
                    cx = t;
                    cz = ChunkLengthZ - 1;
                    nx = t;
                    nz = 0;
                }

                int curIndex = cx + cz * ChunkWidthX + y * ChunkWidthX * ChunkLengthZ;
                int neighIndex = nx + nz * ChunkWidthX + y * ChunkWidthX * ChunkLengthZ;

                uint8_t neighborLight = GET_SKYLIGHT(neighbor->lightData[neighIndex]);

                if (neighborLight <= 1)
                    continue;

                if (GET_SKYLIGHT(chunk->lightData[curIndex]) < neighborLight - 1)
                {
                    SET_SKYLIGHT(chunk->lightData[curIndex], neighborLight - 1);

                    enqueue(
                        targetQueue,
                        chunk->blocks[curIndex].x,
                        chunk->blocks[curIndex].y,
                        chunk->blocks[curIndex].z
                    );
                }
            }
        }
    }
}

void computeInitialLightingForChunk(Chunk *chunk) {
    // this function is only called when the chunk has entered the render radius
    chunk->isInitialLightCreated = 1;
    resetLightingQueue(&lightingQueue);

    // this step is to zero the chunk's block light and compute the skylight for the chunk
    for (int x = 0; x < ChunkWidthX; x++)
    {
        for (int z = 0; z < ChunkLengthZ; z++)
        {
            uint8_t currentLight = BASELINE_SKYLIGHT_VALUE;
            for (int y = ChunkHeightY - 1; y >= 0; y--)
            {
                int index = x + z * (ChunkWidthX) + y * (ChunkWidthX * ChunkLengthZ);
                Block *curBlock = &chunk->blocks[index];

                uint8_t originalLight = chunk->lightData[index];

                SET_SKYLIGHT(chunk->lightData[index], (uint8_t)(currentLight));
                SET_BLOCK_LIGHT(chunk->lightData[index], 0);
                
                if (!curBlock->isAir && currentLight && curBlock->blockType == BLOCK_TYPE_WATER) {
                    currentLight = (currentLight > 3) ? currentLight - 3 : 0;
                } else if (!curBlock->isAir && currentLight && curBlock->isSlope) {
                    currentLight = (currentLight > 2) ? currentLight - 2 : 0;
                } else if (!curBlock->isAir && currentLight && !blockRegistry[curBlock->blockType].isRenderCross) {
                    currentLight = 0;
                }


                if (currentLight) {
                    int isNearSolidBlock = 0;

                    for (int dx = -1; dx <= 1; dx++) {
                        for (int dy = -1; dy <= 1; dy++) {
                            for (int dz = -1; dz <= 1; dz++) {
                                int nIdx = (x+dx) + (z+dz) * (ChunkWidthX) + (y+dy) * (ChunkWidthX * ChunkLengthZ);
                                if (nIdx < 0 || nIdx >= ChunkWidthX * ChunkLengthZ * ChunkHeightY) { continue; }
                                if (!chunk->blocks[nIdx].isAir) {
                                    isNearSolidBlock = 1;
                                    break;
                                }
                            }
                        }
                    }

                    if (isNearSolidBlock ) {
                        // if the block is above ground, make it an emitter and enqueue ot
                        enqueue(&lightingQueue, (int)(curBlock->x), (int)(curBlock->y), (int)(curBlock->z));
                    }
                }
            }
        }
    }

    seedNeighborBorderSkyLighting(&lightingQueue, chunk);
    propagateLightBFS(&lightingQueue, 0);     


    chunk->lightDirty = 1;

    int chunkXUnit = ChunkWidthX * BlockWidthX;
    int chunkZUnit = ChunkLengthZ * BlockLengthZ;

    resetLightingQueue(&lightingQueue);
    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dz = -1; dz <= 1; dz++)
        {
            // go through this chunk and neighbor chunks to ensure that all surrounding chunks contribute their block light
            // this will ensure that the current chunk's light is properly propagated
            uint64_t chunkKey = packChunkKey(
                (int)((chunk->chunkStartX + dx * chunkXUnit) / (chunkXUnit)),
                (int)((chunk->chunkStartZ + dz * chunkZUnit) / (chunkZUnit)));

            BucketEntry *result = getHashmapEntry(chunkKey);

            if (result != NULL)
            {
                result->chunkEntry->lightDirty = 1;
                for (int i = 0; i < 32768; i++)
                {
                    if (blockRegistry[result->chunkEntry->blocks[i].blockType].lightEmissivePower && !result->chunkEntry->blocks[i].isAir)
                    {
                        enqueue(&lightingQueue, result->chunkEntry->blocks[i].x, result->chunkEntry->blocks[i].y, result->chunkEntry->blocks[i].z);
                        SET_BLOCK_LIGHT(result->chunkEntry->lightData[i], blockRegistry[result->chunkEntry->blocks[i].blockType].lightEmissivePower);
                    }
                    
                }
            }
        }
    }

    seedNeighborBorderBlockLighting(&lightingQueue, chunk);
    propagateLightBFS(&lightingQueue, 1);
} 