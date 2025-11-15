#include <GL/glu.h>
#include <GL/glut.h>
#include "chunks.h"

PlayerChunks world;
int ChunkWidthX = 16;
int ChunkLengthZ = 16; 
int ChunkHeightY = 10;

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
            }
        }
    }
}

void initWorld(PlayerChunks *world)
{
    int x = 0;
    int z = 0;
    for (int i = 0; i < 16; i++)
    {
        x = i % (4);
        z = (int)(i / (4));
        createChunk(&(world->chunks[i]), x * ChunkWidthX * BlockWidthX, z * ChunkLengthZ * BlockLengthZ);
    }
}