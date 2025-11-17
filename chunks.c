#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#include "chunks.h"

PlayerChunks world;
// int ChunkWidthX = 1;
// int ChunkLengthZ = 1; 
// int ChunkHeightY = 10;
int ChunkWidthX = 16;
int ChunkLengthZ = 16; 
int ChunkHeightY = 5;

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
    // +Y direction, check upwards
    for (int y = 0; y<ChunkHeightY; y++) {
        int tops[16][16] = {0};
        int bottoms[16][16] = {0};
        int lefts[16][16] = {0};
        for (int x = 0; x<ChunkWidthX; x++) {
            for (int z = 0; z<ChunkLengthZ; z++) {
                int blockIndex = x + z*ChunkWidthX + y*(ChunkWidthX*ChunkLengthZ);
                int topBlockIndex = blockIndex - ChunkWidthX*ChunkLengthZ;
                int bottomBlockIndex = blockIndex + ChunkWidthX*ChunkLengthZ;
                int leftBlockIndex = blockIndex - 1;
                if (
                    (topBlockIndex >= 0 && (!chunk->blocks[blockIndex].isAir && chunk->blocks[topBlockIndex].isAir)) ||                    
                    (!chunk->blocks[blockIndex].isAir && topBlockIndex < 0)    
                ) {
                    // existing block
                    tops[x][z] = 1;
                } else {
                    tops[x][z] = 0;
                }

                if (
                    (bottomBlockIndex < ChunkWidthX*ChunkLengthZ*ChunkHeightY && (!chunk->blocks[blockIndex].isAir && chunk->blocks[bottomBlockIndex].isAir)) ||                    
                    (!chunk->blocks[blockIndex].isAir && bottomBlockIndex >= ChunkWidthX*ChunkLengthZ*ChunkHeightY)    
                ) {
                    // existing block
                    bottoms[x][z] = 1;
                } else {
                    bottoms[x][z] = 0;
                }

                if ((x > 0 && (!chunk->blocks[blockIndex].isAir && chunk->blocks[leftBlockIndex].isAir)) ||
                   (!chunk->blocks[blockIndex].isAir && x == 0)
                ) {
                    // existing block
                    lefts[x][z] = 1;
                } else {
                    lefts[x][z] = 0;
                }
            }        
        }
        for (int i = 0; i < 16; i++)
        {
            for (int j = 0; j < 16; j++)
            {
                printf("%d ", lefts[i][j]);
            }
            printf("\n");
        }
        printf(" ---  new y layer of %d\n", y); 
    }
}