#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#include "chunks.h"

PlayerChunks world;
ChunkMeshQuads chunkMeshQuads;
// int ChunkWidthX = 1;
// int ChunkLengthZ = 1; 
// int ChunkHeightY = 10;

float BlockWidthX = 0.15;
float BlockHeightY = 0.15;
float BlockLengthZ = 0.15;

void createChunk(Chunk *chunk, GLfloat xAdd, GLfloat zAdd)
{
    chunk->chunkStartX = xAdd;
    chunk->chunkStartZ = zAdd;
    printf("new chunk made with %f and %f\n", xAdd, zAdd);
    for (int x = 0; x < ChunkWidthX; x++)
    {
        for (int z = 0; z < ChunkLengthZ; z++)
        {
            for (int y = 0; y < ChunkHeightY; y++)
            {
                Block *curBlock = &(chunk->blocks[x + ChunkWidthX * z + (ChunkWidthX * ChunkLengthZ) * y]);

                curBlock->x = BlockWidthX * x + xAdd;
                curBlock->z = BlockLengthZ * z + zAdd;
                curBlock->y = BlockHeightY * (-y);
                curBlock->isAir = (y <= 2);
            }
        }
    }
}

void initWorld(PlayerChunks *world)
{
    int x = 0;
    int z = 0;
    world->amtChunks = 0;
    for (int i = 0; i < 16; i++)
    {
        x = i % (4);
        z = (int)(i / (4));
        createChunk(&(world->chunks[i]), x * ChunkWidthX, z * ChunkLengthZ);
        world->amtChunks++;
    }
}

void initChunkMeshingSystem() {
    chunkMeshQuads.capacity = 16;
    chunkMeshQuads.amtQuads =  0;
    chunkMeshQuads.quads = malloc(sizeof(MeshQuad)*chunkMeshQuads.capacity);    
}

void handleProgramClose() {
    printf("\n\n\n[MEMORY INFO] Program closing -> freeing chunk quad memory\n\n\n");
    free(chunkMeshQuads.quads);
}

void generateChunkMesh(Chunk *chunk)
{
    printf("mesh amts %d\n", chunkMeshQuads.amtQuads);
    int    tops[ChunkWidthX][ChunkLengthZ][ChunkHeightY] = {0}; // * X-Z plane, y fixed
    int bottoms[ChunkWidthX][ChunkLengthZ][ChunkHeightY] = {0}; // * X-Z plane, y fixed
    int  lefts[ChunkHeightY][ChunkLengthZ][ChunkWidthX]  = {0}; // ? Y-Z plane, x fixed
    int rights[ChunkHeightY][ChunkLengthZ][ChunkWidthX]  = {0}; // ? Y-Z plane, x fixed
    int  fronts[ChunkWidthX][ChunkHeightY][ChunkLengthZ] = {0}; // * X-Y plane, z fixed
    int   backs[ChunkWidthX][ChunkHeightY][ChunkLengthZ] = {0}; // * X-Y plane, z fixed

    // mask generation step (for top, bottom, left, right, front, back)
    for (int y = 0; y<ChunkHeightY; y++) {
        for (int x = 0; x<ChunkWidthX; x++) {
            for (int z = 0; z<ChunkLengthZ; z++) {
                int blockIndex       = x + z*ChunkWidthX + y*(ChunkWidthX*ChunkLengthZ);
                int topBlockIndex    = blockIndex - ChunkWidthX*ChunkLengthZ;
                int bottomBlockIndex = blockIndex + ChunkWidthX*ChunkLengthZ;
                int leftBlockIndex   = blockIndex - 1;
                int rightBlockIndex  = blockIndex + 1;
                int frontBlockIndex  = blockIndex - ChunkWidthX;
                int backBlockIndex   = blockIndex + ChunkWidthX;

                if (
                    (topBlockIndex >= 0 && (!chunk->blocks[blockIndex].isAir && chunk->blocks[topBlockIndex].isAir)) ||                    
                    (!chunk->blocks[blockIndex].isAir && topBlockIndex < 0)    
                ) {
                    // existing block
                    tops[x][z][y] = 1;
                } else {
                    tops[x][z][y] = 0;
                }

                if (
                    (bottomBlockIndex < ChunkWidthX*ChunkLengthZ*ChunkHeightY && (!chunk->blocks[blockIndex].isAir && chunk->blocks[bottomBlockIndex].isAir)) ||                    
                    (!chunk->blocks[blockIndex].isAir && bottomBlockIndex >= ChunkWidthX*ChunkLengthZ*ChunkHeightY)    
                ) {
                    // existing block
                    bottoms[x][z][y] = 1;
                } else {
                    bottoms[x][z][y] = 0;
                }

                if ((x > 0 && (!chunk->blocks[blockIndex].isAir && chunk->blocks[leftBlockIndex].isAir)) ||
                   (!chunk->blocks[blockIndex].isAir && x == 0)
                ) {
                    // existing block
                    lefts[y][z][x] = 1;
                } else {
                    lefts[y][z][x] = 0;
                }

                if ((x < (ChunkWidthX-1) && (!chunk->blocks[blockIndex].isAir && chunk->blocks[rightBlockIndex].isAir)) ||
                   (!chunk->blocks[blockIndex].isAir && x == (ChunkWidthX-1))
                ) {
                    // existing block
                    rights[y][z][x] = 1;
                } else {
                    rights[y][z][x] = 0;
                }
                
                if ((z > 0 && (!chunk->blocks[blockIndex].isAir && chunk->blocks[frontBlockIndex].isAir)) ||
                    (!chunk->blocks[blockIndex].isAir && z == 0)
                ) {
                    // existing block
                    fronts[x][y][z] = 1;
                } else {
                    fronts[x][y][z] = 0;
                }
           
                if ((z < (ChunkLengthZ-1) && (!chunk->blocks[blockIndex].isAir && chunk->blocks[backBlockIndex].isAir)) ||
                    (!chunk->blocks[blockIndex].isAir && z == (ChunkLengthZ-1))
                ) {
                    // existing block
                    backs[x][y][z] = 1;
                } else {
                    backs[x][y][z] = 0;
                }   
            }
        }
        
        // for (int i = 0; i < 16; i++)
        // {
        //     for (int j = 0; j < 16; j++)
        //     {
        //         printf("%d ", tops[i][j][y]);
        //     }
        //     printf("\n");
        // }
        // printf(" ---  new y layer of %d\n", y); 
    }

    // greedy algorithm steps
    int visitedTops[ChunkWidthX][ChunkLengthZ][ChunkHeightY] = {0};
    int width;
    int height;
    int done;
    for (int y = 0; y < ChunkHeightY; y++) {
        for (int x = 0; x < ChunkWidthX; x++) {
            for (int z = 0; z < ChunkLengthZ; z++) {
                if (tops[x][z][y] == 0 || visitedTops[x][z][y]) { continue; }
                
                width = 1;
                while (((x+width) < ChunkWidthX && tops[x+width][z][y] == 1) && !visitedTops[x+width][z][y]) {
                    width++;
                }

                height = 1;
                done   = 0;
                while ((z+height) < ChunkLengthZ && !done) {
                    for (int dx = 0; dx < width; dx++) {
                        if (tops[x+dx][z+height][y] == 0 || visitedTops[x+dx][z+height][y]) {
                            done = 1;
                            break;
                        }
                    }
                    
                    if (!done) {
                        height++;
                    }                    
                }

                for (int dz = 0; dz<ChunkLengthZ; dz++) {
                    for (int dx = 0; dx<ChunkWidthX; dx++) {
                        visitedTops[x+dx][z+dz][y] = 1;
                    }
                }

                if (chunkMeshQuads.amtQuads >= chunkMeshQuads.capacity) {
                    chunkMeshQuads.capacity *= 2;
                    chunkMeshQuads.quads = realloc(chunkMeshQuads.quads, sizeof(MeshQuad)*chunkMeshQuads.capacity);
                }

                MeshQuad *curQuad = &(chunkMeshQuads.quads[chunkMeshQuads.amtQuads]);
                curQuad->x = x+chunk->chunkStartX;
                curQuad->y = y;
                curQuad->z = z+chunk->chunkStartZ;
                curQuad->width = width;
                curQuad->height = height;
                curQuad->faceType = FACE_TOP;

                chunkMeshQuads.amtQuads++;
                printf("  -> %f, %f\n", x, chunk->chunkStartX);
                printf("[CREATED QUAD] x %f, y %f, z %f, width %d, height %d, faceType %d\n", curQuad->x, curQuad->y, curQuad->z, width, height, curQuad->faceType);
            }        
        }    
    }

    int visitedBottoms[ChunkWidthX][ChunkLengthZ][ChunkHeightY-1] = {0};
    for (int y = 0; y < ChunkHeightY-1; y++) {
        for (int x = 0; x < ChunkWidthX; x++) {
            for (int z = 0; z < ChunkLengthZ; z++) {
                if (bottoms[x][z][y+1] == 0 || visitedBottoms[x][z][y+1]) { continue; }
                
                width = 1;
                while (((x+width) < ChunkWidthX && bottoms[x+width][z][y+1] == 1) && !visitedBottoms[x+width][z][y+1]) {
                    width++;
                }

                height = 1;
                done   = 0;
                while ((z+height) < ChunkLengthZ && !done) {
                    for (int dx = 0; dx < width; dx++) {
                        if (bottoms[x+dx][z+height][y+1] == 0 || visitedBottoms[x+dx][z+height][y+1]) {
                            done = 1;
                            break;
                        }
                    }
                    
                    if (!done) {
                        height++;
                    }                    
                }

                for (int dz = 0; dz<ChunkLengthZ; dz++) {
                    for (int dx = 0; dx<ChunkWidthX; dx++) {
                        visitedBottoms[x+dx][z+dz][y+1] = 1;
                    }
                }

                if (chunkMeshQuads.amtQuads >= chunkMeshQuads.capacity) {
                    chunkMeshQuads.capacity *= 2;
                    chunkMeshQuads.quads = realloc(chunkMeshQuads.quads, sizeof(MeshQuad)*chunkMeshQuads.capacity);
                }

                MeshQuad *curQuad = &(chunkMeshQuads.quads[chunkMeshQuads.amtQuads]);
                curQuad->x = x+chunk->chunkStartX;
                curQuad->y = y-1;
                curQuad->z = z+chunk->chunkStartZ;
                curQuad->width = width;
                curQuad->height = height;
                curQuad->faceType = FACE_BOTTOM;

                chunkMeshQuads.amtQuads++;
                printf("[CREATED QUAD] x %f, y %f, z %f, width %d, height %d, faceType %d\n", curQuad->x, curQuad->y, curQuad->z, width, height, curQuad->faceType);
            }        
        }    
    }

    int visitedLeft[ChunkHeightY][ChunkLengthZ][ChunkWidthX] = {0};
    for (int y = 0; y < ChunkHeightY; y++) {
        for (int x = 0; x < ChunkWidthX; x++) {
            for (int z = 0; z < ChunkLengthZ; z++) {
                if (lefts[y][z][x] == 0 || visitedLeft[y][z][x]) { continue; }
                
                width = 1;
                while (((y+width) < ChunkHeightY && lefts[y+width][z][x] == 1) && !visitedLeft[y+width][z][x]) {
                    width++;
                }

                height = 1;
                done   = 0;
                while ((z+height) < ChunkLengthZ && !done) {
                    for (int dy = 0; dy < width; dy++) {
                        if (lefts[y+dy][z+height][x] == 0 || visitedLeft[y+dy][z+height][x]) {
                            done = 1;
                            break;
                        }
                    }
                    
                    if (!done) {
                        height++;
                    }                    
                }

                for (int dz = 0; dz<ChunkLengthZ; dz++) {
                    for (int dy = 0; dy<ChunkHeightY; dy++) {
                        visitedLeft[y+dy][z+dz][x] = 1;
                    }
                }

                if (chunkMeshQuads.amtQuads >= chunkMeshQuads.capacity) {
                    chunkMeshQuads.capacity *= 2;
                    chunkMeshQuads.quads = realloc(chunkMeshQuads.quads, sizeof(MeshQuad)*chunkMeshQuads.capacity);
                }

                MeshQuad *curQuad = &(chunkMeshQuads.quads[chunkMeshQuads.amtQuads]);
                curQuad->x = x+chunk->chunkStartX;
                curQuad->y = y;
                curQuad->z = z+chunk->chunkStartZ;

                curQuad->width = width;
                curQuad->height = height;
                curQuad->faceType = FACE_LEFT;

                chunkMeshQuads.amtQuads++;
                printf("[CREATED QUAD] x %f, y %f, z %f, width %d, height %d, faceType %d\n", curQuad->x, curQuad->y, curQuad->z, width, height, curQuad->faceType);
            }        
        }    
    }

    int visitedRight[ChunkHeightY][ChunkLengthZ][ChunkWidthX] = {0};
    for (int y = 0; y < ChunkHeightY; y++) {
        for (int x = 0; x < ChunkWidthX; x++) {
            for (int z = 0; z < ChunkLengthZ; z++) {
                if (rights[y][z][x] == 0 || visitedRight[y][z][x]) { continue; }
                
                width = 1;
                while (((y+width) < ChunkHeightY && rights[y+width][z][x] == 1) && !visitedRight[y+width][z][x]) {
                    width++;
                }

                height = 1;
                done   = 0;
                while ((z+height) < ChunkLengthZ && !done) {
                    for (int dy = 0; dy < width; dy++) {
                        if (rights[y+dy][z+height][x] == 0 || visitedRight[y+dy][z+height][x]) {
                            done = 1;
                            break;
                        }
                    }
                    
                    if (!done) {
                        height++;
                    }                    
                }

                for (int dz = 0; dz<ChunkLengthZ; dz++) {
                    for (int dy = 0; dy<ChunkHeightY; dy++) {
                        visitedRight[y+dy][z+dz][x] = 1;
                    }
                }

                if (chunkMeshQuads.amtQuads >= chunkMeshQuads.capacity) {
                    chunkMeshQuads.capacity *= 2;
                    chunkMeshQuads.quads = realloc(chunkMeshQuads.quads, sizeof(MeshQuad)*chunkMeshQuads.capacity);
                }

                MeshQuad *curQuad = &(chunkMeshQuads.quads[chunkMeshQuads.amtQuads]);
                curQuad->x = x+chunk->chunkStartX;
                curQuad->y = y;
                curQuad->z = z+chunk->chunkStartZ;
                curQuad->width = width;
                curQuad->height = height;
                curQuad->faceType = FACE_RIGHT;

                chunkMeshQuads.amtQuads++;
                printf("[CREATED QUAD] x %f, y %f, z %f, width %d, height %d, faceType %d\n", curQuad->x, curQuad->y, curQuad->z, width, height, curQuad->faceType);
            }        
        }    
    }

    int visitedFronts[ChunkWidthX][ChunkHeightY][ChunkLengthZ] = {0};
    for (int y = 0; y < ChunkHeightY; y++) {
        for (int x = 0; x < ChunkWidthX; x++) {
            for (int z = 0; z < ChunkLengthZ; z++) {
                if (fronts[x][y][z] == 0 || visitedFronts[x][y][z]) { continue; }
                
                width = 1;
                while (((x+width) < ChunkWidthX && fronts[x+width][y][z] == 1) && !visitedFronts[x+width][y][z]) {
                    width++;
                }

                height = 1;
                done   = 0;
                while ((y+height) < ChunkHeightY && !done) {
                    for (int dx = 0; dx < width; dx++) {
                        if (fronts[x+dx][y+height][z] == 0 || visitedFronts[x+dx][y+height][z]) {
                            done = 1;
                            break;
                        }
                    }
                    
                    if (!done) {
                        height++;
                    }                    
                }

                for (int dy = 0; dy<ChunkHeightY; dy++) {
                    for (int dx = 0; dx<ChunkWidthX; dx++) {
                        visitedFronts[x+dx][y+dy][z] = 1;
                    }
                }

                if (chunkMeshQuads.amtQuads >= chunkMeshQuads.capacity) {
                    chunkMeshQuads.capacity *= 2;
                    chunkMeshQuads.quads = realloc(chunkMeshQuads.quads, sizeof(MeshQuad)*chunkMeshQuads.capacity);
                }

                MeshQuad *curQuad = &(chunkMeshQuads.quads[chunkMeshQuads.amtQuads]);
                curQuad->x = x+chunk->chunkStartX;
                curQuad->y = y;
                curQuad->z = z+chunk->chunkStartZ-1;
                curQuad->width = width;
                curQuad->height = height;
                curQuad->faceType = FACE_FRONT;

                chunkMeshQuads.amtQuads++;
                printf("[CREATED QUAD] x %f, y %f, z %f, width %d, height %d, faceType %d\n", curQuad->x, curQuad->y, curQuad->z, width, height, curQuad->faceType);
            }        
        }    
    }

    int visitedBacks[ChunkWidthX][ChunkHeightY][ChunkLengthZ] = {0};
    for (int y = 0; y < ChunkHeightY; y++) {
        for (int x = 0; x < ChunkWidthX; x++) {
            for (int z = 0; z < ChunkLengthZ; z++) {
                if (backs[x][y][z] == 0 || visitedBacks[x][y][z]) { continue; }
                
                width = 1;
                while (((x+width) < ChunkWidthX && backs[x+width][y][z] == 1) && !visitedBacks[x+width][y][z]) {
                    width++;
                }

                height = 1;
                done   = 0;
                while ((y+height) < ChunkHeightY && !done) {
                    for (int dx = 0; dx < width; dx++) {
                        if (backs[x+dx][y+height][z] == 0 || visitedBacks[x+dx][y+height][z]) {
                            done = 1;
                            break;
                        }
                    }
                    
                    if (!done) {
                        height++;
                    }                    
                }

                for (int dy = 0; dy<ChunkHeightY; dy++) {
                    for (int dx = 0; dx<ChunkWidthX; dx++) {
                        visitedBacks[x+dx][y+dy][z] = 1;
                    }
                }

                if (chunkMeshQuads.amtQuads >= chunkMeshQuads.capacity) {
                    chunkMeshQuads.capacity *= 2;
                    chunkMeshQuads.quads = realloc(chunkMeshQuads.quads, sizeof(MeshQuad)*chunkMeshQuads.capacity);
                }

                MeshQuad *curQuad = &(chunkMeshQuads.quads[chunkMeshQuads.amtQuads]);
                curQuad->x = x+chunk->chunkStartX;
                curQuad->y = y;
                curQuad->z = z+chunk->chunkStartZ+1;
                curQuad->width = width;
                curQuad->height = height;
                curQuad->faceType = FACE_BACK;

                chunkMeshQuads.amtQuads++;
                printf("[CREATED QUAD] x %f, y %f, z %f, width %d, height %d, faceType %d\n", curQuad->x, curQuad->y, curQuad->z, width, height, curQuad->faceType);
            }        
        }    
    }

}