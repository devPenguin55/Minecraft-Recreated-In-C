#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#include "chunks.h"

PlayerChunks world;
// int ChunkWidthX = 1;
// int ChunkLengthZ = 1; 
// int ChunkHeightY = 10;


float BlockWidthX = 0.15;
float BlockHeightY = 0.15;
float BlockLengthZ = 0.15;

void createChunk(Chunk *chunk, GLfloat xAdd, GLfloat zAdd)
{
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
    for (int i = 0; i < 1; i++)
    {
        x = i % (4);
        z = (int)(i / (4));
        createChunk(&(world->chunks[i]), x * ChunkWidthX * BlockWidthX, z * ChunkLengthZ * BlockLengthZ);
    }
}

void generateChunkMesh(Chunk *chunk)
{
    int tops[ChunkWidthX][ChunkLengthZ][ChunkHeightY]    = {0}; // X-Z plane, y fixed
    int bottoms[ChunkWidthX][ChunkLengthZ][ChunkHeightY] = {0}; // X-Z plane, y fixed
    int lefts[ChunkHeightY][ChunkLengthZ][ChunkWidthX]   = {0}; // Y-Z plane, x fixed
    int rights[ChunkHeightY][ChunkLengthZ][ChunkWidthX]  = {0}; // Y-Z plane, x fixed
    int fronts[ChunkWidthX][ChunkHeightY][ChunkLengthZ]  = {0}; // X-Y plane, z fixed
    int backs[ChunkWidthX][ChunkHeightY][ChunkLengthZ]   = {0}; // X-Y plane, z fixed

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
    
    }

    // greedy algorithm
    ChunkMeshQuads chunkMeshQuads;
    chunkMeshQuads.capacity = 16;
    chunkMeshQuads.amtQuads =  0;
    chunkMeshQuads.quads = malloc(sizeof(MeshQuad)*chunkMeshQuads.capacity);

    // if (chunkMeshQuads.amtQuads >= chunkMeshQuads.capacity) {
    //     chunkMeshQuads.capacity *= 2;
    //     chunkMeshQuads.quads = realloc(chunkMeshQuads.quads, sizeof(MeshQuad)*chunkMeshQuads.capacity);
    // }
    
}