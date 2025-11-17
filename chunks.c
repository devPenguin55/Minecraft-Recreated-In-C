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
        for (int x = 0; x<ChunkWidthX; x++) {
            for (int z = 0; z<ChunkLengthZ; z++) {
                int blockIndex = x + z*ChunkWidthX + y*(ChunkWidthX*ChunkLengthZ);
                int aboveBlockIndex = blockIndex - ChunkWidthX*ChunkLengthZ;
                if (
                    (aboveBlockIndex >= 0 && (!chunk->blocks[blockIndex].isAir && chunk->blocks[aboveBlockIndex].isAir)) ||                    
                    (!chunk->blocks[blockIndex].isAir && aboveBlockIndex < 0)    
                ) {
                    // existing block
                    tops[x][z] = 1;
                } else {
                    tops[x][z] = 0;
                }
            }        
        }
        for (int i = 0; i < 16; i++)
        {
            for (int j = 0; j < 16; j++)
            {
                printf("%d ", tops[i][j]);
            }
            printf("\n");
        }
        printf(" ---  new y layer of %d\n", y); 
    }
}